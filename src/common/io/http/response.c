/***********************************************************************************************************************************
HTTP Response
***********************************************************************************************************************************/
#include "build.auto.h"

#include "common/debug.h"
#include "common/io/http/client.h"
#include "common/io/http/common.h"
#include "common/io/http/request.h"
#include "common/io/http/response.h"
#include "common/io/io.h"
#include "common/io/read.intern.h"
#include "common/log.h"
#include "common/type/object.h"
#include "common/wait.h"

/***********************************************************************************************************************************
HTTP constants
***********************************************************************************************************************************/
#define HTTP_HEADER_CONNECTION                                      "connection"
    STRING_STATIC(HTTP_HEADER_CONNECTION_STR,                       HTTP_HEADER_CONNECTION);
#define HTTP_HEADER_TRANSFER_ENCODING                               "transfer-encoding"
    STRING_STATIC(HTTP_HEADER_TRANSFER_ENCODING_STR,                HTTP_HEADER_TRANSFER_ENCODING);

#define HTTP_VALUE_CONNECTION_CLOSE                                 "close"
    STRING_STATIC(HTTP_VALUE_CONNECTION_CLOSE_STR,                  HTTP_VALUE_CONNECTION_CLOSE);
#define HTTP_VALUE_TRANSFER_ENCODING_CHUNKED                        "chunked"
    STRING_STATIC(HTTP_VALUE_TRANSFER_ENCODING_CHUNKED_STR,         HTTP_VALUE_TRANSFER_ENCODING_CHUNKED);

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
struct HttpResponse
{
    MemContext *memContext;                                         // Mem context

    HttpSession *session;                                           // HTTP session
    IoRead *contentRead;                                            // Read interface for response content

    unsigned int code;                                              // Response code (e.g. 200, 404)
    String *reason;                                                 // Response reason e.g. (OK, Not Found)
    HttpHeader *header;                                             // Response headers

    bool contentChunked;                                            // Is the response content chunked?
    uint64_t contentSize;                                           // Content size (ignored for chunked)
    uint64_t contentRemaining;                                      // Content remaining (per chunk if chunked)
    bool closeOnContentEof;                                         // Will server close after content is sent?
    bool contentExists;                                             // Does content exist?
    bool contentEof;                                                // Has all content been read?
    Buffer *content;                                                // Caches content once requested
};

OBJECT_DEFINE_MOVE(HTTP_RESPONSE);
OBJECT_DEFINE_FREE(HTTP_RESPONSE);

OBJECT_DEFINE_GET(IoRead, , HTTP_RESPONSE, IoRead *, contentRead);
OBJECT_DEFINE_GET(Code, const, HTTP_RESPONSE, unsigned int, code);
OBJECT_DEFINE_GET(Header, const, HTTP_RESPONSE, const HttpHeader *, header);
OBJECT_DEFINE_GET(Reason, const, HTTP_RESPONSE, const String *, reason);

/***********************************************************************************************************************************
When response is done close/reuse the connection
***********************************************************************************************************************************/
static void
httpResponseDone(HttpResponse *this)
{
    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(HTTP_RESPONSE, this);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(this->session != NULL);

    // If close was requested by the server then free the session
    if (this->closeOnContentEof)
    {
        httpSessionFree(this->session);

        // Only update the close stats after a successful response so it is not counted if there was an error/retry
        httpClientStat.close++;
    }
    // Else return it to the client so it can be reused
    else
        httpSessionDone(this->session);

    this->session = NULL;

    FUNCTION_LOG_RETURN_VOID();
}

/***********************************************************************************************************************************
Read content
***********************************************************************************************************************************/
static size_t
httpResponseRead(THIS_VOID, Buffer *buffer, bool block)
{
    THIS(HttpResponse);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(HTTP_RESPONSE, this);
        FUNCTION_LOG_PARAM(BUFFER, buffer);
        FUNCTION_LOG_PARAM(BOOL, block);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    ASSERT(buffer != NULL);
    ASSERT(!bufFull(buffer));
    ASSERT(this->contentEof || this->session != NULL);

    // Read if EOF has not been reached
    size_t actualBytes = 0;

    if (!this->contentEof)
    {
        MEM_CONTEXT_TEMP_BEGIN()
        {
            IoRead *rawRead = httpSessionIoRead(this->session);

            // If close was requested and no content specified then the server may send content up until the eof
            if (this->closeOnContentEof && !this->contentChunked && this->contentSize == 0)
            {
                ioRead(rawRead, buffer);
                this->contentEof = ioReadEof(rawRead);
            }
            // Else read using specified encoding or size
            else
            {
                do
                {
                    // If chunked content and no content remaining
                    if (this->contentChunked && this->contentRemaining == 0)
                    {
                        // Read length of next chunk
                        this->contentRemaining = cvtZToUInt64Base(strZ(strTrim(ioReadLine(rawRead))), 16);

                        // If content remaining is still zero then eof
                        if (this->contentRemaining == 0)
                            this->contentEof = true;
                    }

                    // Read if there is content remaining
                    if (this->contentRemaining > 0)
                    {
                        // If the buffer is larger than the content that needs to be read then limit the buffer size so the read
                        // won't block or read too far.  Casting to size_t is safe on 32-bit because we know the max buffer size is
                        // defined as less than 2^32 so content remaining can't be more than that.
                        if (bufRemains(buffer) > this->contentRemaining)
                            bufLimitSet(buffer, bufSize(buffer) - (bufRemains(buffer) - (size_t)this->contentRemaining));

                        actualBytes = bufRemains(buffer);
                        this->contentRemaining -= ioRead(rawRead, buffer);

                        // Error if EOF but content read is not complete
                        if (ioReadEof(rawRead))
                            THROW(FileReadError, "unexpected EOF reading HTTP content");

                        // Clear limit (this works even if the limit was not set and it is easier than checking)
                        bufLimitClear(buffer);
                    }

                    // If no content remaining
                    if (this->contentRemaining == 0)
                    {
                        // If chunked then consume the blank line that follows every chunk.  There might be more chunk data so loop back
                        // around to check.
                        if (this->contentChunked)
                        {
                            ioReadLine(rawRead);
                        }
                        // If total content size was provided then this is eof
                        else
                            this->contentEof = true;
                    }
                }
                while (!bufFull(buffer) && !this->contentEof);
            }

            // If all content has been read
            if (this->contentEof)
                httpResponseDone(this);
        }
        MEM_CONTEXT_TEMP_END();
    }

    FUNCTION_LOG_RETURN(SIZE, (size_t)actualBytes);
}

/***********************************************************************************************************************************
Has all content been read?
***********************************************************************************************************************************/
static bool
httpResponseEof(THIS_VOID)
{
    THIS(HttpResponse);

    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(HTTP_RESPONSE, this);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);

    FUNCTION_LOG_RETURN(BOOL, this->contentEof);
}

/**********************************************************************************************************************************/
HttpResponse *
httpResponseNew(HttpSession *session, const String *verb, bool contentCache)
{
    FUNCTION_LOG_BEGIN(logLevelDebug)
        FUNCTION_LOG_PARAM(HTTP_SESSION, session);
        FUNCTION_LOG_PARAM(STRING, verb);
        FUNCTION_LOG_PARAM(BOOL, contentCache);
    FUNCTION_LOG_END();

    ASSERT(session != NULL);
    ASSERT(verb != NULL);

    HttpResponse *this = NULL;

    MEM_CONTEXT_NEW_BEGIN("HttpResponse")
    {
        this = memNew(sizeof(HttpResponse));

        *this = (HttpResponse)
        {
            .memContext = MEM_CONTEXT_NEW(),
            .session = httpSessionMove(session, memContextCurrent()),
            .header = httpHeaderNew(NULL),
        };

        MEM_CONTEXT_TEMP_BEGIN()
        {
            // Read status
            String *status = ioReadLine(httpSessionIoRead(this->session));

            // Check status ends with a CR and remove it to make error formatting easier and more accurate
            if (!strEndsWith(status, CR_STR))
                THROW_FMT(FormatError, "HTTP response status '%s' should be CR-terminated", strZ(status));

            status = strSubN(status, 0, strSize(status) - 1);

            // Check status is at least the minimum required length to avoid harder to interpret errors later on
            if (strSize(status) < sizeof(HTTP_VERSION) + 4)
                THROW_FMT(FormatError, "HTTP response '%s' has invalid length", strZ(strTrim(status)));

            // Check status starts with the correct http version. HTTP/1.0 is also accepted but does not change any logic.
             if (!strBeginsWith(status, HTTP_VERSION_STR) && !strBeginsWith(status, HTTP_VERSION_10_STR))
                THROW_FMT(FormatError, "HTTP version of response '%s' must be " HTTP_VERSION " or " HTTP_VERSION_10, strZ(status));

            // Read status code
            status = strSub(status, sizeof(HTTP_VERSION));

            int spacePos = strChr(status, ' ');

            if (spacePos != 3)
                THROW_FMT(FormatError, "response status '%s' must have a space after the status code", strZ(status));

            this->code = cvtZToUInt(strZ(strSubN(status, 0, (size_t)spacePos)));

            // Read reason phrase. A missing reason phrase will be represented as an empty string.
            MEM_CONTEXT_BEGIN(this->memContext)
            {
                this->reason = strSub(status, (size_t)spacePos + 1);
            }
            MEM_CONTEXT_END();

            // Read headers
            do
            {
                // Read the next header
                String *header = strTrim(ioReadLine(httpSessionIoRead(this->session)));

                // If the header is empty then we have reached the end of the headers
                if (strSize(header) == 0)
                    break;

                // Split the header and store it
                int colonPos = strChr(header, ':');

                if (colonPos < 0)
                    THROW_FMT(FormatError, "header '%s' missing colon", strZ(strTrim(header)));

                String *headerKey = strLower(strTrim(strSubN(header, 0, (size_t)colonPos)));
                String *headerValue = strTrim(strSub(header, (size_t)colonPos + 1));

                httpHeaderAdd(this->header, headerKey, headerValue);

                // Read transfer encoding (only chunked is supported)
                if (strEq(headerKey, HTTP_HEADER_TRANSFER_ENCODING_STR))
                {
                    // Error if transfer encoding is not chunked
                    if (!strEq(headerValue, HTTP_VALUE_TRANSFER_ENCODING_CHUNKED_STR))
                    {
                        THROW_FMT(
                            FormatError, "only '%s' is supported for '%s' header", HTTP_VALUE_TRANSFER_ENCODING_CHUNKED,
                            HTTP_HEADER_TRANSFER_ENCODING);
                    }

                    this->contentChunked = true;
                }

                // Read content size
                if (strEq(headerKey, HTTP_HEADER_CONTENT_LENGTH_STR))
                {
                    this->contentSize = cvtZToUInt64(strZ(headerValue));
                    this->contentRemaining = this->contentSize;
                }

                // If the server notified of a closed connection then close the client connection after reading content.  This
                // prevents doing a retry on the next request when using the closed connection.
                if (strEq(headerKey, HTTP_HEADER_CONNECTION_STR) && strEq(headerValue, HTTP_VALUE_CONNECTION_CLOSE_STR))
                    this->closeOnContentEof = true;

            }
            while (1);

            // Error if transfer encoding and content length are both set
            if (this->contentChunked && this->contentSize > 0)
            {
                THROW_FMT(
                    FormatError,  "'%s' and '%s' headers are both set", HTTP_HEADER_TRANSFER_ENCODING,
                    HTTP_HEADER_CONTENT_LENGTH);
            }

            // Was content returned in the response?  HEAD will report content but not actually return any.
            this->contentExists =
                (this->contentChunked || this->contentSize > 0 || this->closeOnContentEof) && !strEq(verb, HTTP_VERB_HEAD_STR);
            this->contentEof = !this->contentExists;

            // Create an io object, even if there is no content.  This makes the logic for readers easier -- they can just check eof
            // rather than also checking if the io object exists.
            MEM_CONTEXT_BEGIN(this->memContext)
            {
                this->contentRead = ioReadNewP(this, .eof = httpResponseEof, .read = httpResponseRead);
                ioReadOpen(this->contentRead);
            }
            MEM_CONTEXT_END();

            // If there is no content then we are done with the client
            if (!this->contentExists)
            {
                httpResponseDone(this);
            }
            // Else cache content when requested or on error
            else if (contentCache || !httpResponseCodeOk(this))
            {
                MEM_CONTEXT_BEGIN(this->memContext)
                {
                    httpResponseContent(this);
                }
                MEM_CONTEXT_END();
            }
        }
        MEM_CONTEXT_TEMP_END();
    }
    MEM_CONTEXT_NEW_END();

    FUNCTION_LOG_RETURN(HTTP_RESPONSE, this);
}

/**********************************************************************************************************************************/
const Buffer *
httpResponseContent(HttpResponse *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(HTTP_RESPONSE, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    if (this->content == NULL)
    {
        this->content = bufNew(0);

        if (this->contentExists)
        {
            do
            {
                bufResize(this->content, bufSize(this->content) + ioBufferSize());
                httpResponseRead(this, this->content, true);
            }
            while (!httpResponseEof(this));

            bufResize(this->content, bufUsed(this->content));
        }
    }

    FUNCTION_TEST_RETURN(this->content);
}

/**********************************************************************************************************************************/
bool
httpResponseCodeOk(const HttpResponse *this)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(HTTP_RESPONSE, this);
    FUNCTION_TEST_END();

    ASSERT(this != NULL);

    FUNCTION_TEST_RETURN(this->code / 100 == 2);
}

/**********************************************************************************************************************************/
String *
httpResponseToLog(const HttpResponse *this)
{
    return strNewFmt(
        "{code: %u, reason: %s, header: %s, contentChunked: %s, contentSize: %" PRIu64 ", contentRemaining: %" PRIu64
            ", closeOnContentEof: %s, contentExists: %s, contentEof: %s, contentCached: %s}",
        this->code, strZ(this->reason), strZ(httpHeaderToLog(this->header)), cvtBoolToConstZ(this->contentChunked),
        this->contentSize, this->contentRemaining, cvtBoolToConstZ(this->closeOnContentEof), cvtBoolToConstZ(this->contentExists),
        cvtBoolToConstZ(this->contentEof), cvtBoolToConstZ(this->content != NULL));
}
