#ifndef CMD_CONTEXT_OPEN_H
#define CMD_CONTEXT_OPEN_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandContextOpen : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandContextOpen( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;
    std::string m_contextName;

    std::string m_requestFullText;


};
using PCommandContextOpen = std::shared_ptr<CommandContextOpen>;

#endif // CMD_CONTEXT_OPEN_H
