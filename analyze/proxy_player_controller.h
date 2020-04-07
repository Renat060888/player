#ifndef PLAYER_CONTROLLER_MIRROR_H
#define PLAYER_CONTROLLER_MIRROR_H

#include <microservice_common/communication/network_interface.h>

#include "common/common_types.h"

class ProxyPlayerController
{
public:
    struct SInitSettings {
        PNetworkClient network;
    };

    struct SState{
        SInitSettings settings;
        std::string lastError;
    };

    ProxyPlayerController();

    bool init( const SInitSettings & _settings );
    const SState & getState(){ return m_state; }

    void start();
    void pause();
    void stop();
    bool stepForward();
    bool stepBackward();

    bool setRange( const common_types::TTimeRangeMillisec & _range );
    void switchReverseMode( bool _reverse );
    void switchLoopMode( bool _loop );
    bool playFromPosition( int64_t _stepMillisec );
    bool updatePlayingData();

    bool increasePlayingSpeed();
    bool decreasePlayingSpeed();
    void normalizePlayingSpeed();


private:

    // data
    SState m_state;



    // service



};

#endif // PLAYER_CONTROLLER_MIRROR_H
