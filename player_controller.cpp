
#include <jsoncpp/json/writer.h>
#include <microservice_common/system/logger.h>
#include <microservice_common/communication/amqp_client_c.h>
#include <microservice_common/communication/amqp_controller.h>

#include "system/config_reader.h"
#include "player_controller.h"

using namespace std;

static constexpr int PING_TO_AGENT_INTERVAL_MILLISEC = 1000;
static constexpr const char * PRINT_HEADER = "PlayerController:";

PlayerController::PlayerController()
    : m_trAsyncLaunch(nullptr)
    , m_shutdownCalled(false)
    , m_lastPingAtMillisec(0)
{

}

PlayerController::~PlayerController(){

    m_shutdownCalled = true;
    common_utils::threadShutdown( m_trAsyncLaunch );
}

void PlayerController::callbackNetworkRequest( PEnvironmentRequest _request ){

    if( m_protobufAgentToController.ParseFromString(_request->getIncomingMessage()) ){
        const protocol_player_agent_to_controller::MessagePlayerAgent & msgAgent = m_protobufAgentToController.msg_player_agent();
        switch( msgAgent.cmd_type() ){
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_START : {
            processMsgStart( msgAgent );
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_PAUSE : {
            processMsgPause( msgAgent );
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_STOP : {
            processMsgStop( msgAgent );
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_REVERSE : {
            processMsgSwitchReverseMode( msgAgent );
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_LOOP : {
            processMsgSwitchLoopMode( msgAgent );
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_STEP : {
            processMsgStepForward( msgAgent );
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_FROM_POS : {
            processMsgPlayFromPosition( msgAgent );
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_SPEED : {
            processMsgIncreasePlayingSpeed( msgAgent );
            break;
        }
        default : {
            VS_LOG_ERROR << PRINT_HEADER
                         << " unknown player command from Agent, reason: " << m_protobufAgentToController.DebugString()
                         << endl;
        }
        }
    }
    else{
        VS_LOG_ERROR << PRINT_HEADER
                     << " protobuf parse failed, reason: " << m_protobufAgentToController.DebugString()
                     << endl;
    }
}

void PlayerController::processMsgStart( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    m_worker.start();
}

void PlayerController::processMsgPause( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    m_worker.pause();
}

void PlayerController::processMsgStop( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    m_worker.stop();
}

bool PlayerController::processMsgStepForward( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    m_worker.stepForward();
}

bool PlayerController::processMsgStepBackward( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

}

bool PlayerController::processMsgSetRange( const common_types::TTimeRangeMillisec & _range ){

}

void PlayerController::processMsgSwitchReverseMode( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    m_worker.switchReverseMode( true );
}

void PlayerController::processMsgSwitchLoopMode( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    m_worker.switchLoopMode( true );
}

bool PlayerController::processMsgPlayFromPosition( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    m_worker.playFromPosition( 999999999LL );
}

bool PlayerController::processMsgIncreasePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){


    m_worker.increasePlayingSpeed();
}

bool PlayerController::processMsgDecreasePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

}

void PlayerController::processMsgNormalizePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

}

bool PlayerController::init( const SInitSettings & _settings ){

    // worker
    PlayerWorker::SInitSettings workerSettings;
    workerSettings.ctxId = _settings.ctxId;
    if( ! m_worker.init(workerSettings) ){
        return false;
    }

    // communication with agent
    m_networkClient = connectToNetwork( _settings.id );
    if( ! m_networkClient ){
        return false;
    }

    PNetworkProvider provider = std::dynamic_pointer_cast<INetworkProvider>( m_networkClient );
    provider->addObserver( this );

    m_pingRequest = m_networkClient->getRequestInstance();

    // async activity
    if( _settings.async ){
        m_trAsyncLaunch = new std::thread( & PlayerController::threadAsyncLaunch, this );
    }


    return true;
}

void PlayerController::threadAsyncLaunch(){

    while( ! m_shutdownCalled ){

        launch();

        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
}

void PlayerController::launch(){

    if( (common_utils::getCurrentTimeMillisec() - m_lastPingAtMillisec) > PING_TO_AGENT_INTERVAL_MILLISEC ){
        m_lastPingAtMillisec = common_utils::getCurrentTimeMillisec();

        pingPlayerAgent();
    }
}

std::string PlayerController::createPingMessage(){

    m_worker.getState();

    Json::Value playerState;

    Json::Value rootRecord;
    rootRecord[ "cmd_type" ] = "service";
    rootRecord[ "cmd_name" ] = "ping_from_player";
    rootRecord[ "player_state" ] = playerState;
    rootRecord[ "player_id" ] = m_state.settings.id;
    rootRecord[ "error_occured" ] = false;
    rootRecord[ "error_str" ] = "HOLY_SHIT";

    Json::FastWriter jsonWriter;
    return jsonWriter.write( rootRecord );
}

void PlayerController::pingPlayerAgent(){

    const string & pingMessage = createPingMessage();
    m_pingRequest = m_networkClient->getRequestInstance();
    m_pingRequest->sendMessageAsync( pingMessage );
    return;

    // TODO: do reusable "ping request"
    if( m_pingRequest->isPerforming() ){
        return;
    }
}

PNetworkClient PlayerController::connectToNetwork( const common_types::TPlayerId & _id ){

    // amqp client
    PAmqpClient client = std::make_shared<AmqpClient>( INetworkEntity::INVALID_CONN_ID );

    AmqpClient::SInitSettings clientSettings;
    clientSettings.serverHost = CONFIG_PARAMS.baseParams.COMMUNICATION_AMQP_SERVER_HOST;
    clientSettings.amqpVirtualHost = CONFIG_PARAMS.baseParams.COMMUNICATION_AMQP_VIRTUAL_HOST;
    clientSettings.port = CONFIG_PARAMS.baseParams.COMMUNICATION_AMQP_SERVER_PORT;
    clientSettings.login = CONFIG_PARAMS.baseParams.COMMUNICATION_AMQP_LOGIN;
    clientSettings.pass = CONFIG_PARAMS.baseParams.COMMUNICATION_AMQP_PASS;
    clientSettings.deliveredMessageExpirationSec = 60;

    if( ! client->init(clientSettings) ){
        m_state.lastError = client->getState().m_lastError;
        return nullptr;
    }

    // controller for it
    SAmqpRouteParameters routes;
    routes.predatorExchangePointName = "player_dx_workers";
    routes.predatorQueueName = "player_q_worker_mailbox_" + _id;
    routes.predatorRoutingKeyName = "player_rk_to_worker_" + _id;

    routes.targetExchangePointName = "player_dx_agent";
    routes.targetQueueName = "player_q_agent_mailbox";
    routes.targetRoutingKeyName = "player_rk_to_agent";

    AmqpController::SInitSettings settings2;
    settings2.client = client;
    settings2.route = routes;

    PAmqpController controller = std::make_shared<AmqpController>( INetworkEntity::INVALID_CONN_ID );
    const bool rt = controller->init( settings2 );
    if( ! rt ){
        m_state.lastError = controller->getState().lastError;
        return nullptr;
    }

    return controller;
}








