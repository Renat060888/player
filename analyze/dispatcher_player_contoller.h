#ifndef DISPATCHER_PLAYER_CONTOLLER_H
#define DISPATCHER_PLAYER_CONTOLLER_H

#include <map>

#include "common/common_types.h"
#include "mirror_player_controller.h"

class DispatcherPlayerContoller
{
    friend class CommandPlayerControllerPing;
public:
    class IPlayerControllerDispatcherObserver {
    public:
        virtual ~IPlayerControllerDispatcherObserver(){}

        virtual void callbackPlayerControllerOnline( const common_types::TPlayerId & _id, bool _online ) = 0;
    };

    struct SState {

        std::string lastError;
    };

    DispatcherPlayerContoller();
    const SState & getState(){ return m_state; }

    bool requestPlayer( const common_types::TUserId & _userId, const common_types::TContextId & _ctxId, const common_types::TMissionId & _missionId );
    void releasePlayer( const common_types::TPlayerId & _id );

    void addObserver( IPlayerControllerDispatcherObserver * _observer );
    void removeObserver( IPlayerControllerDispatcherObserver * _observer );

    std::vector<MirrorPlayerController *> getPlayers(){ return m_playerControllers; }
    MirrorPlayerController * getPlayer( const common_types::TPlayerId & _id );
    MirrorPlayerController * getPlayerByUser( const common_types::TUserId & _id );


private:
    void updatePlayerState( const common_types::SPlayerState & _state );

    // data
    SState m_state;
    std::vector<MirrorPlayerController *> m_playerControllers;
    std::map<common_types::TPlayerId, MirrorPlayerController *> m_playersById;
    std::map<common_types::TContextId, MirrorPlayerController *> m_playersByContextId;
    std::map<common_types::TUserId, MirrorPlayerController *> m_playersByUserId;
    std::vector<IPlayerControllerDispatcherObserver *> m_observers;

    // service


};

#endif // DISPATCHER_PLAYER_CONTOLLER_H
