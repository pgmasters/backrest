/***********************************************************************************************************************************
Info Archive Handler for pgbackrest information
***********************************************************************************************************************************/
#ifndef INFO_INFOARCHIVE_H
#define INFO_INFOARCHIVE_H

/***********************************************************************************************************************************
Info Archive object
***********************************************************************************************************************************/
typedef struct InfoArchive InfoArchive;

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
InfoArchive *infoArchiveNew(String *fileName, const bool loadFile, const bool ignoreMissing);

void infoArchiveFree(InfoArchive *this);

#endif
