
#include <jsoncpp/json/writer.h>

#include "common/common_utils.h"
#include "analyze/analytic_manager_facade.h"
#include "cmd_user_ping.h"

using namespace std;

static string serializePlayerState( common_types::IPlayerService * _playerDipatcher ){

    Json::Value rootRecord;
    rootRecord[ "status" ] = common_utils::printPlayingStatus( _playerDipatcher->getServiceState().status );
    rootRecord[ "last_error" ] = _playerDipatcher->getServiceState().lastError;
    rootRecord[ "..." ] = "...";

    // global range (millisec)
    // current step (at millisec)
    // step interval (millisec)
    // simulate & real ranges (millisec)
    // ... ?

    Json::FastWriter jsonWriter;
    return jsonWriter.write(rootRecord);
}

CommandUserPing::CommandUserPing( common_types::SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandUserPing::exec(){

    DispatcherUser * userDispatcher = (( common_types::SIncomingCommandServices * )m_services)->analyticManager->getUserDispatcher();

    if( userDispatcher->isRegistered(m_userId) ){
        common_types::SUserState state;
        state.userId = m_userId;
        userDispatcher->updateUserState( state );

        common_types::IPlayerService * playerDispatcher = (( common_types::SIncomingCommandServices * )m_services)->analyticManager->getPlayer( m_userId );

        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "player_state" ] = serializePlayerState( playerDispatcher );
        rootRecord[ "error_occured" ] = false;
        rootRecord[ "code" ] = "NO_CODE";

        Json::FastWriter jsonWriter;
        sendResponse( jsonWriter.write(rootRecord) );
        return true;
    }
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






