
#include <microservice_common/system/logger.h>

#include "system/system_environment_facade_player.h"
#include "analytic_manager_facade.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "AnalyticMgr:";

AnalyticManagerFacade::AnalyticManagerFacade()
    : shutdownCalled(false)
{

}

AnalyticManagerFacade::~AnalyticManagerFacade(){

    shutdown();
}

bool AnalyticManagerFacade::init( const SInitSettings & _settings ){

    // users dispatching
    DispatcherUser::SInitSettings udSettings;
    udSettings.serviceWriteAheadLogger = _settings.services.systemEnvironment->serviceForWriteAheadLogging();
    if( ! m_userDispatcher.init(udSettings) ){
        return false;
    }

    // player dispatching
    DispatcherPlayer::SInitSettings pcdSettings;
    pcdSettings.serviceInternalCommunication = _settings.services.serviceInternalCommunication;
    pcdSettings.systemEnvironment = _settings.services.systemEnvironment;
    if( ! m_playerControllerDispatcher.init(pcdSettings) ){
        return false;
    }

    m_userDispatcher.addObserver( & m_playerControllerDispatcher );

    m_threadMaintenance = new std::thread( & AnalyticManagerFacade::threadMaintenance, this );

    return true;
}

void AnalyticManagerFacade::shutdown(){

    if( ! shutdownCalled ){
        shutdownCalled = true;




    }
}

void AnalyticManagerFacade::threadMaintenance(){

    VS_LOG_INFO << PRINT_HEADER << " start a maintenance THREAD" << endl;

    while( ! shutdownCalled ){

        m_userDispatcher.runClock();
        m_playerControllerDispatcher.runClock();

        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }

    VS_LOG_INFO << PRINT_HEADER << " exit from a maintenance THREAD" << endl;
}

DispatcherPlayer * AnalyticManagerFacade::getPlayerDispatcher(){
    return & m_playerControllerDispatcher;
}

DispatcherUser * AnalyticManagerFacade::getUserDispatcher(){
    return & m_userDispatcher;
}















