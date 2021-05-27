/***********************************************************************************************************************************
Test CIFS Storage
***********************************************************************************************************************************/
#include "common/harnessConfig.h"

/***********************************************************************************************************************************
Test Run
***********************************************************************************************************************************/
void
testRun(void)
{
    FUNCTION_HARNESS_VOID();

    // *****************************************************************************************************************************
    if (testBegin("storageRepoGet() and StorageDriverCifs"))
    {
        // Load configuration
        StringList *argList = strLstNew();
        strLstAddZ(argList, "--stanza=db");
        hrnCfgArgRawZ(argList, cfgOptPgPath, "/path/to/pg");
        strLstAddZ(argList, "--repo1-type=cifs");
        strLstAddZ(argList, "--repo1-path=" TEST_PATH);
        harnessCfgLoad(cfgCmdArchiveGet, argList);

        const Storage *storage = NULL;
        TEST_ASSIGN(storage, storageRepoGet(0, true), "get cifs repo storage");
        TEST_RESULT_UINT(storageType(storage), STORAGE_CIFS_TYPE, "check storage type");
        TEST_RESULT_BOOL(storageFeature(storage, storageFeaturePath), true, "    check path feature");
        TEST_RESULT_BOOL(storageFeature(storage, storageFeatureCompress), true, "    check compress feature");

        // Create a FileWrite object with path sync enabled and ensure that path sync is false in the write object
        // -------------------------------------------------------------------------------------------------------------------------
        StorageWrite *file = NULL;
        TEST_ASSIGN(file, storageNewWriteP(storage, STRDEF("somefile"), .noSyncPath = false), "new file write");

        TEST_RESULT_BOOL(storageWriteSyncPath(file), false, "path sync is disabled");

        // Test the path sync function -- pass a bogus path to ensure that this is a noop
        // -------------------------------------------------------------------------------------------------------------------------
        TEST_RESULT_VOID(storagePathSyncP(storage, STRDEF(BOGUS_STR)), "path sync is a noop");
    }

    FUNCTION_HARNESS_RETURN_VOID();
}
