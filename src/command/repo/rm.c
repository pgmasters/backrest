/***********************************************************************************************************************************
Repository Remove Command
***********************************************************************************************************************************/
#include "build.auto.h"

#include "common/debug.h"
#include "common/log.h"
#include "common/memContext.h"
#include "config/config.h"
#include "storage/helper.h"

/**********************************************************************************************************************************/
void
cmdStorageRemove(void)
{
    FUNCTION_LOG_VOID(logLevelDebug);

    // Get path
    const String *path = NULL;

    if (strLstSize(cfgCommandParam()) == 1)
        path = strLstGet(cfgCommandParam(), 0);
    else if (strLstSize(cfgCommandParam()) > 1)
        THROW(ParamInvalidError, "only one path may be specified");

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // Check if this is a file
        StorageInfo info = storageInfoP(storageRepo(), path, .ignoreMissing = true);

        if (info.exists && info.type == storageTypeFile)
        {
            storageRemoveP(storageRepoWrite(), path);
        }
        // Else try to remove a path
        else
        {
            bool recurse = cfgOptionBool(cfgOptRecurse);

            if (!recurse && strLstSize(storageListP(storageRepo(), path)) > 0)
                THROW(OptionInvalidError, CFGOPT_RECURSE " option must be used to delete non-empty path");

            storagePathRemoveP(storageRepoWrite(), path, .recurse = recurse);
        }
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}
