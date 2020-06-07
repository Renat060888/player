
#include "analyze/analytic_manager_facade.h"
#include "cmd_player_controller_ping.h"

using namespace std;
using namespace common_types;

CommandPlayerControllerPing::CommandPlayerControllerPing( common_types::SIncomingCommandServices * _services )
    : ICommandExternal(_services)
{

}

bool CommandPlayerControllerPing::exec(){

    DispatcherPlayer * dpc = ((SIncomingCommandServices *)m_services)->analyticManager->getPlayerDispatcher();
    dpc->updatePlayerState( playerState );
    return true;
}
