#ifndef CMD_PLAYER_PAUSE_H
#define CMD_PLAYER_PAUSE_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerPause : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandPlayerPause( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;

};
using PCommandPlayerPause = std::shared_ptr<CommandPlayerPause>;

#endif // CMD_PLAYER_PAUSE_H
