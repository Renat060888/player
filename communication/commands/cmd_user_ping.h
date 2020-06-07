#ifndef CMD_USER_PING_H
#define CMD_USER_PING_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandUserPing : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandUserPing( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::SUserState m_userState;



};
using PCommandUserPing = std::shared_ptr<CommandUserPing>;

#endif // CMD_USER_PING_H
