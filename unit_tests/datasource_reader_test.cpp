
#include "analyze/datasource_reader.h"
#include "datasource_reader_test.h"

using namespace std;

DatabaseManagerBase * DatasourceReaderTest::m_database = nullptr;

DatasourceReaderTest::DatasourceReaderTest()
{

}

void DatasourceReaderTest::SetUpTestCase(){

    m_database = DatabaseManagerBase::getInstance();

    DatabaseManagerBase::SInitSettings settings;
    settings.host = "localhost";
    settings.databaseName = "unit_tests";

    // TODO: check with assert also
    m_database->init(settings);
}

void DatasourceReaderTest::TearDownTestCase(){

    DatabaseManagerBase::destroyInstance( m_database );
}

TEST_F(DatasourceReaderTest, DISABLED_read_steps_test){

    // clear all

    // write metadata & payload ( via database manager )

    // read payload ( via 'datasource reader', but first of all udpate 'datasource descriptor' ) TEST




    const vector<common_types::SPersistenceMetadata> ctxMetadatas = m_database->getPersistenceSetMetadata( _settings.ctxId );
    const common_types::SPersistenceMetadata & ctxMetadata = ctxMetadatas[ 0 ]; // TODO: segfault ?

    for( const SPersistenceMetadataRaw & processedSensor : ctxMetadata.persistenceFromRaw ){

        DatasourceReader * datasource = new DatasourceReader();

        DatasourceReader::SInitSettings settings;
        settings.persistenceSetId = processedSensor.persistenceSetId;
        settings.updateStepMillisec = processedSensor.timeStepIntervalMillisec;
        settings.ctxId = _settings.ctxId;
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

        _datasrc.push_back( datasource );
    }
}
