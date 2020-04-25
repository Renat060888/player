#ifndef CMD_PLAYER_SPEED_H
#define CMD_PLAYER_SPEED_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerSpeed : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandPlayerSpeed( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;
    bool increase;
    bool normalize;

};
using PCommandPlayerSpeed = std::shared_ptr<CommandPlayerSpeed>;

#endif // CMD_PLAYER_SPEED_H
