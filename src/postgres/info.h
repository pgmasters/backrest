/***********************************************************************************************************************************
PostgreSQL Info
***********************************************************************************************************************************/
#ifndef POSTGRES_INFO_H
#define POSTGRES_INFO_H

#include <stdint.h>
#include <sys/types.h>

/***********************************************************************************************************************************
PostgreSQL Control File Info
***********************************************************************************************************************************/
typedef struct PgControlInfo
{
    uint64_t systemId;
    uint32_t controlVersion;
    uint32_t catalogVersion;
    unsigned int version;
} PgControlInfo;

#include "common/type/string.h"

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
PgControlInfo pgControlInfo(const String *pgPath);
void pgVersionValid(const unsigned int version);

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_DEBUG_PG_CONTROL_INFO_TYPE                                                                                        \
    PgControlInfo
#define FUNCTION_DEBUG_PG_CONTROL_INFO_FORMAT(value, buffer, bufferSize)                                                           \
    objToLog(&value, "PgControlInfo", buffer, bufferSize)

#endif
