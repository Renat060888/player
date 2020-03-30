#ifndef CMD_USER_REGISTER_H
#define CMD_USER_REGISTER_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandUserRegister : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandUserRegister( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    std::string m_userIpStr;
    common_types::TPid m_userPid;



};
using PCommandUserRegister = std::shared_ptr<CommandUserRegister>;

#endif // CMD_USER_REGISTER_H
