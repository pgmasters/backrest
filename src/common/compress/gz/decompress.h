/***********************************************************************************************************************************
Gz Decompress

Decompress IO from the gz format.
***********************************************************************************************************************************/
#ifndef COMMON_COMPRESS_GZ_DECOMPRESS_H
#define COMMON_COMPRESS_GZ_DECOMPRESS_H

#include "common/io/filter/filter.h"

/***********************************************************************************************************************************
Filter type constant
***********************************************************************************************************************************/
#define GZ_DECOMPRESS_FILTER_TYPE                                   "gzDecompress"
    STRING_DECLARE(GZ_DECOMPRESS_FILTER_TYPE_STR);

/***********************************************************************************************************************************
Constructor
***********************************************************************************************************************************/
IoFilter *gzDecompressNew(bool raw);
IoFilter *gzDecompressNewVar(const VariantList *paramList);

#endif