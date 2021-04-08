/***********************************************************************************************************************************
Execute Process

Executes a child process and allows the calling process to communicate with it using read/write io.

This object is specially tailored to implement the protocol layer and may or may not be generally applicable to general purpose
execution.
***********************************************************************************************************************************/
#ifndef COMMON_EXEC_H
#define COMMON_EXEC_H

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
typedef struct Exec Exec;

#include "common/io/read.h"
#include "common/io/write.h"
#include "common/time.h"
#include "common/type/object.h"

/***********************************************************************************************************************************
Constructors
***********************************************************************************************************************************/
Exec *execNew(const String *command, const StringList *param, const String *name, TimeMSec timeout);

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Execute command
void execOpen(Exec *this);

/***********************************************************************************************************************************
Getters/Setters
***********************************************************************************************************************************/
// Read interface
IoRead *execIoRead(const Exec *this);

// Write interface
IoWrite *execIoWrite(const Exec *this);

// Exec MemContext
MemContext *execMemContext(const Exec *this);

/***********************************************************************************************************************************
Destructor
***********************************************************************************************************************************/
__attribute__((always_inline)) static inline void
execFree(Exec *this)
{
    objFree(this);
}

/***********************************************************************************************************************************
Macros for function logging
***********************************************************************************************************************************/
#define FUNCTION_LOG_EXEC_TYPE                                                                                                     \
    Exec *
#define FUNCTION_LOG_EXEC_FORMAT(value, buffer, bufferSize)                                                                        \
    objToLog(value, "Exec", buffer, bufferSize)

#endif
