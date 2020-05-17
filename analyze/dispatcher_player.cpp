
#include <microservice_common/system/logger.h>

#include "common/common_utils.h"
#include "dispatcher_player.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "PlayerDispatcher:";
static constexpr int64_t PLAYER_TIMEOUT_MILLISEC = 30000;

struct SPlayerDescriptor {
    SPlayerDescriptor()
        : lastPongMillisec(0)
        , player(nullptr)
        , editablePlayer(nullptr)
    {}
    common_types::TPlayerId id;
    common_types::TUserId userId;
    int64_t lastPongMillisec;
    common_types::IPlayerService * player;
    common_types::IEditablePlayer * editablePlayer;
};

DispatcherPlayer::DispatcherPlayer()
{

}

DispatcherPlayer::~DispatcherPlayer()
{

}

bool DispatcherPlayer::init( const SInitSettings & _settings ){

    m_state.settings = _settings;


    return true;
}

// > functors
class FEqualIPlayerService {
public:
    FEqualIPlayerService( TPlayerId _playerId )
        : playerId(_playerId)
    {}

    bool operator()( const IPlayerService * _rhs ){
        return ( this->playerId == _rhs->getServiceState().playerId );
    }

    TPlayerId playerId;
};

class FEqualSPlayerDescriptor {
public:
    FEqualSPlayerDescriptor( TPlayerId _playerId )
        : playerId(_playerId)
    {}

    bool operator()( SPlayerDescriptor * _rhs ){
        return ( this->playerId == _rhs->id );
    }

    TPlayerId playerId;
};
// < functors

void DispatcherPlayer::runClock(){

    for( auto iter = m_monitoringDescriptors.begin(); iter != m_monitoringDescriptors.end(); ){
        SPlayerDescriptor * descr = ( * iter );
        if( (common_utils::getCurrentTimeMillisec() - descr->lastPongMillisec) > PLAYER_TIMEOUT_MILLISEC ){

            VS_LOG_WARN << PRINT_HEADER << " player disappeared [" << descr->id << "]" << endl;

            auto remIter = std::remove_if( m_players.begin(), m_players.end(), FEqualIPlayerService(descr->id) );
            m_players.erase( remIter, m_players.end() );
            m_playersById.erase( descr->id );
            m_playersByUserId.erase( descr->userId );
            m_playersByContextId.erase( descr->player->getServiceState().ctxId );
            m_playerIdByUserId.erase( descr->id );
            delete descr->player;
            descr->player = nullptr;

            iter = m_monitoringDescriptors.erase( iter );
            m_monitoringDescriptorsByPlayerId.erase( descr->id );
            delete descr;
            descr = nullptr;
        }
        else{
            ++iter;
        }
    }
}

bool DispatcherPlayer::requestPlayer( const common_types::TUserId & _userId,
                                               const common_types::TContextId & _ctxId ){
    //
    const TPlayerId newId = common_utils::generateUniqueId();
    m_playerIdByUserId.insert( {newId, _userId} );

    // NOTE: must be in another process
    PlayerController::SInitSettings settings;
    settings.id = newId;
    settings.ctxId = _ctxId;
    settings.async = true;

    PlayerController * playerContoller = new PlayerController();
    if( ! playerContoller->init(settings) ){
        return false;
    }

    m_realPlayerControllersForTest.push_back( playerContoller );

    // TODO: create new process via ProcessLauncher ( player --contorller --player-id=0 --context-id=0 ... )

    return true;
}

void DispatcherPlayer::releasePlayer( const common_types::TPlayerId & _id ){

    auto iter = m_monitoringDescriptorsByPlayerId.find( _id );
    if( iter != m_monitoringDescriptorsByPlayerId.end() ){
        SPlayerDescriptor * descr = iter->second;

        // destroy player
        auto remIter = std::remove_if( m_players.begin(), m_players.end(), FEqualIPlayerService(descr->id) );
        m_players.erase( remIter, m_players.end() );
        m_playersById.erase( descr->id );
        m_playersByUserId.erase( descr->userId );
        m_playersByContextId.erase( descr->player->getServiceState().ctxId );
        m_playerIdByUserId.erase( descr->id );

        delete descr->player;
        descr->player = nullptr;

        // destroy monitor
        auto remIter2 = std::remove_if( m_monitoringDescriptors.begin(), m_monitoringDescriptors.end(), FEqualSPlayerDescriptor(descr->id) );
        m_monitoringDescriptors.erase( remIter2, m_monitoringDescriptors.end() );
        m_monitoringDescriptorsByPlayerId.erase( iter );

        delete descr;
        descr = nullptr;

        // destroy controller (NOTE: must be in another process)
        for( auto iter = m_realPlayerControllersForTest.begin(); iter != m_realPlayerControllersForTest.end(); ){
            PlayerController * pc = ( * iter );
            if( pc->getState().settings.id == _id ){
                delete pc;
                pc = nullptr;
                m_realPlayerControllersForTest.erase( iter );
                break;
            }
            else{
                ++iter;
            }
        }
    }
}

void DispatcherPlayer::addObserver( IPlayerDispatcherObserver * _observer ){

    m_observers.push_back( _observer );
}

void DispatcherPlayer::removeObserver( IPlayerDispatcherObserver * _observer ){

    for( auto iter = m_observers.begin(); iter != m_observers.end(); ){
        if( (* iter) == _observer ){
            m_observers.erase( iter );
            break;
        }
        else{
            ++iter;
        }
    }
}

std::vector<IPlayerService *> DispatcherPlayer::getPlayers(){
    return m_players;
}

IPlayerService * DispatcherPlayer::getPlayer( const common_types::TPlayerId & _playerId ){

    auto iter = m_playersById.find( _playerId );
    if( iter != m_playersById.end() ){
        return iter->second;
    }
    else{
        m_state.lastError = "player not found by id [" + _playerId + "]";
        return nullptr;
    }
}

IPlayerService * DispatcherPlayer::getPlayerByUser( const common_types::TUserId & _userId ){

    auto iter = m_playersByUserId.find( _userId );
    if( iter != m_playersByUserId.end() ){
        return iter->second;
    }
    else{
        m_state.lastError = "player not found for user [" + _userId + "]";
        return nullptr;
    }
}

IPlayerService * DispatcherPlayer::getPlayerByContext( const common_types::TContextId & _ctxId ){

    auto iter = m_playersByContextId.find( _ctxId );
    if( iter != m_playersByContextId.end() ){
        return iter->second;
    }
    else{
        m_state.lastError = "player not found for context [" + std::to_string(_ctxId) + "]";
        return nullptr;
    }
}

void DispatcherPlayer::updatePlayerState( const common_types::SPlayingServiceState & _state ){

    auto iter = m_monitoringDescriptorsByPlayerId.find( _state.playerId );
    if( iter != m_monitoringDescriptorsByPlayerId.end() ){
        SPlayerDescriptor * descr = iter->second;

        descr->lastPongMillisec = common_utils::getCurrentTimeMillisec();
        descr->editablePlayer->setServiceState( _state );
    }
    else{
        // new proxy controller
        ProxyPlayerController::SInitSettings settings;
        settings.network = m_state.settings.serviceInternalCommunication->getPlayerWorkerCommunicator( _state.playerId );

        ProxyPlayerController * player = new ProxyPlayerController();
        if( player->init(settings) ){
            const TUserId userId = m_playerIdByUserId[ _state.playerId ];

            // containers
            m_players.push_back( player );
            m_playersById.insert( {_state.playerId, player} );
            m_playersByContextId.insert( {_state.ctxId, player} );
            m_playersByUserId.insert( {userId, player} );

            // descriptor
            SPlayerDescriptor * descr = new SPlayerDescriptor();
            descr->id = _state.playerId;
            descr->player = player;
            descr->editablePlayer = player;
            descr->userId = userId;
            descr->lastPongMillisec = common_utils::getCurrentTimeMillisec();
            descr->editablePlayer->setServiceState( _state );

            m_monitoringDescriptors.push_back( descr );
            m_monitoringDescriptorsByPlayerId.insert( {_state.playerId, descr} );
        }
    }
}












