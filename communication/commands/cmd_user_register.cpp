
#include <jsoncpp/json/writer.h>

#include "analyze/analytic_manager_facade.h"
#include "cmd_user_register.h"

using namespace std;

CommandUserRegister::CommandUserRegister( common_types::SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandUserRegister::exec(){

    DispatcherUser * userDipatcher = ((common_types::SIncomingCommandServices *)m_services)->analyticManager->getUserDispatcher();

    const common_types::TUserId newId = userDipatcher->registerUser( m_userIpStr, m_userPid );
    if( newId != DispatcherUser::INVALID_USER_ID ){
        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "register";
        rootRecord[ "user_id" ] = newId;
        rootRecord[ "error_occured" ] = false;
        rootRecord[ "code" ] = "OK";

        Json::FastWriter jsonWriter;
        sendResponse( jsonWriter.write(rootRecord) );
        return true;
    }
    else{
        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "register";
        rootRecord[ "user_id" ] = DispatcherUser::INVALID_USER_ID;
        rootRecord[ "error_occured" ] = true;
        rootRecord[ "code" ] = userDipatcher->getState().lastError;

        Json::FastWriter jsonWriter;
        sendResponse( jsonWriter.write(rootRecord) );
        return false;
    }
}

