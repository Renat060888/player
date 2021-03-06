#ifndef CMD_PLAYER_STEP_H
#define CMD_PLAYER_STEP_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerStep : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandPlayerStep( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;
    bool stepForward;

};
using PCommandPlayerStep = std::shared_ptr<CommandPlayerStep>;

#endif // CMD_PLAYER_STEP_H
