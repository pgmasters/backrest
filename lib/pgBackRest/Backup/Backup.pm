####################################################################################################################################
# BACKUP MODULE
####################################################################################################################################
package pgBackRest::Backup::Backup;

use strict;
use warnings FATAL => qw(all);
use Carp qw(confess);
use English '-no_match_vars';

use Exporter qw(import);
use File::Basename;
use JSON::PP;

use pgBackRest::Archive::Common;
use pgBackRest::Backup::Common;
use pgBackRest::Backup::File;
use pgBackRest::Backup::Info;
use pgBackRest::Common::Cipher;
use pgBackRest::Common::Exception;
use pgBackRest::Common::Ini;
use pgBackRest::Common::Log;
use pgBackRest::Common::Wait;
use pgBackRest::Common::String;
use pgBackRest::Config::Config;
use pgBackRest::Db;
use pgBackRest::DbVersion;
use pgBackRest::Manifest;
use pgBackRest::Protocol::Local::Process;
use pgBackRest::Protocol::Helper;
use pgBackRest::Protocol::Storage::Helper;
use pgBackRest::Common::Io::Handle;
use pgBackRest::Storage::Base;
use pgBackRest::Storage::Helper;
use pgBackRest::Version;

####################################################################################################################################
# new
####################################################################################################################################
sub new
{
    my $class = shift;          # Class name

    # Create the class hash
    my $self = {};
    bless $self, $class;

    # Assign function parameters, defaults, and log debug info
    my ($strOperation) = logDebugParam(__PACKAGE__ . '->new');

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'self', value => $self}
    );
}

####################################################################################################################################
# resumeClean - cleans the directory from a previous failed backup so it can be reused
####################################################################################################################################
sub resumeClean
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $oStorageRepo,
        $strBackupLabel,
        $oManifest,
        $oAbortedManifest,
        $bOnline,
        $bDelta,
        $strTimelineCurrent,
        $strTimelineLast,
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->resumeClean', \@_,
            {name => 'oStorageRepo'},
            {name => 'strBackupLabel'},
            {name => 'oManifest'},
            {name => 'oAbortedManifest'},
            {name => 'bOnline'},
            {name => 'bDelta'},
            {name => 'strTimelineCurrent', required => false},
            {name => 'strTimelineLast', required => false},
        );

    &log(DETAIL, 'clean resumed backup path: ' . $oStorageRepo->pathGet(STORAGE_REPO_BACKUP . "/${strBackupLabel}"));

    # Build manifest for aborted backup path
    my $hFile = $oStorageRepo->manifest(STORAGE_REPO_BACKUP . "/${strBackupLabel}");

    # Get compress flag
    my $bCompressed = $oAbortedManifest->boolGet(MANIFEST_SECTION_BACKUP_OPTION, MANIFEST_KEY_COMPRESS);

    if (!$bDelta)
    {
        # Check to see if delta checksum should be enabled
        $bDelta = $oAbortedManifest->checkDelta(
            'resumed', $oAbortedManifest->boolTest(MANIFEST_SECTION_BACKUP_OPTION, MANIFEST_KEY_ONLINE, undef, $bOnline),
            $strTimelineCurrent, $strTimelineLast);

        # If delta is still false, check the files for anomalies
        if (!$bDelta)
        {
            my @stryFileList = ();

            foreach my $strName (sort(keys(%{$hFile})))
            {
                # Ignore files that will never be in the manifest but should be preserved
                if ($strName eq FILE_MANIFEST_COPY ||
                    $strName eq '.')
                {
                    next;
                }

                if ($hFile->{$strName}{type} eq 'f')
                {
                    # If the original backup was compressed then remove the extension before checking the manifest
                    my $strFile = $strName;

                    if ($bCompressed)
                    {
                        $strFile = substr($strFile, 0, length($strFile) - 3);
                    }

                    # To be preserved the file must exist in the new manifest and not be a reference to a previous backup and must
                    # have a checksum
                    if ($oManifest->test(MANIFEST_SECTION_TARGET_FILE, $strFile) &&
                        !$oManifest->test(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_REFERENCE) &&
                        $oAbortedManifest->test(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_CHECKSUM))
                    {
                        push(@stryFileList, $strFile);
                    }
                }
            }

            # If there are files in the list then check if delta should be enabled
            if (@stryFileList)
            {
                $bDelta = $oManifest->checkDeltaFile(\@stryFileList, $oAbortedManifest, undef);
            }
        }
    }

    # Find paths and files to delete
    my @stryFile;

    foreach my $strName (sort(keys(%{$hFile})))
    {
        # Ignore files that will never be in the manifest but should be preserved
        if ($strName eq FILE_MANIFEST_COPY ||
            $strName eq '.')
        {
            next;
        }

        # Get the file type (all links will be deleted since they are easy to recreate)
        my $cType = $hFile->{$strName}{type};

        # If a directory check if it exists in the new manifest
        if ($cType eq 'd')
        {
            if ($oManifest->test(MANIFEST_SECTION_TARGET_PATH, $strName))
            {
                next;
            }
        }
        # Else if a file
        elsif ($cType eq 'f')
        {
            # If the original backup was compressed then remove the extension before checking the manifest
            my $strFile = $strName;

            if ($bCompressed)
            {
                $strFile = substr($strFile, 0, length($strFile) - 3);
            }

            # To be preserved the file must exist in the new manifest and not be a reference to a previous backup
            if ($oManifest->test(MANIFEST_SECTION_TARGET_FILE, $strFile) &&
                !$oManifest->test(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_REFERENCE))
            {
                # To be preserved the checksum must be defined
                my $strChecksum = $oAbortedManifest->get(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_CHECKSUM, false);

                # If the size and timestamp match OR if the size matches and the delta option is set, then keep the file.
                # In the latter case, if the timestamp had changed then rather than removing and recopying the file, the file
                # will be tested in backupFile to see if the db/repo checksum still matches: if so, it is not necessary to recopy,
                # else it will need to be copied to the new backup.
                if (defined($strChecksum) &&
                    $oManifest->get(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_SIZE) ==
                    $oAbortedManifest->get(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_SIZE)  &&
                    ($bDelta ||
                    $oManifest->get(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_TIMESTAMP) ==
                    $oAbortedManifest->get(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_TIMESTAMP)))
                {
                    $oManifest->set(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_CHECKSUM, $strChecksum);

                    # Also copy page checksum results if they exist
                    my $bChecksumPage =
                        $oAbortedManifest->get(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_CHECKSUM_PAGE, false);

                    if (defined($bChecksumPage))
                    {
                        $oManifest->boolSet(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_CHECKSUM_PAGE, $bChecksumPage);

                        if (!$bChecksumPage &&
                            $oAbortedManifest->test(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_CHECKSUM_PAGE_ERROR))
                        {
                            $oManifest->set(
                                MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_CHECKSUM_PAGE_ERROR,
                                $oAbortedManifest->get(
                                    MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_CHECKSUM_PAGE_ERROR));
                        }
                    }

                    next;
                }
            }
        }

        # If a directory then remove it
        if ($cType eq 'd')
        {
            logDebugMisc($strOperation, "remove path ${strName}");
            $oStorageRepo->pathRemove(STORAGE_REPO_BACKUP . "/${strBackupLabel}/${strName}", {bRecurse => true});
        }
        # Else add the file/link to be deleted later
        else
        {
            logDebugMisc($strOperation, "remove file ${strName}");
            push(@stryFile, STORAGE_REPO_BACKUP . "/${strBackupLabel}/${strName}");
        }
    }

    # Delete files in batch for more efficiency
    if (@stryFile > 0)
    {
        $oStorageRepo->remove(\@stryFile);
    }

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'bDelta', value => $bDelta, trace => true},
    );
}

####################################################################################################################################
# processManifest
#
# Process the file level backup.  Uses the information in the manifest to determine which files need to be copied.  Directories
# and tablespace links are only created when needed, except in the case of a full backup or if hardlinks are requested.
####################################################################################################################################
sub processManifest
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $strDbMasterPath,
        $strDbCopyPath,
        $strType,
        $strDbVersion,
        $bCompress,
        $bHardLink,
        $oBackupManifest,
        $strBackupLabel,
        $strLsnStart,
    ) =
        logDebugParam
    (
        __PACKAGE__ . '->processManifest', \@_,
        {name => 'strDbMasterPath'},
        {name => 'strDbCopyPath'},
        {name => 'strType'},
        {name => 'strDbVersion'},
        {name => 'bCompress'},
        {name => 'bHardLink'},
        {name => 'oBackupManifest'},
        {name => 'strBackupLabel'},
        {name => 'strLsnStart', required => false},
    );

    # Start backup test point
    &log(TEST, TEST_BACKUP_START);

    # Get the master protocol for keep-alive
    my $oProtocolMaster =
        !isDbLocal({iRemoteIdx => $self->{iMasterRemoteIdx}}) ?
            protocolGet(CFGOPTVAL_REMOTE_TYPE_DB, $self->{iMasterRemoteIdx}) : undef;
    defined($oProtocolMaster) && $oProtocolMaster->noOp();

    # Initialize the backup process
    my $oBackupProcess = new pgBackRest::Protocol::Local::Process(CFGOPTVAL_LOCAL_TYPE_DB);

    if ($self->{iCopyRemoteIdx} != $self->{iMasterRemoteIdx})
    {
        $oBackupProcess->hostAdd($self->{iMasterRemoteIdx}, 1);
    }

    $oBackupProcess->hostAdd($self->{iCopyRemoteIdx}, cfgOption(CFGOPT_PROCESS_MAX));

    # Variables used for parallel copy
    my $lFileTotal = 0;
    my $lSizeTotal = 0;

    # If this is a full backup or hard-linked then create all paths and tablespace links
    if ($bHardLink || $strType eq CFGOPTVAL_BACKUP_TYPE_FULL)
    {
        # Create paths
        foreach my $strPath ($oBackupManifest->keys(MANIFEST_SECTION_TARGET_PATH))
        {
            storageRepo()->pathCreate(STORAGE_REPO_BACKUP . "/${strBackupLabel}/${strPath}", {bIgnoreExists => true});
        }

        if (storageRepo()->capability(STORAGE_CAPABILITY_LINK))
        {
            for my $strTarget ($oBackupManifest->keys(MANIFEST_SECTION_BACKUP_TARGET))
            {
                if ($oBackupManifest->isTargetTablespace($strTarget))
                {
                    storageRepo()->linkCreate(
                        STORAGE_REPO_BACKUP . "/${strBackupLabel}/${strTarget}",
                        STORAGE_REPO_BACKUP . "/${strBackupLabel}/" . MANIFEST_TARGET_PGDATA . "/${strTarget}",
                        {bRelative => true});
                }
            }
        }
    }

    # Iterate all files in the manifest
    foreach my $strRepoFile (
        sort {sprintf("%016d-%s", $oBackupManifest->numericGet(MANIFEST_SECTION_TARGET_FILE, $b, MANIFEST_SUBKEY_SIZE), $b) cmp
              sprintf("%016d-%s", $oBackupManifest->numericGet(MANIFEST_SECTION_TARGET_FILE, $a, MANIFEST_SUBKEY_SIZE), $a)}
        ($oBackupManifest->keys(MANIFEST_SECTION_TARGET_FILE, INI_SORT_NONE)))
    {
        # If the file has a reference it does not need to be copied since it can be retrieved from the referenced backup - unless
        # the option to checksum all files is set.  However, if hardlinking is enabled the link will need to be created
        my $strReference = $oBackupManifest->get(MANIFEST_SECTION_TARGET_FILE, $strRepoFile, MANIFEST_SUBKEY_REFERENCE, false);

        if (defined($strReference))
        {
            # If the delta option to checksum all files is not set or it is set and the file size of the referenced file is zero
            # then skip checking/copying this file
            if (!cfgOption(CFGOPT_DELTA) ||
                $oBackupManifest->numericGet(MANIFEST_SECTION_TARGET_FILE, $strRepoFile, MANIFEST_SUBKEY_SIZE) == 0)
            {
                # This file will not need to be copied
                next;
            }
        }

        # By default put everything into a single queue
        my $strQueueKey = MANIFEST_TARGET_PGDATA;

        # If the file belongs in a tablespace then put in a tablespace-specific queue
        if (index($strRepoFile, DB_PATH_PGTBLSPC . '/') == 0)
        {
            $strQueueKey = DB_PATH_PGTBLSPC . '/' . (split('\/', $strRepoFile))[1];
        }

        # Create the file hash
        my $bIgnoreMissing = true;
        my $strDbFile = $oBackupManifest->dbPathGet($strDbCopyPath, $strRepoFile);
        my $iHostConfigIdx = $self->{iCopyRemoteIdx};

        # Certain files must be copied from the master
        if ($oBackupManifest->boolGet(MANIFEST_SECTION_TARGET_FILE, $strRepoFile, MANIFEST_SUBKEY_MASTER))
        {
            $strDbFile = $oBackupManifest->dbPathGet($strDbMasterPath, $strRepoFile);
            $iHostConfigIdx = $self->{iMasterRemoteIdx};
        }

        # Make sure that pg_control is not removed during the backup
        if ($strRepoFile eq MANIFEST_TARGET_PGDATA . '/' . DB_FILE_PGCONTROL)
        {
            $bIgnoreMissing = false;
        }

        # Increment file total and size
        my $lSize = $oBackupManifest->numericGet(MANIFEST_SECTION_TARGET_FILE, $strRepoFile, MANIFEST_SUBKEY_SIZE);

        $lFileTotal++;
        $lSizeTotal += $lSize;

        # Queue for parallel backup
        $oBackupProcess->queueJob(
            $iHostConfigIdx, $strQueueKey, $strRepoFile, OP_BACKUP_FILE,
            [$strDbFile, $bIgnoreMissing, $lSize,
                $oBackupManifest->get(MANIFEST_SECTION_TARGET_FILE, $strRepoFile, MANIFEST_SUBKEY_CHECKSUM, false),
                cfgOption(CFGOPT_CHECKSUM_PAGE) ? isChecksumPage($strRepoFile) : false,
                defined($strLsnStart) ? hex((split('/', $strLsnStart))[0]) : 0xFFFFFFFF,
                defined($strLsnStart) ? hex((split('/', $strLsnStart))[1]) : 0xFFFFFFFF,
                $strRepoFile, defined($strReference) ? true : false, $bCompress, cfgOption(CFGOPT_COMPRESS_LEVEL),
                $strBackupLabel, cfgOption(CFGOPT_DELTA)],
            {rParamSecure => $oBackupManifest->cipherPassSub() ? [$oBackupManifest->cipherPassSub()] : undef});

        # Size and checksum will be removed and then verified later as a sanity check
        $oBackupManifest->remove(MANIFEST_SECTION_TARGET_FILE, $strRepoFile, MANIFEST_SUBKEY_SIZE);
        $oBackupManifest->remove(MANIFEST_SECTION_TARGET_FILE, $strRepoFile, MANIFEST_SUBKEY_CHECKSUM);
    }

    # pg_control should always be in the backup (unless this is an offline backup)
    if (!$oBackupManifest->test(MANIFEST_SECTION_TARGET_FILE, MANIFEST_FILE_PGCONTROL) && cfgOption(CFGOPT_ONLINE))
    {
        confess &log(ERROR, DB_FILE_PGCONTROL . " must be present in all online backups\n" .
                     'HINT: is something wrong with the clock or filesystem timestamps?', ERROR_FILE_MISSING);
    }

    # If there are no files to backup then we'll exit with an error unless in test mode.  The other way this could happen is if
    # the database is down and backup is called with --no-online twice in a row.
    if ($lFileTotal == 0 && !cfgOption(CFGOPT_TEST))
    {
        confess &log(ERROR, "no files have changed since the last backup - this seems unlikely", ERROR_FILE_MISSING);
    }

    # Running total of bytes copied
    my $lSizeCurrent = 0;

    # Determine how often the manifest will be saved
    my $lManifestSaveCurrent = 0;
    my $lManifestSaveSize = int($lSizeTotal / 100);

    if (cfgOptionSource(CFGOPT_MANIFEST_SAVE_THRESHOLD) ne CFGDEF_SOURCE_DEFAULT ||
        $lManifestSaveSize < cfgOption(CFGOPT_MANIFEST_SAVE_THRESHOLD))
    {
        $lManifestSaveSize = cfgOption(CFGOPT_MANIFEST_SAVE_THRESHOLD);
    }

    # Run the backup jobs and process results
    while (my $hyJob = $oBackupProcess->process())
    {
        foreach my $hJob (@{$hyJob})
        {
            ($lSizeCurrent, $lManifestSaveCurrent) = backupManifestUpdate(
                $oBackupManifest, cfgOption(cfgOptionIdFromIndex(CFGOPT_PG_HOST, $hJob->{iHostConfigIdx}), false),
                $hJob->{iProcessId}, @{$hJob->{rParam}}[0], @{$hJob->{rParam}}[7], @{$hJob->{rParam}}[2], @{$hJob->{rParam}}[3],
                @{$hJob->{rParam}}[4], @{$hJob->{rResult}}, $lSizeTotal, $lSizeCurrent, $lManifestSaveSize,
                $lManifestSaveCurrent);
        }

        # A keep-alive is required here because if there are a large number of resumed files that need to be checksummed
        # then the remote might timeout while waiting for a command.
        protocolKeepAlive();
    }

    foreach my $strFile ($oBackupManifest->keys(MANIFEST_SECTION_TARGET_FILE))
    {
        # If the file has a reference, then it was not copied since it can be retrieved from the referenced backup. However, if
        # hardlinking is enabled the link will need to be created.
        my $strReference = $oBackupManifest->get(MANIFEST_SECTION_TARGET_FILE, $strFile, MANIFEST_SUBKEY_REFERENCE, false);

        if ($strReference)
        {
            # If hardlinking is enabled then create a hardlink for files that have not changed since the last backup
            if ($bHardLink)
            {
                &log(DETAIL, "hardlink ${strFile} to ${strReference}");

                storageRepo()->linkCreate(
                    STORAGE_REPO_BACKUP . "/${strReference}/${strFile}" . ($bCompress ? qw{.} . COMPRESS_EXT : ''),
                    STORAGE_REPO_BACKUP . "/${strBackupLabel}/${strFile}" . ($bCompress ? qw{.} . COMPRESS_EXT : ''),
                    {bHard => true});
            }
            # Else log the reference. With delta, it is possible that references may have been removed if a file needed to be
            # recopied.
            else
            {
                logDebugMisc($strOperation, "reference ${strFile} to ${strReference}");
            }
        }
    }

    # Validate the manifest
    $oBackupManifest->validate();

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'lSizeTotal', value => $lSizeTotal}
    );
}

####################################################################################################################################
# process
#
# Process the database backup.
####################################################################################################################################
sub process
{
    my $self = shift;
    my @stryCommandArg = @_;

    # Assign function parameters, defaults, and log debug info
    my ($strOperation) = logDebugParam(__PACKAGE__ . '->process');

    # Get parameters passed from C backup code
    if (@stryCommandArg != 1)
    {
        confess &log(ERROR, "missing command parameter from C");
    }

    my $rhParam = (JSON::PP->new()->allow_nonref())->decode($stryCommandArg[0]);

    # Load backup.info
    my $oBackupInfo = new pgBackRest::Backup::Info(storageRepo()->pathGet(STORAGE_REPO_BACKUP));
    my $strCipherPassManifest = $oBackupInfo->cipherPassSub();

    # Search cluster directory for an aborted backup
    my $strBackupLabel = $rhParam->{backupLabelResume};
    my $strCipherPassBackupSet;
    my $oAbortedManifest;
    my $strBackupPath;
    my $strTimelineAborted;

    if (defined($strBackupLabel))
    {
        $strBackupPath = storageRepo()->pathGet(STORAGE_REPO_BACKUP . "/${strBackupLabel}");

        $oAbortedManifest = new pgBackRest::Manifest(
            storageRepo()->pathGet("${strBackupPath}/" . FILE_MANIFEST), {strCipherPass => $oBackupInfo->cipherPassSub()});

        # Get the archive segment timeline for determining if a timeline switch has occurred. Only defined for prior online
        # backup.
        if ($oAbortedManifest->test(MANIFEST_SECTION_BACKUP, MANIFEST_KEY_ARCHIVE_STOP))
        {
            $strTimelineAborted = substr($oAbortedManifest->get(MANIFEST_SECTION_BACKUP, MANIFEST_KEY_ARCHIVE_STOP), 0, 8);
        }
        elsif ($oAbortedManifest->test(MANIFEST_SECTION_BACKUP, MANIFEST_KEY_ARCHIVE_START))
        {
            $strTimelineAborted = substr($oAbortedManifest->get(MANIFEST_SECTION_BACKUP, MANIFEST_KEY_ARCHIVE_START), 0, 8);
        }
    }

    # Get the backup label path
    if (!defined($strBackupLabel))
    {
        $strBackupLabel = $rhParam->{backupLabel};
        $strBackupPath = storageRepo()->pathGet(STORAGE_REPO_BACKUP . "/${strBackupLabel}");
    }

    # Load manifest passed from C
    my $oBackupManifest = new pgBackRest::Manifest(
        STORAGE_REPO_BACKUP . "/" . FILE_MANIFEST . '.pass', {oStorage => storageRepo(),
        strCipherPass => defined($oBackupInfo->cipherPassSub()) ? $oBackupInfo->cipherPassSub() : undef});
    $oBackupManifest->{strFileName} = STORAGE_REPO_BACKUP . "/${strBackupLabel}/" . FILE_MANIFEST;

    ################################################################################################################################
    # ALL THE ABOVE EXISTS ONLY FOR MIGRATION
    ################################################################################################################################
    # Start backup (unless --no-online is set)
    my $strTimelineCurrent = undef;
    my $oDbMaster = undef;
    my $oDbStandby = undef;

    ($oDbMaster, $self->{iMasterRemoteIdx}, $oDbStandby, $self->{iCopyRemoteIdx}) = dbObjectGet();

    if (!defined($self->{iCopyRemoteIdx}))
    {
        $self->{iCopyRemoteIdx} = $self->{iMasterRemoteIdx};
    }

    my $oStorageDbMaster = storageDb({iRemoteIdx => $self->{iMasterRemoteIdx}});

    my $strDbMasterPath = cfgOption(cfgOptionIdFromIndex(CFGOPT_PG_PATH, $self->{iMasterRemoteIdx}));
    my $strDbCopyPath = cfgOption(cfgOptionIdFromIndex(CFGOPT_PG_PATH, $self->{iCopyRemoteIdx}));

    &log(TEST, TEST_MANIFEST_BUILD);

    # If resuming from an aborted backup
    if (defined($oAbortedManifest))
    {
        &log(WARN, "aborted backup ${strBackupLabel} of same type exists, will be cleaned to remove invalid files and resumed");
        &log(TEST, TEST_BACKUP_RESUME);

        # Clean the backup path before resuming. The delta option may have changed from false to true during the resume clean
        # so set it to the result.
        cfgOptionSet(CFGOPT_DELTA, $self->resumeClean(storageRepo(), $strBackupLabel, $oBackupManifest, $oAbortedManifest,
            cfgOption(CFGOPT_ONLINE), cfgOption(CFGOPT_DELTA), $strTimelineCurrent, $strTimelineAborted));
    }

    # Set the delta option in the manifest
    $oBackupManifest->boolSet(MANIFEST_SECTION_BACKUP_OPTION, MANIFEST_KEY_DELTA, undef, cfgOption(CFGOPT_DELTA));

    # Save the backup manifest
    $oBackupManifest->saveCopy();

    # Perform the backup
    my $lBackupSizeTotal =
        $self->processManifest(
            $strDbMasterPath, $strDbCopyPath, cfgOption(CFGOPT_TYPE), $rhParam->{pgVersion}, cfgOption(CFGOPT_COMPRESS),
            cfgOption(CFGOPT_REPO_HARDLINK), $oBackupManifest, $strBackupLabel, undef);
    &log(INFO, cfgOption(CFGOPT_TYPE) . " backup size = " . fileSizeFormat($lBackupSizeTotal));

    # Master file object no longer needed
    undef($oStorageDbMaster);

    # Stop backup (unless --no-online is set)
    my $strArchiveStop = undef;
    my $strLsnStop = undef;

    if (cfgOption(CFGOPT_ONLINE))
    {
        ($strArchiveStop, $strLsnStop, my $strTimestampDbStop, my $oFileHash) = $oDbMaster->backupStop();

        $oBackupManifest->set(MANIFEST_SECTION_BACKUP, MANIFEST_KEY_ARCHIVE_STOP, undef, $strArchiveStop);
        $oBackupManifest->set(MANIFEST_SECTION_BACKUP, MANIFEST_KEY_LSN_STOP, undef, $strLsnStop);
        &log(INFO, "backup stop archive = ${strArchiveStop}, lsn = ${strLsnStop}");

        # Write out files returned from stop backup
        foreach my $strFile (sort(keys(%{$oFileHash})))
        {
            # Only save the file if it has content
            if (defined($oFileHash->{$strFile}))
            {
                my $rhyFilter = [{strClass => STORAGE_FILTER_SHA}];

                # Add compression filter
                if (cfgOption(CFGOPT_COMPRESS))
                {
                    push(
                        @{$rhyFilter},
                        {strClass => STORAGE_FILTER_GZIP, rxyParam => [STORAGE_COMPRESS, false, cfgOption(CFGOPT_COMPRESS_LEVEL)]});
                }

                # If the backups are encrypted, then the passphrase for the backup set from the manifest file is required to access
                # the file in the repo
                my $oDestinationFileIo = storageRepo()->openWrite(
                    STORAGE_REPO_BACKUP . "/${strBackupLabel}/${strFile}" .
                        (cfgOption(CFGOPT_COMPRESS) ? qw{.} . COMPRESS_EXT : ''),
                    {rhyFilter => $rhyFilter,
                        strCipherPass => defined($strCipherPassBackupSet) ? $strCipherPassBackupSet : undef});

                # Write content out to a file
                storageRepo()->put($oDestinationFileIo, $oFileHash->{$strFile});

                # Add file to manifest
                $oBackupManifest->fileAdd(
                    $strFile, time(), length($oFileHash->{$strFile}), $oDestinationFileIo->result(STORAGE_FILTER_SHA), true);

                &log(DETAIL, "wrote '${strFile}' file returned from pg_stop_backup()");
            }
        }
    }

    # Remotes no longer needed (destroy them here so they don't timeout)
    &log(TEST, TEST_BACKUP_STOP);

    undef($oDbMaster);
    protocolDestroy(undef, undef, true);

    # If archive logs are required to complete the backup, then check them.  This is the default, but can be overridden if the
    # archive logs are going to a different server.  Be careful of this option because there is no way to verify that the backup
    # will be consistent - at least not here.
    if (cfgOption(CFGOPT_ONLINE) && cfgOption(CFGOPT_ARCHIVE_CHECK))
    {
        # Save the backup manifest before getting archive logs in case of failure
        $oBackupManifest->saveCopy();

        # Create the modification time for the archive logs
        my $lModificationTime = time();

        # After the backup has been stopped, need to make a copy of the archive logs to make the db consistent
        logDebugMisc($strOperation, "retrieve archive logs !!!START!!!:!!!STOP!!!");

        my $oArchiveInfo = new pgBackRest::Archive::Info(storageRepo()->pathGet(STORAGE_REPO_ARCHIVE), true);
        my $strArchiveId = $oArchiveInfo->archiveId();
        my @stryArchive = lsnFileRange('START', $strLsnStop, $rhParam->{pgVersion}, 16);

        foreach my $strArchive (@stryArchive)
        {
            my $strArchiveFile = walSegmentFind(
                storageRepo(), $strArchiveId, substr($strArchiveStop, 0, 8) . $strArchive, cfgOption(CFGOPT_ARCHIVE_TIMEOUT));

            $strArchive = substr($strArchiveFile, 0, 24);

            if (cfgOption(CFGOPT_ARCHIVE_COPY))
            {
                logDebugMisc($strOperation, "archive: ${strArchive} (${strArchiveFile})");

                # Copy the log file from the archive repo to the backup
                my $bArchiveCompressed = $strArchiveFile =~ ('^.*\.' . COMPRESS_EXT . '\$');

                storageRepo()->copy(
                    storageRepo()->openRead(STORAGE_REPO_ARCHIVE . "/${strArchiveId}/${strArchiveFile}",
                        {strCipherPass => $oArchiveInfo->cipherPassSub()}),
                    storageRepo()->openWrite(STORAGE_REPO_BACKUP . "/${strBackupLabel}/" . MANIFEST_TARGET_PGDATA . qw{/} .
                        $oBackupManifest->walPath() . "/${strArchive}" . (cfgOption(CFGOPT_COMPRESS) ? qw{.} . COMPRESS_EXT : ''),
                        {bPathCreate => true, strCipherPass => $strCipherPassBackupSet})
                    );

                # Add the archive file to the manifest so it can be part of the restore and checked in validation
                my $strPathLog = MANIFEST_TARGET_PGDATA . qw{/} . $oBackupManifest->walPath();
                my $strFileLog = "${strPathLog}/${strArchive}";

                # Add file to manifest
                $oBackupManifest->fileAdd(
                    $strFileLog, $lModificationTime, PG_WAL_SEGMENT_SIZE, substr($strArchiveFile, 25, 40), true);
            }
        }
    }

    # Record timestamp stop in the config
    my $lTimestampStop = time();
    $oBackupManifest->set(MANIFEST_SECTION_BACKUP, MANIFEST_KEY_TIMESTAMP_STOP, undef, $lTimestampStop + 0);
    $oBackupManifest->set(MANIFEST_SECTION_BACKUP, MANIFEST_KEY_LABEL, undef, $strBackupLabel);

    # Sync backup path if supported
    if (storageRepo()->capability(STORAGE_CAPABILITY_PATH_SYNC))
    {
        # Sync all paths in the backup
        storageRepo()->pathSync(STORAGE_REPO_BACKUP . "/${strBackupLabel}");

        foreach my $strPath ($oBackupManifest->keys(MANIFEST_SECTION_TARGET_PATH))
        {
            my $strPathSync = storageRepo()->pathGet(STORAGE_REPO_BACKUP . "/${strBackupLabel}/$strPath");

            # Not all paths are created for diff/incr backups, so only sync if this is a full backup or the path exists
            if (cfgOption(CFGOPT_TYPE) eq CFGOPTVAL_BACKUP_TYPE_FULL || storageRepo()->pathExists($strPathSync))
            {
                storageRepo()->pathSync($strPathSync);
            }
        }
    }

    # Final save of the backup manifest
    $oBackupManifest->save();

    &log(INFO, "new backup label = ${strBackupLabel}");

    # Copy a compressed version of the manifest to history. If the repo is encrypted then the passphrase to open the manifest is
    # required.
    my $strHistoryPath = storageRepo()->pathGet(
        STORAGE_REPO_BACKUP . qw{/} . PATH_BACKUP_HISTORY . qw{/} . substr($strBackupLabel, 0, 4));

    storageRepo()->copy(
        storageRepo()->openRead(STORAGE_REPO_BACKUP . "/${strBackupLabel}/" . FILE_MANIFEST,
            {'strCipherPass' => $oBackupInfo->cipherPassSub()}),
        storageRepo()->openWrite(
            "${strHistoryPath}/${strBackupLabel}.manifest." . COMPRESS_EXT,
            {rhyFilter => [{strClass => STORAGE_FILTER_GZIP, rxyParam => [STORAGE_COMPRESS, false, 9]}],
                bPathCreate => true, bAtomic => true,
                strCipherPass => defined($oBackupInfo->cipherPassSub()) ? $oBackupInfo->cipherPassSub() : undef}));

    # Sync history path if supported
    if (storageRepo()->capability(STORAGE_CAPABILITY_PATH_SYNC))
    {
        storageRepo()->pathSync(STORAGE_REPO_BACKUP . qw{/} . PATH_BACKUP_HISTORY);
        storageRepo()->pathSync($strHistoryPath);
    }

    # Create a link to the most recent backup
    storageRepo()->remove(STORAGE_REPO_BACKUP . qw(/) . LINK_LATEST);

    if (storageRepo()->capability(STORAGE_CAPABILITY_LINK))
    {
        storageRepo()->linkCreate(
            STORAGE_REPO_BACKUP . "/${strBackupLabel}", STORAGE_REPO_BACKUP . qw{/} . LINK_LATEST, {bRelative => true});
    }

    # Save backup info
    $oBackupInfo->add($oBackupManifest);

    # Sync backup root path if supported
    if (storageRepo()->capability(STORAGE_CAPABILITY_PATH_SYNC))
    {
        storageRepo()->pathSync(STORAGE_REPO_BACKUP);
    }

    # Return from function and log return values if any
    return logDebugReturn($strOperation);
}

1;
