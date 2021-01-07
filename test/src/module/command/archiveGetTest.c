/***********************************************************************************************************************************
Test Archive Get Command
***********************************************************************************************************************************/
#include "common/compress/helper.h"
#include "common/harnessConfig.h"
#include "common/harnessFork.h"
#include "common/io/bufferRead.h"
#include "common/io/bufferWrite.h"
#include "postgres/interface.h"
#include "postgres/version.h"
#include "storage/posix/storage.h"

#include "common/harnessInfo.h"
#include "common/harnessStorage.h"

/***********************************************************************************************************************************
Test Run
***********************************************************************************************************************************/
void
testRun(void)
{
    FUNCTION_HARNESS_VOID();

    Storage *storageTest = storagePosixNewP(strNew(testPath()), .write = true);

    // *****************************************************************************************************************************
    if (testBegin("archiveGetCheck()"))
    {
        // // Load Parameters
        // StringList *argList = strLstNew();
        // strLstAddZ(argList, "--stanza=test1");
        // strLstAdd(argList, strNewFmt("--repo1-path=%s/repo", testPath()));
        // strLstAdd(argList, strNewFmt("--pg1-path=%s/pg", testPath()));
        // harnessCfgLoad(cfgCmdArchiveGet, argList);
        //
        // // Create pg_control file
        // storagePutP(
        //     storageNewWriteP(storageTest, strNew("db/" PG_PATH_GLOBAL "/" PG_FILE_PGCONTROL)),
        //     pgControlTestToBuffer((PgControl){.version = PG_VERSION_10, .systemId = 0xFACEFACEFACEFACE}));
        //
        // // Control and archive info mismatch
        // // -------------------------------------------------------------------------------------------------------------------------
        // storagePutP(
        //     storageNewWriteP(storageTest, strNew("repo/archive/test1/archive.info")),
        //     harnessInfoChecksumZ(
        //         "[db]\n"
        //         "db-id=1\n"
        //         "\n"
        //         "[db:history]\n"
        //         "1={\"db-id\":5555555555555555555,\"db-version\":\"9.4\"}\n"));
        //
        // TEST_ERROR(
        //     archiveGetCheck(strNew("876543218765432187654321"), cipherTypeNone, NULL), ArchiveMismatchError,
        //     "unable to retrieve the archive id for database version '10' and system-id '18072658121562454734'");
        //
        // // Nothing to find in empty archive dir
        // // -------------------------------------------------------------------------------------------------------------------------
        // storagePutP(
        //     storageNewWriteP(storageTest, strNew("repo/archive/test1/archive.info")),
        //     harnessInfoChecksumZ(
        //         "[db]\n"
        //         "db-id=3\n"
        //         "\n"
        //         "[db:history]\n"
        //         "1={\"db-id\":5555555555555555555,\"db-version\":\"9.4\"}\n"
        //         "2={\"db-id\":18072658121562454734,\"db-version\":\"10\"}\n"
        //         "3={\"db-id\":18072658121562454734,\"db-version\":\"9.6\"}\n"
        //         "4={\"db-id\":18072658121562454734,\"db-version\":\"10\"}"));
        //
        // TEST_RESULT_STR(
        //     archiveGetCheck(strNew("876543218765432187654321"), cipherTypeNone, NULL).archiveFileActual, NULL, "no segment found");
        //
        // // Write segment into an older archive path
        // // -------------------------------------------------------------------------------------------------------------------------
        // storagePutP(
        //     storageNewWriteP(
        //         storageTest,
        //         strNew(
        //             "repo/archive/test1/10-2/8765432187654321/876543218765432187654321-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")),
        //     NULL);
        //
        // TEST_RESULT_STR_Z(
        //     archiveGetCheck(strNew("876543218765432187654321"), cipherTypeNone, NULL).archiveFileActual,
        //     "10-2/8765432187654321/876543218765432187654321-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "segment found");
        //
        // // Write segment into an newer archive path
        // // -------------------------------------------------------------------------------------------------------------------------
        // storagePutP(
        //     storageNewWriteP(
        //         storageTest,
        //         strNew(
        //             "repo/archive/test1/10-4/8765432187654321/876543218765432187654321-bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")),
        //     NULL);
        //
        // TEST_RESULT_STR_Z(
        //     archiveGetCheck(strNew("876543218765432187654321"), cipherTypeNone, NULL).archiveFileActual,
        //     "10-4/8765432187654321/876543218765432187654321-bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "newer segment found");
        //
        // // Get history file
        // // -------------------------------------------------------------------------------------------------------------------------
        // TEST_RESULT_STR(
        //     archiveGetCheck(strNew("00000009.history"), cipherTypeNone, NULL).archiveFileActual, NULL, "history file not found");
        //
        // storagePutP(storageNewWriteP(storageTest, strNew("repo/archive/test1/10-4/00000009.history")), NULL);
        //
        // TEST_RESULT_STR_Z(
        //     archiveGetCheck(strNew("00000009.history"), cipherTypeNone, NULL).archiveFileActual, "10-4/00000009.history",
        //     "history file found");
    }

    // *****************************************************************************************************************************
    if (testBegin("queueNeed()"))
    {
        StringList *argList = strLstNew();
        strLstAddZ(argList, "--stanza=test1");
        strLstAddZ(argList, "--archive-async");
        hrnCfgArgRawZ(argList, cfgOptPgPath, "/unused");
        strLstAdd(argList, strNewFmt("--spool-path=%s/spool", testPath()));
        harnessCfgLoad(cfgCmdArchiveGet, argList);

        size_t queueSize = 16 * 1024 * 1024;
        size_t walSegmentSize = 16 * 1024 * 1024;

        TEST_ERROR_FMT(
            queueNeed(strNew("000000010000000100000001"), false, queueSize, walSegmentSize, PG_VERSION_92),
            PathMissingError, "unable to list file info for missing path '%s/spool/archive/test1/in'", testPath());

        // -------------------------------------------------------------------------------------------------------------------------
        storagePathCreateP(storageSpoolWrite(), strNew(STORAGE_SPOOL_ARCHIVE_IN));

        TEST_RESULT_STR_Z(
            strLstJoin(queueNeed(strNew("000000010000000100000001"), false, queueSize, walSegmentSize, PG_VERSION_92), "|"),
            "000000010000000100000001|000000010000000100000002", "queue size smaller than min");

        // -------------------------------------------------------------------------------------------------------------------------
        queueSize = (16 * 1024 * 1024) * 3;

        TEST_RESULT_STR_Z(
            strLstJoin(queueNeed(strNew("000000010000000100000001"), false, queueSize, walSegmentSize, PG_VERSION_92), "|"),
            "000000010000000100000001|000000010000000100000002|000000010000000100000003", "empty queue");

        // -------------------------------------------------------------------------------------------------------------------------
        Buffer *walSegmentBuffer = bufNew(walSegmentSize);
        memset(bufPtr(walSegmentBuffer), 0, walSegmentSize);

        storagePutP(
            storageNewWriteP(
                storageSpoolWrite(), strNew(STORAGE_SPOOL_ARCHIVE_IN "/0000000100000001000000FE")), walSegmentBuffer);
        storagePutP(
            storageNewWriteP(
                storageSpoolWrite(), strNew(STORAGE_SPOOL_ARCHIVE_IN "/0000000100000001000000FF")), walSegmentBuffer);

        TEST_RESULT_STR_Z(
            strLstJoin(queueNeed(strNew("0000000100000001000000FE"), false, queueSize, walSegmentSize, PG_VERSION_92), "|"),
            "000000010000000200000000|000000010000000200000001", "queue has wal < 9.3");

        TEST_RESULT_STR_Z(
            strLstJoin(storageListP(storageSpoolWrite(), strNew(STORAGE_SPOOL_ARCHIVE_IN)), "|"), "0000000100000001000000FE",
            "check queue");

        // -------------------------------------------------------------------------------------------------------------------------
        walSegmentSize = 1024 * 1024;
        queueSize = walSegmentSize * 5;

        storagePutP(storageNewWriteP(storageSpoolWrite(), strNew(STORAGE_SPOOL_ARCHIVE_IN "/junk")), BUFSTRDEF("JUNK"));
        storagePutP(
            storageNewWriteP(
                storageSpoolWrite(), strNew(STORAGE_SPOOL_ARCHIVE_IN "/000000010000000A00000FFE")), walSegmentBuffer);
        storagePutP(
            storageNewWriteP(
                storageSpoolWrite(), strNew(STORAGE_SPOOL_ARCHIVE_IN "/000000010000000A00000FFF")), walSegmentBuffer);

        TEST_RESULT_STR_Z(
            strLstJoin(queueNeed(strNew("000000010000000A00000FFD"), true, queueSize, walSegmentSize, PG_VERSION_11), "|"),
            "000000010000000B00000000|000000010000000B00000001|000000010000000B00000002", "queue has wal >= 9.3");

        TEST_RESULT_STR_Z(
            strLstJoin(strLstSort(storageListP(storageSpool(), strNew(STORAGE_SPOOL_ARCHIVE_IN)), sortOrderAsc), "|"),
            "000000010000000A00000FFE|000000010000000A00000FFF", "check queue");
    }

    // *****************************************************************************************************************************
    if (testBegin("cmdArchiveGetAsync()"))
    {
        harnessLogLevelSet(logLevelDetail);

        // Arguments that must be included
        StringList *argBaseList = strLstNew();
        hrnCfgArgRawZ(argBaseList, cfgOptPgPath, TEST_PATH_PG);
        hrnCfgArgRawZ(argBaseList, cfgOptRepoPath, TEST_PATH_REPO);
        hrnCfgArgRawZ(argBaseList, cfgOptSpoolPath, TEST_PATH_SPOOL);
        hrnCfgArgRawBool(argBaseList, cfgOptArchiveAsync, true);
        hrnCfgArgRawZ(argBaseList, cfgOptStanza, "test2");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("command must be run on the pg host");

        StringList *argList = strLstDup(argBaseList);
        hrnCfgArgRawZ(argList, cfgOptPgHost, BOGUS_STR);
        harnessCfgLoadRole(cfgCmdArchiveGet, cfgCmdRoleAsync, argList);

        TEST_ERROR(cmdArchiveGetAsync(), HostInvalidError, "archive-get command must be run on the PostgreSQL host");

        TEST_FILE_LIST_REMOVE_Z(storageSpoolWrite(), STORAGE_SPOOL_ARCHIVE_IN, "global.error");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("error on no segments");

        argList = strLstDup(argBaseList);
        harnessCfgLoadRole(cfgCmdArchiveGet, cfgCmdRoleAsync, argList);

        TEST_ERROR(cmdArchiveGetAsync(), ParamInvalidError, "at least one wal segment is required");

        TEST_FILE_LIST_REMOVE_Z(storageSpoolWrite(), STORAGE_SPOOL_ARCHIVE_IN, "global.error");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("no segments to find");

        // Create pg_control file and archive.info
        TEST_FILE_PUT_Z_BUF(
            storagePgWrite(), PG_PATH_GLOBAL "/" PG_FILE_PGCONTROL,
            pgControlTestToBuffer((PgControl){.version = PG_VERSION_10, .systemId = 0xFACEFACEFACEFACE}));

        storagePutP(
            storageNewWriteP(storageTest, strNew("repo/archive/test2/archive.info")),
            harnessInfoChecksumZ(
                "[db]\n"
                "db-id=1\n"
                "\n"
                "[db:history]\n"
                "1={\"db-id\":18072658121562454734,\"db-version\":\"10\"}\n"));

        strLstAddZ(argList, "000000010000000100000001");
        harnessCfgLoadRole(cfgCmdArchiveGet, cfgCmdRoleAsync, argList);

        TEST_RESULT_VOID(cmdArchiveGetAsync(), "get async");

        harnessLogResult(
            "P00   INFO: get 1 WAL file(s) from archive: 000000010000000100000001\n"
            "P00 DETAIL: unable to find 000000010000000100000001 in the archive");

        TEST_FILE_LIST_REMOVE_Z(storageSpoolWrite(), STORAGE_SPOOL_ARCHIVE_IN, "000000010000000100000001.ok");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("error on invalid compressed segment");

        TEST_RESULT_VOID(
            storagePutP(
                storageNewWriteP(
                    storageRepoWrite(),
                    STR(STORAGE_REPO_ARCHIVE "/10-1/000000010000000100000001-abcdabcdabcdabcdabcdabcdabcdabcdabcdabcd.gz")), NULL),
            "create broken segment");

        TEST_RESULT_VOID(cmdArchiveGetAsync(), "get async");

        harnessLogResult(
            "P00   INFO: get 1 WAL file(s) from archive: 000000010000000100000001\n"
            "P01   WARN: could not get 000000010000000100000001 from the archive (will be retried):"
                " [29] raised from local-1 protocol: unexpected eof in compressed data");

        TEST_FILE_LIST_Z(
            storageSpool(), STORAGE_SPOOL_ARCHIVE_IN, "000000010000000100000001.error|000000010000000100000001.pgbackrest.tmp");
        TEST_FILE_REMOVE_Z(storageSpoolWrite(), STORAGE_SPOOL_ARCHIVE_IN "/000000010000000100000001.error");

        TEST_FILE_REMOVE_Z(
            storageRepoWrite(), STORAGE_REPO_ARCHIVE "/10-1/000000010000000100000001-abcdabcdabcdabcdabcdabcdabcdabcdabcdabcd.gz");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("single segment");

        TEST_FILE_PUT_EMPTY_Z(
            storageRepoWrite(), STORAGE_REPO_ARCHIVE "/10-1/000000010000000100000001-abcdabcdabcdabcdabcdabcdabcdabcdabcdabcd");

        // There should be a temp file from the previous error. Make sure it exists to test that temp files are removed on retry.
        TEST_FILE_EXISTS_Z(storageSpool(), STORAGE_SPOOL_ARCHIVE_IN "/000000010000000100000001.pgbackrest.tmp");

        TEST_RESULT_VOID(cmdArchiveGetAsync(), "archive async");

        harnessLogResult(
            "P00   INFO: get 1 WAL file(s) from archive: 000000010000000100000001\n"
            "P01 DETAIL: found 000000010000000100000001 in the archive");

        TEST_FILE_LIST_REMOVE_Z(storageSpoolWrite(), STORAGE_SPOOL_ARCHIVE_IN, "000000010000000100000001");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("multiple segments where some are missing or errored");

        argList = strLstDup(argBaseList);
        strLstAddZ(argList, "000000010000000100000001");
        strLstAddZ(argList, "000000010000000100000002");
        strLstAddZ(argList, "000000010000000100000003");
        harnessCfgLoadRole(cfgCmdArchiveGet, cfgCmdRoleAsync, argList);

        // Create segment duplicates
        TEST_FILE_PUT_EMPTY_Z(
            storageRepoWrite(), STORAGE_REPO_ARCHIVE "/10-1/000000010000000100000003-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        TEST_FILE_PUT_EMPTY_Z(
            storageRepoWrite(), STORAGE_REPO_ARCHIVE "/10-1/000000010000000100000003-bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

        TEST_RESULT_VOID(cmdArchiveGetAsync(), "archive async");

        harnessLogResult(
            "P00   INFO: get 3 WAL file(s) from archive: 000000010000000100000001...000000010000000100000003\n"
            "P01 DETAIL: found 000000010000000100000001 in the archive\n"
            "P00 DETAIL: unable to find 000000010000000100000002 in the archive");

        TEST_FILE_LIST_REMOVE_Z(
            storageSpoolWrite(), STORAGE_SPOOL_ARCHIVE_IN, "000000010000000100000001|000000010000000100000002.ok");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("error on duplicates now that no segments are missing");

        TEST_FILE_PUT_EMPTY_Z(
            storageRepoWrite(), STORAGE_REPO_ARCHIVE "/10-1/000000010000000100000002-efefefefefefefefefefefefefefefefefefefef");

        TEST_RESULT_VOID(cmdArchiveGetAsync(), "archive async");

        harnessLogResult(
            "P00   INFO: get 3 WAL file(s) from archive: 000000010000000100000001...000000010000000100000003\n"
            "P01 DETAIL: found 000000010000000100000001 in the archive\n"
            "P01 DETAIL: found 000000010000000100000002 in the archive\n"
            "P00   WARN: could not get 000000010000000100000003 from the archive (will be retried): "
                "[45] duplicates found in archive for WAL segment 000000010000000100000003: "
                "000000010000000100000003-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa, "
                "000000010000000100000003-bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n"
            "            HINT: are multiple primaries archiving to this stanza?");

        TEST_FILE_LIST_REMOVE_Z(
            storageSpoolWrite(), STORAGE_SPOOL_ARCHIVE_IN,
            "000000010000000100000001|000000010000000100000002|000000010000000100000003.error");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("global error on invalid executable");

        argList = strLstNew();
        strLstAddZ(argList, "pgbackrest-bogus");
        hrnCfgArgRawZ(argList, cfgOptPgPath, TEST_PATH_PG);
        hrnCfgArgRawZ(argList, cfgOptRepoPath, TEST_PATH_REPO);
        hrnCfgArgRawZ(argList, cfgOptSpoolPath, TEST_PATH_SPOOL);
        hrnCfgArgRawBool(argList, cfgOptArchiveAsync, true);
        hrnCfgArgRawZ(argList, cfgOptStanza, "test2");
        strLstAddZ(argList, CFGCMD_ARCHIVE_GET ":" CONFIG_COMMAND_ROLE_ASYNC);
        strLstAddZ(argList, "000000010000000100000001");
        strLstAddZ(argList, "000000010000000100000002");
        strLstAddZ(argList, "000000010000000100000003");
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        TEST_ERROR(
            cmdArchiveGetAsync(), ExecuteError,
            "local-1 process terminated unexpectedly [102]: unable to execute 'pgbackrest-bogus': [2] No such file or directory");

        harnessLogResult(
            "P00   INFO: get 3 WAL file(s) from archive: 000000010000000100000001...000000010000000100000003");

        TEST_RESULT_STR_Z(
            strNewBuf(storageGetP(storageNewReadP(storageSpool(), strNew(STORAGE_SPOOL_ARCHIVE_IN "/global.error")))),
            "102\nlocal-1 process terminated unexpectedly [102]: unable to execute 'pgbackrest-bogus': "
                "[2] No such file or directory",
            "check global error");

        TEST_FILE_LIST_REMOVE_Z(storageSpoolWrite(), STORAGE_SPOOL_ARCHIVE_IN, "global.error");
    }

    // *****************************************************************************************************************************
    if (testBegin("cmdArchiveGet()"))
    {
        harnessLogLevelSet(logLevelDetail);

        // Arguments that must be included. Use raw config here because we need to keep the
        StringList *argBaseList = strLstNew();
        strLstAddZ(argBaseList, "pgbackrest-bogus");
        hrnCfgArgRawZ(argBaseList, cfgOptPgPath, TEST_PATH_PG);
        hrnCfgArgRawZ(argBaseList, cfgOptRepoPath, TEST_PATH_REPO);
        hrnCfgArgRawZ(argBaseList, cfgOptStanza, "test1");
        hrnCfgArgRawZ(argBaseList, cfgOptArchiveTimeout, "1");
        strLstAddZ(argBaseList, CFGCMD_ARCHIVE_GET);

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("command must be run on the pg host");

        StringList *argList = strLstDup(argBaseList);
        hrnCfgArgRawZ(argList, cfgOptPgHost, BOGUS_STR);
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        TEST_ERROR(cmdArchiveGet(), HostInvalidError, "archive-get command must be run on the PostgreSQL host");

        // -------------------------------------------------------------------------------------------------------------------------
        argList = strLstDup(argBaseList);
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        TEST_ERROR(cmdArchiveGet(), ParamRequiredError, "WAL segment to get required");

        // -------------------------------------------------------------------------------------------------------------------------
        argList = strLstDup(argBaseList);
        String *walSegment = strNew("000000010000000100000001");
        strLstAdd(argList, walSegment);
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        TEST_ERROR(cmdArchiveGet(), ParamRequiredError, "path to copy WAL segment required");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_FILE_PUT_Z_BUF(
            storagePgWrite(), PG_PATH_GLOBAL "/" PG_FILE_PGCONTROL,
            pgControlTestToBuffer((PgControl){.version = PG_VERSION_10, .systemId = 0xFACEFACEFACEFACE}));

        storagePathCreateP(storageTest, strNewFmt("%s/pg/pg_wal", testPath()));

        const String *walFile = strNewFmt("%s/pg/pg_wal/RECOVERYXLOG", testPath());
        strLstAdd(argList, walFile);
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        TEST_ERROR_FMT(
            cmdArchiveGet(), FileMissingError,
            "unable to load info file '%s/archive/test1/archive.info' or '%s/archive/test1/archive.info.copy':\n"
            "FileMissingError: " STORAGE_ERROR_READ_MISSING "\n"
            "FileMissingError: " STORAGE_ERROR_READ_MISSING "\n"
            "HINT: archive.info cannot be opened but is required to push/get WAL segments.\n"
            "HINT: is archive_command configured correctly in postgresql.conf?\n"
            "HINT: has a stanza-create been performed?\n"
            "HINT: use --no-archive-check to disable archive checks during backup if you have an alternate archiving"
                " scheme.",
            strZ(cfgOptionStr(cfgOptRepoPath)), strZ(cfgOptionStr(cfgOptRepoPath)),
            strZ(strNewFmt("%s/archive/test1/archive.info", strZ(cfgOptionStr(cfgOptRepoPath)))),
            strZ(strNewFmt("%s/archive/test1/archive.info.copy", strZ(cfgOptionStr(cfgOptRepoPath)))));

        // -------------------------------------------------------------------------------------------------------------------------
        argList = strLstDup(argBaseList);
        strLstAddZ(argList, "00000001.history");
        strLstAdd(argList, walFile);
        strLstAddZ(argList, "--archive-async");
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        TEST_ERROR_FMT(
            cmdArchiveGet(), FileMissingError,
            "unable to load info file '%s/archive/test1/archive.info' or '%s/archive/test1/archive.info.copy':\n"
            "FileMissingError: " STORAGE_ERROR_READ_MISSING "\n"
            "FileMissingError: " STORAGE_ERROR_READ_MISSING "\n"
            "HINT: archive.info cannot be opened but is required to push/get WAL segments.\n"
            "HINT: is archive_command configured correctly in postgresql.conf?\n"
            "HINT: has a stanza-create been performed?\n"
            "HINT: use --no-archive-check to disable archive checks during backup if you have an alternate archiving"
                " scheme.",
            strZ(cfgOptionStr(cfgOptRepoPath)), strZ(cfgOptionStr(cfgOptRepoPath)),
            strZ(strNewFmt("%s/archive/test1/archive.info", strZ(cfgOptionStr(cfgOptRepoPath)))),
            strZ(strNewFmt("%s/archive/test1/archive.info.copy", strZ(cfgOptionStr(cfgOptRepoPath)))));

        // Make sure the process times out when there is nothing to get
        // -------------------------------------------------------------------------------------------------------------------------
        argList = strLstDup(argBaseList);
        hrnCfgArgRawZ(argList, cfgOptSpoolPath, TEST_PATH_SPOOL);
        hrnCfgArgRawBool(argList, cfgOptArchiveAsync, true);
        strLstAdd(argList, walSegment);
        strLstAddZ(argList, "pg_wal/RECOVERYXLOG");
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        THROW_ON_SYS_ERROR(chdir(strZ(cfgOptionStr(cfgOptPgPath))) != 0, PathMissingError, "unable to chdir()");

        TEST_RESULT_INT(cmdArchiveGet(), 1, "timeout getting WAL segment");

        harnessLogResult("P00   INFO: unable to find 000000010000000100000001 in the archive");

        // Check for missing WAL
        // -------------------------------------------------------------------------------------------------------------------------
        storagePutP(storageNewWriteP(storageSpoolWrite(), strNewFmt(STORAGE_SPOOL_ARCHIVE_IN "/%s.ok", strZ(walSegment))), NULL);

        TEST_RESULT_INT(cmdArchiveGet(), 1, "successful get of missing WAL");

        harnessLogResult("P00   INFO: unable to find 000000010000000100000001 in the archive");

        TEST_RESULT_BOOL(
            storageExistsP(storageSpool(), strNewFmt(STORAGE_SPOOL_ARCHIVE_IN "/%s.ok", strZ(walSegment))), false,
            "check OK file was removed");

        // Write out a WAL segment for success
        // -------------------------------------------------------------------------------------------------------------------------
        storagePutP(
            storageNewWriteP(storageSpoolWrite(), strNewFmt(STORAGE_SPOOL_ARCHIVE_IN "/%s", strZ(walSegment))),
            BUFSTRDEF("SHOULD-BE-A-REAL-WAL-FILE"));

        TEST_RESULT_INT(cmdArchiveGet(), 0, "successful get");

        TEST_RESULT_VOID(harnessLogResult("P00   INFO: found 000000010000000100000001 in the archive"), "check log");

        TEST_FILE_LIST_Z(storageSpool(), STORAGE_SPOOL_ARCHIVE_IN, "");
        TEST_FILE_LIST_REMOVE_STR_Z(storageTest, strPath(walFile), "RECOVERYXLOG");

        // Write more WAL segments (in this case queue should be full)
        // -------------------------------------------------------------------------------------------------------------------------
        strLstAddZ(argList, "--archive-get-queue-max=48");
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        String *walSegment2 = strNew("000000010000000100000002");

        storagePutP(
            storageNewWriteP(storageSpoolWrite(), strNewFmt(STORAGE_SPOOL_ARCHIVE_IN "/%s", strZ(walSegment))),
            BUFSTRDEF("SHOULD-BE-A-REAL-WAL-FILE"));
        storagePutP(
            storageNewWriteP(storageSpoolWrite(), strNewFmt(STORAGE_SPOOL_ARCHIVE_IN "/%s", strZ(walSegment2))),
            BUFSTRDEF("SHOULD-BE-A-REAL-WAL-FILE"));

        TEST_RESULT_INT(cmdArchiveGet(), 0, "successful get");

        TEST_RESULT_VOID(harnessLogResult("P00   INFO: found 000000010000000100000001 in the archive"), "check log");

        TEST_FILE_LIST_REMOVE_STR_Z(storageTest, strPath(walFile), "RECOVERYXLOG");
        TEST_FILE_LIST_STR_Z(storageTest, strPath(walFile), "");

        // Make sure the process times out when it can't get a lock
        // -------------------------------------------------------------------------------------------------------------------------
        TEST_RESULT_VOID(
            lockAcquire(
                cfgOptionStr(cfgOptLockPath), cfgOptionStr(cfgOptStanza), STRDEF("999-dededede"), cfgLockType(), 30000, true),
            "acquire lock");
        TEST_RESULT_VOID(lockClear(true), "clear lock");

        TEST_RESULT_INT(cmdArchiveGet(), 1, "timeout waiting for lock");

        harnessLogResult("P00   INFO: unable to find 000000010000000100000001 in the archive");

        // -------------------------------------------------------------------------------------------------------------------------
        strLstAddZ(argList, BOGUS_STR);
        harnessCfgLoadRaw(strLstSize(argList), strLstPtr(argList));

        TEST_ERROR(cmdArchiveGet(), ParamInvalidError, "extra parameters found");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("file is missing");

        // Create pg_control file
        storagePutP(
            storageNewWriteP(storageTest, strNew("pg/" PG_PATH_GLOBAL "/" PG_FILE_PGCONTROL)),
            pgControlTestToBuffer((PgControl){.version = PG_VERSION_10, .systemId = 0xFACEFACEFACEFACE}));

        // Create archive.info
        storagePutP(
            storageNewWriteP(storageTest, strNew("repo/archive/test1/archive.info")),
            harnessInfoChecksumZ(
                "[db]\n"
                "db-id=1\n"
                "\n"
                "[db:history]\n"
                "1={\"db-id\":18072658121562454734,\"db-version\":\"10\"}"));

        walFile = STRDEF("01ABCDEF01ABCDEF01ABCDEF");
        const String *walDestination = strNewFmt("%s/pg/pg_wal/RECOVERYXLOG", testPath());

        argList = strLstNew();
        hrnCfgArgRawZ(argList, cfgOptPgPath, TEST_PATH_PG);
        hrnCfgArgRawZ(argList, cfgOptRepoPath, TEST_PATH_REPO);
        hrnCfgArgRawZ(argList, cfgOptStanza, "test1");
        strLstAdd(argList, walFile);
        strLstAdd(argList, walDestination);
        harnessCfgLoad(cfgCmdArchiveGet, argList);

        TEST_RESULT_INT(cmdArchiveGet(), 1, "get");

        harnessLogResult("P00   INFO: unable to find 01ABCDEF01ABCDEF01ABCDEF in the archive");

        TEST_FILE_LIST_STR_Z(storageTest, strPath(walDestination), "");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("get WAL segment");

        Buffer *buffer = bufNew(16 * 1024 * 1024);
        memset(bufPtr(buffer), 0, bufSize(buffer));
        bufUsedSet(buffer, bufSize(buffer));

        TEST_FILE_PUT_Z_BUF(
            storageRepoWrite(), STORAGE_REPO_ARCHIVE "/10-1/01ABCDEF01ABCDEF01ABCDEF-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            buffer);

        TEST_RESULT_INT(cmdArchiveGet(), 0, "get");

        harnessLogResult("P00   INFO: found 01ABCDEF01ABCDEF01ABCDEF in the archive");

        TEST_RESULT_UINT(storageInfoP(storageTest, walDestination).size, 16 * 1024 * 1024, "check size");
        TEST_FILE_LIST_REMOVE_STR_Z(storageTest, strPath(walDestination), "RECOVERYXLOG");

        TEST_FILE_REMOVE_Z(
            storageRepoWrite(), STORAGE_REPO_ARCHIVE "/10-1/01ABCDEF01ABCDEF01ABCDEF-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("get compressed and encrypted WAL segment");

        // Create encrypted archive.info
        StorageWrite *infoWrite = storageNewWriteP(storageTest, strNew("repo/archive/test1/archive.info"));

        ioFilterGroupAdd(
            ioWriteFilterGroup(storageWriteIo(infoWrite)), cipherBlockNew(cipherModeEncrypt, cipherTypeAes256Cbc,
            BUFSTRDEF("12345678"), NULL));

        storagePutP(
            infoWrite,
            harnessInfoChecksumZ(
                "[cipher]\n"
                "cipher-pass=\"worstpassphraseever\"\n"
                "\n"
                "[db]\n"
                "db-id=1\n"
                "\n"
                "[db:history]\n"
                "1={\"db-id\":18072658121562454734,\"db-version\":\"10\"}"));

        // Create compressed and encrypted WAL segment
        StorageWrite *destination = storageNewWriteP(
            storageTest,
            strNew(
                "repo/archive/test1/10-1/01ABCDEF01ABCDEF/01ABCDEF01ABCDEF01ABCDEF-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.gz"));

        IoFilterGroup *filterGroup = ioWriteFilterGroup(storageWriteIo(destination));
        ioFilterGroupAdd(filterGroup, compressFilter(compressTypeGz, 3));
        ioFilterGroupAdd(
            filterGroup, cipherBlockNew(cipherModeEncrypt, cipherTypeAes256Cbc, BUFSTRDEF("worstpassphraseever"), NULL));
        storagePutP(destination, buffer);

        // Add encryption options
        hrnCfgArgRawZ(argList, cfgOptRepoCipherType, CIPHER_TYPE_AES_256_CBC);
        hrnCfgEnvRawZ(cfgOptRepoCipherPass, "12345678");
        harnessCfgLoad(cfgCmdArchiveGet, argList);
        hrnCfgEnvRemoveRaw(cfgOptRepoCipherPass);

        TEST_RESULT_INT(cmdArchiveGet(), 0, "get");

        harnessLogResult("P00   INFO: found 01ABCDEF01ABCDEF01ABCDEF in the archive");

        TEST_FILE_LIST_STR_Z(storageTest, strPath(walDestination), "RECOVERYXLOG");
        TEST_RESULT_UINT(storageInfoP(storageTest, walDestination).size, 16 * 1024 * 1024, "check size");

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("check protocol function directly");

        // Start a protocol server
        Buffer *serverWrite = bufNew(8192);
        IoWrite *serverWriteIo = ioBufferWriteNew(serverWrite);
        ioWriteOpen(serverWriteIo);

        ProtocolServer *server = protocolServerNew(
            strNew("test"), strNew("test"), ioBufferReadNew(bufNew(0)), serverWriteIo);

        bufUsedSet(serverWrite, 0);

        // Add spool path
        argList = strLstNew();
        hrnCfgArgRawZ(argList, cfgOptPgPath, TEST_PATH_PG);
        hrnCfgArgRawZ(argList, cfgOptRepoPath, TEST_PATH_REPO);
        hrnCfgArgRawZ(argList, cfgOptStanza, "test1");
        hrnCfgArgRawZ(argList, cfgOptRepoCipherType, CIPHER_TYPE_AES_256_CBC);
        hrnCfgArgRawZ(argList, cfgOptSpoolPath, TEST_PATH_SPOOL);
        hrnCfgArgRawBool(argList, cfgOptArchiveAsync, true);
        hrnCfgEnvRawZ(cfgOptRepoCipherPass, "12345678");
        harnessCfgLoadRole(cfgCmdArchiveGet, cfgCmdRoleLocal, argList);
        hrnCfgEnvRemoveRaw(cfgOptRepoCipherPass);

        // Setup protocol command
        VariantList *paramList = varLstNew();
        varLstAdd(paramList, varNewStrZ("01ABCDEF01ABCDEF01ABCDEF"));
        varLstAdd(
            paramList, varNewStrZ("10-1/01ABCDEF01ABCDEF/01ABCDEF01ABCDEF01ABCDEF-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.gz"));
        varLstAdd(paramList, varNewUInt(cipherTypeAes256Cbc));
        varLstAdd(paramList, varNewStrZ("worstpassphraseever"));

        TEST_RESULT_BOOL(
            archiveGetProtocol(PROTOCOL_COMMAND_ARCHIVE_GET_STR, paramList, server), true, "protocol archive get");

        TEST_RESULT_STR_Z(strNewBuf(serverWrite), "{}\n", "check result");
        TEST_FILE_LIST_Z(storageSpool(), STORAGE_SPOOL_ARCHIVE_IN, "000000010000000100000002|01ABCDEF01ABCDEF01ABCDEF");

        bufUsedSet(serverWrite, 0);

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("check invalid protocol function");

        TEST_RESULT_BOOL(archiveGetProtocol(strNew(BOGUS_STR), paramList, server), false, "invalid function");
    }

    FUNCTION_HARNESS_RESULT_VOID();
}
