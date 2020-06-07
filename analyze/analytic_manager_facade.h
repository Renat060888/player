#ifndef ANALYTIC_MANAGER_FACADE_H
#define ANALYTIC_MANAGER_FACADE_H

#include <thread>

#include "dispatcher_user.h"
#include "dispatcher_player.h"

class AnalyticManagerFacade
{
public:
    struct SServiceLocator {
        common_types::IServiceInternalCommunication * serviceInternalCommunication;
        common_types::IServiceExternalCommunication * serviceExternalCommunication;
        SystemEnvironmentFacadePlayer * systemEnvironment;
    };

    struct SInitSettings {
        SServiceLocator services;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    AnalyticManagerFacade();
    ~AnalyticManagerFacade();

    bool init( const SInitSettings & _settings );
    const SState & getState(){ return m_state; }
    void shutdown();

    DispatcherUser * getUserDispatcher();
    DispatcherPlayer * getPlayerDispatcher();


private:
    void threadMaintenance();

    // data
    SState m_state;
    bool shutdownCalled;

    // service
    DispatcherUser m_userDispatcher;
    DispatcherPlayer m_playerControllerDispatcher;
    std::thread * m_threadMaintenance;
};

#endif // ANALYTIC_MANAGER_FACADE_H
