
#include <gtest/gtest.h>

#include <microservice_common/system/logger.h>

#include "system/config_reader.h"
#include "analyze/player_worker.h"
#include "unit_tests.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "UnitTest:";
static const TContextId CONTEXT_ID = 777;
static const TMissionId MISSION_ID = 555;
static const int64_t QUANTUM_INTERVAL_MILLISEC = 1000;

UnitTests::UnitTests( int _argc, char ** _argv )
{
    ::testing::InitGoogleTest( & _argc, _argv );
}

bool UnitTests::run(){

//    return ::RUN_ALL_TESTS();

    PlayerWorker::SInitSettings settingsPlayer;
    settingsPlayer.ctxId = 372;

    PlayerWorker player;
    if( ! player.init(settingsPlayer) ){
        ::exit( EXIT_FAILURE );
    }

    player.start();

    while( true ){
        // dummy
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }

    ::exit( EXIT_SUCCESS );
}

void UnitTests::insertTestData(){

    // db
    DatabaseManagerBase::SInitSettings settings;
    settings.host = "localhost";
    settings.databaseName = "unit_tests";

    DatabaseManagerBase * databaseMgr = DatabaseManagerBase::getInstance();
    const bool success = databaseMgr->init(settings);
    assert( success && "db must be inited properly" );

    // clear everything
    databaseMgr->deleteTotalData( CONTEXT_ID );
    databaseMgr->deleteSessionDescription( CONTEXT_ID );
    databaseMgr->deletePersistenceSetMetadata( CONTEXT_ID );

    // metadata
    SPersistenceMetadataRaw rawMetadataInput;
    rawMetadataInput.contextId = CONTEXT_ID;
    rawMetadataInput.missionId = MISSION_ID;
    rawMetadataInput.lastRecordedSession = 1;
    rawMetadataInput.sourceType = common_types::EPersistenceSourceType::AUTONOMOUS_RECORDER;
    rawMetadataInput.timeStepIntervalMillisec = QUANTUM_INTERVAL_MILLISEC;

    const TPersistenceSetId persId = databaseMgr->writePersistenceSetMetadata( rawMetadataInput );
    rawMetadataInput.persistenceSetId = persId;
    assert( persId != -1 && "pers id must be correct" );

    // payload

}














