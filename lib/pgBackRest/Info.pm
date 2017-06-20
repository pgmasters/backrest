####################################################################################################################################
# INFO MODULE
####################################################################################################################################
package pgBackRest::Info;

use strict;
use warnings FATAL => qw(all);
use Carp qw(confess);

use Exporter qw(import);
    our @EXPORT = qw();
use File::Basename qw(dirname);

use pgBackRest::Backup::Common;
use pgBackRest::Backup::Info;
use pgBackRest::Common::Log;
use pgBackRest::Common::Ini;
use pgBackRest::Common::String;
use pgBackRest::Backup::Common;
use pgBackRest::Backup::Info;
use pgBackRest::Config::Config;
use pgBackRest::InfoCommon;
use pgBackRest::Manifest;
use pgBackRest::Protocol::Helper;
use pgBackRest::Protocol::Storage::Helper;
use pgBackRest::Storage::Helper;

####################################################################################################################################
# Info constants
####################################################################################################################################
use constant INFO_SECTION_BACKREST                                  => 'backrest';
use constant INFO_SECTION_ARCHIVE                                   => 'archive';
use constant INFO_SECTION_DB                                        => 'database';
use constant INFO_SECTION_INFO                                      => 'info';
use constant INFO_SECTION_REPO                                      => 'repository';
use constant INFO_SECTION_TIMESTAMP                                 => 'timestamp';
use constant INFO_SECTION_STATUS                                    => 'status';

use constant INFO_STANZA_NAME                                       => 'name';

use constant INFO_STANZA_STATUS_OK                                  => 'ok';
use constant INFO_STANZA_STATUS_ERROR                               => 'error';

use constant INFO_STANZA_STATUS_OK_CODE                             => 0;
use constant INFO_STANZA_STATUS_OK_MESSAGE                          => INFO_STANZA_STATUS_OK;
use constant INFO_STANZA_STATUS_MISSING_STANZA_CODE                 => 1;
use constant INFO_STANZA_STATUS_MISSING_STANZA_MESSAGE              => 'missing stanza path';
use constant INFO_STANZA_STATUS_NO_BACKUP_CODE                      => 2;
use constant INFO_STANZA_STATUS_NO_BACKUP_MESSAGE                   => 'no valid backups';

use constant INFO_KEY_CODE                                          => 'code';
use constant INFO_KEY_DELTA                                         => 'delta';
use constant INFO_KEY_FORMAT                                        => 'format';
use constant INFO_KEY_ID                                            => INFO_HISTORY_ID;
use constant INFO_KEY_LABEL                                         => 'label';
use constant INFO_KEY_MAX                                           => 'max';
use constant INFO_KEY_MIN                                           => 'min';
use constant INFO_KEY_MESSAGE                                       => 'message';
use constant INFO_KEY_PRIOR                                         => 'prior';
use constant INFO_KEY_REFERENCE                                     => 'reference';
use constant INFO_KEY_SIZE                                          => 'size';
use constant INFO_KEY_START                                         => 'start';
use constant INFO_KEY_STOP                                          => 'stop';
use constant INFO_KEY_SYSTEM_ID                                     => INFO_SYSTEM_ID;
use constant INFO_KEY_TYPE                                          => 'type';
use constant INFO_KEY_VERSION                                       => INFO_DB_VERSION;

####################################################################################################################################
# CONSTRUCTOR
####################################################################################################################################
sub new
{
    my $class = shift;       # Class name

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
# process
####################################################################################################################################
sub process
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my ($strOperation) = logDebugParam(__PACKAGE__ . '->process');

    # Get stanza if specified
    my $strStanza = optionTest(OPTION_STANZA) ? optionGet(OPTION_STANZA) : undef;

    # Get the stanza list with all info
    my $oyStanzaList = $self->stanzaList($strStanza);

    if (optionTest(OPTION_OUTPUT, INFO_OUTPUT_TEXT))
    {
        my $strOutput = $self->formatText($oyStanzaList);

        if (defined($strOutput))
        {
            syswrite(*STDOUT, $strOutput);
        }
        else
        {
            syswrite(*STDOUT, 'No stanzas exist in ' . storageRepo()->pathGet() . ".\n");
        }
    }
    elsif (optionTest(OPTION_OUTPUT, INFO_OUTPUT_JSON))
    {
        my $oJSON = JSON::PP->new()->canonical()->pretty()->indent_length(4);
        my $strJSON = $oJSON->encode($oyStanzaList);

        syswrite(*STDOUT, $strJSON);

        # On some systems a linefeed will be appended by encode() but others will not have it.  In our case there should always
        # be a terminating linefeed.
        if ($strJSON !~ /\n$/)
        {
            syswrite(*STDOUT, "\n");
        }
    }
    else
    {
        confess &log(ASSERT, "invalid info output option '" . optionGet(OPTION_OUTPUT) . "'");
    }

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'iResult', value => 0, trace => true}
    );
}

####################################################################################################################################
# formatText
#
# Format --output=text info.
####################################################################################################################################
sub formatText
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $oyStanzaList,
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->formatText', \@_,
            {name => 'oyStanzaList', trace => true},
        );

    my $strOutput;
    my $strArchiveNotPresent = "\n        wal archive min/max: none present\n";

    foreach my $oStanzaInfo (@{$oyStanzaList})
    {
        # Output stanza name and status
        $strOutput .= (defined($strOutput) ? "\n" : '')  . $self->formatTextStanza($oStanzaInfo) . "\n";

        my $bDbCurrent = true;

        # Reorder the DB history for the stanza from newest to oldest
        @{$oStanzaInfo->{&INFO_BACKUP_SECTION_DB}} = sort {$b cmp $a} @{$oStanzaInfo->{&INFO_BACKUP_SECTION_DB}};

        foreach my $hDbInfo (@{$oStanzaInfo->{&INFO_BACKUP_SECTION_DB}})
        {
            my $strArchiveOutput = undef;
            my $strBackupOutput = undef;
            my $strDbOutput = $bDbCurrent ? "\n    db (current)" : "\n    db (prior)";

            # Get the archive information for the DB
            foreach my $hDbArchive (@{$oStanzaInfo->{&INFO_SECTION_ARCHIVE}})
            {
                if ($hDbArchive->{&INFO_SECTION_DB}{&INFO_HISTORY_ID} == $hDbInfo->{&INFO_HISTORY_ID})
                {
                    # Output archive start / stop values
                    $strArchiveOutput = "\n        wal archive min/max (" . $hDbArchive->{&INFO_KEY_ID} . "): ";

                    if (defined($hDbArchive->{&INFO_KEY_MIN}))
                    {
                        $strArchiveOutput .= $hDbArchive->{&INFO_KEY_MIN} . ' / ' . $hDbArchive->{&INFO_KEY_MAX};
                    }
                    else
                    {
                        $strArchiveOutput .= 'none present';
                    }

                    $strArchiveOutput .= "\n";
                }
            }

            # Get information for each stanza backup for the DB, from oldest to newest
            foreach my $oBackupInfo (@{$$oStanzaInfo{&INFO_BACKUP_SECTION_BACKUP}})
            {
                if ($oBackupInfo->{&INFO_SECTION_DB}{&INFO_KEY_ID} == $hDbInfo->{&INFO_HISTORY_ID})
                {
                    $strBackupOutput .= "\n" . $self->formatTextBackup($oBackupInfo) . "\n";
                }
            }

            # If the archive info is not defined and this is the current database or if it is a prior database
            # and there is backup info, then set the output for the archive info to not present.
            if  (!defined($strArchiveOutput) && ($bDbCurrent || (!$bDbCurrent && defined($strBackupOutput))))
            {
                $strArchiveOutput = $strArchiveNotPresent;
            }

            # Add the sections to display to the output
            if (defined($strArchiveOutput))
            {
                $strOutput .= $strDbOutput . $strArchiveOutput . (defined($strBackupOutput) ? $strBackupOutput : '');
            }

            $bDbCurrent = false;
        }
    }

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'strOutput', value => $strOutput, trace => true}
    );
}

####################################################################################################################################
# formatTextStanza
#
# Format --output=text stanza info.
####################################################################################################################################
sub formatTextStanza
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $oStanzaInfo,
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->formatTextStanza', \@_,
            {name => 'oStanzaInfo', trace => true},
        );

    # Output stanza name and status
    my $strOutput =
        'stanza: ' . $oStanzaInfo->{&INFO_STANZA_NAME} . "\n" .
        "    status: " . ($oStanzaInfo->{&INFO_SECTION_STATUS}{&INFO_KEY_CODE} == 0 ? INFO_STANZA_STATUS_OK :
        INFO_STANZA_STATUS_ERROR . ' (' . $oStanzaInfo->{&INFO_SECTION_STATUS}{&INFO_KEY_MESSAGE} . ')');

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'strOutput', value => $strOutput, trace => true}
    );
}

####################################################################################################################################
# formatTextBackup
#
# Format --output=text backup info.
####################################################################################################################################
sub formatTextBackup
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $oBackupInfo,
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->formatTextBackup', \@_,
            {name => 'oBackupInfo', trace => true},
        );

    my $strOutput =
        '        ' . $$oBackupInfo{&INFO_KEY_TYPE} . ' backup: ' . $$oBackupInfo{&INFO_KEY_LABEL} . "\n" .

        '            timestamp start/stop: ' .
        timestampFormat(undef, $$oBackupInfo{&INFO_SECTION_TIMESTAMP}{&INFO_KEY_START}) .
        ' / ' .
        timestampFormat(undef, $$oBackupInfo{&INFO_SECTION_TIMESTAMP}{&INFO_KEY_STOP}) . "\n" .

        "            wal start/stop: ";

    if (defined($oBackupInfo->{&INFO_SECTION_ARCHIVE}{&INFO_KEY_START}) &&
        defined($oBackupInfo->{&INFO_SECTION_ARCHIVE}{&INFO_KEY_STOP}))
    {
        $strOutput .=
            $oBackupInfo->{&INFO_SECTION_ARCHIVE}{&INFO_KEY_START} . ' / ' . $oBackupInfo->{&INFO_SECTION_ARCHIVE}{&INFO_KEY_STOP};
    }
    else
    {
        $strOutput .= 'n/a';
    }

    $strOutput .=
        "\n            database size: " .
        (defined($$oBackupInfo{&INFO_SECTION_INFO}{&INFO_KEY_SIZE}) ?
            fileSizeFormat($$oBackupInfo{&INFO_SECTION_INFO}{&INFO_KEY_SIZE}) : '') .
        ', backup size: ' .
        (defined($$oBackupInfo{&INFO_SECTION_INFO}{&INFO_KEY_DELTA}) ?
            fileSizeFormat($$oBackupInfo{&INFO_SECTION_INFO}{&INFO_KEY_DELTA}) : '') . "\n" .

        '            repository size: ' .
        (defined($$oBackupInfo{&INFO_SECTION_INFO}{&INFO_SECTION_REPO}{&INFO_KEY_SIZE}) ?
            fileSizeFormat($$oBackupInfo{&INFO_SECTION_INFO}{&INFO_SECTION_REPO}{&INFO_KEY_SIZE}) : '') .
        ', repository backup size: ' .
        (defined($$oBackupInfo{&INFO_SECTION_INFO}{&INFO_SECTION_REPO}{&INFO_KEY_DELTA}) ?
            fileSizeFormat($$oBackupInfo{&INFO_SECTION_INFO}{&INFO_SECTION_REPO}{&INFO_KEY_DELTA}) : '');

    # List the backup reference chain, if any, for this backup
    if (defined($oBackupInfo->{&INFO_KEY_REFERENCE}) && @{$oBackupInfo->{&INFO_KEY_REFERENCE}} > 0)
    {
        $strOutput .= "\n            backup reference list: " . (join(', ', @{$$oBackupInfo{&INFO_KEY_REFERENCE}}));
    }

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'strOutput', value => $strOutput, trace => true}
    );
}

####################################################################################################################################
# stanzaList
####################################################################################################################################
sub stanzaList
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $strStanza
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->stanzaList', \@_,
            {name => 'strStanza', required => false}
        );

    my @oyStanzaList;

    # Run remotely
    if (!isRepoLocal())
    {
        @oyStanzaList = @{protocolGet(BACKUP)->cmdExecute(OP_INFO_STANZA_LIST, [$strStanza], true)};
    }
    # Run locally
    else
    {
        my @stryStanza = storageRepo()->list(CMD_BACKUP, {bIgnoreMissing => true});

        foreach my $strStanzaFound (@stryStanza)
        {
            if (defined($strStanza) && $strStanza ne $strStanzaFound)
            {
                next;
            }

            my $oStanzaInfo = {};
            $$oStanzaInfo{&INFO_STANZA_NAME} = $strStanzaFound;
            ($$oStanzaInfo{&INFO_BACKUP_SECTION_BACKUP}, $$oStanzaInfo{&INFO_BACKUP_SECTION_DB}) =
                $self->backupList($strStanzaFound);

            # If there are no backups then set status to no backup
            if (@{$$oStanzaInfo{&INFO_BACKUP_SECTION_BACKUP}} == 0)
            {
                $$oStanzaInfo{&INFO_SECTION_STATUS} =
                {
                    &INFO_KEY_CODE => INFO_STANZA_STATUS_NO_BACKUP_CODE,
                    &INFO_KEY_MESSAGE => INFO_STANZA_STATUS_NO_BACKUP_MESSAGE
                };
            }
            # Else status is OK
            else
            {
                $$oStanzaInfo{&INFO_SECTION_STATUS} =
                {
                    &INFO_KEY_CODE => INFO_STANZA_STATUS_OK_CODE,
                    &INFO_KEY_MESSAGE => INFO_STANZA_STATUS_OK_MESSAGE
                };
            }

            # Get the first/last WAL
            my $strArchiveStart;
            my $strArchiveStop;

            # Array to store tne min/max archive for each database for which there are archives
            my @oyDbArchiveList = ();

            # Loop through the DB history from oldest to newest
            foreach my $hDbInfo (@{$oStanzaInfo->{&INFO_BACKUP_SECTION_DB}})
            {
                $strArchiveStart = undef;
                $strArchiveStop = undef;

                my $strArchiveStanzaPath = "archive/" . $strStanzaFound;
                my $strDbVersion = $hDbInfo->{&INFO_KEY_VERSION};
                my $ullDbSysId = $hDbInfo->{&INFO_KEY_SYSTEM_ID};

                # With multiple DB versions, the backup.info history-id may not be the same as archive.info history-id, so the
                # archive path must be built by retrieving the archive id given the db version and system id of the backup
                my $oArchiveInfo = new pgBackRest::Archive::ArchiveInfo(storageRepo()->pathGet($strArchiveStanzaPath));
                my $strArchiveId = $oArchiveInfo->archiveId({strDbVersion => $hDbInfo->{&INFO_KEY_VERSION},
                    ullDbSysId => $hDbInfo->{&INFO_KEY_SYSTEM_ID}});
                my $strArchivePath = "archive/${strStanzaFound}/${strArchiveId}";

                if (storageRepo()->pathExists($strArchivePath))
                {
                    my @stryWalMajor = storageRepo()->list($strArchivePath, {strExpression => '^[0-F]{16}$'});

                    # Get first WAL segment
                    foreach my $strWalMajor (@stryWalMajor)
                    {
                        my @stryWalFile = storageRepo()->list(
                            "${strArchivePath}/${strWalMajor}",
                            {strExpression => "^[0-F]{24}-[0-f]{40}(\\." . COMPRESS_EXT . "){0,1}\$"});

                        if (@stryWalFile > 0)
                        {
                            $strArchiveStart = substr($stryWalFile[0], 0, 24);
                            last;
                        }
                    }

                    # Get last WAL segment
                    foreach my $strWalMajor (sort({$b cmp $a} @stryWalMajor))
                    {
                        my @stryWalFile = storageRepo()->list(
                            "${strArchivePath}/${strWalMajor}",
                            {strExpression => "^[0-F]{24}-[0-f]{40}(\\." . COMPRESS_EXT . "){0,1}\$", strSortOrder => 'reverse'});

                        if (@stryWalFile > 0)
                        {
                            $strArchiveStop = substr($stryWalFile[0], 0, 24);
                            last;
                        }
                    }

                    my $hDbArchive =
                    {
                        &INFO_KEY_ID => $strArchiveId,
                        &INFO_KEY_MIN => $strArchiveStart,
                        &INFO_KEY_MAX => $strArchiveStop,
                        &INFO_SECTION_DB =>
                        {
                            &INFO_HISTORY_ID => $hDbInfo->{&INFO_HISTORY_ID},
                        },
                    };

                    push(@oyDbArchiveList, $hDbArchive);
                }
            }

            # Store the archive min/max for each database in the archive section
            $oStanzaInfo->{&INFO_SECTION_ARCHIVE} = \@oyDbArchiveList;

            push @oyStanzaList, $oStanzaInfo;
        }

        if (defined($strStanza) && @oyStanzaList == 0)
        {
            my $oStanzaInfo = {};

            $$oStanzaInfo{&INFO_STANZA_NAME} = $strStanza;

            $$oStanzaInfo{&INFO_SECTION_STATUS} =
            {
                &INFO_KEY_CODE => INFO_STANZA_STATUS_MISSING_STANZA_CODE,
                &INFO_KEY_MESSAGE => INFO_STANZA_STATUS_MISSING_STANZA_MESSAGE
            };

            $$oStanzaInfo{&INFO_BACKUP_SECTION_BACKUP} = [];
            $$oStanzaInfo{&INFO_BACKUP_SECTION_DB} = [];

            push @oyStanzaList, $oStanzaInfo;
        }
    }

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'oyStanzaList', value => \@oyStanzaList, log => false, ref => true}
    );
}

####################################################################################################################################
# backupList
###################################################################################################################################
sub backupList
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $strStanza
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->backupList', \@_,
            {name => 'strStanza'}
        );

    # Load the backup.info but do not attempt to validate it or confirm it's existence
    my $oBackupInfo = new pgBackRest::Backup::Info(storageRepo()->pathGet(CMD_BACKUP . "/${strStanza}"), false, false, {bIgnoreMissing => true});

    # Build the db list
    my @oyDbList;

    foreach my $iHistoryId ($oBackupInfo->keys(INFO_BACKUP_SECTION_DB_HISTORY))
    {
        my $oDbHash =
        {
            &INFO_HISTORY_ID => $iHistoryId,
            &INFO_DB_VERSION =>
                $oBackupInfo->get(INFO_BACKUP_SECTION_DB_HISTORY, $iHistoryId, INFO_BACKUP_KEY_DB_VERSION),
            &INFO_SYSTEM_ID =>
                $oBackupInfo->get(INFO_BACKUP_SECTION_DB_HISTORY, $iHistoryId, INFO_BACKUP_KEY_SYSTEM_ID)
        };

        push(@oyDbList, $oDbHash);
    }

    # Build the backup list
    my @oyBackupList;

    foreach my $strBackup ($oBackupInfo->keys(INFO_BACKUP_SECTION_BACKUP_CURRENT))
    {
        my $oBackupHash =
        {
            &INFO_SECTION_ARCHIVE =>
            {
                &INFO_KEY_START =>
                    $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_ARCHIVE_START, false),
                &INFO_KEY_STOP =>
                    $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_ARCHIVE_STOP, false),
            },
            &INFO_SECTION_BACKREST =>
            {
                &INFO_KEY_FORMAT =>
                    $oBackupInfo->numericGet(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INI_KEY_FORMAT),
                &INFO_KEY_VERSION =>
                    $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INI_KEY_VERSION)
            },
            &INFO_SECTION_DB =>
            {
                &INFO_KEY_ID =>
                    $oBackupInfo->numericGet(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_HISTORY_ID)
            },
            &INFO_SECTION_INFO =>
            {
                &INFO_SECTION_REPO =>
                {
                    # Size of the backup in the repository, taking compression into account
                    &INFO_KEY_SIZE =>
                        $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_BACKUP_REPO_SIZE),
                    # Size of this backup only (does not include referenced backups like repo->size)
                    &INFO_KEY_DELTA =>
                        $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_BACKUP_REPO_SIZE_DELTA),
                },
                # Original database size
                &INFO_KEY_SIZE =>
                    $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_BACKUP_SIZE),
                # Amount of database backed up (will be equal for full backups)
                &INFO_KEY_DELTA =>
                    $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_BACKUP_SIZE_DELTA),
            },
            &INFO_SECTION_TIMESTAMP =>
            {
                &INFO_KEY_START =>
                    $oBackupInfo->numericGet(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_TIMESTAMP_START),
                &INFO_KEY_STOP =>
                    $oBackupInfo->numericGet(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_TIMESTAMP_STOP),
            },
            &INFO_KEY_LABEL => $strBackup,
            &INFO_KEY_PRIOR =>
                $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_PRIOR, false),
            &INFO_KEY_REFERENCE =>
                $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_REFERENCE, false),
            &INFO_KEY_TYPE =>
                $oBackupInfo->get(INFO_BACKUP_SECTION_BACKUP_CURRENT, $strBackup, INFO_BACKUP_KEY_TYPE)
        };

        push(@oyBackupList, $oBackupHash);
    }

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'oyBackupList', value => \@oyBackupList, log => false, ref => true},
        {name => 'oyDbList', value => \@oyDbList, log => false, ref => true}
    );
}

1;
