#ifndef CMD_PLAYER_LOOP_H
#define CMD_PLAYER_LOOP_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerLoop : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandPlayerLoop( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;
    bool loop;

};
using PCommandPlayerLoop = std::shared_ptr<CommandPlayerLoop>;

#endif // CMD_PLAYER_LOOP_H
