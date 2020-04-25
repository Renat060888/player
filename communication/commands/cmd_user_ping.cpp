
#include <jsoncpp/json/writer.h>

#include "common/common_utils.h"
#include "analyze/analytic_manager_facade.h"
#include "cmd_user_ping.h"

using namespace std;
using namespace common_types;

static Json::Value serializePlayerState( common_types::IPlayerService * _player ){

    if( _player ){
        Json::Value rootRecord;
        rootRecord[ "status" ] = common_utils::printPlayingStatus( _player->getServiceState().status );
        rootRecord[ "last_error" ] = _player->getServiceState().lastError;
        rootRecord[ "..." ] = "...";

        // global range (millisec)
        // current step (at millisec)
        // step interval (millisec)
        // simulate & real ranges (millisec)
        // ... ?

        return rootRecord;
    }
    else{
        Json::Value unavailableState;
        unavailableState["status"] = "UNAVAILABLE";
        return unavailableState;
    }
}

static bool userHasPermission( const TUserId & _userId, common_types::SIncomingCommandServices * _services, std::string & _errMsg ){

    DispatcherUser * userDispatcher = _services->analyticManager->getUserDispatcher();

    if( ! userDispatcher->isRegistered(_userId) ){
        Json::Value unavailableState;
        unavailableState["status"] = "UNAVAILABLE";

        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "player_state" ] = unavailableState;
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

CommandUserPing::CommandUserPing( common_types::SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandUserPing::exec(){

    string errMsg;
    if( userHasPermission(m_userId, (SIncomingCommandServices *)m_services, errMsg) ){
        // take user snapshot
        DispatcherUser * du = ((SIncomingCommandServices *)m_services)->analyticManager->getUserDispatcher();
        common_types::SUserState state;
        state.userId = m_userId;
        du->updateUserState( state );

        // reflect to user his player state
        common_types::IPlayerService * player = ((SIncomingCommandServices *)m_services)->analyticManager->getPlayer( m_userId );

        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "player_state" ] = serializePlayerState( player );
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






