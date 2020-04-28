
#include "proxy_player_controller.h"

using namespace std;
using namespace common_types;

ProxyPlayerController::ProxyPlayerController()
{

}

ProxyPlayerController::~ProxyPlayerController()
{

}

bool ProxyPlayerController::init( const SInitSettings & _settings ){

    m_state.settings = _settings;




    return true;
}

void ProxyPlayerController::setServiceState( const common_types::SPlayingServiceState & _state ){
    m_state.m_serviceState = _state;
}

const SPlayingServiceState & ProxyPlayerController::getServiceState(){
    return m_state.m_serviceState;
}

void ProxyPlayerController::start(){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestPlayStart msgStartPlaying;
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_START );
    msgAgent.mutable_header()->CopyFrom( header );
    msgAgent.mutable_msg_play_start()->CopyFrom( msgStartPlaying );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
}

void ProxyPlayerController::pause(){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestPlayPause msgPausePlaying;
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_PAUSE );
    msgAgent.set_allocated_header( & header );
    msgAgent.set_allocated_msg_play_pause( & msgPausePlaying );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
}

void ProxyPlayerController::stop(){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestPlayStop msgStopPlaying;
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_STOP );
    msgAgent.set_allocated_header( & header );
    msgAgent.set_allocated_msg_play_stop( & msgStopPlaying );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
}

bool ProxyPlayerController::stepForward(){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestPlayStep msgPlayStep;
    msgPlayStep.set_forward( true );
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_STEP );
    msgAgent.set_allocated_header( & header );
    msgAgent.set_allocated_msg_play_step( & msgPlayStep );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
    return true;
}

bool ProxyPlayerController::stepBackward(){


    // TODO: I think it's redundant
}

bool ProxyPlayerController::setRange( const TTimeRangeMillisec & _range ){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestSetRange msgSetPlayingRange;
    msgSetPlayingRange.set_leftplayingrangemillisec( _range.first );
    msgSetPlayingRange.set_rightplayingrangemillisec( _range.second );
    msgSetPlayingRange.set_resetrange( false ); // TODO: come up with an agreement between client & player
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_SET_RANGE );
    msgAgent.set_allocated_header( & header );
    msgAgent.set_allocated_msg_set_range( & msgSetPlayingRange );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
    return true;
}

void ProxyPlayerController::switchReverseMode( bool _reverse ){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestPlayReverse msgSwitchReverse;
    msgSwitchReverse.set_reverse( _reverse );
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_REVERSE );
    msgAgent.set_allocated_header( & header );
    msgAgent.set_allocated_msg_play_reverse( & msgSwitchReverse );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
}

void ProxyPlayerController::switchLoopMode( bool _loop ){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestPlayLoop msgSwitchLoop;
    msgSwitchLoop.set_loop( _loop );
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_LOOP );
    msgAgent.set_allocated_header( & header );
    msgAgent.set_allocated_msg_play_loop( & msgSwitchLoop );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
}

bool ProxyPlayerController::playFromPosition( int64_t _stepMillisec ){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestPlayFromPos msgPlayFromPos;
    msgPlayFromPos.set_positionmillisec( _stepMillisec );
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_FROM_POS );
    msgAgent.set_allocated_header( & header );
    msgAgent.set_allocated_msg_play_from_pos( & msgPlayFromPos );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
    return true;
}

bool ProxyPlayerController::updatePlayingData(){

}

bool ProxyPlayerController::increasePlayingSpeed(){

    protocol_player_agent_to_controller::MessageHeader header;
    protocol_player_agent_to_controller::MessageRequestPlaySpeed msgPlaySpeed;
    msgPlaySpeed.set_normalize( false );
    msgPlaySpeed.set_increase( true );
    protocol_player_agent_to_controller::MessagePlayerAgent msgAgent;
    msgAgent.set_cmd_type( protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_SPEED );
    msgAgent.set_allocated_header( & header );
    msgAgent.set_allocated_msg_play_speed( & msgPlaySpeed );

    m_protobufAgentToController.set_sender( protocol_player_agent_to_controller::W_PLAYER_AGENT );
    m_protobufAgentToController.set_allocated_msg_player_agent( & msgAgent );

    const string serializedCommand = m_protobufAgentToController.SerializeAsString();

    PEnvironmentRequest request = m_state.settings.network->getRequestInstance();
    request->sendMessageAsync( serializedCommand );
    return true;
}

bool ProxyPlayerController::decreasePlayingSpeed(){

}

void ProxyPlayerController::normalizePlayingSpeed(){

}














