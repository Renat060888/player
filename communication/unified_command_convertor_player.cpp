
#include "unified_command_convertor_player.h"

using namespace std;

UnifiedCommandConvertorPlayer::UnifiedCommandConvertorPlayer()
{

}

std::string UnifiedCommandConvertorPlayer::getCommandFromProgramArgs( const std::map<common_types::TCommandLineArgKey, common_types::TCommandLineArgVal> & _args ){

    assert( false && "TODO: do" );
}

std::string UnifiedCommandConvertorPlayer::getCommandFromConfigFile( const std::string & _commands ){

    assert( false && "TODO: do" );
}

std::string UnifiedCommandConvertorPlayer::getCommandFromHTTPRequest( const std::string & _httpMethod,
                                        const std::string & _uri,
                                        const std::string & _queryString,
                                        const std::string & _body ){

    assert( false && "TODO: do" );
}
