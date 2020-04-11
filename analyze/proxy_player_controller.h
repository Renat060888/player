#ifndef PLAYER_CONTROLLER_MIRROR_H
#define PLAYER_CONTROLLER_MIRROR_H

#include <microservice_common/communication/network_interface.h>

#include "common/common_types.h"

class ProxyPlayerController : public common_types::IPlayerService
{
public:
    struct SInitSettings {
        PNetworkClient network;
    };

    struct SState{
        SServiceState m_serviceState;
        SInitSettings settings;

        common_types::EPlayerStatus status;
        std::string lastError;
    };

    ProxyPlayerController();

    bool init( const SInitSettings & _settings );
    const SState & getState(){ return m_state; }

    const SServiceState & getServiceState() override;

    void start() override;
    void pause() override;
    void stop() override;
    bool stepForward() override;
    bool stepBackward() override;

    bool setRange( const common_types::TTimeRangeMillisec & _range ) override;
    void switchReverseMode( bool _reverse ) override;
    void switchLoopMode( bool _loop ) override;
    bool playFromPosition( int64_t _stepMillisec ) override;
    bool updatePlayingData() override;

    bool increasePlayingSpeed() override;
    bool decreasePlayingSpeed() override;
    void normalizePlayingSpeed() override;


private:

    // data
    SState m_state;



    // service



};

#endif // PLAYER_CONTROLLER_MIRROR_H
