
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

        const string resp = "{ \"cmd_name\" : \"pong\", \"player\" : \"hello client!\", \"error_occured\" : \"false\", \"code\" : \"NO_CODE\" }";
        sendResponse( resp  );

        return true;
    }
    else{
        const string resp = "{ \"cmd_name\" : \"pong\", \"player\" : \"you have a problem man!\", \"error_occured\" : \"true\", \"code\" : \"NOT_REGISTERED\" }";
        sendResponse( resp  );

        return true;
    }
}

