
#include <jsoncpp/json/writer.h>
#include <microservice_common/system/logger.h>
#include <microservice_common/communication/amqp_client_c.h>
#include <microservice_common/communication/amqp_controller.h>

#include "common/common_utils.h"
#include "system/objrepr_bus_player.h"
#include "system/config_reader.h"
#include "player_controller.h"

using namespace std;
using namespace common_types;

static constexpr int PING_TO_AGENT_INTERVAL_MILLISEC = 1000;
static constexpr const char * PRINT_HEADER = "PlayerController:";

PlayerController::PlayerController()
    : m_trAsyncLaunch(nullptr)
    , m_shutdownCalled(false)
    , m_lastPingAtMillisec(0)
{

}

PlayerController::~PlayerController(){

    VS_LOG_INFO << PRINT_HEADER << " begin shutdown" << endl;

    m_worker.stop(); // NOTE: player must stop emit into objrepr before it's closing
    m_shutdownCalled = true;
    common_utils::threadShutdown( m_trAsyncLaunch );
    OBJREPR_BUS.closeContext();

    VS_LOG_INFO << PRINT_HEADER << " shutdown success" << endl;
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
            const ::protocol_player_agent_to_controller::MessageRequestPlayStep & step = msgAgent.msg_play_step();
            if( step.forward() ){
                processMsgStepForward( msgAgent );
            }
            else{
                processMsgStepBackward( msgAgent );
            }
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_FROM_POS : {
            processMsgPlayFromPosition( msgAgent );
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_PLAY_SPEED : {
            const ::protocol_player_agent_to_controller::MessageRequestPlaySpeed & speed = msgAgent.msg_play_speed();
            if( speed.normalize() ){
                processMsgNormalizePlayingSpeed( msgAgent );
            }
            else if( speed.increase() ){
                processMsgIncreasePlayingSpeed( msgAgent );
            }
            else if( ! speed.increase() ){
                processMsgDecreasePlayingSpeed( msgAgent );
            }
            break;
        }
        case protocol_player_agent_to_controller::EPlayerAgentCommandType::PACT_SET_RANGE : {
            processMsgSetRange( msgAgent );
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

    return m_worker.stepForward();
}

bool PlayerController::processMsgStepBackward( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    return m_worker.stepBackward();
}

bool PlayerController::processMsgSetRange( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    const protocol_player_agent_to_controller::MessageRequestSetRange & msgRange = _msgAgent.msg_set_range();
    return m_worker.setRange( { msgRange.leftplayingrangemillisec(), msgRange.rightplayingrangemillisec() } );
}

void PlayerController::processMsgSwitchReverseMode( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    const protocol_player_agent_to_controller::MessageRequestPlayReverse & msgReverse = _msgAgent.msg_play_reverse();
    m_worker.switchReverseMode( msgReverse.reverse() );
}

void PlayerController::processMsgSwitchLoopMode( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    const protocol_player_agent_to_controller::MessageRequestPlayLoop & msgLoop = _msgAgent.msg_play_loop();
    m_worker.switchLoopMode( msgLoop.loop() );
}

bool PlayerController::processMsgPlayFromPosition( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    const protocol_player_agent_to_controller::MessageRequestPlayFromPos & msgFromPos = _msgAgent.msg_play_from_pos();
    return m_worker.playFromPosition( msgFromPos.positionmillisec() );
}

bool PlayerController::processMsgIncreasePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    return m_worker.increasePlayingSpeed();
}

bool PlayerController::processMsgDecreasePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    return m_worker.increasePlayingSpeed();
}

void PlayerController::processMsgNormalizePlayingSpeed( const protocol_player_agent_to_controller::MessagePlayerAgent & _msgAgent ){

    m_worker.normalizePlayingSpeed();
}

void PlayerController::processMsgShutdownController(){

    m_shutdownCalled = true;
}

bool PlayerController::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

#if 0 // NOTE: in separate process mode
    // objrepr-bus
    ObjreprBus::SInitSettings busSettings;
    busSettings.objreprConfigPath = CONFIG_PARAMS.baseParams.OBJREPR_CONFIG_PATH;
    busSettings.initialContextName = OBJREPR_BUS.getContextNameById( _settings.ctxId );
    if( ! OBJREPR_BUS.init(busSettings) ){
        VS_LOG_CRITICAL << "objrepr init failed: " << OBJREPR_BUS.getLastError() << endl;
        return false;
    }

    // TODO: register in GDM
#else
    if( ! OBJREPR_BUS.openContext(_settings.ctxId) ){
        return false;
    }
#endif

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
    m_networkProvider = provider;

    m_pingRequest = m_networkClient->getRequestInstance();

    // async activity
    if( _settings.async ){
        m_trAsyncLaunch = new std::thread( & PlayerController::threadAsyncLaunch, this );
    }

    return true;
}

void PlayerController::threadAsyncLaunch(){
    assert( m_state.settings.async );

    while( ! m_shutdownCalled ){
        pingPlayerAgent();

        m_networkProvider->runNetworkCallbacks();

        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
}

void PlayerController::launch(){
    assert( ! m_state.settings.async );

    while( ! m_shutdownCalled ){
        pingPlayerAgent();

        m_networkProvider->runNetworkCallbacks();

        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
}

std::string PlayerController::createPingMessage(){

    const PlayerWorker::SState * state = & m_worker.getState();

    // player state
    Json::Value playerState;
    playerState["step_interval"] = (long long)state->playIterator->getState().m_updateGenerationMillisec;
    playerState["current_step"] = (long long)state->playIterator->getState().m_currentPlayStep;
    playerState["global_range_left"] = (long long)state->playIterator->getState().m_globalTimeRangeMillisec.first;
    playerState["global_range_right"] = (long long)state->playIterator->getState().m_globalTimeRangeMillisec.second;

    Json::Value datasetsRecord;
    const DatasourceMixer::SState & mixerState = state->mixer->getState();
    for( const DatasourceReader * datasrc : mixerState.settings.datasourceReaders ){

        Json::Value rangesRecord;
//        for( const DatasourceReader::SBeacon::SDataBlock * block : datasrc->getState().payloadDataRangesInfo ){
//            Json::Value rangeRecord;
//            rangeRecord["range_left"] = (long long)block->timestampRangeMillisec.first;
//            rangeRecord["range_right"] = (long long)block->timestampRangeMillisec.second;
//            rangesRecord.append( rangeRecord );
//        }
        for( const TTimeRangeMillisec & timeRange : datasrc->getState().sessionsTimeRangeMillisec ){
            Json::Value rangeRecord;
            rangeRecord["range_left"] = (long long)timeRange.first;
            rangeRecord["range_right"] = (long long)timeRange.second;
            rangesRecord.append( rangeRecord );
        }

        Json::Value dsRecord;
        dsRecord["unique_id"] = (long long)datasrc->getState().settings.persistenceSetId;
        dsRecord["real"] = true;
        dsRecord["ranges"] = rangesRecord;
        datasetsRecord.append( dsRecord );
    }
    playerState["datasets"] = datasetsRecord;

    // general message
    Json::Value rootRecord;
    rootRecord[ "cmd_type" ] = "service";
    rootRecord[ "cmd_name" ] = "ping_from_player";
    rootRecord[ "player_state" ] = playerState;
    rootRecord[ "status" ] = common_utils::printPlayingStatus( state->playingStatus );
    rootRecord[ "player_id" ] = m_state.settings.id;
    rootRecord[ "error_occured" ] = false;
    rootRecord[ "error_str" ] = "HOLY_SHIT";

    Json::FastWriter jsonWriter;

//    cout << "state: " << jsonWriter.write( rootRecord ) << endl;

    return jsonWriter.write( rootRecord );
}

void PlayerController::pingPlayerAgent(){

    if( (common_utils::getCurrentTimeMillisec() - m_lastPingAtMillisec) > PING_TO_AGENT_INTERVAL_MILLISEC ){
        m_lastPingAtMillisec = common_utils::getCurrentTimeMillisec();

        const string & pingMessage = createPingMessage();
        m_pingRequest = m_networkClient->getRequestInstance();
        m_pingRequest->sendMessageAsync( pingMessage );
        return;

        // TODO: do reusable "ping request"
        if( m_pingRequest->isPerforming() ){
            return;
        }
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








