
#include <microservice_common/system/logger.h>

#include "common/common_utils.h"
#include "dispatcher_user.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "UserDispatcher:";
static constexpr int64_t USER_TIMEOUT_MILLISEC = 30000;

const common_types::TUserId DispatcherUser::INVALID_USER_ID = "invalid_user_id";

DispatcherUser::DispatcherUser()
{

}

DispatcherUser::~DispatcherUser()
{

}

bool DispatcherUser::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

    loadPreviousSessionActiveUsers();



    return true;
}

void DispatcherUser::runClock(){

    for( auto iter = m_users.begin(); iter != m_users.end(); ){
        PUserState state = ( * iter );

        if( (common_utils::getCurrentTimeMillisec() - state->lastPingMillisec) > USER_TIMEOUT_MILLISEC ){
            VS_LOG_WARN << PRINT_HEADER << " user disappeared [" << state->userId << "]" << endl;

            iter = m_users.erase( iter );
            m_usersById.erase( state->userId );

            // NOTE: remove record from WAL - ONLY if user is missing
            m_state.settings.serviceWriteAheadLogger->closeUserRegistration( state->userId );

            for( IUserDispatcherObserver * observer : m_observers ){
                observer->callbackUserOnline( state->userId, false );
            }
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
        PUserState & state = iter->second;
        state->lastPingMillisec = common_utils::getCurrentTimeMillisec();
        return true;
    }
    else{
        return false;
    }
}

PUserState DispatcherUser::getUser( const common_types::TUserId & _userId ){

    auto iter = m_usersById.find( _userId );
    if( iter != m_usersById.end() ){
        return iter->second;
    }
    else{
        m_state.lastError = "such user not found: " + _userId;
        return nullptr;
    }
}

common_types::TUserId DispatcherUser::registerUser( std::string _userIp, common_types::TPid _userPid ){

    // check for valid ip & pid
    // TODO: do

    // check for user with same ip/pid
    for( const PUserState & state : m_users ){
        if( state->userIp == _userIp && state->userPid == _userPid ){
            VS_LOG_INFO << PRINT_HEADER << " register already existing user [" << state->userId << "]" << endl;
            return state->userId;
        }
    }

    //
    PUserState state = std::make_shared<SUserState>();
    state->userId = common_utils::generateUniqueId();
    state->userIp = _userIp;
    state->userPid = _userPid;
    state->lastPingMillisec = common_utils::getCurrentTimeMillisec();

    m_users.push_back( state );
    m_usersById.insert( {state->userId, state} );

    // write to WAL
    SWALUserRegistration userReg;
    userReg.registerId = state->userId;
    userReg.userIp = state->userIp;
    userReg.userPid = state->userPid;
    userReg.registeredAtDateTime = common_utils::getCurrentDateTimeStr();
    const bool rt = m_state.settings.serviceWriteAheadLogger->openUserRegistration( userReg );

    // notify
    for( IUserDispatcherObserver * observer : m_observers ){
        observer->callbackUserOnline( state->userId, true );
    }

    VS_LOG_INFO << PRINT_HEADER << " register new user [" << state->userId << "]" << endl;
    return state->userId;
}

void DispatcherUser::updateUserState( const common_types::SUserState & _state ){

    auto iter = m_usersById.find( _state.userId );
    if( iter != m_usersById.end() ){
        PUserState user = iter->second;

        user->lastPingMillisec = common_utils::getCurrentTimeMillisec();
    }
    else{
        VS_LOG_ERROR << PRINT_HEADER << " unknown user id: " << _state.userId << endl;
        assert( false && "user id must be known to the system" );
    }
}

void DispatcherUser::loadPreviousSessionActiveUsers(){

    const vector<SWALUserRegistration> pastUsers = m_state.settings.serviceWriteAheadLogger->getUserRegistrations();
    for( const SWALUserRegistration & userReg : pastUsers ){

        PUserState state = std::make_shared<SUserState>();
        state->userId = userReg.registerId;
        state->userIp = userReg.userIp;
        state->userPid = userReg.userPid;
        state->lastPingMillisec = common_utils::getCurrentTimeMillisec();

        m_users.push_back( state );
        m_usersById.insert( {state->userId, state} );
    }
}

















