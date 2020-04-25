#ifndef CMD_PLAYER_START_H
#define CMD_PLAYER_START_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerStart : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandPlayerStart( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;

};
using PCommandPlayerStart = std::shared_ptr<CommandPlayerStart>;

#endif // CMD_PLAYER_START_H
