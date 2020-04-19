
#include "dispatcher_player_contoller.h"

using namespace std;
using namespace common_types;

DispatcherPlayerContoller::DispatcherPlayerContoller()
{

}

bool DispatcherPlayerContoller::requestPlayer( const common_types::TUserId & _userId,
                                               const common_types::TContextId & _ctxId ){





    return true;
}

void DispatcherPlayerContoller::releasePlayer( const common_types::TPlayerId & _id ){

}

void DispatcherPlayerContoller::addObserver( IPlayerDispatcherObserver * _observer ){

}

void DispatcherPlayerContoller::removeObserver( IPlayerDispatcherObserver * _observer ){

}

std::vector<IPlayerService *> DispatcherPlayerContoller::getPlayers(){
    return m_playerControllers;
}

IPlayerService * DispatcherPlayerContoller::getPlayer( const common_types::TPlayerId & _playerId ){



}

IPlayerService * DispatcherPlayerContoller::getPlayerByUser( const common_types::TUserId & _userId ){

    return nullptr;
}

IPlayerService * DispatcherPlayerContoller::getPlayerByContext( const common_types::TContextId & _ctxId ){



}

void DispatcherPlayerContoller::updatePlayerState( const common_types::SPlayingServiceState & _state ){

}












