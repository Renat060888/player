
#include <jsoncpp/json/writer.h>

#include "analyze/analytic_manager_facade.h"
#include "cmd_player_loop.h"

using namespace std;
using namespace common_types;

static bool userHasPermission( const TUserId & _userId, common_types::SIncomingCommandServices * _services, std::string & _errMsg ){

    DispatcherUser * userDispatcher = _services->analyticManager->getUserDispatcher();

    if( ! userDispatcher->isRegistered(_userId) ){
        Json::Value rootRecord;
        rootRecord[ "cmd_name" ] = "pong";
        rootRecord[ "error_occured" ] = true;
        rootRecord[ "code" ] = "NOT_REGISTERED";

        Json::FastWriter jsonWriter;
        _errMsg = jsonWriter.write( rootRecord );
        return false;
    }
    else{
        return true;
    }
}

CommandPlayerLoop::CommandPlayerLoop( SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandPlayerLoop::exec(){

    string errMsg;
    if( userHasPermission(m_userId, (SIncomingCommandServices *)m_services, errMsg) ){
        DispatcherPlayer * playerDipatcher = ((SIncomingCommandServices *)m_services)->analyticManager->getPlayerDispatcher();
        IPlayerService * player = playerDipatcher->getPlayerByUser( m_userId );
        if( player ){
            player->switchLoopMode( loop );

            Json::Value rootRecord;
            rootRecord[ "cmd_name" ] = "player_loop";
            rootRecord[ "error_occured" ] = false;
            rootRecord[ "code" ] = "OK";

            Json::FastWriter jsonWriter;
            sendResponse( jsonWriter.write(rootRecord) );
            return true;
        }
        else{
            Json::Value rootRecord;
            rootRecord[ "cmd_name" ] = "player_loop";
            rootRecord[ "error_occured" ] = true;
            rootRecord[ "code" ] = playerDipatcher->getState().lastError;

            Json::FastWriter jsonWriter;
            sendResponse( jsonWriter.write(rootRecord) );
            return false;
        }
    }
    else{
        sendResponse( errMsg );
        return false;
    }
}
