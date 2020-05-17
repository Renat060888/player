#ifndef DISPATCHER_PLAYER_CONTOLLER_H
#define DISPATCHER_PLAYER_CONTOLLER_H

#include <map>

#include "common/common_types.h"
#include "proxy_player_controller.h"

// TODO: temp ( must be in separate process )
#include "player_controller.h"

class SPlayerDescriptor;

class DispatcherPlayer
{
    friend class CommandPlayerControllerPing;
public:
    class IPlayerDispatcherObserver {
    public:
        virtual ~IPlayerDispatcherObserver(){}
        virtual void callbackPlayerControllerOnline( const common_types::TPlayerId & _id, bool _online ) = 0;
    };

    struct SInitSettings {
        common_types::IServiceInternalCommunication * serviceInternalCommunication;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    DispatcherPlayer();
    ~DispatcherPlayer();

    bool init( const SInitSettings & _settings );
    const SState & getState(){ return m_state; }

    void runClock();

    bool requestPlayer( const common_types::TUserId & _userId, const common_types::TContextId & _ctxId );
    void releasePlayer( const common_types::TPlayerId & _id );

    void addObserver( IPlayerDispatcherObserver * _observer );
    void removeObserver( IPlayerDispatcherObserver * _observer );

    std::vector<common_types::IPlayerService *> getPlayers();
    common_types::IPlayerService * getPlayer( const common_types::TPlayerId & _playerId );
    common_types::IPlayerService * getPlayerByUser( const common_types::TUserId & _userId );
    common_types::IPlayerService * getPlayerByContext( const common_types::TContextId & _ctxId );


private:
    void updatePlayerState( const common_types::SPlayingServiceState & _state );

    // data
    SState m_state;
    std::vector<common_types::IPlayerService *> m_players;
    std::map<common_types::TPlayerId, common_types::IPlayerService *> m_playersById;
    std::map<common_types::TContextId, common_types::IPlayerService *> m_playersByContextId;
    std::map<common_types::TUserId, common_types::IPlayerService *> m_playersByUserId;
    std::vector<IPlayerDispatcherObserver *> m_observers;
    std::map<common_types::TPlayerId, common_types::TUserId> m_playerIdByUserId;
    // ( monitoring )
    std::map<common_types::TPlayerId, SPlayerDescriptor *> m_monitoringDescriptorsByPlayerId;
    std::vector<SPlayerDescriptor *> m_monitoringDescriptors;

    // TODO: temp ( must be in separate process )
    std::vector<PlayerController *> m_realPlayerControllersForTest;

    // service


};

#endif // DISPATCHER_PLAYER_CONTOLLER_H
