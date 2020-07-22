/***********************************************************************************************************************************
Verify Command
***********************************************************************************************************************************/
#include "build.auto.h"

#include "command/archive/common.h"
#include "command/check/common.h"
#include "command/verify/file.h"
#include "command/verify/protocol.h"
#include "common/compress/helper.h"
#include "common/crypto/cipherBlock.h"
#include "common/debug.h"
#include "common/io/io.h"
#include "common/log.h"
#include "config/config.h"
#include "info/infoArchive.h"
#include "info/infoBackup.h"
#include "postgres/interface.h"
#include "postgres/version.h"
#include "protocol/helper.h"
#include "protocol/parallel.h"
#include "storage/helper.h"

/* https://github.com/pgbackrest/pgbackrest/issues/1032
Add a command to verify the contents of the repository. By the default the command will:

    * Verify that backup/archive.info and their copies are valid
        - May have to expose infoBackupNewLoad so can load the backup.info and then the backup.info.copy and compare the 2 - need to know if one is corrupt and always failing over to the other.
        - want to WARN if only one exists
        - If backup.info/copy AND/OR archive.info/copy is missing then do not abort. ONLY error and abort if encrypted since we can't read the archive dir or backup dirs
        - checkStanzaInfoPg() makes sure the archive and backup info files (not copies just that one or the other) exist and are valid for the database version - BUT the db being available is not allowed because restore doesn't require it - so maybe just check archive and backup info DB sections match each other. BUT this throws an error if can't open one or the other - so would need to catch this and do LOG_ERROR() - maybe prefix with VERIFY-ERROR - or just take out the conditions and check for what we need OR maybe call checkStanzaInfo
        - We should probably check archive and backup history lists match (expire checks that the backup.info history contains at least the same history as archive.info (it can have more)) since we need to get backup db-id needs to be able to translate exactly to the archiveId (i.e. db-id=2, db-version=9.6 in backu.info must translate to archiveId 9.6-2.
        - If archive-copy set the WAL can be in the backup dirs so just because archive.info is missing, don't want to error

    * Reconstruct backup.info and WARN on mismatches - NO because this doesn't really help us since we do not want to take a lock which means things can be added/removed as we're verifying. So instead:
        - Get a list of backup on disk and a list of archive ids and pass that to job data
        - When checking backups, if the label is missing then skip (probably expired from underneath us) and then if it is there but only the manifest.copy exists, then skip (could be aborted or in progress so can't really check anything)
        - It is possible to have a backup without all the WAL if archive-check is not set but in this is not on then all bets are off

    * Check for missing WAL and backup files
        - This would be where we'd need to start the parallel executor, right? YES
        - At this point, we would have valid info files, so, starting with the WAL, get start/stop list for each archiveId so maybe a structure with archiveID and then a start/stop list for gaps? e.g.
            [0] 9.4-1
                [0] start, stop
                                <--- gap
                [1] start, stop
            [1] 10-2
                [0] start, stop

            BUT what does "missing WAL" mean - there can be gaps so "missing" is only if it is expected to be there for a backup to be consistent
        - The filename always include the WAL segment (timeline?) so all I need is the filename. BUT if I have to verify the checksums, so should I do it in the verifyFile function

    * Verify all manifests/copies are valid
        - Both the manifest and manifest.copy exist, loadable and are identical. WARN if any condition is false (this should be in the jobCallback)

    * Verify the checksum of all WAL/backup files
        - Pass the checksum to verifyFile - this needs to be stripped off from file in WAL but for backup it must be read from the manifest (which we need to read in the jobCallback
        - Skip checking size for WAL file but should check for size equality with the backup files - if one or the other doesn't match, then corrupt
        - in verifyFile() function be sure to add IoSync filter and then do IoReadDrain and then look at the results of the filters. See restore/file.c lines 85 though 87 (although need more filters e.g. decrypt, decompress, size, sha, sync).

            // Generate checksum for the file if size is not zero
            IoRead *read = NULL;

                read = storageReadIo(storageNewReadP(storageRepo(), filename));
                IoFilterGroup *filterGroup = ioReadFilterGroup(read);

                // Add decryption filter
                if (cipherPass != NULL)
                    ioFilterGroupAdd(filterGroup, cipherBlockNew(cipherModeDecrypt, cipherTypeAes256Cbc, BUFSTR(cipherPass), NULL));


                // Add decompression filter
                if (repoFileCompressType != compressTypeNone)
                    ioFilterGroupAdd(filterGroup, decompressFilter(repoFileCompressType));

                // Add sha1 filter
                ioFilterGroupAdd(filterGroup, cryptoHashNew(HASH_TYPE_SHA1_STR));

                // Add size filter
                ioFilterGroupAdd(filterGroup, ioSizeNew());

                // Add IoSink - this just throws away data so we don't want to move the data from the remote. The data is thrown away after the file is read, checksummed, etc
                ioFilterGroupAdd(filterGroup, ioSinkNew());

                ioReadDrain(read); // this will throw away if ioSink was not implemented

                // Then filtergroup get result to validate checksum
                if (!strEq(pgFileChecksum, varStr(ioFilterGroupResult(filterGroup, CRYPTO_HASH_FILTER_TYPE_STR))))

        - For the first pass we only check that all the files in the manifest are on disk. Future we may also WARN if there are files that are on disk but not in the manifest.
        - verifyJobResult() would check the status of the file (bad) and can either add to a corrupt list or add gaps in the WAL (probaby former) - I need to know what result I was coming from so that is the jobKey - maybe we use the filename as the key - does it begin with Archive then it is WAL, else it's a backup (wait, what about manifest) - maybe we can make the key a variant instead of a string. Need list of backups start/stop to be put into jobData and use this for final reconciliation.
        - Would this be doing mostly what restoreFile() does without the actualy copy? But we have to "sort-of" copy it (you mentioned a technique where we can just throw it away and not write to disk) to get the correct size and checksum...
        - If a corrupt WAL file then warn/log that we have a "corrupt/missing" file and remove it from the range list so it will cause a "gap" then when checking backup it will log another error that the backup that relies on that particular WAL is not valid

    * Ignore files with references when every backup will be validated, otherwise, check each file referenced to another file in the backup set. For example, if ask for an incremental backup verify, then the manifest may have references to files in another (prior) backup so go check them.

The command can be modified with the following options:

    --set - only verify the specified backup and associated WAL (including PITR)
    --no-pitr only verify the WAL required to make the backup consistent.
    --fast - check only that the file is present and has the correct size.

We would like these additional features, but are willing to release the first version without them:

    * WARN on extra files
    * Check that history files exist for timelines after the first (the first timeline will not have a history file if it is the oldest timeline ever written to the repo)
    * Check that copies of the manifests exist in backup.history

Questions/Concerns
- How much crossover with the Check command are we anticipating?
- The command can be run locally or remotely, so from any pgbackrest host, right? YES
- Data.pm: ALL OF THESE ARE NO
    * Will we be verifying links? Does the cmd_verify need to be in Data.pm CFGOPT_LINK_ALL, CFGOPT_LINK_MAP
    * Same question for tablespaces
- How to check WAL for PITR (e.g. after end of last backup?)? ==> If doing async archive and process max = 8 then could be 8 missing. But we don't have access to process-max and we don't know if they had asynch archiving, so if we see gaps in the last 5 minutes of the WAL stream and the last backup stop WAL is there, then we'll just ignore that PITR has gaps. MUST always assume is we don't have access to the configuration.
*/
/**********************************************************************************************************************************/
// typedef enum // CSHANG Don't think I will need
// {
//     verifyWal,
//     verifyBackup,
// } VerifyState;
//
// typedef struct VerifyData
// {
//     bool fast;                                                      // Has the fast option been requested
//     VerifyState state; // CSHANG Don't think I need this as I will just be checking the Lists and removing(?) each item verified
//         // CSHANG list of WAL / WAL directories / archiveId to check
//         StringList *archiveIdList;
//         StringList *walPathList;
//         StringList *walFileList;
//
//         // CSHANG list of Backup directories/files to check
//         StringList *backupList;
//         StringList *backupFileList;
// // } VerifyData;

// static ProtocolParallelJob *
// verifyJobCallback(void *data, unsigned int clientIdx)
// {
//     FUNCTION_TEST_BEGIN();
//         FUNCTION_TEST_PARAM_P(VOID, data);
//         FUNCTION_TEST_PARAM(UINT, clientIdx);
//     FUNCTION_TEST_END();
//
//     ASSERT(data != NULL); -- or can it be - like when there is no more data
//
//     // No special logic based on the client, we'll just get the next job
//     (void)clientIdx;
//
//     // Get a new job if there are any left
//     VerifyData *jobData = data;
// if archiveIDlist ==Null
//     go get a list of archiveid.
//     done.
//
// if (walPathList size == 0)
//     go read the walpath
//     add all walPathlist
//     remove archiveid from list
//
// if walPathlist > 0
//     go read the walPathlist[i]
//     add all walFilelist
//     Here we check for gaps? Then when check for backup consistency, we warn if a specific backup can't do pitr or can do pitr
//     remove walPathlist[i] from list
//
// maybe have some info log (like "checking archinve id 9.4-1" and then detail level logging could in addition list the archive files being checked).
//
//     if (walFilelist)
//     {
//         process the 1 walFile from the list
//         remove that from the list
//
//         below are the guts from the PROTOCOL_COMMAND_ARCHIVE_GET_STR
//
//         const String *walSegment = strLstGet(jobData->walSegmentList, jobData->walSegmentIdx);
//         jobData->walSegmentIdx++;
//
//         ProtocolCommand *command = protocolCommandNew(PROTOCOL_COMMAND_ARCHIVE_GET_STR); // CSHANG Replace with PROTOCOL_COMMAND_VERIFY_FILE_STR
//         protocolCommandParamAdd(command, VARSTR(walSegment));
//
//         FUNCTION_TEST_RETURN(protocolParallelJobNew(VARSTR(walSegment), command));
//     }
// else // OR maybe just have a verifyFile - may be that it just takes a file name and it doesn't know what type of file it is - like pass it the compress type, encryption, etc - whatever it needs to read the file. Make the verifyFile dumb. There is a funtion to get the compression type. BUT can't test compression on the files in backup because need to read the manifest to determine that.
//     if (backupList)
//         {
//             next backup thing to process
//
//             FUNCTION_TEST_RETURN(protocolParallelJobNew(VARSTR(walSegment), command));
//         }
//
//     FUNCTION_TEST_RETURN(NULL);
// }
//
// verifyJobResult(...)
// {
//     // Check that the job was successful
//     if (protocolParallelJobErrorCode(job) == 0)
//     {
//         MEM_CONTEXT_TEMP_BEGIN()
//         {
//             THISMAYBEASTRUCT result = varSOMETHING(protocolParallelJobResult(job));
// // CSHANG This is just an example - we need to do logging based on INFO for what we're checking, DETAIL for each files checked, then BACKUP success, WAL success would also be INFO, WARN logLevelWarn or ERROR logLevelError
//             LOG_PID(logLevelInfo, protocolParallelJobProcessId(job), 0, strPtr(log));
//         }
//         MEM_CONTEXT_TEMP_END();
//
//         // Free the job
//         protocolParallelJobFree(job);
//     }
//     else
//         THROW_CODE(protocolParallelJobErrorCode(job), strPtr(protocolParallelJobErrorMessage(job)));
// }

static StorageRead *
verifyFileLoad(const String *fileName, const String *cipherPass)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRING, fileName);
        FUNCTION_TEST_PARAM(STRING, cipherPass);
    FUNCTION_TEST_END();

// CSHANG But how does this type of reading help with manifest? Won't we still be pulling in the entire file into memory to get the checksum or will I need to chunk it up and add all the checksums together?

    // Read the file and error if missing
    StorageRead *result = storageNewReadP(storageRepo(), fileName);
    IoRead *read = storageReadIo(result);
    cipherBlockFilterGroupAdd(
        ioReadFilterGroup(read), cipherType(cfgOptionStr(cfgOptRepoCipherType)), cipherModeDecrypt, cipherPass);
    ioFilterGroupAdd(ioReadFilterGroup(read), cryptoHashNew(HASH_TYPE_SHA1_STR));

    // If the file is compressed, add a decompression filter
    if (compressTypeFromName(fileName) != compressTypeNone)
        ioFilterGroupAdd(ioReadFilterGroup(read), decompressFilter(compressTypeFromName(fileName)));

    FUNCTION_TEST_RETURN(result);
}

/***********************************************************************************************************************************
Get status of info files in repository
***********************************************************************************************************************************/
#define FUNCTION_LOG_VERIFY_INFO_FILE_RESULT_TYPE                                                                                  \
    VerifyInfoFile
#define FUNCTION_LOG_VERIFY_INFO_FILE_RESULT_FORMAT(value, buffer, bufferSize)                                                     \
    objToLog(&value, "VerifyInfoFile", buffer, bufferSize)

typedef struct VerifyInfoFile
{
    InfoBackup *backup;                                             // Backup.info file contents
    InfoArchive *archive;                                           // Archive.info file contents
    const String *checksum;                                         // File checksum
    int errorCode;                                                  // Error code else 0 for no error
} VerifyInfoFile;

static VerifyInfoFile
verifyInfoFile(const String *infoPathFile, bool loadFile, const String *cipherPass)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(STRING, infoPathFile);
        FUNCTION_LOG_PARAM(BOOL, loadFile);
        FUNCTION_TEST_PARAM(STRING, cipherPass);
    FUNCTION_LOG_END();

    ASSERT(infoPathFile != NULL);

    VerifyInfoFile result = {.errorCode = 0};

    MEM_CONTEXT_TEMP_BEGIN()
    {
        TRY_BEGIN()
        {
            IoRead *infoRead = storageReadIo(verifyFileLoad(infoPathFile, cipherPass));

            if (loadFile)
            {
                if (strBeginsWith(infoPathFile, INFO_BACKUP_PATH_FILE_STR))
                {
                    result.backup = infoBackupMove(infoBackupNewLoad(infoRead), memContextPrior());
                }
                else
                {
                    result.archive = infoArchiveMove(infoArchiveNewLoad(infoRead), memContextPrior());
                }
            }
            else
                ioReadDrain(infoRead);

            MEM_CONTEXT_PRIOR_BEGIN()
            {
                result.checksum = strDup(varStr(ioFilterGroupResult(ioReadFilterGroup(infoRead), CRYPTO_HASH_FILTER_TYPE_STR)));
            }
            MEM_CONTEXT_PRIOR_END();
        }
        CATCH_ANY()
        {
            result.errorCode = errorCode();
            String *errorMsg = strNew(errorMessage());

            if (result.errorCode == errorTypeCode(&ChecksumError))
                strCat(errorMsg, strNewFmt(" %s", strPtr(infoPathFile)));

            LOG_WARN(strPtr(errorMsg));
        }
        TRY_END();
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(VERIFY_INFO_FILE_RESULT, result);
}

static InfoArchive *
verifyArchiveInfoFile(void)
{
    FUNCTION_LOG_VOID(logLevelDebug);

    InfoArchive *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Get the main info file
        VerifyInfoFile verifyArchiveInfo = verifyInfoFile(INFO_ARCHIVE_PATH_FILE_STR, true, cfgOptionStrNull(cfgOptRepoCipherPass));

        // If the main file did not error, then report on the copy's status and check checksums
        if (verifyArchiveInfo.errorCode == 0)
        {
            result = verifyArchiveInfo.archive;
            infoArchiveMove(result, memContextPrior());

            // Attempt to load the copy and report on it's status but don't keep it in memory
            VerifyInfoFile verifyArchiveInfoCopy = verifyInfoFile(
                INFO_ARCHIVE_PATH_FILE_COPY_STR, false, cfgOptionStrNull(cfgOptRepoCipherPass));

            // If the copy loaded successfuly, then check the checksums
            if (verifyArchiveInfoCopy.errorCode == 0)
            {
                // If the info and info.copy checksums don't match each other than one (or both) of the files could be corrupt so
                // log a warning but must trust main
                if (!strEq(verifyArchiveInfo.checksum, verifyArchiveInfoCopy.checksum))
                    LOG_WARN("archive.info.copy doesn't match archive.info");
            }
        }
        else
        {
            // Attempt to load the copy
            VerifyInfoFile verifyArchiveInfoCopy = verifyInfoFile(
                INFO_ARCHIVE_PATH_FILE_COPY_STR, true, cfgOptionStrNull(cfgOptRepoCipherPass));

            // If loaded successfully, then return the copy as usable
            if (verifyArchiveInfoCopy.errorCode == 0)
            {
                result = verifyArchiveInfoCopy.archive;
                infoArchiveMove(result, memContextPrior());
            }
        }
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(INFO_ARCHIVE, result);
}

static InfoBackup *
verifyBackupInfoFile(void)
{
    FUNCTION_LOG_VOID(logLevelDebug);

    InfoBackup *result = NULL;

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Get the main info file
        VerifyInfoFile verifyBackupInfo = verifyInfoFile(INFO_BACKUP_PATH_FILE_STR, true, cfgOptionStrNull(cfgOptRepoCipherPass));

        // If the main file did not error, then report on the copy's status and check checksums
        if (verifyBackupInfo.errorCode == 0)
        {
            result = verifyBackupInfo.backup;
            infoBackupMove(result, memContextPrior());

            // Attempt to load the copy and report on it's status but don't keep it in memory
            VerifyInfoFile verifyBackupInfoCopy = verifyInfoFile(
                INFO_BACKUP_PATH_FILE_COPY_STR, false, cfgOptionStrNull(cfgOptRepoCipherPass));

            // If the copy loaded successfuly, then check the checksums
            if (verifyBackupInfoCopy.errorCode == 0)
            {
                // If the info and info.copy checksums don't match each other than one (or both) of the files could be corrupt so
                // log a warning but must trust main
                if (!strEq(verifyBackupInfo.checksum, verifyBackupInfoCopy.checksum))
                    LOG_WARN("backup.info.copy doesn't match backup.info");
            }
        }
        else
        {
            // Attempt to load the copy
            VerifyInfoFile verifyBackupInfoCopy = verifyInfoFile(
                INFO_BACKUP_PATH_FILE_COPY_STR, true, cfgOptionStrNull(cfgOptRepoCipherPass));

            // If loaded successfully, then return the copy as usable
            if (verifyBackupInfoCopy.errorCode == 0)
            {
                result = verifyBackupInfoCopy.backup;
                infoBackupMove(result, memContextPrior());
            }
        }
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN(INFO_BACKUP, result);
}
//
// static Manifest *
// verifyManifestFile(void)
// {
//     FUNCTION_LOG_VOID(logLevelDebug);
//
//     InfoBackup *result = NULL;
// CSHANG Need this somewhere: infoPgCipherPass(infoBackupPg(info))                      cfgOptionStrNull(cfgOptRepoCipherPass)
//     MEM_CONTEXT_TEMP_BEGIN()
//     {
//         // Get the main info file
//         VerifyInfoFile verifyBackupInfo = verifyInfoFile(INFO_BACKUP_PATH_FILE_STR, true, cfgOptionStrNull(cfgOptRepoCipherPass));
//
//         // If the main file did not error, then report on the copy's status and check checksums
//         if (verifyBackupInfo.errorCode == 0)
//         {
//             result = verifyBackupInfo.backup;
//             infoBackupMove(result, memContextPrior());
//
//             // Attempt to load the copy and report on it's status but don't keep it in memory
//             VerifyInfoFile verifyBackupInfoCopy = verifyInfoFile(
//                 INFO_BACKUP_PATH_FILE_COPY_STR, false, cfgOptionStrNull(cfgOptRepoCipherPass));
//
//             // If the copy loaded successfuly, then check the checksums
//             if (verifyBackupInfoCopy.errorCode == 0)
//             {
//                 // If the info and info.copy checksums don't match each other than one (or both) of the files could be corrupt so
//                 // log a warning but must trust main
//                 if (!strEq(verifyBackupInfo.checksum, verifyBackupInfoCopy.checksum))
//                     LOG_WARN("backup.info.copy doesn't match backup.info");
//             }
//         }
//         else
//         {
//             // Attempt to load the copy
//             VerifyInfoFile verifyBackupInfoCopy = verifyInfoFile(
//                 INFO_BACKUP_PATH_FILE_COPY_STR, true, cfgOptionStrNull(cfgOptRepoCipherPass));
//
//             // If loaded successfully, then return the copy as usable
//             if (verifyBackupInfoCopy.errorCode == 0)
//             {
//                 result = verifyBackupInfoCopy.backup;
//                 infoBackupMove(result, memContextPrior());
//             }
//         }
//     }
//     MEM_CONTEXT_TEMP_END();
// //
//     FUNCTION_LOG_RETURN(INFO_BACKUP, result);
// }

void
verifyPgHistory(const InfoPg *archiveInfoPg, const InfoPg *backupInfoPg)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(INFO_PG, archiveInfoPg);
        FUNCTION_TEST_PARAM(INFO_PG, backupInfoPg);
    FUNCTION_TEST_END();

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Check archive.info and backup.info current PG data matches. If there is a mismatch, verify cannot continue since
        // the database is not considered accessible during the verify command so no way to tell which would be valid.
        InfoPgData archiveInfoPgData = infoPgData(archiveInfoPg, infoPgDataCurrentId(archiveInfoPg));
        InfoPgData backupInfoPgData = infoPgData(backupInfoPg, infoPgDataCurrentId(backupInfoPg));
        checkStanzaInfo(&archiveInfoPgData, &backupInfoPgData);

        unsigned int archiveInfoHistoryTotal = infoPgDataTotal(archiveInfoPg);
        unsigned int backupInfoHistoryTotal = infoPgDataTotal(backupInfoPg);

        String *errMsg = strNew("archive and backup history lists do not match");

        if (archiveInfoHistoryTotal != backupInfoHistoryTotal)
            THROW(FormatError, strPtr(errMsg));

        // Confirm the lists are the same
        for (unsigned int infoPgIdx = 0; infoPgIdx < archiveInfoHistoryTotal; infoPgIdx++)
        {
            InfoPgData archiveInfoPgHistory = infoPgData(archiveInfoPg, infoPgIdx);
            InfoPgData backupInfoPgHistory = infoPgData(backupInfoPg, infoPgIdx);

            if (archiveInfoPgHistory.id != backupInfoPgHistory.id ||
                archiveInfoPgHistory.systemId != backupInfoPgHistory.systemId ||
                archiveInfoPgHistory.version != backupInfoPgHistory.version)
            {
                THROW(FormatError, strPtr(errMsg));
            }
        }
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN_VOID();
}


typedef struct ArchiveIdRange
{
    String *archiveId;
    PgWal pgWalInfo;
    List *walRangeList;
} ArchiveIdRange;

typedef struct WalRange
{
    String *stop;
    String *start;
    List *invalidFileList;    // After all jobs complete: If not NULL (or maybe size > 0) then there is a problem in this range
} WalRange;

typedef struct InvalidFile      // this structure can be used for backup file stuff as well
{
    String *fileName;
    VerifyResult reason;            // this enum is the reason
} InvalidFile;

typedef struct VerifyJobData
{
    StringList *archiveIdList;                                      // List of archive ids to verify
    StringList *walPathList;                                        // WAL path list for a single archive id
    StringList *walFileList;                                        // WAL file list for a single WAL path
    StringList *backupList;                                         // List of backups to verify
    StringList *backupFileList;                                     // List of files in a single backup directory
    bool backupProcessing;                                          // Are we processing WAL or are we processing backups
    const String *manifestCipherPass;                               // Cipher pass for reading backup manifests
    const String *walCipherPass;                                    // Cipher pass for reading WAL files
    unsigned int jobErrorTotal;                                     // Total errors that occurred during the job execution
    List *archiveIdRangeList;
} VerifyJobData;

static void
updateArchiveIdRange(ArchiveIdRange *archiveIdRange, StringList *walFileList, List *archiveIdRangeList)
{
    // Initialize the WAL range
    WalRange walRange =
    {
        .start = strSubN(strLstGet(walFileList, 0), 0, WAL_SEGMENT_NAME_SIZE),
        .stop = strSubN(strLstGet(walFileList, 0), 0, WAL_SEGMENT_NAME_SIZE),
    };

    // Get the ranges by comparing the next WAL in the list to see if it is WAL Segment Size distance from
    // the last WAL
    for (unsigned int walFileIdx = 1; walFileIdx < strLstSize(walFileList); walFileIdx++)
    {
        String *walSegment = strSubN(strLstGet(walFileList, walFileIdx), 0, WAL_SEGMENT_NAME_SIZE);

        // If the next WAL is the appropriate distance away, then there is no gap. For versions less than or equal to 9.2,
        // the WAL size is static at 16MB but for some unknown reason, WAL ending in FF is skipped so it should never exist, so
        // the next WAL is 2 times the distance (WAL segment size) away, not one.
// CSHANG What to do if it does exist? Do we LOG_ERROR here and then skip it? Or do we create a range where the previous wal range ends at FE and then a new range starts at FF and continues until end. If we do the latter, then it will cause an error to be reported in the final results reconciliation that the backup is bad - which may or may not be true because we don't know why there is a "gap" - why there is an extra file FF
        if (pgLsnFromWalSegment(walSegment, archiveIdRange->pgWalInfo.size) -
            pgLsnFromWalSegment(walRange.stop, archiveIdRange->pgWalInfo.size) ==
                ((archiveIdRange->pgWalInfo.version <= PG_VERSION_92 && strEndsWithZ(walRange.stop, "FE"))
                ? archiveIdRange->pgWalInfo.size * 2 : archiveIdRange->pgWalInfo.size))
        {
            walRange.stop = walSegment;
        }
        else
        {
            // A gap was found so add the current range to the list and start a new range
            lstAdd(archiveIdRange->walRangeList, &walRange);
            walRange = (WalRange)
            {
                .start = strDup(walSegment),
                .stop = strDup(walSegment),
            };
        }
    }

    // Add the last walRange
    lstAdd(archiveIdRange->walRangeList, &walRange);

    // Now we have our ranges for this archiveId so add them
    lstAdd(archiveIdRangeList, archiveIdRange);
}

static ProtocolParallelJob *
verifyArchive(VerifyJobData *jobData)
{
    ProtocolParallelJob *result = NULL;

    // Process archive files, if any
    while (strLstSize(jobData->archiveIdList) > 0)
    {
        String *archiveId = strLstGet(jobData->archiveIdList, 0);
        result = NULL;

        ArchiveIdRange archiveIdRange =
        {
            .archiveId = strDup(archiveId),
            .walRangeList = lstNewP(sizeof(WalRange), .comparator =  lstComparatorStr),
        };

        if (strLstSize(jobData->walPathList) == 0)
        {
            // Get the WAL paths for the first item in the archive Id list
            jobData->walPathList =
                strLstSort(
                    storageListP(
                        storageRepo(), strNewFmt(STORAGE_REPO_ARCHIVE "/%s", strPtr(archiveId)),
                        .expression = WAL_SEGMENT_DIR_REGEXP_STR),
                    sortOrderAsc);
        }

        // If there are WAL paths then get the file lists
        if (strLstSize(jobData->walPathList) > 0)
        {
            do
            {
                String *walPath = strLstGet(jobData->walPathList, 0);

                // Get the WAL files for the first item in the WAL paths list
                if (strLstSize(jobData->walFileList) == 0)
                {
                    jobData->walFileList =
                        strLstSort(
                            storageListP(
                                storageRepo(),
                                strNewFmt(STORAGE_REPO_ARCHIVE "/%s/%s", strPtr(archiveId), strPtr(walPath)),
                                .expression = WAL_SEGMENT_FILE_REGEXP_STR),
                            sortOrderAsc);
                }

// CSHANG we will have to check every file - pg_resetwal can change the wal segment size at any time - grrrr. We can spot check in each timeline by checking the first file, but that won't help as we'll just wind up with a bunch of ranges since the segment size will stop matching at some point.  If WAL segment size is reset, then can't do PITR.
/* CSHANG per David:
16MB is the whole segment.
The header is like 40 bytes.
What we need is for pgWalFromFile() to look more like pgControlFromFile().
i.e. takes a storage object and reads the first few bytes from the file.
But, if you just want to read the first 512 bytes for now and use pgWalFromBuffer() then that's fine. We can improve that later.
But people do use pg_resetwal just to change the WAL size. You won't lose data if you do a clean shutdown and then pg_resetwal.
So, for our purposes the size should never change for a db-id.
We'll need to put in code to check if the size changes and force them to do a stanza-upgrade in that case. (<- in archive push/get)
So, for now you'll need to check the first WAL and use that size to verify the sizes of the rest. Later we'll pull size info from archive.info.
*/
                if (strLstSize(jobData->walFileList) > 0)
                {
                    // If the WAL segment size for this archiveId has not been set, get it from the first WAL
                    if (archiveIdRange.pgWalInfo.size == 0)
                    {
                        StorageRead *walRead = verifyFileLoad(
                            strNewFmt(
                                STORAGE_REPO_ARCHIVE "/%s/%s/%s", strPtr(archiveId), strPtr(walPath),
                                strPtr(strLstGet(jobData->walFileList, 0))),
                            jobData->walCipherPass);

                        PgWal walInfo = pgWalFromBuffer(storageGetP(walRead, .exactSize = PG_WAL_HEADER_SIZE));

                        archiveIdRange.pgWalInfo.size = walInfo.size;
                        archiveIdRange.pgWalInfo.version = walInfo.version;
                    }
// CSHANG Switch memory context to the VerifyJobData structure?
                    updateArchiveIdRange(&archiveIdRange, jobData->walFileList, jobData->archiveIdRangeList);

// CSHANG Switch memory context back to current?

                    do
                    {
                        const String *fileName = strLstGet(jobData->walFileList, 0);

                        // Get the fully qualified file name
                        const String *filePathName = strNewFmt(
                            STORAGE_REPO_ARCHIVE "/%s/%s/%s", strPtr(archiveId), strPtr(walPath), strPtr(fileName));

                        // Get the checksum
                        String *checksum = strSubN(fileName, WAL_SEGMENT_NAME_SIZE + 1, HASH_TYPE_SHA1_SIZE_HEX);

                        ProtocolCommand *command = protocolCommandNew(PROTOCOL_COMMAND_VERIFY_FILE_STR);
                        protocolCommandParamAdd(command, VARSTR(filePathName));
                        protocolCommandParamAdd(command, VARSTR(checksum));
                        protocolCommandParamAdd(command, VARBOOL(false));   // Can the size be verified?
                        protocolCommandParamAdd(command, VARUINT64(0));     // File size to verify
                        protocolCommandParamAdd(command, VARUINT(compressTypeFromName(fileName)));
                        protocolCommandParamAdd(command, VARSTR(jobData->walCipherPass));

                        // Assign job to result
                        result = protocolParallelJobNew(VARSTR(fileName), command);
                        // CSHANG for now, since no temp context then no move
                        // result = protocolParallelJobMove(
                        //     protocolParallelJobNew(VARSTR(filePathName), command), memContextPrior());

                        // Remove the file to process from the list
                        strLstRemoveIdx(jobData->walFileList, 0);

                        // If this is the last file to process for this timeline, then remove the path
                        if (strLstSize(jobData->walFileList) == 0)
                            strLstRemoveIdx(jobData->walPathList, 0);

                        // Return to process the job found
                        break;
                    }
                    while (strLstSize(jobData->walFileList) > 0);
                }
                else
                {
                    // Log no WAL exists in the WAL path and remove the WAL path from the list (nothing to process)
                    LOG_WARN_FMT("path '%s/%s' is empty", strPtr(archiveId), strPtr(walPath));
                    strLstRemoveIdx(jobData->walPathList, 0);
                }

                // If a job was found to be processed then break out to process it
                if (result != NULL)
                    break;
            }
            while (strLstSize(jobData->walPathList) > 0);

            // If this is the last timeline to process for this archiveId, then remove the archiveId
            if (strLstSize(jobData->walPathList) == 0)
                strLstRemoveIdx(jobData->archiveIdList, 0);

            // If a file was sent to be processed then break so can process it
            if (result != NULL)
                break;
        }
        else
        {
            // Log that no WAL paths exist in the archive Id dir - remove the archive Id from the list (nothing to process)
            LOG_WARN_FMT("path '%s' is empty", strPtr(archiveId));
            strLstRemoveIdx(jobData->archiveIdList, 0);
        }
    }
    FUNCTION_TEST_RETURN(result);
}

static ProtocolParallelJob *
verifyJobCallback(void *data, unsigned int clientIdx)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(VOID, data);
        FUNCTION_TEST_PARAM(UINT, clientIdx);
    FUNCTION_TEST_END();

    ASSERT(data != NULL);

    // No special logic based on the client, since only operating against the repo, so just get the next job
    (void)clientIdx;

    // Initialize the result
    ProtocolParallelJob *result = NULL;

    // MEM_CONTEXT_TEMP_BEGIN() // cshang remove temp block FOR NOW - will later have a memContext management
    // {
        // Get a new job if there are any left
        VerifyJobData *jobData = data;

        if (!jobData->backupProcessing)
        {

            result = verifyArchive(jobData);
            jobData->backupProcessing = strLstSize(jobData->archiveIdList) == 0;

            // If there is a result from archiving, then return it
            if (result != NULL)
                FUNCTION_TEST_RETURN(result);  // CSHANG can only do if don't have a temp mem context
        }

        // Process backups - get manifest and verify it first thru function here vs sending verifyFile, log errors and incr job error
        if (jobData->backupProcessing)
        {
            // Process backup files, if any
            while (strLstSize(jobData->backupList) > 0)
            {
LOG_WARN("Processing BACKUPS"); // CSHANG Remove
                // result == NULL;
/* If most rececnt has only copy, then move on since it cuold be the latest backup in progress. If missing both, then expired so skip. But if only copy and not the most recent then the backup still needs to be checked since restore will just try to read the manifest BUT it checks the manifest against the backup.info current section so if not in there. If main is not there and copy is but it is not the latest then warn that main is missing and skip.
*/
                        // // Get a usable backup manifest file
                        // InfoBackup *backupInfo = verifyBackupInfoFile();
                        //
                        // // If a usable backup.info file is not found, then report an error in the log
                        // if (backupInfo == NULL)
                        // {
                        //     LOG_ERROR(errorTypeCode(&FormatError), "No usable backup.info file");
                        //     errorTotal++;
                        // }
            }

            // If there is a result from backups, then return it
            if (result != NULL)
            {
                LOG_WARN("BACKUP RESULT != NULL"); // CSHANG Remove
                FUNCTION_TEST_RETURN(result);
            }
        }



    // }
    // MEM_CONTEXT_TEMP_END();

    FUNCTION_TEST_RETURN(result);
}


void
cmdVerify(void)
{
    FUNCTION_LOG_VOID(logLevelDebug);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        unsigned int errorTotal = 0;

        // Get the repo storage in case it is remote and encryption settings need to be pulled down
        const Storage *storage = storageRepo();

        // Get a usable backup info file
        InfoBackup *backupInfo = verifyBackupInfoFile();

        // If a usable backup.info file is not found, then report an error in the log
        if (backupInfo == NULL)
        {
            LOG_ERROR(errorTypeCode(&FormatError), "No usable backup.info file");
            errorTotal++;
        }

        // Get a usable archive info file
        InfoArchive *archiveInfo = verifyArchiveInfoFile();

        // If a usable archive.info file is not found, then report an error in the log
        if (archiveInfo == NULL)
        {
            LOG_ERROR(errorTypeCode(&FormatError), "No usable archive.info file");
            errorTotal++;
        }

        // If both a usable archive info and backup info file were found, then proceed with verification
        if (archiveInfo != NULL && backupInfo != NULL)
        {
            TRY_BEGIN()
            {
                // Verify that the archive.info and backup.info current database info and history lists are the same
                verifyPgHistory(infoArchivePg(archiveInfo), infoBackupPg(backupInfo));
            }
            CATCH_ANY()
            {
                LOG_ERROR(errorTypeCode(&FormatError), errorMessage());
                errorTotal++;
            }
            TRY_END();
        }

        // If valid info files, then begin process of checking backups and archives in the repo
        if (errorTotal == 0)
        {
// CSHANG TODO: May need to replace strLstNew() with strLstNewP()
            // Initialize the job data
            VerifyJobData jobData =
            {
                .walPathList = strLstNew(),  // cshang need to create memcontex and later after processing loop, memContextDiscard(); see manifest.c line 1793
                .walFileList = strLstNew(),
                .backupFileList = strLstNew(),
                .manifestCipherPass = infoPgCipherPass(infoBackupPg(backupInfo)),
                .walCipherPass = infoPgCipherPass(infoArchivePg(archiveInfo)),
                .archiveIdRangeList = lstNewP(sizeof(ArchiveIdRange), .comparator =  archiveIdComparator),
            };

            // Get a list of backups in the repo
            jobData.backupList = strLstSort(
                storageListP(
                    storage, STORAGE_REPO_BACKUP_STR,
                    .expression = backupRegExpP(.full = true, .differential = true, .incremental = true)),
                sortOrderAsc);

            // Get a list of archive Ids in the repo (e.g. 9.4-1, 10-2, etc) sorted by the db-id (number after the dash)
            jobData.archiveIdList = strLstSort(
                strLstComparatorSet(
                    storageListP(storage, STORAGE_REPO_ARCHIVE_STR, .expression = STRDEF(REGEX_ARCHIVE_DIR_DB_VERSION)),
                    archiveIdComparator),
                sortOrderAsc);

            // Only begin processing if there are some archives or backups in the repo
            if (strLstSize(jobData.archiveIdList) > 0 || strLstSize(jobData.backupList) > 0)
            {
                // WARN if there are no archives or there are no backups in the repo so that the callback need not try to
                // distinguish between having processed all of the list or if the list was missing in the first place
                if (strLstSize(jobData.archiveIdList) == 0 || strLstSize(jobData.backupList) == 0)
                    LOG_WARN_FMT("no %s exist in the repo", strLstSize(jobData.archiveIdList) == 0 ? "archives" : "backups");

                // Create the parallel executor
                ProtocolParallel *parallelExec = protocolParallelNew(
                    (TimeMSec)(cfgOptionDbl(cfgOptProtocolTimeout) * MSEC_PER_SEC) / 2, verifyJobCallback, &jobData);

                // CSHANG Add this option
                // // If a fast option has been requested, then only create one process to handle, else create as many as process-max
                // unsigned int numProcesses = cfgOptionTest(cfgOptFast) ? 1 : cfgOptionUInt(cfgOptProcessMax);

                for (unsigned int processIdx = 1; processIdx <= cfgOptionUInt(cfgOptProcessMax); processIdx++)
                    protocolParallelClientAdd(parallelExec, protocolLocalGet(protocolStorageTypeRepo, 1, processIdx));

                // Process jobs
                do
                {
                    unsigned int completed = protocolParallelProcess(parallelExec);

                    // Process completed jobs
                    for (unsigned int jobIdx = 0; jobIdx < completed; jobIdx++)
                    {
LOG_WARN_FMT("NUM JOB COMPLETED %u,      JOBIDX %u", completed, jobIdx);  // CSHANG Remove
                        // Get the job and job key
                        ProtocolParallelJob *job = protocolParallelResult(parallelExec);
                        unsigned int processId = protocolParallelJobProcessId(job);
                        const String *fileName = varStr(protocolParallelJobKey(job)); // CSHANG Actually, can probably make this the full filename again bcause we can just split the string on the forward slashes

                        // CSHANG The key will tell us what we just processed
                        // const VerifyResult verifyResult = (VerifyResult)varUIntForce(varLstGet(jobResult, 0))
                        // const String *filename = varStr(varLstGet(varVarLst(protocolParallelJobKey(job)), 1);
                        // const ENUM type = varUInt(varLstGet(varVarLst(protocolParallelJobKey(job)), 2); -- May need a type?
                        //
                        //
                        // The job was successful
                        if (protocolParallelJobErrorCode(job) == 0)
                        {
                            const VariantList *const jobResult = varVarLst(protocolParallelJobResult(job));
                            const VerifyResult verifyResult = (VerifyResult)varUIntForce(varLstGet(jobResult, 0));
                            const String *const filePathName = varStr(varLstGet(jobResult, 1));

                            if (verifyResult != verifyOk)
                            {
                                LOG_WARN_PID_FMT(processId, "SOME ERROR %u, ON %s, %s", verifyResult, strPtr(filePathName), strPtr(fileName));

                                /* CSHANG pseudo code:
                                    Get the type of file this is (e.g. backup or archive file - probably just split the string into a stringlist by "/" and check that stringlist[0] = STORAGE_REPO_ARCHIVE or STORAGE_REPO_BACKUP)

                                    If archive file
                                        Get the archiveId from the filePathName (stringList[1] - is there a better way?)
                                        Loop through verifyResultData.archiveIdRangeList and find the archiveId
                                        If found, find the range in which this file falls into and add the file name and the reason to the invalidFileList

                                    If backup file
                                        Get the backup label from the filePathName (stringList[1] - is there a better way?)
                                        Loop through verifyResultData.backupList and find the backup label
                                        If found, add the file name and the reason to the invalidFileList


                                    Final stage, after all jobs are complete, is to reconcile the archive with the backup data which, it seems at this pioint is just determining if the backup is 1) consistent (no gaps) 2) can run through PITR (trickier - not sure what this would look like....)
                                */


                                // CSHANG No - maybe what we need to do is just store the full names in a list because we have to know which DB-ID the wal belongs to and tie that back to the backup data (from the manifest file) A: David says we shouldn't be tying back to backup.info, but rather the manifest - which is where the data in backup.info is coming from anyway
                                // CSHANG and what about individual backup files, if any one of them is invalid (or any gaps in archive), that entire backup needs to be marked invalid, right? So maybe we need to be creating a list of invalid backups such that String *strLstAddIfMissing(StringList *this, const String *string); is called when we find a backup that is not good. And remove from the jobdata.backupList()?

                            }
                        }
                        // CSHANG I am assuming that there will only be an error if something went horribly wrong and the program needs to exit...
                        // Else the job errored
                        else
                            THROW_CODE(protocolParallelJobErrorCode(job), strPtr(protocolParallelJobErrorMessage(job)));

                        // Free the job
                        protocolParallelJobFree(job);
                    }
                }
                while (!protocolParallelDone(parallelExec));

                // HERE we will need to do the final reconciliation - checking backup required WAL against, valid WAL
            }
            else
                LOG_WARN("no archives or backups exist in the repo");
        }

        // Throw an error is cannot proceed
        if (errorTotal > 0)
            THROW(RuntimeError, "fatal errors encountered, see log for details");

    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}
