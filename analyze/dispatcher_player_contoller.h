#ifndef DISPATCHER_PLAYER_CONTOLLER_H
#define DISPATCHER_PLAYER_CONTOLLER_H

#include <map>

#include "common/common_types.h"
#include "proxy_player_controller.h"

#include "player_controller.h"

class DispatcherPlayerContoller
{
    friend class CommandPlayerControllerPing;
public:
    class IPlayerDispatcherObserver {
    public:
        virtual ~IPlayerDispatcherObserver(){}

        virtual void callbackPlayerControllerOnline( const common_types::TPlayerId & _id, bool _online ) = 0;
    };

    struct SState {

        std::string lastError;
    };

    DispatcherPlayerContoller();
    const SState & getState(){ return m_state; }

    bool requestPlayer( const common_types::TUserId & _userId, const common_types::TContextId & _ctxId );
    void releasePlayer( const common_types::TPlayerId & _id );

    void addObserver( IPlayerDispatcherObserver * _observer );
    void removeObserver( IPlayerDispatcherObserver * _observer );

    std::vector<common_types::IPlayerService *> getPlayers();
    common_types::IPlayerService * getPlayer( const common_types::TPlayerId & _id );
    common_types::IPlayerService * getPlayerByUser( const common_types::TUserId & _id );


private:
    void updatePlayerState( const common_types::SPlayerState & _state );

    // data
    SState m_state;
    std::vector<common_types::IPlayerService *> m_playerControllers;
    std::map<common_types::TPlayerId, common_types::IPlayerService *> m_playersById;
    std::map<common_types::TContextId, common_types::IPlayerService *> m_playersByContextId;
    std::map<common_types::TUserId, common_types::IPlayerService *> m_playersByUserId;
    std::vector<IPlayerDispatcherObserver *> m_observers;

    // TODO: temp ( must be in separate process )
    std::vector<PlayerController *> m_realPlayerControllersForTest;

    // service


};

#endif // DISPATCHER_PLAYER_CONTOLLER_H
