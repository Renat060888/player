
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

TEST_F(TestDatasourceReader, read_steps_test){

    // clear all
    {
        m_database->deleteTotalData( CONTEXT_ID );
        m_database->deleteSessionDescription( CONTEXT_ID );
        m_database->deletePersistenceSetMetadata( CONTEXT_ID );
    }

    // I write metadata common_types::SPersistenceMetadataRaw rawMetadata;
    TPersistenceSetId persId = common_vars::INVALID_PERS_ID;
    {
        SPersistenceMetadataRaw rawMetadataInput;
        rawMetadataInput.contextId = CONTEXT_ID;
        rawMetadataInput.missionId = MISSION_ID;
        rawMetadataInput.lastRecordedSession = 1;
        rawMetadataInput.sourceType = common_types::EPersistenceSourceType::AUTONOMOUS_RECORDER;
        rawMetadataInput.dataType = EPersistenceDataType::TRAJECTORY;
        rawMetadataInput.timeStepIntervalMillisec = QUANTUM_INTERVAL_MILLISEC;

        persId = m_database->writePersistenceSetMetadata( rawMetadataInput );
        rawMetadataInput.persistenceSetId = persId;
        ASSERT_NE( persId, common_vars::INVALID_PERS_ID );
    }

    // II write payload
    vector<common_types::SPersistenceTrajectory> dataToWrite;
    {
        SPersistenceTrajectory trajInput;
        trajInput.ctxId = CONTEXT_ID;
        trajInput.missionId = MISSION_ID;
        trajInput.objId = 123;
        trajInput.state = SPersistenceObj::EState::ACTIVE;
        trajInput.latDeg = 40.0f;
        trajInput.lonDeg = 90.0f;

        // reproduce all possible cases ( '*' step exist, '-' empty cell )
        // |***** *****|***** -----|----- -----|----- *****|***** -----|---** **---|***-- --***|
        // P1 (s1)     P2          P3          P4     s2   P5          P6 (s3)     P7 (s4)  s5

        // S1
        trajInput.sessionNum = 1;
        trajInput.logicTime = -1;
        trajInput.astroTimeMillisec = 9000;

        for( int i = 0; i < 15; i++ ){
            trajInput.logicTime++;
            trajInput.latDeg += 0.1f;
            trajInput.lonDeg += 0.2f;
            trajInput.astroTimeMillisec += 1000;
            dataToWrite.push_back( trajInput );
        }

        // S2
        trajInput.sessionNum = 2;
        trajInput.logicTime = -1;
        trajInput.astroTimeMillisec += 20000;

        for( int i = 0; i < 10; i++ ){
            trajInput.logicTime++;
            trajInput.latDeg += 0.1f;
            trajInput.lonDeg += 0.2f;
            trajInput.astroTimeMillisec += 1000;
            dataToWrite.push_back( trajInput );
        }

        // S3
        trajInput.sessionNum = 3;
        trajInput.logicTime = -1;
        trajInput.astroTimeMillisec += 8000;

        for( int i = 0; i < 4; i++ ){
            trajInput.logicTime++;
            trajInput.latDeg += 0.1f;
            trajInput.lonDeg += 0.2f;
            trajInput.astroTimeMillisec += 1000;
            dataToWrite.push_back( trajInput );
        }

        // S4
        trajInput.sessionNum = 4;
        trajInput.logicTime = -1;
        trajInput.astroTimeMillisec += 3000;

        for( int i = 0; i < 3; i++ ){
            trajInput.logicTime++;
            trajInput.latDeg += 0.1f;
            trajInput.lonDeg += 0.2f;
            trajInput.astroTimeMillisec += 1000;
            dataToWrite.push_back( trajInput );
        }

        // S5
        trajInput.sessionNum = 5;
        trajInput.logicTime = -1;
        trajInput.astroTimeMillisec += 4000;

        for( int i = 0; i < 3; i++ ){
            trajInput.logicTime++;
            trajInput.latDeg += 0.1f;
            trajInput.lonDeg += 0.2f;
            trajInput.astroTimeMillisec += 1000;
            dataToWrite.push_back( trajInput );
        }

        ASSERT_TRUE( m_database->writeTrajectoryData( persId, dataToWrite ) );
    }

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
        for( int stepIdx = 0; stepIdx < 70; stepIdx++ ){
            ASSERT_TRUE( datasourceReader->read( stepIdx ) );

            // payload
            if( stepIdx >= 0 && stepIdx < 15 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( ! stepObjects.empty() );
            }
            // empty
            else if( stepIdx >= 15 && stepIdx < 35 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( stepObjects.empty() );
            }
            // payload
            else if( stepIdx >= 35 && stepIdx < 45 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( ! stepObjects.empty() );
            }
            // empty
            else if( stepIdx >= 45 && stepIdx < 53 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( stepObjects.empty() );
            }
            // payload
            else if( stepIdx >= 53 && stepIdx < 57 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( ! stepObjects.empty() );
            }
            // empty
            else if( stepIdx >= 57 && stepIdx < 60 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( stepObjects.empty() );
            }
            // payload
            else if( stepIdx >= 60 && stepIdx < 63 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( ! stepObjects.empty() );
            }
            // empty
            else if( stepIdx >= 63 && stepIdx < 67 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( stepObjects.empty() );
            }
            // payload
            else if( stepIdx >= 67 && stepIdx < 70 ){
                const DatasourceReader::TObjectsAtOneStep & stepObjects = datasourceReader->getCurrentStep();
                ASSERT_TRUE( ! stepObjects.empty() );
            }
            else{
                assert( false && "out of range" );
            }
        }
    }
}

TEST_F(TestDatasourceReader, read_single_random_steps_test){





}
















