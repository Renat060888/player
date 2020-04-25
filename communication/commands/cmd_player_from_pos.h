#ifndef CMD_PLAYER_FROM_POS_H
#define CMD_PLAYER_FROM_POS_H

#include <microservice_common/communication/i_command_external.h>

#include "common/common_types.h"

class CommandPlayerFromPos : public ICommandExternal
{
    friend class CommandFactory;
public:
    CommandPlayerFromPos( common_types::SIncomingCommandServices * _services );

    virtual bool exec() override;


private:
    common_types::TUserId m_userId;
    int64_t m_positionMillisec;

};
using PCommandPlayerFromPos = std::shared_ptr<CommandPlayerFromPos>;

#endif // CMD_PLAYER_FROM_POS_H
