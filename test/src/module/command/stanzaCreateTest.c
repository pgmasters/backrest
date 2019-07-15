/***********************************************************************************************************************************
Test Stanza Create Command
***********************************************************************************************************************************/
#include "storage/posix/storage.h"

#include "common/harnessConfig.h"
#include "common/harnessInfo.h"
#include "postgres/version.h"

/***********************************************************************************************************************************
Test Run
***********************************************************************************************************************************/
void
testRun(void)
{
    FUNCTION_HARNESS_VOID();

    Storage *storageTest = storagePosixNew(
        strNew(testPath()), STORAGE_MODE_FILE_DEFAULT, STORAGE_MODE_PATH_DEFAULT, true, NULL);

    String *stanza = strNew("db");
    String *fileName = strNew("test.info");
    String *backupStanzaPath = strNewFmt("repo/backup/%s", strPtr(stanza));
    String *backupInfoFileName = strNewFmt("%s/backup.info", strPtr(backupStanzaPath));
    String *archiveStanzaPath = strNewFmt("repo/archive/%s", strPtr(stanza));
    String *archiveInfoFileName = strNewFmt("%s/archive.info", strPtr(archiveStanzaPath));

    StringList *argList = strLstNew();
    strLstAddZ(argList, "pgbackrest");
    strLstAdd(argList,  strNewFmt("--stanza=%s", strPtr(stanza)));
    strLstAdd(argList, strNewFmt("--pg1-path=%s/%s", testPath(), strPtr(stanza)));
    strLstAdd(argList, strNewFmt("--repo1-path=%s/repo", testPath()));
    strLstAddZ(argList, "stanza-create");


    // *****************************************************************************************************************************
    if (testBegin("cmdStanzaCreate(), infoValidate()"))
    {
        // Load Parameters
        harnessCfgLoad(strLstSize(argList), strLstPtr(argList));

        // Create pg_control
        storagePutNP(
            storageNewWriteNP(storageTest, strNewFmt("%s/" PG_PATH_GLOBAL "/" PG_FILE_PGCONTROL, strPtr(stanza))),
            pgControlTestToBuffer((PgControl){.version = PG_VERSION_96, .systemId = 6569239123849665679}));

        TEST_RESULT_VOID(cmdStanzaCreate(), "stanza create - no files exist");

        String *contentArchive = strNew
        (
            "[db]\n"
            "db-id=1\n"
            "db-system-id=6569239123849665679\n"
            "db-version=\"9.6\"\n"
            "\n"
            "[db:history]\n"
            "1={\"db-id\":6569239123849665679,\"db-version\":\"9.6\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, fileName), harnessInfoChecksum(contentArchive)),
                "put archive info to test file");

        TEST_RESULT_BOOL(
            (bufEq(
                storageGetNP(storageNewReadNP(storageTest, archiveInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest,  strNewFmt("%s" INFO_COPY_EXT, strPtr(archiveInfoFileName))))) &&
            bufEq(
                storageGetNP(storageNewReadNP(storageTest, archiveInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest, fileName)))),
            true, "    test and stanza archive info files are equal");

        String *contentBackup = strNew
        (
            "[db]\n"
            "db-catalog-version=201608131\n"
            "db-control-version=960\n"
            "db-id=1\n"
            "db-system-id=6569239123849665679\n"
            "db-version=\"9.6\"\n"
            "\n"
            "[db:history]\n"
            "1={\"db-catalog-version\":201608131,\"db-control-version\":960,\"db-system-id\":6569239123849665679,"
                "\"db-version\":\"9.6\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, fileName), harnessInfoChecksum(contentBackup)),
                "put backup info to test file");

        TEST_RESULT_BOOL(
            (bufEq(
                storageGetNP(storageNewReadNP(storageTest, backupInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest,  strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName))))) &&
            bufEq(
                storageGetNP(storageNewReadNP(storageTest, backupInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest, fileName)))),
            true, "    test and stanza backup info files are equal");

        //--------------------------------------------------------------------------------------------------------------------------
        TEST_RESULT_VOID(cmdStanzaCreate(), "stanza create - files already exist and both are valid");
        harnessLogResult("P00   INFO: stanza already exists and is valid");

        // Remove backup.info
        TEST_RESULT_VOID(storageRemoveP(storageTest, backupInfoFileName, .errorOnMissing = true), "backup.info removed");
        TEST_RESULT_VOID(cmdStanzaCreate(), "    stanza create - success with archive.info files and only backup.info.copy");
        TEST_RESULT_BOOL(
            bufEq(
                storageGetNP(storageNewReadNP(storageTest, backupInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest,  strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName))))),
            true, "backup.info recreated from backup.info.copy");

        // Remove archive.info
        TEST_RESULT_VOID(storageRemoveP(storageTest, archiveInfoFileName, .errorOnMissing = true), "archive.info removed");
        TEST_RESULT_VOID(cmdStanzaCreate(), "    stanza create - success with backup.info files and only archive.info.copy");
        TEST_RESULT_BOOL(
            bufEq(
                storageGetNP(storageNewReadNP(storageTest, archiveInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest,  strNewFmt("%s" INFO_COPY_EXT, strPtr(archiveInfoFileName))))),
            true, "archive.info recreated from archive.info.copy");

        // Remove info files
        TEST_RESULT_VOID(storageRemoveP(storageTest, archiveInfoFileName, .errorOnMissing = true), "archive.info removed");
        TEST_RESULT_VOID(storageRemoveP(storageTest, backupInfoFileName, .errorOnMissing = true), "backup.info removed");
        TEST_RESULT_VOID(cmdStanzaCreate(), "stanza create - success with copy files only");
        TEST_RESULT_BOOL(
            (bufEq(
                storageGetNP(storageNewReadNP(storageTest, backupInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest,  strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName))))) &&
            bufEq(
                storageGetNP(storageNewReadNP(storageTest, archiveInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest,  strNewFmt("%s" INFO_COPY_EXT, strPtr(archiveInfoFileName)))))),
            true, "info files recreated from copy files");

        // Remove copy files
        TEST_RESULT_VOID(
            storageRemoveP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(archiveInfoFileName)), .errorOnMissing = true),
            "archive.info.copy removed");
        TEST_RESULT_VOID(
            storageRemoveP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName)), .errorOnMissing = true),
            "backup.info.copy removed");
        TEST_RESULT_VOID(cmdStanzaCreate(), "stanza create - success with info files only");
        TEST_RESULT_BOOL(
            (bufEq(
                storageGetNP(storageNewReadNP(storageTest, backupInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest,  strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName))))) &&
            bufEq(
                storageGetNP(storageNewReadNP(storageTest, archiveInfoFileName)),
                storageGetNP(storageNewReadNP(storageTest,  strNewFmt("%s" INFO_COPY_EXT, strPtr(archiveInfoFileName)))))),
            true, "info files recreated from info files");

        // Errors
        //--------------------------------------------------------------------------------------------------------------------------
        // Archive files removed - backup.info and backup.info.copy exist
        TEST_RESULT_VOID(
            storageRemoveP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(archiveInfoFileName)), .errorOnMissing = true),
            "archive.info.copy removed");
        TEST_RESULT_VOID(storageRemoveP(storageTest, archiveInfoFileName, .errorOnMissing = true), "archive.info removed");
        TEST_ERROR_FMT(cmdStanzaCreate(), FileMissingError,
            "backup.info exists but archive.info is missing\n"
            "HINT: this may be a symptom of repository corruption!");

        // Archive files removed - backup.info.copy exists
        TEST_RESULT_VOID(
            storageRemoveP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName)), .errorOnMissing = true),
            "backup.info.copy removed");
        TEST_ERROR_FMT(cmdStanzaCreate(), FileMissingError,
            "backup.info exists but archive.info is missing\n"
            "HINT: this may be a symptom of repository corruption!");

        // Archive files removed - backup.info exists
        TEST_RESULT_VOID(
            storageMoveNP(storageTest,
                storageNewReadNP(storageTest, backupInfoFileName),
                storageNewWriteNP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName)))),
            "backup.info moved to backup.info.copy");
        TEST_ERROR_FMT(cmdStanzaCreate(), FileMissingError,
            "backup.info exists but archive.info is missing\n"
            "HINT: this may be a symptom of repository corruption!");

        // Backup files removed - archive.info file exists
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, archiveInfoFileName), harnessInfoChecksum(contentArchive)),
                "put archive info to file");
        TEST_RESULT_VOID(
            storageRemoveP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName)), .errorOnMissing = true),
            "backup.info.copy removed");
        TEST_ERROR_FMT(cmdStanzaCreate(), FileMissingError,
            "archive.info exists but backup.info is missing\n"
            "HINT: this may be a symptom of repository corruption!");

        // Backup files removed - archive.info.copy file exists
        TEST_RESULT_VOID(
            storageMoveNP(storageTest,
                storageNewReadNP(storageTest, archiveInfoFileName),
                storageNewWriteNP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(archiveInfoFileName)))),
                "archive.info moved to archive.info.copy");
        TEST_ERROR_FMT(cmdStanzaCreate(), FileMissingError,
            "archive.info exists but backup.info is missing\n"
            "HINT: this may be a symptom of repository corruption!");

        // Backup files removed - archive.info and archive.info.copy exist
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, archiveInfoFileName), harnessInfoChecksum(contentArchive)),
                "put archive info to file");
        TEST_ERROR_FMT(cmdStanzaCreate(), FileMissingError,
            "archive.info exists but backup.info is missing\n"
            "HINT: this may be a symptom of repository corruption!");

        // infoValidate()
        //--------------------------------------------------------------------------------------------------------------------------
        // Create a corrupted backup file - db id
        contentBackup = strNew
        (
            "[db]\n"
            "db-catalog-version=201608131\n"
            "db-control-version=960\n"
            "db-id=2\n"
            "db-system-id=6569239123849665679\n"
            "db-version=\"9.6\"\n"
            "\n"
            "[db:history]\n"
            "2={\"db-catalog-version\":201608131,\"db-control-version\":960,\"db-system-id\":6569239123849665679,"
                "\"db-version\":\"9.6\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, backupInfoFileName), harnessInfoChecksum(contentBackup)),
                "put back info to file - bad db-id");

        TEST_ERROR_FMT(cmdStanzaCreate(), FileInvalidError,
            "backup info file and archive info file do not match\n"
            "archive: id = 1, version = 9.6, system-id = 6569239123849665679\n"
            "backup : id = 2, version = 9.6, system-id = 6569239123849665679\n"
            "HINT: this may be a symptom of repository corruption!");

        // Create a corrupted backup file - system id
        contentBackup = strNew
        (
            "[db]\n"
            "db-catalog-version=201608131\n"
            "db-control-version=960\n"
            "db-id=1\n"
            "db-system-id=6569239123849665999\n"
            "db-version=\"9.6\"\n"
            "\n"
            "[db:history]\n"
            "1={\"db-catalog-version\":201608131,\"db-control-version\":960,\"db-system-id\":6569239123849665999,"
                "\"db-version\":\"9.6\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, backupInfoFileName), harnessInfoChecksum(contentBackup)),
                "put back info to file - bad system-id");

        TEST_ERROR_FMT(cmdStanzaCreate(), FileInvalidError,
            "backup info file and archive info file do not match\n"
            "archive: id = 1, version = 9.6, system-id = 6569239123849665679\n"
            "backup : id = 1, version = 9.6, system-id = 6569239123849665999\n"
            "HINT: this may be a symptom of repository corruption!");

        // Create a corrupted backup file - system id and version
        contentBackup = strNew
        (
            "[db]\n"
            "db-catalog-version=201608131\n"
            "db-control-version=960\n"
            "db-id=1\n"
            "db-system-id=6569239123849665999\n"
            "db-version=\"9.5\"\n"
            "\n"
            "[db:history]\n"
            "1={\"db-catalog-version\":201608131,\"db-control-version\":960,\"db-system-id\":6569239123849665999,"
                "\"db-version\":\"9.5\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, backupInfoFileName), harnessInfoChecksum(contentBackup)),
                "put back info to file - bad system-id and version");

        TEST_ERROR_FMT(cmdStanzaCreate(), FileInvalidError,
            "backup info file and archive info file do not match\n"
            "archive: id = 1, version = 9.6, system-id = 6569239123849665679\n"
            "backup : id = 1, version = 9.5, system-id = 6569239123849665999\n"
            "HINT: this may be a symptom of repository corruption!");

        // Create a corrupted backup file - version
        contentBackup = strNew
        (
            "[db]\n"
            "db-catalog-version=201608131\n"
            "db-control-version=960\n"
            "db-id=1\n"
            "db-system-id=6569239123849665679\n"
            "db-version=\"9.5\"\n"
            "\n"
            "[db:history]\n"
            "1={\"db-catalog-version\":201608131,\"db-control-version\":960,\"db-system-id\":6569239123849665679,"
                "\"db-version\":\"9.5\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, backupInfoFileName), harnessInfoChecksum(contentBackup)),
                "put back info to file - bad version");

        TEST_ERROR_FMT(cmdStanzaCreate(), FileInvalidError,
            "backup info file and archive info file do not match\n"
            "archive: id = 1, version = 9.6, system-id = 6569239123849665679\n"
            "backup : id = 1, version = 9.5, system-id = 6569239123849665679\n"
            "HINT: this may be a symptom of repository corruption!");

        //--------------------------------------------------------------------------------------------------------------------------
        // Copy files may or may not exist - remove
        storageRemoveP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(archiveInfoFileName)));
        storageRemoveP(storageTest, strNewFmt("%s" INFO_COPY_EXT, strPtr(backupInfoFileName)));

        // Create an archive.info file that matches the backup.info file but does not match the current database version
        contentArchive = strNew
        (
            "[db]\n"
            "db-id=1\n"
            "db-system-id=6569239123849665679\n"
            "db-version=\"9.5\"\n"
            "\n"
            "[db:history]\n"
            "1={\"db-id\":6569239123849665679,\"db-version\":\"9.5\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, archiveInfoFileName), harnessInfoChecksum(contentArchive)),
                "put archive info file");

        TEST_ERROR_FMT(
            cmdStanzaCreate(), FileInvalidError, "backup and archive info files already exist but do not match the database\n"
            "HINT: is this the correct stanza?\n"
            "HINT: did an error occur during stanza-upgrade?");

        // Create archive.info and backup.info files that match but do not match the current database system-id
        contentArchive = strNew
        (
            "[db]\n"
            "db-id=1\n"
            "db-system-id=6569239123849665999\n"
            "db-version=\"9.6\"\n"
            "\n"
            "[db:history]\n"
            "1={\"db-id\":6569239123849665999,\"db-version\":\"9.6\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, archiveInfoFileName), harnessInfoChecksum(contentArchive)),
                "put archive info to file");

        contentBackup = strNew
        (
            "[db]\n"
            "db-catalog-version=201608131\n"
            "db-control-version=960\n"
            "db-id=1\n"
            "db-system-id=6569239123849665999\n"
            "db-version=\"9.6\"\n"
            "\n"
            "[db:history]\n"
            "1={\"db-catalog-version\":201608131,\"db-control-version\":960,\"db-system-id\":6569239123849665999,"
                "\"db-version\":\"9.6\"}\n"
        );
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, backupInfoFileName), harnessInfoChecksum(contentBackup)),
                "put back info to file");

        TEST_ERROR_FMT(
            cmdStanzaCreate(), FileInvalidError, "backup and archive info files already exist but do not match the database\n"
            "HINT: is this the correct stanza?\n"
            "HINT: did an error occur during stanza-upgrade?");

        // Remove the info files and add sub directory to backup
        TEST_RESULT_VOID(storageRemoveP(storageTest, archiveInfoFileName, .errorOnMissing = true), "archive.info removed");
        TEST_RESULT_VOID(storageRemoveP(storageTest, backupInfoFileName, .errorOnMissing = true), "backup.info removed");

        TEST_RESULT_VOID(storagePathCreateNP(storageTest, strNewFmt("%s/backup.history", strPtr(backupStanzaPath))),
            "create directory in backup");
        TEST_ERROR_FMT(cmdStanzaCreate(), PathNotEmptyError, "backup directory not empty");

        // File in archive, directory in backup
        TEST_RESULT_VOID(
            storagePutNP(
                storageNewWriteNP(storageTest, strNewFmt("%s/somefile", strPtr(archiveStanzaPath))), BUFSTRDEF("some content")),
            "create file in archive");
        TEST_ERROR_FMT(cmdStanzaCreate(), PathNotEmptyError, "backup directory and/or archive directory not empty");

        // File in archive, backup empty
        TEST_RESULT_VOID(
            storagePathRemoveP(storageTest, strNewFmt("%s/backup.history", strPtr(backupStanzaPath))), "remove backup subdir");
        TEST_ERROR_FMT(cmdStanzaCreate(), PathNotEmptyError, "archive directory not empty");
    }

    // *****************************************************************************************************************************
    if (testBegin("cmdStanzaCreate() - encryption"))
    {
        strLstAddZ(argList, "--repo1-cipher-type=aes-256-cbc");
        setenv("PGBACKREST_REPO1_CIPHER_PASS", "12345678", true);
        harnessCfgLoad(strLstSize(argList), strLstPtr(argList));

        // Create pg_control
        storagePutNP(
            storageNewWriteNP(storageTest, strNewFmt("%s/" PG_PATH_GLOBAL "/" PG_FILE_PGCONTROL, strPtr(stanza))),
            pgControlTestToBuffer((PgControl){.version = PG_VERSION_96, .systemId = 6569239123849665679}));

        TEST_RESULT_VOID(cmdStanzaCreate(), "stanza create - encryption");

        InfoArchive *infoArchive = NULL;
        TEST_ASSIGN(
            infoArchive, infoArchiveNewLoad(storageTest, archiveInfoFileName, cipherTypeAes256Cbc, strNew("12345678")),
            "  load archive info");
        TEST_RESULT_PTR_NE(infoArchiveCipherPass(infoArchive), NULL, "  cipher sub set");

        InfoBackup *infoBackup = NULL;
        TEST_ASSIGN(
            infoBackup, infoBackupNewLoad(storageTest, backupInfoFileName, cipherTypeAes256Cbc, strNew("12345678")),
            "  load backup info");
        TEST_RESULT_PTR_NE(infoBackupCipherPass(infoBackup), NULL, "  cipher sub set");
    }

    FUNCTION_HARNESS_RESULT_VOID();
}
