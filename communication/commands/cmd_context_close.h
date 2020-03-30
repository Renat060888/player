#ifndef CMD_CONTEXT_CLOSE_H
#define CMD_CONTEXT_CLOSE_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandContextClose : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandContextClose( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;


};
using PCommandContextClose = std::shared_ptr<CommandContextClose>;

#endif // CMD_CONTEXT_CLOSE_H
