#ifndef CMD_PLAYER_REVERSE_H
#define CMD_PLAYER_REVERSE_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerReverse : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandPlayerReverse( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;
    bool reverse;

};
using PCommandPlayerReverse = std::shared_ptr<CommandPlayerReverse>;

#endif // CMD_PLAYER_REVERSE_H
