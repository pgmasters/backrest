/***********************************************************************************************************************************
Test LZ4
***********************************************************************************************************************************/
#include "common/io/filter/group.h"
#include "common/io/bufferRead.h"
#include "common/io/bufferWrite.h"
#include "common/io/io.h"

/***********************************************************************************************************************************
Compress data
***********************************************************************************************************************************/
// static Buffer *
// testCompress(GzipCompress *compress, Buffer *decompressed, size_t inputSize, size_t outputSize)
// {
//     Buffer *compressed = bufNew(1024 * 1024);
//     size_t inputTotal = 0;
//     ioBufferSizeSet(outputSize);
//
//     IoFilterGroup *filterGroup = ioFilterGroupNew();
//     ioFilterGroupAdd(filterGroup, gzipCompressFilter(compress));
//     IoWrite *write = ioBufferWriteIo(ioBufferWriteNew(compressed));
//     ioWriteFilterGroupSet(write, filterGroup);
//     ioWriteOpen(write);
//
//     // Compress input data
//     while (inputTotal < bufSize(decompressed))
//     {
//         // Generate the input buffer based on input size.  This breaks the data up into chunks as it would be in a real scenario.
//         Buffer *input = bufNewC(
//             inputSize > bufSize(decompressed) - inputTotal ? bufSize(decompressed) - inputTotal : inputSize,
//             bufPtr(decompressed) + inputTotal);
//
//         ioWrite(write, input);
//
//         inputTotal += bufUsed(input);
//         bufFree(input);
//     }
//
//     ioWriteClose(write);
//     gzipCompressFree(compress);
//
//     return compressed;
// }

/***********************************************************************************************************************************
Decompress data
***********************************************************************************************************************************/
// static Buffer *
// testDecompress(GzipDecompress *decompress, Buffer *compressed, size_t inputSize, size_t outputSize)
// {
//     Buffer *decompressed = bufNew(1024 * 1024);
//     Buffer *output = bufNew(outputSize);
//     ioBufferSizeSet(inputSize);
//
//     IoFilterGroup *filterGroup = ioFilterGroupNew();
//     ioFilterGroupAdd(filterGroup, gzipDecompressFilter(decompress));
//     IoRead *read = ioBufferReadIo(ioBufferReadNew(compressed));
//     ioReadFilterGroupSet(read, filterGroup);
//     ioReadOpen(read);
//
//     while (!ioReadEof(read))
//     {
//         ioRead(read, output);
//         bufCat(decompressed, output);
//         bufUsedZero(output);
//     }
//
//     ioReadClose(read);
//     bufFree(output);
//     gzipDecompressFree(decompress);
//
//     return decompressed;
// }

/***********************************************************************************************************************************
Test Run
***********************************************************************************************************************************/
void
testRun(void)
{
    FUNCTION_HARNESS_VOID();

    // *****************************************************************************************************************************
    if (testBegin("lz4Error"))
    {
        #ifndef WITHOUT_LZ4

        TEST_RESULT_UINT(lz4Error(0), 0, "check success");
        TEST_ERROR(lz4Error((size_t)-2), FormatError, "lz4 error: [18446744073709551614] ERROR_maxBlockSize_invalid");

        #endif // WITHOUT_LZ4
    }

    // *****************************************************************************************************************************
    if (testBegin("Lz4Compress and Lz4Decompress"))
    {
        #ifndef WITHOUT_LZ4

        // const char *simpleData = "A simple string";
        // Buffer *compressed = NULL;
        // Buffer *decompressed = bufNewC(strlen(simpleData), simpleData);
        //
        // TEST_ASSIGN(
        //     compressed, testCompress(gzipCompressNew(3, false), decompressed, 1024, 1024),
        //     "simple data - compress large in/large out buffer");
        //
        // TEST_RESULT_BOOL(
        //     bufEq(compressed, testCompress(gzipCompressNew(3, false), decompressed, 1024, 1)), true,
        //     "simple data - compress large in/small out buffer");
        //
        // TEST_RESULT_BOOL(
        //     bufEq(compressed, testCompress(gzipCompressNew(3, false), decompressed, 1, 1024)), true,
        //     "simple data - compress small in/large out buffer");
        //
        // TEST_RESULT_BOOL(
        //     bufEq(compressed, testCompress(gzipCompressNew(3, false), decompressed, 1, 1)), true,
        //     "simple data - compress small in/small out buffer");
        //
        // TEST_RESULT_BOOL(
        //     bufEq(decompressed, testDecompress(gzipDecompressNew(false), compressed, 1024, 1024)), true,
        //     "simple data - decompress large in/large out buffer");
        //
        // TEST_RESULT_BOOL(
        //     bufEq(decompressed, testDecompress(gzipDecompressNew(false), compressed, 1024, 1)), true,
        //     "simple data - decompress large in/small out buffer");
        //
        // TEST_RESULT_BOOL(
        //     bufEq(decompressed, testDecompress(gzipDecompressNew(false), compressed, 1, 1024)), true,
        //     "simple data - decompress small in/large out buffer");
        //
        // TEST_RESULT_BOOL(
        //     bufEq(decompressed, testDecompress(gzipDecompressNew(false), compressed, 1, 1)), true,
        //     "simple data - decompress small in/small out buffer");
        //
        // // Compress a large zero input buffer into small output buffer
        // // -------------------------------------------------------------------------------------------------------------------------
        // decompressed = bufNew(1024 * 1024 - 1);
        // memset(bufPtr(decompressed), 0, bufSize(decompressed));
        // bufUsedSet(decompressed, bufSize(decompressed));
        //
        // TEST_ASSIGN(
        //     compressed, testCompress(gzipCompressNew(3, true), decompressed, bufSize(decompressed), 1024),
        //     "zero data - compress large in/small out buffer");
        //
        // TEST_RESULT_BOOL(
        //     bufEq(decompressed, testDecompress(gzipDecompressNew(true), compressed, bufSize(compressed), 1024 * 256)), true,
        //     "zero data - decompress large in/small out buffer");
        //
        // // -------------------------------------------------------------------------------------------------------------------------
        // TEST_RESULT_VOID(gzipCompressFree(NULL), "free null decompress object");
        // TEST_RESULT_VOID(gzipDecompressFree(NULL), "free null decompress object");

        #endif // WITHOUT_LZ4
    }

    // *****************************************************************************************************************************
    if (testBegin("lz4DecompressToLog() and lz4CompressToLog()"))
    {
        #ifndef WITHOUT_LZ4

        // GzipDecompress *decompress = gzipDecompressNew(false);
        //
        // TEST_RESULT_STR(strPtr(gzipDecompressToLog(decompress)), "{inputSame: false, done: false, availIn: 0}", "format object");
        //
        // decompress->inputSame = true;
        // decompress->done = true;
        // TEST_RESULT_STR(strPtr(gzipDecompressToLog(decompress)), "{inputSame: true, done: true, availIn: 0}", "format object");

        #endif // WITHOUT_LZ4
    }

    FUNCTION_HARNESS_RESULT_VOID();
}
