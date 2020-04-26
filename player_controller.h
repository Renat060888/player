#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include "communication/protocols/protocol_player_agent_to_controller.pb.h"
#include <microservice_common/communication/network_interface.h>

#include "analyze/player_worker.h"

class PlayerController : public INetworkObserver
{
public:
    struct SInitSettings {
        SInitSettings()
            : async(false)
        {}
        bool async;
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
    const SState & getState(){ return m_state; }
    void launch();




private:
    virtual void callbackNetworkRequest( PEnvironmentRequest _request ) override;

    void threadAsyncLaunch();

    void pingPlayerAgent();
    std::string createPingMessage();
    PNetworkClient connectToNetwork( const common_types::TPlayerId & _id );

    void processMsgStart( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    void processMsgPause( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    void processMsgStop( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    bool processMsgStepForward( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    bool processMsgStepBackward( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );

    bool processMsgSetRange( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    void processMsgSwitchReverseMode( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    void processMsgSwitchLoopMode( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    bool processMsgPlayFromPosition( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );

    bool processMsgIncreasePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    bool processMsgDecreasePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );
    void processMsgNormalizePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent );

    // data
    SState m_state;
    PlayerWorker m_worker;
    bool m_shutdownCalled;
    int64_t m_lastPingAtMillisec;

    // service
    PNetworkClient m_networkClient;
    PEnvironmentRequest m_pingRequest;
    std::thread * m_trAsyncLaunch;
    protocol_player_agent_to_controller::ProtobufPlayerAgentToController m_protobufAgentToController;
};

#endif // PLAYER_CONTROLLER_H
