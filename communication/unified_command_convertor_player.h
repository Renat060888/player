#ifndef UNIFIED_COMMAND_CONVERTOR_DSS_H
#define UNIFIED_COMMAND_CONVERTOR_DSS_H

#include <microservice_common/communication/unified_command_convertor.h>

class UnifiedCommandConvertorPlayer : public UnifiedCommandConvertor
{
public:
    static UnifiedCommandConvertorPlayer & singleton(){
        static UnifiedCommandConvertorPlayer instance;
        return instance;
    }


    virtual std::string getCommandFromProgramArgs( const std::map<common_types::TCommandLineArgKey, common_types::TCommandLineArgVal> & _args ) override;
    virtual std::string getCommandFromConfigFile( const std::string & _commands ) override;
    virtual std::string getCommandFromHTTPRequest( const std::string & _httpMethod,
                                                    const std::string & _uri,
                                                    const std::string & _queryString,
                                                    const std::string & _body ) override;

private:
    UnifiedCommandConvertorPlayer();




};
#define UNIFIED_COMMAND_CONVERTOR UnifiedCommandConvertorPlayer::singleton()

#endif // UNIFIED_COMMAND_CONVERTOR_DSS_H
