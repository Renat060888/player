
#include "dispatcher_player_contoller.h"

using namespace std;

DispatcherPlayerContoller::DispatcherPlayerContoller()
{

}

bool DispatcherPlayerContoller::requestPlayer( const common_types::TUserId & _userId, const common_types::TContextId & _ctxId, const common_types::TMissionId & _missionId ){

}

void DispatcherPlayerContoller::releasePlayer( const common_types::TPlayerId & _id ){

}

void DispatcherPlayerContoller::addObserver( IPlayerControllerDispatcherObserver * _observer ){

}

void DispatcherPlayerContoller::removeObserver( IPlayerControllerDispatcherObserver * _observer ){

}

MirrorPlayerController * DispatcherPlayerContoller::getPlayer( const common_types::TPlayerId & _id ){

}

MirrorPlayerController * DispatcherPlayerContoller::getPlayerByUser( const common_types::TUserId & _id ){

}

void DispatcherPlayerContoller::updatePlayerState( const common_types::SPlayerState & _state ){

}
