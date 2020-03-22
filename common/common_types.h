#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <string>
#include <functional>

#include <microservice_common/common/ms_common_types.h>

// ---------------------------------------------------------------------------
// forwards
// ---------------------------------------------------------------------------
class SourceManagerFacade;
class AnalyticManagerFacade;
class StorageEngineFacade;
class SystemEnvironment;
class CommunicationGatewayFacadePlayer;

namespace common_types{

// ---------------------------------------------------------------------------
// global typedefs
// ---------------------------------------------------------------------------
using TPlayerId = std::string;
using TUserId = std::string;
using TContextId = uint32_t;
using TSession = int32_t;
using TLogicStep = int64_t;


// ---------------------------------------------------------------------------
// enums
// ---------------------------------------------------------------------------
enum class EPlayerStatus {
    INITED,
    PREPARING,
    READY,

    PLAYING,
    PAUSED,
    STOPPED,
    PLAY_FROM_POSITION,
    ALL_STEPS_PASSED,
    NOT_ENOUGH_STEPS,
    CLOSE,

    ACTIVE,
    IDLE,

    CRASHED,
    UNDEFINED
};




// ---------------------------------------------------------------------------
// simple ADT
// ---------------------------------------------------------------------------
struct SObjectStep {
    TLogicStep logicStep;
    int64_t timestampMillisec;
};

struct FunctorObjectStep {
    FunctorObjectStep( TLogicStep _stepToFind )
        : stepToFind(_stepToFind)
    {}

    bool operator()( const SObjectStep & _rhs ){
        return ( stepToFind == _rhs.logicStep );
    }

    TLogicStep stepToFind;
};

struct SEventsSessionInfo {
    SEventsSessionInfo(){
        clear();
    }

    void clear(){
        number = 0;
        minLogicStep = 0;
        maxLogicStep = 0;
        minTimestampMillisec = 0;
        maxTimestampMillisec = 0;
        steps.clear();
    }

    bool empty(){
        return (
        number == 0 &&
        minLogicStep == 0 &&
        maxLogicStep == 0 &&
        minTimestampMillisec == 0 &&
        maxTimestampMillisec == 0 &&
        steps.empty() );
    }

    TSession number;
    TLogicStep minLogicStep;
    TLogicStep maxLogicStep;
    int64_t minTimestampMillisec;
    int64_t maxTimestampMillisec;
    std::vector<SObjectStep> steps;
};

struct SUserState {
    TUserId userId;
    std::string userIp;
    TPid userPid;
    int64_t lastPingMillisec;
};



struct SPlayerState {
    TPlayerId playerId;
    EPlayerStatus status;
};

// TODO: temp
struct SAnalyticEvent {

    TSensorId sensorId;
    TContextId ctxId;

    std::string eventMessage;
    std::string pluginName;
};





// ---------------------------------------------------------------------------
// exchange ADT ( component <-> store, component <-> network, etc... )
// ---------------------------------------------------------------------------





// ---------------------------------------------------------------------------
// types deduction
// ---------------------------------------------------------------------------





// ---------------------------------------------------------------------------
// service interfaces
// ---------------------------------------------------------------------------
class IContextService {
public:
    virtual ~IContextService(){}

    virtual bool open( int _contextId ) = 0;
    virtual bool close( int _contextId ) = 0;
};

class IServiceInternalCommunication {
public:
    virtual ~IServiceInternalCommunication(){}

    virtual PNetworkClient getPlayerWorkerCommunicator( const std::string & _uniqueId ) = 0;
};

class IServiceExternalCommunication {
public:
    virtual ~IServiceExternalCommunication(){}

    virtual PNetworkClient getUserCommunicator( const std::string & _uniqueId ) = 0;
};

class IUserDispatcherObserver {
public:
    virtual ~IUserDispatcherObserver(){}

    virtual void callbackUserOnline( const common_types::TUserId & _id, bool _online ) = 0;
};

class IServiceUserAuthorization {
public:
    virtual ~IServiceUserAuthorization(){}

    virtual bool isRegistered( const TUserId & _id ) = 0;
    virtual void addObserver( IUserDispatcherObserver * _observer ) = 0;
    virtual void removeObserver( IUserDispatcherObserver * _observer ) = 0;
};




// ---------------------------------------------------------------------------
// service locator
// ---------------------------------------------------------------------------
struct SIncomingCommandServices : SIncomingCommandGlobalServices {
    SIncomingCommandServices()
        : systemEnvironment(nullptr)
        , analyticManager(nullptr)
        , storageEngine(nullptr)
        , communicationGateway(nullptr)
    {}

    SystemEnvironment * systemEnvironment;
    AnalyticManagerFacade * analyticManager;
    StorageEngineFacade * storageEngine;
    CommunicationGatewayFacadePlayer * communicationGateway;
    std::function<void()> signalShutdownServer;
};


}

#endif // COMMON_TYPES_H

