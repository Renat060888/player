
#include <microservice_common/common/ms_common_utils.h>
#include <microservice_common/system/logger.h>
#include <microservice_common/system/process_launcher.h>
#include <microservice_common/system/wal.h>
#include <microservice_common/system/system_monitor.h>

#include "system/path_locator.h"
#include "system/config_reader.h"
#include "player_agent.h"

using namespace std;

boost::signals2::signal<void()> PlayerAgent::m_unixInterruptSignal;
static constexpr const char * PRINT_HEADER = "PlayerAgent:";

PlayerAgent::PlayerAgent()
    : m_systemEnvironment(nullptr)
    , m_communicateGateway(nullptr)
    , m_storageEngine(nullptr)
    , m_analyticManager(nullptr)
    , m_shutdownCalled(false)
{
    // system facility
    m_systemEnvironment = new SystemEnvironment();

    // I data generation ( NOTE: Player does not generate data )

    // II data store
    m_storageEngine = new StorageEngineFacade();

    // III data analyze
    m_analyticManager = new AnalyticManagerFacade();

    // IV communication between actors
    m_commandServices.analyticManager = m_analyticManager;
    m_commandServices.storageEngine = m_storageEngine;
    m_commandServices.systemEnvironment = m_systemEnvironment;
    m_commandServices.signalShutdownServer = std::bind( & PlayerAgent::shutdown, this );

    m_communicateGateway = new CommunicationGatewayFacadePlayer();
}

PlayerAgent::~PlayerAgent()
{
    shutdown();
}

bool PlayerAgent::init( const SInitSettings & _settings ){

    VS_LOG_INFO << PRINT_HEADER << " ============================ START INIT ========================" << endl;

    PATH_LOCATOR.removePreviousSessionSocketFiles();

    const SystemMonitor::STotalInfo info = SYSTEM_MONITOR.getTotalSnapshot();
    SYSTEM_MONITOR.printOnScreen( info );

    SystemEnvironment::SInitSettings settings0;
    settings0.services;
    settings0.databaseHost = CONFIG_PARAMS.MONGO_DB_ADDRESS;
    settings0.databaseName = CONFIG_PARAMS.MONGO_DB_NAME;
    settings0.restoreSystemAfterInterrupt = CONFIG_PARAMS.SYSTEM_RESTORE_INTERRUPTED_SESSION;
    settings0.uniqueLockFileFullPath = PATH_LOCATOR.getUniqueLockFile();
    if( ! m_systemEnvironment->init(settings0) ){
        return false;
    }

    CommunicationGatewayFacadePlayer::SInitSettings settings1;
    settings1.requestsFromConfig = CONFIG_PARAMS.INITIAL_REQUESTS;
    settings1.requestsFromWAL = m_systemEnvironment->serviceForWriteAheadLogging()->getInterruptedOperations();
    settings1.asyncNetwork = true;
    settings1.services = m_commandServices;
    if( ! m_communicateGateway->init(settings1) ){
        return false;
    }

    StorageEngineFacade::SInitSettings settings3;
    settings3.services;
    if( ! m_storageEngine->init(settings3) ){
        return false;
    }

    AnalyticManagerFacade::SInitSettings settings4;
    settings4.services;
    if( ! m_analyticManager->init(settings4) ){
        return false;
    }

    // CTRL+C handler
    m_unixInterruptSignal.connect( boost::bind( & PlayerAgent::shutdownByUnixInterruptSignal, this) );
    common_utils::connectInterruptSignalHandler( ( common_utils::TSignalHandlerFunction ) & PlayerAgent::callbackUnixInterruptSignal );

    checkForSelfShutdown();

    // NOTE: если инициализация прошла успешно, то история прерванных операций окончательно очищается,
    // а их возобновление начнется с выборкой команд в DistributedSimulationSystem::launch()
    m_systemEnvironment->serviceForWriteAheadLogging()->cleanJournal();

    VS_LOG_INFO << PRINT_HEADER << " ============================ INIT SUCCESS ============================" << endl;
    return true;
}

void PlayerAgent::launch(){

    while( ! m_shutdownCalled.load() ){

        if( ! m_communicateGateway->getInitSettings().asyncNetwork ){
            m_communicateGateway->runNetworkCallbacks();
        }

        if( m_communicateGateway->isCommandAvailable() ){
            PCommand command = m_communicateGateway->nextIncomingCommand();
            command->exec();
        }

        // TODO: may be throught about CV ?
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
}

void PlayerAgent::checkForSelfShutdown(){

    if( CONFIG_PARAMS.SYSTEM_SELF_SHUTDOWN_SEC != 0 ){
        m_selfShutdownFuture = std::async( std::launch::async, [this](){
            VS_LOG_WARN << PRINT_HEADER
                        << " ------------------------------ (!) SELF-SHUTDOWN AFTER"
                        << " [" << CONFIG_PARAMS.SYSTEM_SELF_SHUTDOWN_SEC << "] sec"
                        << " (!) ------------------------------"
                     << endl;

            std::this_thread::sleep_for( std::chrono::seconds(CONFIG_PARAMS.SYSTEM_SELF_SHUTDOWN_SEC) );

            VS_LOG_WARN << PRINT_HEADER
                        << " ------------------------------ (!) SELF-SHUTDOWN INITIATE (!) ------------------------------"
                        << endl;

            m_shutdownCalled.store( true );
        });
    }
}

void PlayerAgent::shutdown(){

    // NOTE: order of system shutdown following:
    // - first fo all - interrupt communication with outside world;
    // - then destroy source consumers;
    // - and finally close connections to sources;
    m_communicateGateway->shutdown();
    m_analyticManager->shutdown();
    m_storageEngine->shutdown();

    delete m_communicateGateway;
    m_communicateGateway = nullptr;
    delete m_storageEngine;
    m_storageEngine = nullptr;
    delete m_analyticManager;
    m_analyticManager = nullptr;
    delete m_systemEnvironment;
    m_systemEnvironment = nullptr;

    PROCESS_LAUNCHER.shutdown();

    VS_LOG_INFO << PRINT_HEADER << " ============================ SHUTDOWN SUCCESS ========================" << endl;
}

void PlayerAgent::shutdownByUnixInterruptSignal(){

    m_shutdownCalled.store( true );
}

void PlayerAgent::callbackUnixInterruptSignal(){

    VS_LOG_INFO << PRINT_HEADER << " ============================ catched SIGINT, initiate shutdown ============================" << endl;
    m_unixInterruptSignal();
}







