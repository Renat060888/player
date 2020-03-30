#ifndef PLAYER_AGENT_H
#define PLAYER_AGENT_H

#include <string>
#include <future>

#include <boost/signals2.hpp>
#include <microservice_common/system/system_environment_facade.h>

#include "common/common_types.h"
#include "communication/communication_gateway_facade_player.h"
#include "storage/storage_engine_facade.h"
#include "analyze/analytic_manager_facade.h"

class PlayerAgent
{
    static void callbackUnixInterruptSignal();
    static boost::signals2::signal<void()> m_unixInterruptSignal;
public:
    struct SInitSettings {

    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    PlayerAgent();
    ~PlayerAgent();

    bool init( const SInitSettings & _settings );
    const SState & getState(){ return m_state; }

    void launch();


private:
    void shutdown();
    void shutdownByUnixInterruptSignal();
    void checkForSelfShutdown();

    // data
    SState m_state;
    common_types::SIncomingCommandServices m_commandServices;
    std::atomic<bool> m_shutdownCalled;
    std::future<void> m_selfShutdownFuture;

    // service
    SystemEnvironmentFacade * m_systemEnvironment;
    CommunicationGatewayFacadePlayer * m_communicateGateway;
    StorageEngineFacade * m_storageEngine;
    AnalyticManagerFacade * m_analyticManager;
};

#endif // PLAYER_AGENT_H
