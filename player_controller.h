#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <microservice_common/communication/network_interface.h>

#include "player_worker.h"

class PlayerController : public INetworkObserver
{
public:
    struct SInitSettings {
        common_types::TPlayerId id;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    PlayerController();

    bool init( const SInitSettings & _settings );
    void launch();




private:
    virtual void callbackNetworkRequest( PEnvironmentRequest _request ) override;

    PNetworkClient connectToNetwork( const common_types::TPlayerId & _id );

    // data
    PlayerWorker m_worker;
    SState m_state;


    // service
    PNetworkClient m_networkClient;

};

#endif // PLAYER_CONTROLLER_H
