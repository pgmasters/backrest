/***********************************************************************************************************************************
LZ4 Common
***********************************************************************************************************************************/
#ifdef HAVE_LIBLZ4

#ifndef COMMON_COMPRESS_LZ4_COMMON_H
#define COMMON_COMPRESS_LZ4_COMMON_H

/***********************************************************************************************************************************
LZ4 extension
***********************************************************************************************************************************/
#define LZ4_EXT                                                     "lz4"

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
size_t lz4Error(size_t error);

#endif

#endif // HAVE_LIBLZ4
