/***********************************************************************************************************************************
Db Protocol Handler
***********************************************************************************************************************************/
#include "build.auto.h"

#include "common/debug.h"
#include "common/io/io.h"
#include "common/log.h"
#include "common/memContext.h"
#include "common/type/list.h"
#include "config/config.h"
#include "db/db.h"
#include "db/protocol.h"
#include "postgres/client.h"
#include "postgres/interface.h"

/***********************************************************************************************************************************
Local variables
***********************************************************************************************************************************/
static struct
{
    List *pgClientList;                                             // List of db objects
} dbProtocolLocal;

/**********************************************************************************************************************************/
void
dbOpenProtocol(const VariantList *paramList, ProtocolServer *server)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(VARIANT_LIST, paramList);
        FUNCTION_LOG_PARAM(PROTOCOL_SERVER, server);
    FUNCTION_LOG_END();

    ASSERT(paramList == NULL);
    ASSERT(server != NULL);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        // If the db list does not exist then create it in the prior context (which should be persistent)
        if (dbProtocolLocal.pgClientList == NULL)
        {
            MEM_CONTEXT_PRIOR_BEGIN()
            {
                dbProtocolLocal.pgClientList = lstNewP(sizeof(PgClient *));
            }
            MEM_CONTEXT_PRIOR_END();
        }

        // Add db to the list
        unsigned int dbIdx = lstSize(dbProtocolLocal.pgClientList);

        MEM_CONTEXT_BEGIN(lstMemContext(dbProtocolLocal.pgClientList))
        {
            // Only a single db is passed to the remote
            PgClient *pgClient = pgClientNew(
                cfgOptionStrNull(cfgOptPgSocketPath), cfgOptionUInt(cfgOptPgPort), cfgOptionStr(cfgOptPgDatabase),
                cfgOptionStrNull(cfgOptPgUser), cfgOptionUInt64(cfgOptDbTimeout));
            pgClientOpen(pgClient);

            lstAdd(dbProtocolLocal.pgClientList, &pgClient);
        }
        MEM_CONTEXT_END();

        // Return db index which should be included in subsequent calls
        protocolServerResponse(server, VARUINT(dbIdx));
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
void
dbQueryProtocol(const VariantList *paramList, ProtocolServer *server)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(VARIANT_LIST, paramList);
        FUNCTION_LOG_PARAM(PROTOCOL_SERVER, server);
    FUNCTION_LOG_END();

    ASSERT(paramList != NULL);
    ASSERT(server != NULL);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        protocolServerResponse(
            server,
            varNewVarLst(
                pgClientQuery(
                    *(PgClient **)lstGet(dbProtocolLocal.pgClientList, varUIntForce(varLstGet(paramList, 0))),
                    varStr(varLstGet(paramList, 1)))));
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
// !!!
void dbSyncCheckHelper(PgClient *pgClient, const String *path);

void
dbSyncCheckProtocol(const VariantList *const paramList, ProtocolServer *const server)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(VARIANT_LIST, paramList);
        FUNCTION_LOG_PARAM(PROTOCOL_SERVER, server);
    FUNCTION_LOG_END();

    ASSERT(paramList != NULL);
    ASSERT(server != NULL);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        PgClient *const pgClient = *(PgClient **)lstGet(dbProtocolLocal.pgClientList, varUIntForce(varLstGet(paramList, 0)));
        const String *const path = varStr(varLstGet(paramList, 1));

        dbSyncCheckHelper(pgClient, path);

        protocolServerResponse(server, NULL);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}

/**********************************************************************************************************************************/
void
dbCloseProtocol(const VariantList *paramList, ProtocolServer *server)
{
    FUNCTION_LOG_BEGIN(logLevelDebug);
        FUNCTION_LOG_PARAM(VARIANT_LIST, paramList);
        FUNCTION_LOG_PARAM(PROTOCOL_SERVER, server);
    FUNCTION_LOG_END();

    ASSERT(paramList != NULL);
    ASSERT(server != NULL);

    MEM_CONTEXT_TEMP_BEGIN()
    {
        PgClient **pgClient = lstGet(dbProtocolLocal.pgClientList, varUIntForce(varLstGet(paramList, 0)));
        CHECK(*pgClient != NULL);

        pgClientClose(*pgClient);
        *pgClient = NULL;

        protocolServerResponse(server, NULL);
    }
    MEM_CONTEXT_TEMP_END();

    FUNCTION_LOG_RETURN_VOID();
}
