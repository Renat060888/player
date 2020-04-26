#ifndef DISPATCHER_USER_H
#define DISPATCHER_USER_H

#include <map>

#include <microservice_common/system/wal.h>

#include "common/common_types.h"

class DispatcherUser : public common_types::IServiceUserAuthorization
{
    friend class CommandUserPing;
public:
    static const common_types::TUserId INVALID_USER_ID;

    struct SInitSettings {
        WriteAheadLogger * serviceWriteAheadLogger;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    DispatcherUser();
    ~DispatcherUser();

    bool init( const SInitSettings & _settings );
    const SState & getState(){ return m_state; }
    void runClock();

    virtual void addObserver( common_types::IUserDispatcherObserver * _observer ) override;
    virtual void removeObserver( common_types::IUserDispatcherObserver * _observer ) override;

    virtual bool isRegistered( const common_types::TUserId & _userId ) override;
    common_types::TUserId registerUser( std::string _userIp, common_types::TPid _userPid );

    common_types::PUserState getUser( const common_types::TUserId & _userId );


private:
    void updateUserState( const common_types::SUserState & _state );

    void loadPreviousSessionActiveUsers();

    // data
    SState m_state;
    std::vector<common_types::PUserState> m_users;
    std::map<common_types::TUserId, common_types::PUserState> m_usersById;
    std::map<common_types::TContextId, common_types::PUserState> m_usersByContextId;
    std::vector<common_types::IUserDispatcherObserver *> m_observers;

    // service



};

#endif // DISPATCHER_USER_H
