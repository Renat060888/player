
#include <jsoncpp/json/writer.h>

#include "common/common_utils.h"
#include "analyze/analytic_manager_facade.h"
#include "cmd_user_ping.h"

using namespace std;
using namespace common_types;

static Json::Value serializePlayerState( common_types::IPlayerService * _player ){

    if( _player ){
        const SPlayingServiceState & state = _player->getServiceState();

        Json::Value rootRecord;
        rootRecord[ "status" ] = common_utils::printPlayingStatus( state.status );
        rootRecord[ "last_error" ] = state.lastError;
        rootRecord[ "global_range_left" ] = (long long)state.info.globalRangeMillisec.first;
        rootRecord[ "global_range_right" ] = (long long)state.info.globalRangeMillisec.second;
        rootRecord[ "current_step" ] = (long long)state.info.currentStepMillisec;
        rootRecord[ "step_interval" ] = (long long)state.info.stepIntervalMillisec;

        Json::Value datasetsRecord;
        for( const SPlayingDataSet & dataset : state.info.playingData ){

            Json::Value rangesRecord;
            for( const TTimeRangeMillisec & range : dataset.dataRanges ){
                Json::Value rangeRecord;
                rangeRecord["range_left"] = (long long)range.first;
                rangeRecord["range_right"] = (long long)range.second;
                rangesRecord.append( rangeRecord );
            }

            Json::Value dsRecord;
            dsRecord["unique_id"] = (long long)dataset.uniqueId;
            dsRecord["real"] = dataset.real;
            dsRecord["ranges"] = rangesRecord;
            datasetsRecord.append( dsRecord );
        }
        rootRecord["datasets"] = datasetsRecord;

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
    if( userHasPermission(m_userState.userId, (SIncomingCommandServices *)m_services, errMsg) ){
        // take user snapshot
        DispatcherUser * du = ((SIncomingCommandServices *)m_services)->analyticManager->getUserDispatcher();
        du->updateUserState( m_userState );

        // reflect to user his player state
        DispatcherPlayerContoller * dpc = ((SIncomingCommandServices *)m_services)->analyticManager->getPlayerDispatcher();
        IPlayerService * player = dpc->getPlayerByUser( m_userState.userId );
        if( player ){
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
            // it's like a player is not ready yet
            Json::Value unavailableState;
            unavailableState["status"] = "UNAVAILABLE";

            Json::Value rootRecord;
            rootRecord[ "cmd_name" ] = "pong";
            rootRecord[ "player_state" ] = unavailableState;
            rootRecord[ "error_occured" ] = true;
            rootRecord[ "code" ] = dpc->getState().lastError;

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






