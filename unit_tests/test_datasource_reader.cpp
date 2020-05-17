
#include "analyze/datasource_reader.h"
#include "analyze/datasource_descriptor.h"
#include "test_datasource_reader.h"

using namespace std;
using namespace common_types;

static const TContextId CONTEXT_ID = 777;
static const TMissionId MISSION_ID = 555;
static const int64_t QUANTUM_INTERVAL_MILLISEC = 1000;

DatabaseManagerBase * TestDatasourceReader::m_database = nullptr;

TestDatasourceReader::TestDatasourceReader()
{

}

void TestDatasourceReader::SetUpTestCase(){

    m_database = DatabaseManagerBase::getInstance();

    DatabaseManagerBase::SInitSettings settings;
    settings.host = "localhost";
    settings.databaseName = "unit_tests";

    // TODO: check with assert also
    m_database->init(settings);
}

void TestDatasourceReader::TearDownTestCase(){

    DatabaseManagerBase::destroyInstance( m_database );
}

TEST_F(TestDatasourceReader, DISABLED_read_steps_test){

    // clear all
    {
        m_database->deleteTotalData( CONTEXT_ID );
        m_database->deleteSessionDescription( CONTEXT_ID );
        m_database->deletePersistenceSetMetadata( CONTEXT_ID );
    }

    // write metadata & payload ( via database manager )

    // read payload ( via 'datasource reader', but first of all udpate 'datasource descriptor' ) TEST
    {
        const vector<common_types::SPersistenceMetadata> ctxMetadatas = m_database->getPersistenceSetMetadata( CONTEXT_ID );
        const SPersistenceMetadataRaw & rawMetadataOutput = ctxMetadatas[ 0 ].persistenceFromRaw.front();

        // descriptor
        DatasourceDescriptor * datasourceDescriptor = new DatasourceDescriptor();

        DatasourceDescriptor::SInitSettings settingsDescriptor;
        settingsDescriptor.persId = rawMetadataOutput.persistenceSetId;
        settingsDescriptor.databaseMgr = m_database;
        settingsDescriptor.sessionGapsThreshold = 0; // TODO: may be from cfg ?
        ASSERT_TRUE( datasourceDescriptor->init(settingsDescriptor) );

        // reader
        DatasourceReader * datasourceReader = new DatasourceReader();

        DatasourceReader::SInitSettings settings;
        settings.persistenceSetId = rawMetadataOutput.persistenceSetId;
        settings.updateStepMillisec = rawMetadataOutput.timeStepIntervalMillisec;
        settings.databaseMgr = m_database;
        settings.descriptor = datasourceDescriptor;
        ASSERT_TRUE( datasourceReader->init(settings) );

        // check read results




    }
}

TEST_F(TestDatasourceReader, DISABLED_read_single_random_steps_test){





}
















