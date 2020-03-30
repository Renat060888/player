
#include <jsoncpp/json/writer.h>

#include "analyze/analytic_manager_facade.h"
#include "cmd_user_ping.h"

using namespace std;

CommandUserPing::CommandUserPing( common_types::SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandUserPing::exec(){

    DispatcherUser * userDipatcher = (( common_types::SIncomingCommandServices * )m_services)->analyticManager->getUserDispatcher();

    if( userDipatcher->isRegistered(m_userId) ){

//        common_types::SUserState state;
//        state.userId = m_userId;

//        userDipatcher->updateUserState( state );

        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "player" ] = "hello client!";
        rootRecord[ "error_occured" ] = false;
        rootRecord[ "code" ] = "NO_CODE";

        Json::FastWriter jsonWriter;
        sendResponse( jsonWriter.write(rootRecord) );
        return true;
    }
    else{
        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "player" ] = "you have a problem man!";
        rootRecord[ "error_occured" ] = true;
        rootRecord[ "code" ] = "NOT_REGISTERED";

        Json::FastWriter jsonWriter;
        sendResponse( jsonWriter.write(rootRecord) );
        return true;
    }
}

