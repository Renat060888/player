
#include <microservice_common/system/logger.h>

#include "common/common_utils.h"
#include "dispatcher_player_contoller.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "PlayerDispatcher:";
static constexpr int64_t PLAYER_TIMEOUT_MILLISEC = 60000;

DispatcherPlayerContoller::DispatcherPlayerContoller()
{

}

bool DispatcherPlayerContoller::init( const SInitSettings & _settings ){

    m_state.settings = _settings;


    return true;
}

void DispatcherPlayerContoller::runClock(){

    for( auto iter = m_monitoringDescriptors.begin(); iter != m_monitoringDescriptors.end(); ){
        SPlayerDescriptor & descr = ( * iter );
        if( (common_utils::getCurrentTimeMillisec() - descr.lastPongMillisec) > PLAYER_TIMEOUT_MILLISEC ){

            VS_LOG_WARN << PRINT_HEADER << " player disappeared [" << descr.id << "]" << endl;
//            m_players.erase( iter );
            m_playersById.erase( descr.id );
            m_playersByUserId.erase( descr.userId );
            m_playersByContextId.erase( descr.player->getServiceState().ctxId );

            delete descr.player;
            descr.player = nullptr;

            iter = m_monitoringDescriptors.erase( iter );
        }
        else{
            ++iter;
        }
    }
}

bool DispatcherPlayerContoller::requestPlayer( const common_types::TUserId & _userId,
                                               const common_types::TContextId & _ctxId ){

    //
    const TPlayerId newId = common_utils::generateUniqueId();

    //
    PlayerController::SInitSettings settings;
    settings.id = newId;
    settings.ctxId = _ctxId;

    PlayerController * playerContolller = new PlayerController();
    if( ! playerContolller->init(settings) ){
        return false;
    }


    return true;
}

void DispatcherPlayerContoller::releasePlayer( const common_types::TPlayerId & _id ){

}

void DispatcherPlayerContoller::addObserver( IPlayerDispatcherObserver * _observer ){

}

void DispatcherPlayerContoller::removeObserver( IPlayerDispatcherObserver * _observer ){

}

std::vector<IPlayerService *> DispatcherPlayerContoller::getPlayers(){
    return m_players;
}

IPlayerService * DispatcherPlayerContoller::getPlayer( const common_types::TPlayerId & _playerId ){



}

IPlayerService * DispatcherPlayerContoller::getPlayerByUser( const common_types::TUserId & _userId ){

    m_state.lastError = "'get player by user' not yet implemented";
    return nullptr;
}

IPlayerService * DispatcherPlayerContoller::getPlayerByContext( const common_types::TContextId & _ctxId ){



}

void DispatcherPlayerContoller::updatePlayerState( const common_types::SPlayingServiceState & _state ){

    auto iter = m_monitoringDescriptors.find( _state.playerId );
    if( iter != m_monitoringDescriptors.end() ){
        SPlayerDescriptor & descr = iter->second;

        descr.lastPongMillisec = common_utils::getCurrentTimeMillisec();
        // update ProxyPlayerController state by 'SPlayingServiceState'
    }
    else{
        // new proxy controller
        ProxyPlayerController::SInitSettings settings;
        settings.network = m_state.settings.serviceInternalCommunication->getPlayerWorkerCommunicator( _state.playerId );

        ProxyPlayerController * player = new ProxyPlayerController();
        if( player->init(settings) ){
            // descriptor
            SPlayerDescriptor descr;
            descr.lastPongMillisec = common_utils::getCurrentTimeMillisec();
            descr.id = _state.playerId;
            descr.player = player;
            descr.userId = "";

            m_monitoringDescriptors.insert( {_state.playerId, descr} );
        }
    }
}












