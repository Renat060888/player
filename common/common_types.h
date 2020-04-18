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
class SystemEnvironmentFacade;
class CommunicationGatewayFacadePlayer;

namespace common_types{

// ---------------------------------------------------------------------------
// global typedefs
// ---------------------------------------------------------------------------
using TPlayerId = std::string;


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







struct SPlayerState {
    TPlayerId playerId;
    EPlayerStatus status;
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

class IPlayerService {
public:
    struct SPlayingDataSet {
        int uniqueId;
        std::string description;
        bool real;
        std::vector<TTimeRangeMillisec> dataRanges;
    };

    struct SPlayingInfo {
        TTimeRangeMillisec globalRangeMillisec;
        int64_t currentStepMillisec;
        int64_t stepIntervalMillisec;
        std::vector<SPlayingDataSet> playingData;
    };

    struct SServiceState {
        EPlayerStatus status;
        std::string lastError;
        TPlayerId playerId;
        SPlayingInfo data;
    };

    virtual ~IPlayerService(){}

    virtual const SServiceState & getServiceState() = 0;

    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual bool stepForward() = 0;
    virtual bool stepBackward() = 0;

    virtual bool setRange( const common_types::TTimeRangeMillisec & _range ) = 0;
    virtual void switchReverseMode( bool _reverse ) = 0;
    virtual void switchLoopMode( bool _loop ) = 0;
    virtual bool playFromPosition( int64_t _stepMillisec ) = 0;
    virtual bool updatePlayingData() = 0;

    virtual bool increasePlayingSpeed() = 0;
    virtual bool decreasePlayingSpeed() = 0;
    virtual void normalizePlayingSpeed() = 0;
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

    SystemEnvironmentFacade * systemEnvironment;
    AnalyticManagerFacade * analyticManager;
    StorageEngineFacade * storageEngine;
    CommunicationGatewayFacadePlayer * communicationGateway;
    std::function<void()> signalShutdownServer;
};


}

#endif // COMMON_TYPES_H

