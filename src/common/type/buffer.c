/***********************************************************************************************************************************
String Handler
***********************************************************************************************************************************/
#include <string.h>

#include "common/type/buffer.h"

/***********************************************************************************************************************************
Contains information about the buffer
***********************************************************************************************************************************/
struct Buffer
{
    MemContext *memContext;
    size_t size;
    unsigned char *buffer;
};

/***********************************************************************************************************************************
Create a new buffer
***********************************************************************************************************************************/
Buffer *
bufNew(size_t size)
{
    Buffer *this = NULL;

    MEM_CONTEXT_NEW_BEGIN("Buffer")
    {
        // Create object
        this = memNew(sizeof(Buffer));
        this->memContext = MEM_CONTEXT_NEW();
        this->size = size;

        // Allocate buffer
        if (size > 0)
            this->buffer = memNewRaw(this->size);
    }
    MEM_CONTEXT_NEW_END();

    return this;
}

/***********************************************************************************************************************************
Create a new buffer from a string
***********************************************************************************************************************************/
Buffer *
bufNewStr(const String *string)
{
    // Create object
    Buffer *this = bufNew(strSize(string));

    // Copy the data
    memcpy(this->buffer, strPtr(string), this->size);

    return this;
}

/***********************************************************************************************************************************
Move buffer to a new mem context
***********************************************************************************************************************************/
Buffer *
bufMove(Buffer *this, MemContext *parentNew)
{
    if (this != NULL)
        memContextMove(this->memContext, parentNew);

    return this;
}

/***********************************************************************************************************************************
Return buffer ptr
***********************************************************************************************************************************/
unsigned char *
bufPtr(const Buffer *this)
{
    return this->buffer;
}

/***********************************************************************************************************************************
Resize the buffer
***********************************************************************************************************************************/
Buffer *
bufResize(Buffer *this, size_t size)
{
    // If new size is zero then free memory if allocated
    if (size == 0)
    {
        if (this->buffer != NULL)
        {
            MEM_CONTEXT_BEGIN(this->memContext)
            {
                memFree(this->buffer);
            }
            MEM_CONTEXT_END();
        }

        this->buffer = NULL;
        this->size = 0;
    }
    // Else allocate or resize
    else
    {
        MEM_CONTEXT_BEGIN(this->memContext)
        {
            if (this->buffer == NULL)
                this->buffer = memNew(size);
            else
                this->buffer = memGrowRaw(this->buffer, size);
        }
        MEM_CONTEXT_END();

        this->size = size;
    }

    return this;
}

/***********************************************************************************************************************************
Return buffer size
***********************************************************************************************************************************/
size_t
bufSize(const Buffer *this)
{
    return this->size;
}

/***********************************************************************************************************************************
Free the buffer
***********************************************************************************************************************************/
void
bufFree(Buffer *this)
{
    if (this != NULL)
        memContextFree(this->memContext);
}
