
#include "analytic_manager_facade.h"

using namespace std;

AnalyticManagerFacade::AnalyticManagerFacade()
    : shutdownCalled(false)
{

}

AnalyticManagerFacade::~AnalyticManagerFacade(){

    shutdown();
}

bool AnalyticManagerFacade::init( const SInitSettings & _settings ){


    m_threadMaintenance = new std::thread( & AnalyticManagerFacade::threadMaintenance, this );

    return true;
}

void AnalyticManagerFacade::shutdown(){

    if( ! shutdownCalled ){
        shutdownCalled = true;




    }
}

void AnalyticManagerFacade::threadMaintenance(){

    while( ! shutdownCalled ){

        m_userDispatcher.runClock();
        m_playerControllerDispatcher.runClock();

        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
}

common_types::IPlayerService * AnalyticManagerFacade::getPlayer( const common_types::TUserId & _id ){

    common_types::IPlayerService * player = m_playerControllerDispatcher.getPlayerByUser( _id );
    if( ! player ){
        m_state.lastError = m_playerControllerDispatcher.getState().lastError;
        return nullptr;
    }

    return player;
}

DispatcherPlayerContoller * AnalyticManagerFacade::getPlayerDispatcher(){
    return & m_playerControllerDispatcher;
}

DispatcherUser * AnalyticManagerFacade::getUserDispatcher(){
    return & m_userDispatcher;
}















