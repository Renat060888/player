
#include <jsoncpp/json/writer.h>

#include "system/system_environment_facade_player.h"
#include "system/objrepr_bus_player.h"
#include "analyze/analytic_manager_facade.h"
#include "cmd_context_open.h"

using namespace std;
using namespace common_types;

static bool userHasPermission( const TUserId & _userId, common_types::SIncomingCommandServices * _services, std::string & _errMsg ){

    DispatcherUser * userDispatcher = _services->analyticManager->getUserDispatcher();

    if( ! userDispatcher->isRegistered(_userId) ){
        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "error_occured" ] = true;
        rootRecord[ "code" ] = "NOT_REGISTERED";

        Json::FastWriter jsonWriter;
        _errMsg = jsonWriter.write( rootRecord );
        return false;
    }
    else{
        return true;
    }
}

CommandContextOpen::CommandContextOpen( SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandContextOpen::exec(){

    string errMsg;
    if( userHasPermission(m_userId, (SIncomingCommandServices *)m_services, errMsg) ){
        const TContextId ctxId = OBJREPR_BUS.getContextIdByName( m_contextName );

        TPlayerId playerId;
        DispatcherPlayer * playerDipatcher = ((SIncomingCommandServices *)m_services)->analyticManager->getPlayerDispatcher();
        if( playerDipatcher->requestPlayer(m_userId, ctxId, playerId) ){
            Json::Value rootRecord;
            rootRecord[ "cmd_name" ] = "ctx_open";
            rootRecord[ "error_occured" ] = false;
            rootRecord[ "code" ] = "OK";

            //
            SWALClientOperation operation;
            operation.commandFullText = m_requestFullText;
            operation.uniqueKey = playerId; // TODO: come up with a more elegant solution
            operation.begin = true;
            ((SIncomingCommandServices *)m_services)->systemEnvironment->serviceForWriteAheadLogging()->openOperation( operation );

            Json::FastWriter jsonWriter;
            sendResponse( jsonWriter.write(rootRecord) );
            return true;
        }
        else{
            Json::Value rootRecord;
            rootRecord[ "cmd_name" ] = "ctx_open";
            rootRecord[ "error_occured" ] = true;
            rootRecord[ "code" ] = playerDipatcher->getState().lastError;

            Json::FastWriter jsonWriter;
            sendResponse( jsonWriter.write(rootRecord) );
            return false;
        }
    }
    else{
        sendResponse( errMsg );
        return false;
    }
}






