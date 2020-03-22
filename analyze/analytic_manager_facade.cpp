
#include "analytic_manager_facade.h"

using namespace std;

AnalyticManagerFacade::AnalyticManagerFacade()
    : shutdownCalled(false)
{

}

bool AnalyticManagerFacade::init( const SInitSettings & _settings ){


    m_threadMaintenance = new std::thread( & AnalyticManagerFacade::threadMaintenance, this );

    return true;
}

void AnalyticManagerFacade::shutdown(){

}

void AnalyticManagerFacade::threadMaintenance(){

    while( ! shutdownCalled ){

        m_userDispatcher.runClock();

        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
}

MirrorPlayerController * AnalyticManagerFacade::getPlayer( const common_types::TUserId & _id ){

    MirrorPlayerController * player = m_playerControllerDispatcher.getPlayerByUser( _id );
    if( ! player ){
        m_state.lastError = m_playerControllerDispatcher.getState().lastError;
        return nullptr;
    }

    return player;
}

DispatcherUser * AnalyticManagerFacade::getUserDispatcher(){
    return & m_userDispatcher;
}

