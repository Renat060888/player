
#include <microservice_common/communication/amqp_client_c.h>
#include <microservice_common/communication/amqp_controller.h>

#include "system/config_reader.h"
#include "player_controller.h"

using namespace std;

PlayerController::PlayerController()
{

}

void PlayerController::callbackNetworkRequest( PEnvironmentRequest _request ){






}

bool PlayerController::init( const SInitSettings & _settings ){





    return true;
}

void PlayerController::launch(){

}

PNetworkClient PlayerController::connectToNetwork( const common_types::TPlayerId & _id ){

    // amqp client
    PAmqpClient client = std::make_shared<AmqpClient>( INetworkEntity::INVALID_CONN_ID );

    AmqpClient::SInitSettings clientSettings;
    clientSettings.serverHost = CONFIG_PARAMS.COMMUNICATION_AMQP_SERVER_HOST;
    clientSettings.amqpVirtualHost = CONFIG_PARAMS.COMMUNICATION_AMQP_VIRTUAL_HOST;
    clientSettings.port = CONFIG_PARAMS.COMMUNICATION_AMQP_SERVER_PORT;
    clientSettings.login = CONFIG_PARAMS.COMMUNICATION_AMQP_LOGIN;
    clientSettings.pass = CONFIG_PARAMS.COMMUNICATION_AMQP_PASS;
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








