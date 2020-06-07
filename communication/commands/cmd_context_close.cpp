
#include <jsoncpp/json/writer.h>

#include "system/system_environment_facade_player.h"
#include "analyze/analytic_manager_facade.h"
#include "cmd_context_close.h"

using namespace std;
using namespace common_types;

static bool userHasPermission( const TUserId & _userId, common_types::SIncomingCommandServices * _services, std::string & _errMsg ){

    DispatcherUser * userDispatcher = _services->analyticManager->getUserDispatcher();

    if( ! userDispatcher->isRegistered(_userId) ){
        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "ctx_close";
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

CommandContextClose::CommandContextClose( common_types::SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandContextClose::exec(){

    string errMsg;
    if( userHasPermission(m_userId, (SIncomingCommandServices *)m_services, errMsg) ){
        DispatcherPlayer * playerDipatcher = ((SIncomingCommandServices *)m_services)->analyticManager->getPlayerDispatcher();

        IPlayerService * player = playerDipatcher->getPlayerByUser( m_userId );
        if( player ){
            const TPlayerId playerId = player->getServiceState().playerId;
            playerDipatcher->releasePlayer( player->getServiceState().playerId );

            Json::Value rootRecord;
            rootRecord[ "cmd_name" ] = "ctx_close";
            rootRecord[ "error_occured" ] = false;
            rootRecord[ "code" ] = "OK";

            //
            ((SIncomingCommandServices *)m_services)->systemEnvironment->serviceForWriteAheadLogging()->closeClientOperation( playerId );

            Json::FastWriter jsonWriter;
            sendResponse( jsonWriter.write(rootRecord) );
            return true;
        }
        else{
            Json::Value rootRecord;
            rootRecord[ "cmd_name" ] = "ctx_close";
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
