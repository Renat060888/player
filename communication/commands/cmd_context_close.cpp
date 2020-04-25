
#include <jsoncpp/json/writer.h>

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
        DispatcherPlayerContoller * playerDipatcher = ((SIncomingCommandServices *)m_services)->analyticManager->getPlayerDispatcher();

        IPlayerService * player = playerDipatcher->getPlayerByUser( m_userId );
        playerDipatcher->releasePlayer( player->getServiceState().playerId );

        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "ctx_close";
        rootRecord[ "error_occured" ] = false;
        rootRecord[ "code" ] = "OK";

        Json::FastWriter jsonWriter;
        sendResponse( jsonWriter.write(rootRecord) );
        return true;
    }
    else{
        sendResponse( errMsg );
        return false;
    }
}
