#ifndef CMD_PLAYER_REALTIME_H
#define CMD_PLAYER_REALTIME_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerLive : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandPlayerLive( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;
    bool live;

};
using PCommandPlayerLive = std::shared_ptr<CommandPlayerLive>;

#endif // CMD_PLAYER_REALTIME_H
