
#include <microservice_common/system/logger.h>

#include "common/common_utils.h"
#include "dispatcher_user.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "UserDispatcher:";
const common_types::TUserId DispatcherUser::INVALID_USER_ID = "invalid_user_id";

DispatcherUser::DispatcherUser()
{

}

void DispatcherUser::runClock(){

    for( auto iter = m_users.begin(); iter != m_users.end(); ){
        SUserState & state = ( * iter );
        if( (common_utils::getCurrentTimeMillisec() - state.lastPingMillisec) > 60000 ){

            VS_LOG_WARN << PRINT_HEADER << " user disappeared [" << state.userId << "]" << endl;
            iter = m_users.erase( iter );
            m_usersById.erase( state.userId );
        }
        else{
            ++iter;
        }
    }


}

void DispatcherUser::addObserver( common_types::IUserDispatcherObserver * _observer ){

    for( const IUserDispatcherObserver * const observer : m_observers ){
        if( observer == _observer ){
            return;
        }
    }

    m_observers.push_back( _observer );
}

void DispatcherUser::removeObserver( common_types::IUserDispatcherObserver * _observer ){

    for( auto iter = m_observers.begin(); iter != m_observers.end(); ){
        if( (* iter) == _observer ){
            iter = m_observers.erase( iter );
            return;
        }
        else{
            ++iter;
        }
    }
}

bool DispatcherUser::isRegistered( const common_types::TUserId & _userId ){

    auto iter = m_usersById.find( _userId );
    if( iter != m_usersById.end() ){
        SUserState & state = iter->second;
        state.lastPingMillisec = common_utils::getCurrentTimeMillisec();
        return true;
    }
    else{
        return false;
    }
}

const common_types::SUserState & DispatcherUser::getUser( const common_types::TUserId & _userId ){


}

common_types::TUserId DispatcherUser::registerUser( std::string _userIp, common_types::TPid _userPid ){

    // check for valid ip & pid

    // check for user with same ip/pid
    for( const SUserState & state : m_users ){
        if( state.userIp == _userIp && state.userPid == _userPid ){
            VS_LOG_INFO << PRINT_HEADER << " register already existing user [" << state.userId << "]" << endl;
            return state.userId;
        }
    }

    //
    SUserState state;
    state.userId = common_utils::generateUniqueId();
    state.userIp = _userIp;
    state.userPid = _userPid;
    state.lastPingMillisec = common_utils::getCurrentTimeMillisec();

    m_users.push_back( state );
    m_usersById.insert( {state.userId, state} );

    VS_LOG_INFO << PRINT_HEADER << " register new user [" << state.userId << "]" << endl;
    return state.userId;
}

void DispatcherUser::updateUserState( const common_types::SUserState & _state ){

    // TODO: watch for user is online
}



















