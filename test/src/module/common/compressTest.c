/***********************************************************************************************************************************
Test Compression
***********************************************************************************************************************************/

/***********************************************************************************************************************************
Test Run
***********************************************************************************************************************************/
void
testRun(void)
{
    FUNCTION_HARNESS_VOID();

    // *****************************************************************************************************************************
    if (testBegin("compress*()"))
    {
        TEST_TITLE("no compression");
        {
            TEST_RESULT_UINT(compressTypeEnum(STRDEF("none")), compressTypeNone, "check enum");
            TEST_RESULT_Z(compressExtZ(compressTypeNone), "", "check ext");
            TEST_RESULT_Z(compressTypeZ(compressTypeNone), "none", "check type z");
            TEST_RESULT_UINT(compressTypeFromName(STRDEF("file")), compressTypeNone, "check type from name");

            IoFilterGroup *filterGroup = ioFilterGroupNew();
            TEST_RESULT_VOID(compressFilterAdd(filterGroup, compressTypeNone, 0), "try to add compress filter");
            TEST_RESULT_UINT(lstSize(filterGroup->filterList), 0, "   check no filter was added");

            filterGroup = ioFilterGroupNew();
            TEST_RESULT_VOID(decompressFilterAdd(filterGroup, compressTypeNone), "try to add decompress filter");
            TEST_RESULT_UINT(lstSize(filterGroup->filterList), 0, "   check no filter was added");
        }

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("gzip compression");
        {
            TEST_RESULT_UINT(compressTypeEnum(STRDEF("gz")), compressTypeGzip, "check enum");
            TEST_RESULT_Z(compressExtZ(compressTypeGzip), ".gz", "check ext");
            TEST_RESULT_Z(compressTypeZ(compressTypeGzip), "gz", "check type z");
            TEST_RESULT_UINT(compressTypeFromName(STRDEF("file.gz")), compressTypeGzip, "check type from name");

            IoFilterGroup *filterGroup = ioFilterGroupNew();
            TEST_RESULT_VOID(compressFilterAdd(filterGroup, compressTypeGzip, 6), "try to add compress filter");
            TEST_RESULT_UINT(lstSize(filterGroup->filterList), 1, "   check filter was added");
            TEST_RESULT_STR(
                ioFilterType(ioFilterGroupGet(filterGroup, 0)->filter), GZIP_COMPRESS_FILTER_TYPE_STR, "   check filter type");

            filterGroup = ioFilterGroupNew();
            TEST_RESULT_VOID(decompressFilterAdd(filterGroup, compressTypeGzip), "try to add decompress filter");
            TEST_RESULT_UINT(lstSize(filterGroup->filterList), 1, "   check filter was added");
            TEST_RESULT_STR(
                ioFilterType(ioFilterGroupGet(filterGroup, 0)->filter), GZIP_DECOMPRESS_FILTER_TYPE_STR, "   check filter type");
        }

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("cat extension");
        {
            String *file = strNew("file");
            TEST_RESULT_VOID(compressExtCat(file, compressTypeGzip), "cat gzip ext");
            TEST_RESULT_STR_Z(file, "file.gz", "    check gzip ext");
        }

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("strip extension");
        {
            TEST_ERROR(compressExtStrip(STRDEF("file"), compressTypeGzip), FormatError, "'file' must have '.gz' extension");
            TEST_RESULT_STR_Z(compressExtStrip(STRDEF("file"), compressTypeNone), "file", "nothing to strip");
            TEST_RESULT_STR_Z(compressExtStrip(STRDEF("file.gz"), compressTypeGzip), "file", "strip gz");
        }

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("lz4 compression");
        {
            TEST_RESULT_UINT(compressTypeFromName(STRDEF("file.lz4")), compressTypeLz4, "check type from name");

#ifdef HAVE_LIBLZ4
            TEST_RESULT_UINT(compressTypeEnum(STRDEF("lz4")), compressTypeLz4, "check enum");
            TEST_RESULT_Z(compressExtZ(compressTypeLz4), ".lz4", "check ext");
            TEST_RESULT_Z(compressTypeZ(compressTypeLz4), "lz4", "check type z");

            IoFilterGroup *filterGroup = ioFilterGroupNew();
            TEST_RESULT_VOID(compressFilterAdd(filterGroup, compressTypeLz4, 1), "try to add compress filter");
            TEST_RESULT_UINT(lstSize(filterGroup->filterList), 1, "   check filter was added");
            TEST_RESULT_STR(
                ioFilterType(ioFilterGroupGet(filterGroup, 0)->filter), LZ4_COMPRESS_FILTER_TYPE_STR, "   check filter type");

            filterGroup = ioFilterGroupNew();
            TEST_RESULT_VOID(decompressFilterAdd(filterGroup, compressTypeLz4), "try to add decompress filter");
            TEST_RESULT_UINT(lstSize(filterGroup->filterList), 1, "   check filter was added");
            TEST_RESULT_STR(
                ioFilterType(ioFilterGroupGet(filterGroup, 0)->filter), LZ4_DECOMPRESS_FILTER_TYPE_STR, "   check filter type");
#else
            TEST_ERROR(compressTypeEnum(STRDEF("lz4")), OptionInvalidValueError, "pgBackRest not compiled with lz4 support");
#endif
        }

        // -------------------------------------------------------------------------------------------------------------------------
        TEST_TITLE("bogus compression");
        {
            TEST_ERROR(compressTypeEnum(strNew(BOGUS_STR)), AssertError, "invalid compression type 'BOGUS'");
            TEST_ERROR(compressTypeEnum(STRDEF("zst")), OptionInvalidValueError, "pgBackRest not compiled with zst support");

            // TEST_ERROR(compressExtZ((CompressType)999999), AssertError, "invalid compression type 999999");

            // IoFilterGroup *filterGroup = ioFilterGroupNew();
            // TEST_ERROR(compressFilterAdd(filterGroup, (CompressType)999999, 0), AssertError, "invalid compression type 999999");
            // TEST_ERROR(decompressFilterAdd(filterGroup, (CompressType)999999), AssertError, "invalid compression type 999999");
        }
    }

    FUNCTION_HARNESS_RESULT_VOID();
}
