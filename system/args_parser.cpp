
#include "args_parser.h"

using namespace std;

bpo::options_description ArgsParser::getArgumentsDescrTemplateMethodPart() {

    bpo::options_description desc("Available options");

    desc.add_options()
            ("help,h","show this help")
            ("version,V", "version of program")
            ("about,A", "description")
            ("main-config,C", bpo::value<std::string>(), "main config file")
            ("main-config-sample,P", "print main config sample")
            ("daemon,D", "start as daemon")
            ("start-agent", "start player agent")
            ("start-contoller", "start player controller")
            ("stop", "stop server")
            ;

    return desc;
}

void ArgsParser::checkArgumentsTemplateMethodPart( const bpo::variables_map & _varMap ) {

    if( _varMap.find("main-config") != _varMap.end() ){
        m_commmandLineArgs[EPlayerArguments::MAIN_CONFIG_PATH] = _varMap["main-config"].as<std::string>();
    }

    if( _varMap.find("daemon") != _varMap.end() ){
        m_commmandLineArgs[EPlayerArguments::AS_DAEMON] = "bla-bla";
    }

    if( _varMap.find("start-agent") != _varMap.end() ){
        m_commmandLineArgs[EPlayerArguments::SHELL_CMD_START_PLAYER_AGENT] = "bla-bla";
    }

    if( _varMap.find("start-contoller") != _varMap.end() ){
        m_commmandLineArgs[EPlayerArguments::SHELL_CMD_START_PLAYER_CONTROLLER] = "bla-bla";
    }

    if( _varMap.find("stop") != _varMap.end() ){
        m_commmandLineArgs[EPlayerArguments::SHELL_CMD_TO_PLAYER]
            = AArgsParser::getSettings().commandConvertor->getCommandFromProgramArgs( { {"", ""} } );
    }
}

void ArgsParser::version() {

    // TODO: do
}

void ArgsParser::about() {

    // TODO: do
}
