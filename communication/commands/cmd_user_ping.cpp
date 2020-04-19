
#include <jsoncpp/json/writer.h>

#include "common/common_utils.h"
#include "analyze/analytic_manager_facade.h"
#include "cmd_user_ping.h"

using namespace std;

static Json::Value serializePlayerState( common_types::IPlayerService * _player ){

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

CommandUserPing::CommandUserPing( common_types::SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandUserPing::exec(){

    DispatcherUser * userDispatcher = (( common_types::SIncomingCommandServices * )m_services)->analyticManager->getUserDispatcher();

    // authorization passed
    if( userDispatcher->isRegistered(m_userId) ){
        common_types::SUserState state;
        state.userId = m_userId;
        userDispatcher->updateUserState( state );

        // player available ?
        Json::Value playerState;
        common_types::IPlayerService * player = (( common_types::SIncomingCommandServices * )m_services)->analyticManager->getPlayer( m_userId );
        if( player ){
            playerState = serializePlayerState( player );
        }
        else{
            Json::Value unavailableState;
            unavailableState["status"] = "UNAVAILABLE";
            playerState = unavailableState;
        }

        // send
        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "player_state" ] = playerState;
        rootRecord[ "error_occured" ] = false;
        rootRecord[ "code" ] = "OK";

        Json::FastWriter jsonWriter;
        sendResponse( jsonWriter.write(rootRecord) );
        return true;
    }
    // unknown user
    else{
        Json::Value unavailableState;
        unavailableState["status"] = "UNAVAILABLE";

        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "player_state" ] = unavailableState;
        rootRecord[ "error_occured" ] = true;
        rootRecord[ "code" ] = "NOT_REGISTERED";

        Json::FastWriter jsonWriter;
        sendResponse( jsonWriter.write(rootRecord) );
        return false;
    }
}






