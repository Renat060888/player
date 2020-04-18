
#include <jsoncpp/json/writer.h>
#include <microservice_common/communication/amqp_client_c.h>
#include <microservice_common/communication/amqp_controller.h>

#include "system/config_reader.h"
#include "player_controller.h"

using namespace std;

static constexpr int PING_INTERVAL_MILLISEC = 1000;

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

    // incoming commands for the player




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
    m_trAsyncLaunch = new std::thread( & PlayerController::threadAsyncLaunch, this );

    return true;
}

void PlayerController::threadAsyncLaunch(){

    while( ! m_shutdownCalled ){

        launch();
    }
}

void PlayerController::launch(){

    if( (common_utils::getCurrentTimeMillisec() - m_lastPingAtMillisec) > PING_INTERVAL_MILLISEC ){
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








