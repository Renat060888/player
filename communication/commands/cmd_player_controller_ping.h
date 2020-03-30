#ifndef CMD_NODE_PING_H
#define CMD_NODE_PING_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerControllerPing : public ICommandExternal
{
public:
    CommandPlayerControllerPing( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:



};
using PCommandPlayerControllerPing = std::shared_ptr<CommandPlayerControllerPing>;

#endif // CMD_NODE_PING_H
