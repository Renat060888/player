#ifndef DISPATCHER_PLAYER_CONTOLLER_H
#define DISPATCHER_PLAYER_CONTOLLER_H

#include <map>
#include <future>

#include "common/common_types.h"
#include "proxy_player_controller.h"

// TODO: temp ( must be in separate process )
#include "player_controller.h"

class SPlayerDescriptor;

class DispatcherPlayer : public common_types::IUserDispatcherObserver
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
        SystemEnvironmentFacadePlayer * systemEnvironment;
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

    bool requestPlayer( const common_types::TUserId & _userId, const common_types::TContextId & _ctxId, common_types::TPlayerId & _playerId );
    void releasePlayer( const common_types::TPlayerId _id );

    void addObserver( IPlayerDispatcherObserver * _observer );
    void removeObserver( IPlayerDispatcherObserver * _observer );

    std::vector<common_types::IPlayerService *> getPlayers();
    common_types::IPlayerService * getPlayer( const common_types::TPlayerId & _playerId );
    common_types::IPlayerService * getPlayerByUser( const common_types::TUserId & _userId );
    common_types::IPlayerService * getPlayerByContext( const common_types::TContextId & _ctxId );


private:
    virtual void callbackUserOnline( const common_types::TUserId & _id, bool _online ) override;

    void updatePlayerState( const common_types::SPlayingServiceState & _state );

    bool playerControllerAsyncLaunch( const common_types::TPlayerId newId, const common_types::TContextId _ctxId ); // TODO: temp

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

    std::vector<PlayerController *> m_realPlayerControllersForTest; // TODO: temp ( must be in separate process )

    // service    
    std::future<bool> m_fuPlayerControllerAsyncLaunch; // TODO: temp

};

#endif // DISPATCHER_PLAYER_CONTOLLER_H
