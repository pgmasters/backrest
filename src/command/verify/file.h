/***********************************************************************************************************************************
Verify File
***********************************************************************************************************************************/
#ifndef COMMAND_VERIFY_FILE_H
#define COMMAND_VERIFY_FILE_H

#include "common/compress/helper.h"
#include "common/crypto/common.h"

/***********************************************************************************************************************************
Verify file types
***********************************************************************************************************************************/
typedef enum
{
    verifyFileArchive,
    verifyFileBackup,
} VerifyFileType;

/***********************************************************************************************************************************
File result
***********************************************************************************************************************************/
typedef enum
{
    verifyOk,                                                       // default result - file OK
    verifyFileMissing,
    verifyChecksumMismatch,
    verifySizeInvalid,
} VerifyResult;

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Verify a file in the pgBackRest repository
typedef struct VerifyFileResult
{
    VerifyResult fileResult;
    VerifyFileType fileType;
} VerifyFileResult;

VerifyFileResult verifyFile(
    const String *fileName, VerifyFileType type, const String *fileChecksum, uint64_t fileSize, CompressType fileCompressType,
    CipherType cipherType, const String *cipherPass);

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_LOG_VERIFY_FILE_RESULT_TYPE                                                                                       \
    VerifyFileResult
#define FUNCTION_LOG_VERIFY_FILE_RESULT_FORMAT(value, buffer, bufferSize)                                                          \
    objToLog(&value, "VerifyFileResult", buffer, bufferSize)

#endif