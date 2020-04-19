
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>

#include <boost/property_tree/json_parser.hpp>
#include <microservice_common/system/logger.h>

#include "common/common_utils.h"
#include "command_factory.h"
#include "commands/cmd_user_ping.h"
#include "commands/cmd_user_register.h"
#include "commands/cmd_context_open.h"
#include "commands/cmd_context_close.h"
#include "commands/cmd_player_controller_ping.h"
#include "common/common_vars.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "CommandFactory:";

static void parsePlayerState( const boost::property_tree::ptree & playerState, common_types::SPlayingInfo & info ){

    info.stepIntervalMillisec = playerState.get<int64_t>("step_interval");
    info.currentStepMillisec = playerState.get<int64_t>("current_step");
    info.globalRangeMillisec.first = playerState.get<int64_t>("global_range_left");
    info.globalRangeMillisec.second = playerState.get<int64_t>("global_range_right");

    boost::property_tree::ptree dataSets = playerState.get_child("datasets");
    for( auto iter = dataSets.begin(); iter != dataSets.end(); ++iter ){
        boost::property_tree::ptree arrElement = iter->second;

        common_types::SPlayingDataSet dataSet;
        dataSet.uniqueId = arrElement.get<int64_t>("unique_id");
        dataSet.real = arrElement.get<bool>("real");
//        dataSet.description = arrElement.get<std::string>("descr");

        boost::property_tree::ptree ranges = dataSets.get_child("ranges");
        for( auto iter = ranges.begin(); iter != ranges.end(); ++iter ){
            boost::property_tree::ptree arrElement = iter->second;

            dataSet.dataRanges.push_back( {arrElement.get<int64_t>("range_left"), arrElement.get<int64_t>("range_right")} );
        }

        info.playingData.push_back( dataSet );
    }
}

CommandFactory::CommandFactory( common_types::SIncomingCommandServices & _commandServices )
    : m_commandServices(_commandServices)
{

}

PCommand CommandFactory::createCommand( PEnvironmentRequest _request ){

//        VS_LOG_DBG << PRINT_HEADER << common_utils::getCurrentDateTimeStr() << " incoming msg [" << _request->getIncomingMessage() << "]" << endl;

    if( _request->getIncomingMessage().empty() ){
        sendFailToExternal( _request, "I don't see the command (-_-)" );
        return nullptr;
    }

    boost::property_tree::ptree parsedJson;
    try{
        istringstream contentStream( _request->getIncomingMessage() );
        boost::property_tree::json_parser::read_json( contentStream, parsedJson );
    }
    catch( boost::property_tree::json_parser::json_parser_error & _ex ){
        VS_LOG_ERROR << PRINT_HEADER
                     << " parse failed of [" << _request->getIncomingMessage() << "]" << endl
                     << " reason: [" << _ex.what() << "]" << endl;
        return nullptr;
    }

    PCommand cmd;

    // -------------------------------------------------------------------------------------
    // service
    // -------------------------------------------------------------------------------------
    if( "service" == parsedJson.get<string>(common_vars::cmd::COMMAND_TYPE) ){
        if( "ping" == parsedJson.get<string>(common_vars::cmd::COMMAND_NAME) ){
            PCommandUserPing cmd1 = std::make_shared<CommandUserPing>( & m_commandServices );
            cmd1->m_userId = parsedJson.get<string>(common_vars::cmd::USER_ID);
            cmd = cmd1;
        }
        else if( "ping_from_player" == parsedJson.get<string>(common_vars::cmd::COMMAND_NAME) ){
            PCommandPlayerControllerPing cmd1 = std::make_shared<CommandPlayerControllerPing>( & m_commandServices );

            const boost::property_tree::ptree & playerState = parsedJson.get_child( "player_state" );
            parsePlayerState( playerState, cmd1->playerState.info );

            cmd1->playerState.ctxId = parsedJson.get<uint32_t>("ctx_id");
            cmd1->playerState.playerId = parsedJson.get<string>("player_id");
            cmd1->playerState.lastError = parsedJson.get<string>("error_str");
            cmd1->playerState.status = common_utils::convertPlayingStatusFromStr( parsedJson.get<string>("status") );
            cmd = cmd1;
        }
        else if( "register" == parsedJson.get<string>(common_vars::cmd::COMMAND_NAME) ){
            PCommandUserRegister cmd1 = std::make_shared<CommandUserRegister>( & m_commandServices );
            cmd1->m_userIpStr = parsedJson.get<string>(common_vars::cmd::USER_IP);
            cmd1->m_userPid = parsedJson.get<common_types::TPid>(common_vars::cmd::USER_PID);
            cmd = cmd1;
        }
        else if( "context_open" == parsedJson.get<string>(common_vars::cmd::COMMAND_NAME) ){
            PCommandContextOpen cmd1 = std::make_shared<CommandContextOpen>( & m_commandServices );
            cmd1->m_userId = parsedJson.get<string>(common_vars::cmd::USER_ID);
            cmd1->m_contextName = parsedJson.get<string>(common_vars::cmd::CONTEXT_NAME);
            cmd = cmd1;
        }
        else if( "context_close" == parsedJson.get<string>(common_vars::cmd::COMMAND_NAME) ){
            PCommandContextClose cmd1 = std::make_shared<CommandContextClose>( & m_commandServices );
            cmd1->m_userId = parsedJson.get<string>(common_vars::cmd::USER_ID);
            cmd = cmd1;
        }
        else{
            VS_LOG_WARN << PRINT_HEADER << " unknown command name [" << parsedJson.get<string>(common_vars::cmd::COMMAND_NAME) << "]" << endl;
            sendFailToExternal( _request, "I don't know such command name of 'service' command type (-_-)" );
            return nullptr;
        }
    }
    // -------------------------------------------------------------------------------------
    // source
    // -------------------------------------------------------------------------------------
    else if( "source" == parsedJson.get<string>(common_vars::cmd::COMMAND_TYPE) ){



    }
    else{
        VS_LOG_WARN << PRINT_HEADER << " unknown command type [" << parsedJson.get<string>(common_vars::cmd::COMMAND_TYPE) << "]" << endl;
        sendFailToExternal( _request, "I don't know such command type (-_-)" );
        return nullptr;
    }

    cmd->m_request = _request;
    return cmd;
}

void CommandFactory::sendFailToExternal( PEnvironmentRequest _request, const string _msg ){

    Json::Value root;

    root["response"] = "fail";
    root["body"] = _msg;

    Json::FastWriter writer;

    _request->setOutcomingMessage( writer.write( root ) );
}














