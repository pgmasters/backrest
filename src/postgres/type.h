/***********************************************************************************************************************************
PostreSQL Types
***********************************************************************************************************************************/
#ifndef POSTGRES_TYPE_H
#define POSTGRES_TYPE_H

/***********************************************************************************************************************************
Define page size

Page size can only be changed at compile time and and is not known to be well-tested, so only the default page size is supported.
***********************************************************************************************************************************/
#define PG_PAGE_SIZE                                                8192

/***********************************************************************************************************************************
Defines for various Postgres paths and files
***********************************************************************************************************************************/
#define PG_FILE_PGCONTROL                                           "pg_control"

#define PG_PATH_GLOBAL                                              "global"

/***********************************************************************************************************************************
pg_control file data
***********************************************************************************************************************************/
typedef struct PgControlFile
{
    uint64_t systemId;
    uint32_t controlVersion;
    uint32_t catalogVersion;
} PgControlFile;

#endif
