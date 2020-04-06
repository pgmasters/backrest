/***********************************************************************************************************************************
Socket Client
***********************************************************************************************************************************/
#include "build.auto.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common/debug.h"
#include "common/log.h"
#include "common/io/socket/client.h"
#include "common/io/socket/common.h"
#include "common/memContext.h"
#include "common/type/object.h"
#include "common/wait.h"

/***********************************************************************************************************************************
Statistics
***********************************************************************************************************************************/
static SocketClientStat sckClientStatLocal;

/***********************************************************************************************************************************
Object type
***********************************************************************************************************************************/
struct SocketClient
{
    MemContext *memContext;                                         // Mem context
    String *host;                                                   // Hostname or IP address
    unsigned int port;                                              // Port to connect to host on
    TimeMSec timeout;                                               // Timeout for any i/o operation (connect, read, etc.)

    int fd;                                                         // File descriptor
};

OBJECT_DEFINE_GET(Fd, , SOCKET_CLIENT, int, fd);
OBJECT_DEFINE_GET(Host, const, SOCKET_CLIENT, const String *, host);
OBJECT_DEFINE_GET(Port, const, SOCKET_CLIENT, unsigned int, port);

OBJECT_DEFINE_MOVE(SOCKET_CLIENT);

/***********************************************************************************************************************************
Free connection
***********************************************************************************************************************************/
OBJECT_DEFINE_FREE_RESOURCE_BEGIN(SOCKET_CLIENT, LOG, logLevelTrace)
{
    close(this->fd);
}
OBJECT_DEFINE_FREE_RESOURCE_END(LOG);

/**********************************************************************************************************************************/
SocketClient *
sckClientNew(const String *host, unsigned int port, TimeMSec timeout)
{
    FUNCTION_LOG_BEGIN(logLevelDebug)
        FUNCTION_LOG_PARAM(STRING, host);
        FUNCTION_LOG_PARAM(UINT, port);
        FUNCTION_LOG_PARAM(TIME_MSEC, timeout);
    FUNCTION_LOG_END();

    ASSERT(host != NULL);
    ASSERT(timeout > 0);

    SocketClient *this = NULL;

    MEM_CONTEXT_NEW_BEGIN("SocketClient")
    {
        this = memNew(sizeof(SocketClient));

        *this = (SocketClient)
        {
            .memContext = MEM_CONTEXT_NEW(),
            .host = strDup(host),
            .port = port,
            .timeout = timeout,

            // Initialize file descriptor to -1 so we know when the socket is disconnected
            .fd = -1,
        };

        sckClientStatLocal.object++;
    }
    MEM_CONTEXT_NEW_END();

    FUNCTION_LOG_RETURN(SOCKET_CLIENT, this);
}

/**********************************************************************************************************************************/
void
sckClientOpen(SocketClient *this)
{
    FUNCTION_LOG_BEGIN(logLevelTrace)
        FUNCTION_LOG_PARAM(SOCKET_CLIENT, this);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);
    CHECK(this->fd == -1);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        bool connected = false;
        bool retry;
        Wait *wait = waitNew(this->timeout);

        do
        {
            // Assume there will be no retry
            retry = false;

            TRY_BEGIN()
            {
                // Set hints that narrow the type of address we are looking for -- we'll take ipv4 or ipv6
                struct addrinfo hints = (struct addrinfo)
                {
                    .ai_family = AF_UNSPEC,
                    .ai_socktype = SOCK_STREAM,
                    .ai_protocol = IPPROTO_TCP,
                };

                // Convert the port to a zero-terminated string for use with getaddrinfo()
                char port[CVT_BASE10_BUFFER_SIZE];
                cvtUIntToZ(this->port, port, sizeof(port));

                // Get an address for the host.  We are only going to try the first address returned.
                struct addrinfo *hostAddress;
                int result;

                if ((result = getaddrinfo(strPtr(this->host), port, &hints, &hostAddress)) != 0)
                {
                    THROW_FMT(
                        HostConnectError, "unable to get address for '%s': [%d] %s", strPtr(this->host), result,
                        gai_strerror(result));
                }

                // Connect to the host
                TRY_BEGIN()
                {
                    this->fd = socket(hostAddress->ai_family, hostAddress->ai_socktype, hostAddress->ai_protocol);
                    THROW_ON_SYS_ERROR(this->fd == -1, HostConnectError, "unable to create socket");

                    memContextCallbackSet(this->memContext, sckClientFreeResource, this);

                    sckOptionSet(this->fd);

                    // Attempt connection
                    CHECK(connect(this->fd, hostAddress->ai_addr, hostAddress->ai_addrlen) == -1);

                    // Save the error
                    int errNo = errno;

                    // The connection has started but since we are in non-blocking mode it has not completed yet
                    CHECK(errno == EINPROGRESS);

                    // Wait for write-ready
                    if (!sckPoll(this->fd, false, true, waitRemaining(wait)))
                        THROW_FMT(HostConnectError, "timeout connecting to '%s:%u'", strPtr(this->host), this->port);

                    // Check that the connection was successful
                    socklen_t errNoLen = sizeof(errNo);

                    THROW_ON_SYS_ERROR(
                        getsockopt(this->fd, SOL_SOCKET, SO_ERROR, &errNo, &errNoLen) == -1, HostConnectError,
                        "unable to get socket error");

                    // Throw error if it is still set
                    if (errNo != 0)
                    {
                        THROW_SYS_ERROR_CODE_FMT(
                            errNo, HostConnectError, "unable to connect to '%s:%u'", strPtr(this->host), this->port);
                    }
                }
                FINALLY()
                {
                    freeaddrinfo(hostAddress);
                }
                TRY_END();

                // Connection was successful
                connected = true;
            }
            CATCH_ANY()
            {
                // Retry if wait time has not expired
                if (waitMore(wait))
                {
                    LOG_DEBUG_FMT("retry %s: %s", errorTypeName(errorType()), errorMessage());
                    retry = true;

                    sckClientStatLocal.retry++;
                }

                sckClientClose(this);
            }
            TRY_END();
        }
        while (!connected && retry);

        if (!connected)
            RETHROW();

        sckClientStatLocal.session++;
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
void
sckClientPoll(SocketClient *this, bool read, bool write)
{
    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(SOCKET_CLIENT, this);
        FUNCTION_LOG_PARAM(BOOL, read);
        FUNCTION_LOG_PARAM(BOOL, write);
    FUNCTION_LOG_END();

    if (!sckPoll(this->fd, read, write, this->timeout))
    {
        THROW_FMT(
            FileReadError, "timeout after %" PRIu64 "ms waiting for read from '%s:%u'", this->timeout, strPtr(this->host),
            this->port);
    }

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
void
sckClientClose(SocketClient *this)
{
    FUNCTION_LOG_BEGIN(logLevelTrace);
        FUNCTION_LOG_PARAM(SOCKET_CLIENT, this);
    FUNCTION_LOG_END();

    ASSERT(this != NULL);

    // Close the socket
    if (this->fd != -1)
    {
        memContextCallbackClear(this->memContext);
        sckClientFreeResource(this);
        this->fd = -1;
    }

    FUNCTION_LOG_RETURN_VOID();
}


/**********************************************************************************************************************************/
String *
sckClientStatStr(void)
{
    FUNCTION_TEST_VOID();

    String *result = NULL;

    if (sckClientStatLocal.object > 0)
    {
        result = strNewFmt(
            "socket statistics: objects %" PRIu64 ", sessions %" PRIu64 ", retries %" PRIu64, sckClientStatLocal.object,
            sckClientStatLocal.session, sckClientStatLocal.retry);
    }

    FUNCTION_TEST_RETURN(result);
}

/**********************************************************************************************************************************/
String *
sckClientToLog(const SocketClient *this)
{
    return strNewFmt("{host: %s, port: %u, timeout: %" PRIu64 "}", strPtr(this->host), this->port, this->timeout);
}
