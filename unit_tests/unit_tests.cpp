
#include <gtest/gtest.h>

#include <microservice_common/system/logger.h>

#include "system/config_reader.h"
#include "analyze/player_worker.h"
#include "unit_tests.h"

using namespace std;

UnitTests::UnitTests( int _argc, char ** _argv )
{
    ::testing::InitGoogleTest( & _argc, _argv );
}

bool UnitTests::run(){

    return RUN_ALL_TESTS();

    static constexpr const char * PRINT_HEADER = "UnitTest:";

    // db
    DatabaseManagerBase * database = DatabaseManagerBase::getInstance();

    DatabaseManagerBase::SInitSettings settings;
    settings.host = CONFIG_PARAMS.baseParams.MONGO_DB_ADDRESS;
    settings.databaseName = CONFIG_PARAMS.baseParams.MONGO_DB_NAME;

    if( ! database->init(settings) ){
        ::exit( EXIT_FAILURE );
    }

    // pd
    const vector<common_types::SPersistenceMetadata> ctxMetadatas = database->getPersistenceSetMetadata( (common_types::TContextId)777 );
    const common_types::SPersistenceMetadata & ctxMetadata = ctxMetadatas[ 0 ];

    for( const common_types::SPersistenceMetadataRaw & processedSensor : ctxMetadata.persistenceFromRaw ){
        DatasourceReader * datasource = new DatasourceReader();

        DatasourceReader::SInitSettings settings;
        settings.persistenceSetId = processedSensor.persistenceSetId;
        settings.updateStepMillisec = processedSensor.timeStepIntervalMillisec;
        settings.ctxId = 777;
        if( ! datasource->init(settings) ){
            VS_LOG_ERROR << PRINT_HEADER << " failed to create datasource for mission: " << processedSensor.missionId << endl;
            continue;
        }

        const DatasourceReader::SState & state = datasource->getState();

        VS_LOG_TRACE << PRINT_HEADER
                     << " raw datasrc on mission " << processedSensor.missionId
                     << " time begin ms " << state.globalTimeRangeMillisec.first
                     << " time end ms " << state.globalTimeRangeMillisec.second
                     << " steps " << state.stepsCount
                     << endl;
    }

    ::exit( EXIT_SUCCESS );
}
