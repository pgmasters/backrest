/***********************************************************************************************************************************
LZ4 Common
***********************************************************************************************************************************/
#ifndef WITHOUT_LZ4

#include <lz4frame.h>

#include "common/compress/lz4/common.h"
#include "common/debug.h"

/***********************************************************************************************************************************
Process gzip errors
***********************************************************************************************************************************/
LZ4F_errorCode_t
lz4Error(LZ4F_errorCode_t error)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(SIZE, error);
    FUNCTION_TEST_END();

    if (LZ4F_isError(error))
        THROW_FMT(FormatError, "lz4 error: [%zu] %s", error, LZ4F_getErrorName(error));

    FUNCTION_TEST_RETURN(error);
}

#endif // WITHOUT_LZ4
