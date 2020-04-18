#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <microservice_common/communication/network_interface.h>

#include "analyze/player_worker.h"

class PlayerController : public INetworkObserver
{
public:
    struct SInitSettings {
        common_types::TPlayerId id;
        common_types::TContextId ctxId;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    PlayerController();
    ~PlayerController();

    bool init( const SInitSettings & _settings );
    void launch();




private:
    virtual void callbackNetworkRequest( PEnvironmentRequest _request ) override;

    void threadAsyncLaunch();

    void pingPlayerAgent();
    std::string createPingMessage();
    PNetworkClient connectToNetwork( const common_types::TPlayerId & _id );

    // data
    SState m_state;
    PlayerWorker m_worker;
    bool m_shutdownCalled;
    int64_t m_lastPingAtMillisec;

    // service
    PNetworkClient m_networkClient;
    PEnvironmentRequest m_pingRequest;
    std::thread * m_trAsyncLaunch;
};

#endif // PLAYER_CONTROLLER_H
