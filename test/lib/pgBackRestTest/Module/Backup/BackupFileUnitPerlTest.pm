####################################################################################################################################
# Tests for Backup File module
####################################################################################################################################
package pgBackRestTest::Module::Backup::BackupFileUnitPerlTest;
use parent 'pgBackRestTest::Env::HostEnvTest';

####################################################################################################################################
# Perl includes
####################################################################################################################################
use strict;
use warnings FATAL => qw(all);
use Carp qw(confess);

use File::Basename qw(dirname);
use Storable qw(dclone);

use pgBackRest::Backup::File;
use pgBackRest::Common::Exception;
use pgBackRest::Common::Ini;
use pgBackRest::Common::Log;
use pgBackRest::Common::String;
use pgBackRest::Common::Wait;
use pgBackRest::Config::Config;
use pgBackRest::DbVersion;
use pgBackRest::Manifest;
use pgBackRest::Protocol::Helper;
use pgBackRest::Protocol::Storage::Helper;
use pgBackRest::Storage::Helper;

use pgBackRestTest::Common::ExecuteTest;
use pgBackRestTest::Common::RunTest;
use pgBackRestTest::Env::Host::HostBackupTest;

####################################################################################################################################
# initModule
####################################################################################################################################
sub initModule
{
    my $self = shift;

    $self->{strDbPath} = $self->testPath() . '/db';
    $self->{strRepoPath} = $self->testPath() . '/repo';
    $self->{strBackupPath} = "$self->{strRepoPath}/backup/" . $self->stanza();
    $self->{strPgControl} = $self->{strDbPath} . '/' . DB_FILE_PGCONTROL;

    # Create backup path
    storageTest()->pathCreate($self->{strBackupPath}, {bIgnoreExists => true, bCreateParent => true});

    # Generate pg_control file
    storageTest()->pathCreate($self->{strDbPath} . '/' . DB_PATH_GLOBAL, {bCreateParent => true});
    $self->controlGenerate($self->{strDbPath}, PG_VERSION_94);
}

####################################################################################################################################
# run
####################################################################################################################################
sub run
{
    my $self = shift;

    $self->optionTestSet(CFGOPT_STANZA, $self->stanza());
    $self->optionTestSet(CFGOPT_PG_PATH, $self->{strDbPath});
    $self->optionTestSet(CFGOPT_REPO_PATH, $self->{strRepoPath});
    $self->optionTestSet(CFGOPT_LOG_PATH, $self->testPath());

    $self->optionTestSetBool(CFGOPT_ONLINE, false);

    $self->optionTestSet(CFGOPT_DB_TIMEOUT, 5);
    $self->optionTestSet(CFGOPT_PROTOCOL_TIMEOUT, 6);
	$self->optionTestSet(CFGOPT_COMPRESS_LEVEL, 3);

	$self->configTestLoad(CFGCMD_BACKUP);

	# Repo
	my $strRepoBackupPath = storageRepo()->pathGet(STORAGE_REPO_BACKUP);
    my $strBackupLabel = "20180724-182750F";

	# File
    my $strFileName = "12345";
    my $strFileDb = $self->{strDbPath} . "/$strFileName";
    my $strFileHash = '1c7e00fd09b9dd11fc2966590b3e3274645dd031';
	my $strFileRepo = storageRepo()->pathGet(
		STORAGE_REPO_BACKUP . "/$strBackupLabel/" . MANIFEST_TARGET_PGDATA . "/$strFileName");
    my $strRepoFile = MANIFEST_TARGET_PGDATA . "/$strFileName";
    my $strRepoPgControl = MANIFEST_FILE_PGCONTROL;
	my $strPgControlRepo = storageRepo()->pathGet(STORAGE_REPO_BACKUP . "/$strBackupLabel/$strRepoPgControl");
    my $strPgControlHash = 'b4a3adade1e81ebfc7e9a27bca0887a347d81522';

	# Copy file to db path
    executeTest('cp ' . $self->dataPath() . "/filecopy.archive2.bin ${strFileDb}");

	# Get size and data info for the files in the db path
    my $hManifest = storageDb()->manifest($self->{strDbPath});
    my $lFileSize = $hManifest->{$strFileName}{size} + 0;
    my $lFileTime = $hManifest->{$strFileName}{modification_time} + 0;
    my $lPgControlSize = $hManifest->{&DB_FILE_PGCONTROL}{size} + 0;
    my $lPgControlTime = $hManifest->{&DB_FILE_PGCONTROL}{modification_time} + 0;

    my $strBackupPath = $self->{strBackupPath} . "/$strBackupLabel";
    my $strHost = "host";
    my $iLocalId = 1;

    # Initialize the manifest
    my $oBackupManifest = new pgBackRest::Manifest("$strBackupPath/" . FILE_MANIFEST,
        {bLoad => false, strDbVersion => PG_VERSION_94, iDbCatalogVersion => 201409291});
    $oBackupManifest->build(storageDb(), $self->{strDbPath}, undef, true);

    # Set the initial size values for backupManifestUpdate - running size and size for when to save the file
    my $lSizeCurrent = 0;
    my $lSizeTotal = 16785408;
    my $lManifestSaveCurrent = 0;
    my $lManifestSaveSize = int($lSizeTotal / 100);


    ################################################################################################################################
    if ($self->begin('backupFile(), backupManifestUpdate()'))
    {
		# Result variables
        my $iResultCopyResult;
        my $lResultCopySize;
        my $lResultRepoSize;
        my $strResultCopyChecksum;
        my $rResultExtra;

		#---------------------------------------------------------------------------------------------------------------------------
        # Copy pg_control and confirm manifestUpdate does not save the manifest yet
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile($self->{strPgControl}, MANIFEST_FILE_PGCONTROL, $lPgControlSize, undef, false, $strBackupLabel, false,
			cfgOption(CFGOPT_COMPRESS_LEVEL), $lPgControlTime, true, undef, false, false, undef);

		$self->testResult(sub {storageTest()->exists($strPgControlRepo)}, true, 'pg_control file exists in repo');

		$self->testResult(($iResultCopyResult == BACKUP_FILE_COPY && $strResultCopyChecksum eq $strPgControlHash &&
			$lResultCopySize == $lPgControlSize && $lResultRepoSize == $lPgControlSize), true,
            'pg_control file copied to repo successfully');

        ($lSizeCurrent, $lManifestSaveCurrent) = backupManifestUpdate(
                $oBackupManifest,
                $strHost,
                $iLocalId,
                $self->{strPgControl},
                $strRepoPgControl,
                $lPgControlSize,
                undef,
                false,
                $iResultCopyResult,
                $lResultCopySize,
                $lResultRepoSize,
                $strResultCopyChecksum,
                $rResultExtra,
                $lSizeTotal,
                $lSizeCurrent,
                $lManifestSaveSize,
                $lManifestSaveCurrent);

        # Accumulators should be same size as pg_control
        $self->testResult(($lSizeCurrent == $lPgControlSize && $lManifestSaveCurrent == $lPgControlSize), true,
            "file size in repo and repo size equal pg_control size");

        $self->testResult(sub {$oBackupManifest->test(MANIFEST_SECTION_TARGET_FILE, MANIFEST_FILE_PGCONTROL,
            MANIFEST_SUBKEY_CHECKSUM, $strPgControlHash)}, true, "manifest updated for pg_control");

        # Neither backup.manifest nor backup.manifest.copy written because size threshold not met
        $self->testException(sub {storageRepo()->openRead("$strBackupPath/" . FILE_MANIFEST . INI_COPY_EXT)}, ERROR_FILE_MISSING,
            "unable to open '$strBackupPath/" . FILE_MANIFEST . INI_COPY_EXT . "': No such file or directory");
        $self->testException(sub {storageRepo()->openRead("$strBackupPath/" . FILE_MANIFEST)}, ERROR_FILE_MISSING,
            "unable to open '$strBackupPath/" . FILE_MANIFEST . "': No such file or directory");

		#---------------------------------------------------------------------------------------------------------------------------
		# No prior checksum, no compression, no page checksum, no extra, no delta, no hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile($strFileDb, MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize, undef, false, $strBackupLabel, false,
			cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, false, false, undef);

		$self->testResult(sub {storageTest()->exists($strFileRepo)}, true, 'non-compressed file exists in repo');

		$self->testResult(($iResultCopyResult == BACKUP_FILE_COPY && $strResultCopyChecksum eq $strFileHash &&
			$lResultCopySize == $lFileSize && $lResultRepoSize == $lFileSize), true,
            'file copied to repo successfully');

        $self->testException(sub {storageRepo()->openRead("$strFileRepo.gz")}, ERROR_FILE_MISSING,
            "unable to open '$strFileRepo.gz': No such file or directory");

        ($lSizeCurrent, $lManifestSaveCurrent) = backupManifestUpdate(
                $oBackupManifest,
                $strHost,
                $iLocalId,
                $strFileDb,
                $strRepoFile,
                $lFileSize,
                $strFileHash,
                false,
                $iResultCopyResult,
                $lResultCopySize,
                $lResultRepoSize,
                $strResultCopyChecksum,
                $rResultExtra,
                $lSizeTotal,
                $lSizeCurrent,
                $lManifestSaveSize,
                $lManifestSaveCurrent);

        # Accumulator includes size of pg_control and file. Manifest saved so ManifestSaveCurrent returns to 0
        $self->testResult(($lSizeCurrent == ($lPgControlSize + $lFileSize) && $lManifestSaveCurrent == 0), true,
            "repo size increased and ManifestSaveCurrent returns to 0");

        $self->testResult(sub {$oBackupManifest->test(MANIFEST_SECTION_TARGET_FILE, $strRepoFile,
            MANIFEST_SUBKEY_CHECKSUM, $strFileHash)}, true, "manifest updated for $strRepoFile");

        # Backup.manifest not written but backup.manifest.copy written because size threshold  met
		$self->testResult(sub {storageTest()->exists("$strBackupPath/" . FILE_MANIFEST . INI_COPY_EXT)}, true,
            'backup.manifest.copy exists in repo');
        $self->testException(sub {storageRepo()->openRead("$strBackupPath/" . FILE_MANIFEST)}, ERROR_FILE_MISSING,
            "unable to open '$strBackupPath/" . FILE_MANIFEST . "': No such file or directory");

		storageTest()->remove($strFileRepo);
		storageTest()->remove($strPgControlRepo);

		#---------------------------------------------------------------------------------------------------------------------------
		# No prior checksum, yes compression, yes page checksum, no extra, no delta, no hasReference
		$self->testException(sub {backupFile($strFileDb, MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize, undef, true,
			$strBackupLabel, true, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, false, false, undef)}, ERROR_ASSERT,
			"iWalId is required in Backup::Filter::PageChecksum->new");

	    # Build the lsn start parameter to pass to the extra function
	    my $hStartLsnParam =
	    {
	        iWalId => 0xFFFF,
	        iWalOffset => 0xFFFF,
	    };

		#---------------------------------------------------------------------------------------------------------------------------
		# No prior checksum, yes compression, yes page checksum, yes extra, no delta, no hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile($strFileDb, MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize, undef, true, $strBackupLabel, true,
			cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, $hStartLsnParam, false, false, undef);

		$self->testResult(sub {storageTest()->exists("$strFileRepo.gz")}, true, 'compressed file exists in repo');

		$self->testResult(($iResultCopyResult == BACKUP_FILE_COPY && $strResultCopyChecksum eq $strFileHash &&
			$lResultRepoSize < $lFileSize && $rResultExtra->{bValid}), true, 'file copied to repo successfully');

        # Only the compressed version of the file exists
        $self->testException(sub {storageRepo()->openRead("$strFileRepo")}, ERROR_FILE_MISSING,
            "unable to open '$strFileRepo': No such file or directory");

        # Save the compressed file for later test
		executeTest('mv ' . "$strFileRepo.gz $strFileRepo.gz.SAVE");

		#---------------------------------------------------------------------------------------------------------------------------
		# Add a segment number for bChecksumPage code coverage
		executeTest('cp ' . "$strFileDb $strFileDb.1");

		# No prior checksum, no compression, yes page checksum, yes extra, no delta, no hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb.1", MANIFEST_TARGET_PGDATA . "/$strFileName.1", $lFileSize, undef, true, $strBackupLabel, false,
			cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, $hStartLsnParam, false, false, undef);

		$self->testResult(sub {storageTest()->exists("$strFileRepo.1")}, true, 'non-compressed segment file exists in repo');

		$self->testResult(($iResultCopyResult == BACKUP_FILE_COPY && $strResultCopyChecksum eq $strFileHash &&
			$lResultRepoSize == $lFileSize && $rResultExtra->{bValid}), true, 'segment file copied to repo successfully');

		#---------------------------------------------------------------------------------------------------------------------------
		# Remove the db file and try to back it up
		storageTest()->remove("$strFileDb.1");

		# No prior checksum, no compression, no page checksum, no extra, No delta, no hasReference, no db file
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb.1", MANIFEST_TARGET_PGDATA . "/$strFileName.1", $lFileSize, undef, false, $strBackupLabel,
            false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, false, false, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_SKIP && !defined($strResultCopyChecksum) &&
			!defined($lResultRepoSize)), true, 'backup file skipped');

		# Yes prior checksum, no compression, no page checksum, no extra, yes delta, no hasReference, no db file
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb.1", MANIFEST_TARGET_PGDATA . "/$strFileName.1", $lFileSize, $strFileHash, false, $strBackupLabel,
            false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, true, false, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_SKIP && !defined($strResultCopyChecksum) &&
			!defined($lResultRepoSize)), true, 'backup file skipped - in prior backup');

		# Yes prior checksum, no compression, no page checksum, no extra, yes delta, no hasReference, no db file,
        # do not ignoreMissing
		$self->testException(sub {backupFile("$strFileDb.1", MANIFEST_TARGET_PGDATA . "/$strFileName.1", $lFileSize, $strFileHash,
            false, $strBackupLabel, false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, false, undef, true, false, undef)},
            ERROR_FILE_MISSING, "unable to open '$strFileDb.1': No such file or directory");

		#---------------------------------------------------------------------------------------------------------------------------
        # Restore the compressed file
        executeTest('mv ' . "$strFileRepo.gz.SAVE $strFileRepo.gz");

		# Yes prior checksum, yes compression, no page checksum, no extra, yes delta, no hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb", MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize, $strFileHash, false, $strBackupLabel,
            true, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, true, false, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_CHECKSUM && $strResultCopyChecksum eq $strFileHash &&
			$lResultCopySize == $lFileSize), true, 'db checksum and repo same - no copy file');

		#---------------------------------------------------------------------------------------------------------------------------
        # DB Checksum mismatch
        storageTest()->remove("$strFileRepo", {bIgnoreMissing => true});
        # Save the compressed file for later test
		executeTest('mv ' . "$strFileRepo.gz $strFileRepo.gz.SAVE");

		# Yes prior checksum, no compression, no page checksum, no extra, yes delta, no hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb", MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize, $strFileHash . "ff", false,
            $strBackupLabel, false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, true, false, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_CHECKSUM && $strResultCopyChecksum eq $strFileHash &&
			$lResultCopySize == $lFileSize && $lResultRepoSize == $lFileSize), true, 'db checksum mismatch - copy file');

		#---------------------------------------------------------------------------------------------------------------------------
        # DB file size mismatch
		# Yes prior checksum, no compression, no page checksum, no extra, yes delta, no hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb", MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize + 1, $strFileHash, false,
            $strBackupLabel, false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, true, false, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_CHECKSUM && $strResultCopyChecksum eq $strFileHash &&
			$lResultCopySize == $lFileSize && $lResultRepoSize == $lFileSize), true, 'db file size mismatch - copy file');

		#---------------------------------------------------------------------------------------------------------------------------
        # Repo mismatch

        # Restore the compressed file as if non-compressed so checksum won't match
        executeTest('cp ' . "$strFileRepo.gz.SAVE $strFileRepo");

		# Yes prior checksum, no compression, no page checksum, no extra, yes delta, no hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb", MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize, $strFileHash, false,
            $strBackupLabel, false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, true, false, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_RECOPY && $strResultCopyChecksum eq $strFileHash &&
			$lResultCopySize == $lFileSize && $lResultRepoSize == $lFileSize), true, 'repo checksum mismatch - recopy file');

        # Restore the compressed file
        executeTest('mv ' . "$strFileRepo.gz.SAVE $strFileRepo.gz");

		# Yes prior checksum, yes compression, no page checksum, no extra, no delta, no hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb", MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize + 1, $strFileHash, false,
            $strBackupLabel, true, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, false, false, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_RECOPY && $strResultCopyChecksum eq $strFileHash &&
			$lResultCopySize == $lFileSize), true, 'repo size mismatch - recopy file');

		#---------------------------------------------------------------------------------------------------------------------------
        # Has reference

		# Yes prior checksum, no compression, no page checksum, no extra, yes delta, yes hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb", MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize + 1, $strFileHash, false,
            $strBackupLabel, false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, true, true, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_CHECKSUM && $strResultCopyChecksum eq $strFileHash &&
			$lResultCopySize == $lFileSize && $lResultRepoSize == $lFileSize), true, 'db file size mismatch has reference - copy');

		# Yes prior checksum, no compression, no page checksum, no extra, yes delta, yes hasReference
		($iResultCopyResult, $lResultCopySize, $lResultRepoSize, $strResultCopyChecksum, $rResultExtra) =
			backupFile("$strFileDb", MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize, $strFileHash, false,
            $strBackupLabel, false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, true, true, undef);

		$self->testResult(($iResultCopyResult == BACKUP_FILE_NOOP && $strResultCopyChecksum eq $strFileHash &&
			$lResultCopySize == $lFileSize), true, 'db file same has reference - copy');

		#---------------------------------------------------------------------------------------------------------------------------
        # Remove file from repo. No reference so should hard error since this means sometime between the building of the manifest
        # for the aborted backup, the file went missing from the aborted backup dir.
        storageTest()->remove("$strFileRepo", {bIgnoreMissing => true});

        $self->testException(sub {backupFile("$strFileDb", MANIFEST_TARGET_PGDATA . "/$strFileName", $lFileSize, $strFileHash,
            false, $strBackupLabel, false, cfgOption(CFGOPT_COMPRESS_LEVEL), $lFileTime, true, undef, true, false, undef)},
            ERROR_FILE_MISSING, "unable to open '$strFileRepo': No such file or directory");
    }

    ################################################################################################################################
    if ($self->begin('backupManifestUpdate()'))
    {
# {name => 'oManifest', trace => true},
# {name => 'strHost', required => false, trace => true},  -- this is for log statements only so can be undef
# {name => 'iLocalId', required => false, trace => true}, -- only for a single log statement
#
# # Parameters to backupFile()
# {name => 'strDbFile', trace => true}, -- this is for log statements but is required
# {name => 'strRepoFile', trace => true},
# {name => 'lSize', required => false, trace => true},
# {name => 'strChecksum', required => false, trace => true},
# {name => 'bChecksumPage', trace => true},
#
# # Results from backupFile()
# {name => 'iCopyResult', trace => true},
# {name => 'lSizeCopy', required => false, trace => true},
# {name => 'lSizeRepo', required => false, trace => true},
# {name => 'strChecksumCopy', required => false, trace => true},
# {name => 'rExtra', required => false, trace => true},
#
# # Accumulators
# {name => 'lSizeTotal', trace => true},
# {name => 'lSizeCurrent', trace => true},
# {name => 'lManifestSaveSize', trace => true},
# {name => 'lManifestSaveCurrent', trace => true}
$iLocalId = 2;

    }
}

1;
