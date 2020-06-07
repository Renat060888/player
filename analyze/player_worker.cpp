
#include <microservice_common/system/logger.h>

#include "system/config_reader.h"
#include "common/common_utils.h"

#include "player_worker.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "Player:";

#ifdef OBJREPR_LIBRARY_EXIST
static inline objrepr::SpatialObject::TemporalState convertObjectStateToObjrepr( SPersistenceObj::EState _state ){

    switch( _state ){
    case SPersistenceObj::EState::ACTIVE : { return objrepr::SpatialObject::TemporalState::TS_Active; }
    case SPersistenceObj::EState::ABSENT : { return objrepr::SpatialObject::TemporalState::TS_Absent; }
    case SPersistenceObj::EState::DESTROYED : { return objrepr::SpatialObject::TemporalState::TS_Destroyed; }
    case SPersistenceObj::EState::UNDEFINED : { return objrepr::SpatialObject::TemporalState::TS_Unspecified; }
    }
    return objrepr::SpatialObject::TemporalState::TS_Unspecified;
}
#endif

PlayerWorker::PlayerWorker()
    : m_dataForPlayingExist(false)
{

}

PlayerWorker::~PlayerWorker(){

    m_state.playingStatus = EPlayerStatus::CLOSE;
    m_cvPlayStartEvent.notify_one();
    m_cvDatasourceMonitoringSleep.notify_one();
    common_utils::threadShutdown( m_threadPlaying );
    common_utils::threadShutdown( m_threadDatasourceMonitoring );

    DatabaseManagerBase::destroyInstance( m_database );

    VS_LOG_INFO << PRINT_HEADER << " destroy instance for context: " << m_state.settings.ctxId << endl;

    // TODO: destroy datasrc
}

const PlayerWorker::SState & PlayerWorker::getState(){

    // NOTE: because mixer & iterator creates every time when ThreadDatasrcMonitoing update changed readers

    m_muEmitStepProtect.lock();
    m_state.mixer = & m_datasourcesMixer;
    m_state.playIterator = & m_playIterator;
    m_muEmitStepProtect.unlock();

    return m_state;
}

bool PlayerWorker::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

    // objrepr
#ifdef OBJREPR_LIBRARY_EXIST
    m_objectManager = objrepr::RepresentationServer::instance()->objectManager();
#endif

    // database
    DatabaseManagerBase::SInitSettings settings;
    settings.host = CONFIG_PARAMS.baseParams.MONGO_DB_ADDRESS;
    settings.databaseName = CONFIG_PARAMS.baseParams.MONGO_DB_NAME;

    m_database = DatabaseManagerBase::getInstance();
    if( ! m_database->init(settings) ){
        m_state.playingStatus = EPlayerStatus::CRASHED;
        return false;
    }

    // initial data retrieving
    if( ! preparePlayingInfrastructure() ){
        m_state.playingStatus = EPlayerStatus::CRASHED;
        return false;
    }

    // threads
    m_threadPlaying = new thread( & PlayerWorker::threadPlaying, this );
    m_threadDatasourceMonitoring = new thread( & PlayerWorker::threadDatasourceMonitoring, this );

    m_state.playingStatus = EPlayerStatus::INITED;
    return true;
}

bool PlayerWorker::createDatasourceServices( const SInitSettings & _settings, std::vector<DatasourceReader *> & _datasrc ){

    // I get metadata about persistences in this context
    const vector<common_types::SPersistenceMetadata> ctxMetadatas = m_database->getPersistenceSetMetadata( _settings.ctxId );
    if( ctxMetadatas.empty() ){
        VS_LOG_WARN << PRINT_HEADER << " no datasource is created due to context are empty" << endl;
        return true;
    }
    const common_types::SPersistenceMetadata & ctxMetadata = ctxMetadatas[ 0 ]; // NOTE: 0 - because only ONE context is requested

    // TODO: check 'update time' equality of all datasources

    // II create datasources based on them
    // ( video )
    for( const SPersistenceMetadataVideo & processedSensor : ctxMetadata.persistenceFromVideo ){       
        PDatasourceServices datasrcServices;
        auto iter = m_datasrcByPersId.find( processedSensor.persistenceSetId );
        if( iter != m_datasrcByPersId.end() ){
            datasrcServices = iter->second;
        }
        else{
            datasrcServices = std::make_shared<SDatasourceServices>();
            m_datasrcByPersId.insert( {processedSensor.persistenceSetId, datasrcServices} );
        }

        if( ! createDatasource( processedSensor.persistenceSetId, processedSensor.timeStepIntervalMillisec, datasrcServices) ){
            continue;
        }

        const DatasourceReader::SState & state = datasrcServices->reader->getState();

        VS_LOG_TRACE << PRINT_HEADER
                     << " video datasrc-reader on sensor " << processedSensor.recordedFromSensorId
                     << " time begin ms " << state.globalTimeRangeMillisec.first
                     << " time end ms " << state.globalTimeRangeMillisec.second
                     << " steps " << state.stepsCount
                     << endl;

        _datasrc.push_back( datasrcServices->reader );
    }

    // ( dss )
    for( const SPersistenceMetadataDSS & processedSituation : ctxMetadata.persistenceFromDSS ){
        PDatasourceServices datasrcServices;
        auto iter = m_datasrcByPersId.find( processedSituation.persistenceSetId );
        if( iter != m_datasrcByPersId.end() ){
            datasrcServices = iter->second;
        }
        else{
            datasrcServices = std::make_shared<SDatasourceServices>();
            m_datasrcByPersId.insert( {processedSituation.persistenceSetId, datasrcServices} );
        }

        if( ! createDatasource( processedSituation.persistenceSetId, processedSituation.timeStepIntervalMillisec, datasrcServices) ){
            continue;
        }

        const DatasourceReader::SState & state = datasrcServices->reader->getState();

        VS_LOG_TRACE << PRINT_HEADER
                     << " dss datasrc-reader on data " << ( processedSituation.realData ? "REAL" : "SIMULATE" )
                     << " mission " << processedSituation.missionId
                     << " time begin ms " << state.globalTimeRangeMillisec.first
                     << " time end ms " << state.globalTimeRangeMillisec.second
                     << " steps " << state.stepsCount
                     << endl;

        _datasrc.push_back( datasrcServices->reader );
    }

    // ( raw )
    for( const SPersistenceMetadataRaw & processedRaw : ctxMetadata.persistenceFromRaw ){
        PDatasourceServices datasrcServices;
        auto iter = m_datasrcByPersId.find( processedRaw.persistenceSetId );
        if( iter != m_datasrcByPersId.end() ){
            datasrcServices = iter->second;
        }
        else{
            datasrcServices = std::make_shared<SDatasourceServices>();
            m_datasrcByPersId.insert( {processedRaw.persistenceSetId, datasrcServices} );
        }

        if( ! createDatasource( processedRaw.persistenceSetId, processedRaw.timeStepIntervalMillisec, datasrcServices) ){
            continue;
        }

        const DatasourceReader::SState & state = datasrcServices->reader->getState();

        VS_LOG_TRACE << PRINT_HEADER
                     << " raw datasrc-reader on mission " << processedRaw.missionId
                     << " time begin ms " << state.globalTimeRangeMillisec.first
                     << " time end ms " << state.globalTimeRangeMillisec.second
                     << " steps " << state.stepsCount
                     << endl;

        _datasrc.push_back( datasrcServices->reader );
    }

    return true;
}

bool PlayerWorker::createDatasource( TPersistenceSetId _persId, int64_t _timeStepIntervalMillisec, PlayerWorker::PDatasourceServices & _datasrc ){

    // database
    if( ! _datasrc->databaseMgr ){
        DatabaseManagerBase * databaseMgr = DatabaseManagerBase::getInstance();

        DatabaseManagerBase::SInitSettings settingsDb;
        settingsDb.host = CONFIG_PARAMS.baseParams.MONGO_DB_ADDRESS;
        settingsDb.databaseName = CONFIG_PARAMS.baseParams.MONGO_DB_NAME;
        if( ! databaseMgr->init(settingsDb) ){
            return false;
        }

        _datasrc->databaseMgr = databaseMgr;

        // for updated reader background creation
        {
            DatabaseManagerBase * databaseMgr = DatabaseManagerBase::getInstance();

            DatabaseManagerBase::SInitSettings settingsDb;
            settingsDb.host = CONFIG_PARAMS.baseParams.MONGO_DB_ADDRESS;
            settingsDb.databaseName = CONFIG_PARAMS.baseParams.MONGO_DB_NAME;
            if( ! databaseMgr->init(settingsDb) ){
                return false;
            }

            _datasrc->databaseMgrSecond = databaseMgr;
        }
    }

    // descriptor
    if( ! _datasrc->descriptor ){
        // temp
        DatabaseManagerBase * databaseMgr = DatabaseManagerBase::getInstance();

        DatabaseManagerBase::SInitSettings settingsDb;
        settingsDb.host = CONFIG_PARAMS.baseParams.MONGO_DB_ADDRESS;
        settingsDb.databaseName = CONFIG_PARAMS.baseParams.MONGO_DB_NAME;
        if( ! databaseMgr->init(settingsDb) ){
            return false;
        }
        // temp

        DatasourceDescriptor * datasourceDescriptor = new DatasourceDescriptor();

        DatasourceDescriptor::SInitSettings settingsDescriptor;
        settingsDescriptor.persId = _persId;
        settingsDescriptor.databaseMgr = databaseMgr; // _datasrc->databaseMgr;
        settingsDescriptor.sessionGapsThreshold = 0; // TODO: may be from cfg ?
        settingsDescriptor.updateStepMillisec = _timeStepIntervalMillisec;
        if( ! datasourceDescriptor->init(settingsDescriptor) ){
            VS_LOG_ERROR << PRINT_HEADER
                         << " failed to create datasource descriptor for pers id: " << _persId
                         << " table name: " << _datasrc->databaseMgr->getTableName( _persId )
                         << endl;
            return false;
        }

        _datasrc->descriptor = datasourceDescriptor;
    }

    // reader
    if( ! _datasrc->reader ){
        DatasourceReader * datasourceReader = new DatasourceReader();

        DatasourceReader::SInitSettings settingsReader;
        settingsReader.persistenceSetId = _persId;
        settingsReader.updateStepMillisec = _timeStepIntervalMillisec;
        settingsReader.databaseMgr = _datasrc->databaseMgr;
        settingsReader.descriptor = _datasrc->descriptor;
        if( ! datasourceReader->init(settingsReader) ){
            VS_LOG_ERROR << PRINT_HEADER
                         << " failed to create datasource reader for pers id: " << _persId
                         << " table name: " << _datasrc->databaseMgr->getTableName( _persId )
                         << endl;
            return false;
        }

        _datasrc->reader = datasourceReader;
    }

    // editor
    if( ! _datasrc->editor ){

        // TODO: do
        _datasrc->editor = nullptr;
    }

    return true;
}

bool PlayerWorker::preparePlayingInfrastructure(){

    // 0 level - datasource(s)
    std::vector<DatasourceReader *> datasourceReaders;
    if( ! createDatasourceServices(m_state.settings, datasourceReaders) ){
        VS_LOG_ERROR << PRINT_HEADER << " datasources creation failed" << endl;
        return false;
    }

    //
    m_dataForPlayingExist = ! datasourceReaders.empty();
    if( ! m_dataForPlayingExist ){
        VS_LOG_WARN << PRINT_HEADER << "------------------------------" << endl;
        VS_LOG_WARN << PRINT_HEADER << " data for playing is NOT EXIST" << endl;
        VS_LOG_WARN << PRINT_HEADER << "------------------------------" << endl;
        return true;
    }

    // 1 level - mixer
    DatasourceMixer::SInitSettings settings2;
    settings2.datasourceReaders = datasourceReaders;

    DatasourceMixer mixer;
    if( ! mixer.init(settings2) ){
        VS_LOG_ERROR << PRINT_HEADER << " datasources mixing failed" << endl;
        return false;
    }

    // 2 level - iterator
    const DatasourceMixer::SState & state = mixer.getState();
    PlayerStepIterator::SInitSettings settings3;
    settings3._stepUpdateMillisec = state.globalStepUpdateMillisec;
    settings3._globalRange = state.globalDataRangeMillisec;
    settings3._totalSimulSteps = state.globalStepCount;

    PlayerStepIterator playIterator;
    if( ! playIterator.init(settings3) ){
        VS_LOG_ERROR << PRINT_HEADER << " play iterator init failed" << endl;
        return false;
    }

    // transparent substitution
    m_muEmitStepProtect.lock();
    m_datasourcesMixer = mixer;
    playIterator.setStep( m_playIterator.getState().m_currentPlayStep );
    m_playIterator = playIterator;
    m_muEmitStepProtect.unlock();

    return true;
}

void PlayerWorker::threadDatasourceMonitoring(){

    static constexpr int64_t DATASOURCE_MONITORING_INTERVAL_MILLISEC = 10000;

    VS_LOG_INFO << PRINT_HEADER << " start a datasource monitoring THREAD" << endl;    

    while( EPlayerStatus::CLOSE != m_state.playingStatus ){

        if( isDataForPlayingExist() ){
            checkForChangedDatasources();
            checkForNewDatasources();
        }

        std::mutex lockMutex;
        std::unique_lock<std::mutex> cvLock( lockMutex );
        m_cvDatasourceMonitoringSleep.wait_for( cvLock,
                                                chrono::milliseconds(DATASOURCE_MONITORING_INTERVAL_MILLISEC),
                                                [this](){ return EPlayerStatus::CLOSE == m_state.playingStatus; } );
    }

    VS_LOG_INFO << PRINT_HEADER << " exit from a datasource monitoring THREAD" << endl;
}

void PlayerWorker::checkForChangedDatasources(){

    unordered_map<TPersistenceSetId, DatasourceReader *> changedDatasrcMap;
    vector<DatasourceReader *> changedDatasrc;

    // find changed datasources ( DUPLICATE PART OF SERVICE SECTION )
    for( auto & valuePair : m_datasrcByPersId ){
        PDatasourceServices & datasourceService = valuePair.second;

        if( datasourceService->descriptor->isDescriptionChanged() ){
            VS_LOG_INFO << PRINT_HEADER << " > datasrc changed for pers id: " << datasourceService->descriptor->getState().settings.persId << endl;

            // flip-flop database managers between playing thread & this thread
            DatabaseManagerBase * db = nullptr;
            if( datasourceService->firstDatabaseCurrent ){
                db = datasourceService->databaseMgrSecond;
                datasourceService->firstDatabaseCurrent = false;
            }
            else{
                db = datasourceService->databaseMgr;
                datasourceService->firstDatabaseCurrent = true;
            }

            // new reader
            DatasourceReader * datasourceReader = new DatasourceReader();

            DatasourceReader::SInitSettings settingsReader;
            settingsReader.persistenceSetId = valuePair.first;
            settingsReader.updateStepMillisec = datasourceService->descriptor->getState().settings.updateStepMillisec;
            settingsReader.databaseMgr = db;
            settingsReader.descriptor = datasourceService->descriptor;
            if( ! datasourceReader->init(settingsReader) ){
                VS_LOG_ERROR << PRINT_HEADER
                             << " failed to recreate second datasource reader for pers id: " << valuePair.first
                             << " table name: " << datasourceService->databaseMgr->getTableName( valuePair.first )
                             << endl;
                delete datasourceReader;
                continue;
            }

            changedDatasrcMap.insert( {valuePair.first, datasourceReader} );
            changedDatasrc.push_back( datasourceReader );
        }
    }

    // if change occured ( DUPLICATE OF PREPARING SECTION )
    if( ! changedDatasrcMap.empty() ){
        vector<DatasourceReader *> outdatedDatasrc;
        vector<DatasourceReader *> actualDatasrc;

        // accumulate outdated active readers
        m_muEmitStepProtect.lock();
        for( DatasourceReader * activeReader : m_datasourcesMixer.getState().settings.datasourceReaders ){
            auto iter = changedDatasrcMap.find( activeReader->getState().settings.persistenceSetId );
            if( iter != changedDatasrcMap.end() ){
                outdatedDatasrc.push_back( activeReader );
            }
            else{
                actualDatasrc.push_back( activeReader );
            }
        }
        m_muEmitStepProtect.unlock();

        // new mixer
        DatasourceMixer mixer;

        DatasourceMixer::SInitSettings settings2;
        settings2.datasourceReaders.insert( settings2.datasourceReaders.end(), actualDatasrc.begin(), actualDatasrc.end() );
        settings2.datasourceReaders.insert( settings2.datasourceReaders.end(), changedDatasrc.begin(), changedDatasrc.end() );
        if( ! mixer.init(settings2) ){
            VS_LOG_ERROR << PRINT_HEADER << " datasources mixing failed" << endl;
            return;
        }

        // new iterator
        PlayerStepIterator playIterator;

        const DatasourceMixer::SState & state = mixer.getState();
        PlayerStepIterator::SInitSettings settings3;
        settings3._stepUpdateMillisec = state.globalStepUpdateMillisec;
        settings3._globalRange = state.globalDataRangeMillisec;
        settings3._totalSimulSteps = state.globalStepCount;
        if( ! playIterator.init(settings3) ){
            VS_LOG_ERROR << PRINT_HEADER << " play iterator init failed" << endl;
            return;
        }

        // swap with current playing ones
        m_muEmitStepProtect.lock();
        m_datasourcesMixer = mixer;
        playIterator.setStep( m_playIterator.getState().m_currentPlayStep );
        playIterator.setLoopMode( m_playIterator.getState().m_loopMode );
        playIterator.setDirectionMode( m_playIterator.getState().m_directionMode );
        playIterator.setDelayTime( m_playIterator.getState().m_updateDelayMillisec );
        m_playIterator = playIterator;
        m_muEmitStepProtect.unlock();

        // destroy old sources HERE
        for( DatasourceReader * reader : outdatedDatasrc ){
            delete reader;
        }
    }
}

void PlayerWorker::checkForNewDatasources(){

    // TODO: check not only changed datasrc, but also new ones
}

bool PlayerWorker::isDataForPlayingExist(){

    if( m_dataForPlayingExist ){
        return true;
    }
    else{
        const vector<common_types::SPersistenceMetadata> ctxMetadatas = m_database->getPersistenceSetMetadata( m_state.settings.ctxId );
        if( ctxMetadatas.empty() ){
            return false;
        }
        else{
            return preparePlayingInfrastructure();
        }
    }
}

void PlayerWorker::threadPlaying(){

    VS_LOG_INFO << PRINT_HEADER << " start a playing THREAD" << endl;

    while( EPlayerStatus::CLOSE != m_state.playingStatus ){
        if( EPlayerStatus::PLAYING == m_state.playingStatus ){
            playingLoop();
        }

        std::mutex cvMutex;
        std::unique_lock<std::mutex> lockCV( cvMutex );
        m_cvPlayStartEvent.wait( lockCV, [this](){ return EPlayerStatus::PLAYING == m_state.playingStatus ||
                EPlayerStatus::CLOSE == m_state.playingStatus; }
                );
    }

    VS_LOG_INFO << PRINT_HEADER << " exit from a playing THREAD" << endl;
}

void PlayerWorker::playingLoop(){

    while( EPlayerStatus::PLAYING == m_state.playingStatus ){
        if( 1 == m_playIterator.getState().m_currentPlayStep ){
            hideFutureObjects();
        }

        emitStep( m_playIterator.getState().m_currentPlayStep );

        if( ! m_playIterator.goNextStep() ){
            m_state.playingStatus = EPlayerStatus::ALL_STEPS_PASSED;
            break;
        }

        this_thread::sleep_for( chrono::milliseconds((int64_t)m_playIterator.getState().m_updateDelayMillisec) );
    }

    VS_LOG_INFO << PRINT_HEADER
                << " exit from playing loop: status = [" << common_utils::printPlayingStatus(m_state.playingStatus) << "]"
                << " step number [" << m_playIterator.getState().m_currentPlayStep << "]"
                << endl;
}

bool PlayerWorker::playOneStep(){

    emitStep( m_playIterator.getState().m_currentPlayStep );
    return true;
}

inline void PlayerWorker::emitStep( TLogicStep _step ){

    std::lock_guard<std::mutex> lock( m_muEmitStepProtect );

    // read
    if( m_datasourcesMixer.read(_step) ){
        const DatasourceReader::TObjectsAtOneStep & objectsStep = m_datasourcesMixer.getCurrentStep();
#if 0
        if( ! objectsStep.empty() ){
            VS_LOG_TRACE << PRINT_HEADER
                         << " abstr step " << _step
                         << " id " << objectsStep.front().id
                         << " sess " << objectsStep.front().session
                         << " log t " << objectsStep.front().logicTime
                         << " ast t " << objectsStep.front().timestampMillisec
                         << " total " << objectsStep.size()
                         << endl;
            // emit
            playObjects( objectsStep );
        }
        else{
            VS_LOG_TRACE << PRINT_HEADER
                         << " abstract step " << _step
                         << " empty step"
                         << endl;
        }        
#else
        // emit
        playObjects( objectsStep );
#endif
    }
    else{
        m_state.playingStatus = EPlayerStatus::NOT_ENOUGH_STEPS;
    }
}

inline void PlayerWorker::emitStepInstant( TLogicStep _step ){

    std::lock_guard<std::mutex> lock( m_muEmitStepProtect );

    // read
    if( m_datasourcesMixer.readInstant(_step) ){
        const DatasourceReader::TObjectsAtOneStep & objectsStep = m_datasourcesMixer.getCurrentStep();

        if( ! objectsStep.empty() ){
            VS_LOG_TRACE << PRINT_HEADER
                         << " abstr step " << _step
                         << " id " << objectsStep.front().objId
                         << " sess " << objectsStep.front().sessionNum
                         << " log t " << objectsStep.front().logicTime
                         << " ast t " << objectsStep.front().astroTimeMillisec
                         << " total " << objectsStep.size()
                         << endl;
            // emit
            playObjects( objectsStep );
        }
        else{
            VS_LOG_TRACE << PRINT_HEADER
                         << " abstract step " << _step
                         << " empty step"
                         << endl;
        }
    }
    else{
        m_state.playingStatus = EPlayerStatus::NOT_ENOUGH_STEPS;
    }
}

inline void PlayerWorker::playObjects( const DatasourceReader::TObjectsAtOneStep & _objectsStep ){

//    if( _objectsStep.empty() ){
//        VS_LOG_DBG << "empty step" << endl;
//    }

    for( const common_types::SPersistenceTrajectory & object : _objectsStep ){

//        VS_LOG_DBG << "obj id: " << object.objId
//                   << " lt: " << object.logicTime
//                   << " at: " << object.astroTimeMillisec
//                   << " session : " << object.sessionNum
//                   << " yaw: " << object.yawDeg
//                   << endl;
//        continue;

        //
        auto iter = m_playingObjects.find( object.objId );
        if( iter != m_playingObjects.end() ){
            SPlayableObject & playObj = iter->second;
#ifdef OBJREPR_LIBRARY_EXIST
            objrepr::GeoCoord point( object.lonDeg, object.latDeg, object.height );
            objrepr::CoordTransform::instance()->EPSG4978_EPSG4326( & point );

//            VS_LOG_DBG << " srf: " << playObj.objreprObject->spatialRefCode() << endl;

            playObj.objreprObject->changePoint( 0, 0, point );
//            playObj.objreprObject->setTemporalState( convertObjectStateToObjrepr(object.state) );
            playObj.objreprObject->setTemporalState( objrepr::SpatialObject::TemporalState::TS_Active );
            playObj.objreprObject->setOrientationHeading( object.yawDeg );
            playObj.objreprObject->push();
#endif
        }
        else{
            SPlayableObject playObj;

#ifdef OBJREPR_LIBRARY_EXIST
            playObj.objreprObject = m_objectManager->loadObject( object.objId );
            if( ! playObj.objreprObject ){
                playObj.objreprObject = m_objectManager->getObject( object.objId );

                if( ! playObj.objreprObject ){
                    VS_LOG_ERROR << PRINT_HEADER << " such objrepr object not found: " << object.objId << endl;
                    continue;
                }
            }

            objrepr::GeoCoord point( object.lonDeg, object.latDeg, object.height );
            objrepr::CoordTransform::instance()->EPSG4978_EPSG4326( & point );

            playObj.objreprObject->changePoint( 0, 0, point );
//            playObj.objreprObject->setTemporalState( convertObjectStateToObjrepr(object.state) );
            playObj.objreprObject->setTemporalState( objrepr::SpatialObject::TemporalState::TS_Active );
            playObj.objreprObject->setOrientationHeading( object.yawDeg );
            playObj.objreprObject->push();
#endif
            m_playingObjects.insert( {object.objId, playObj} );
        }
        //
    }
}

void PlayerWorker::start(){

    if( ! m_dataForPlayingExist ){
        return;
    }

    m_state.playingStatus = EPlayerStatus::PLAYING;
    m_cvPlayStartEvent.notify_one();
}

void PlayerWorker::pause(){

    if( ! m_dataForPlayingExist ){
        return;
    }

    m_state.playingStatus = EPlayerStatus::PAUSED;
}

void PlayerWorker::stop(){

    if( ! m_dataForPlayingExist ){
        return;
    }

    m_state.playingStatus = EPlayerStatus::STOPPED;
    m_playIterator.resetStep();
    hideFutureObjects();
    playOneStep(); // чтобы объекты появились на карте в начальной позиции
}

bool PlayerWorker::stepForward(){

    if( ! m_dataForPlayingExist ){
        return false;
    }

    if( EPlayerStatus::PLAYING == m_state.playingStatus ){
        m_state.lastError = "stop or pause playing before make a step";
        return false;
    }

    bool res = true;
    if( m_playIterator.goNextStep() ){
        playOneStep();
    }
    else{
        res = false;
    }

    return res;
}

bool PlayerWorker::stepBackward(){

    if( ! m_dataForPlayingExist ){
        return false;
    }

    if( EPlayerStatus::PLAYING == m_state.playingStatus ){
        m_state.lastError = "stop or pause playing before make a step";
        return false;
    }

    // save current mode
    const PlayerStepIterator::EPlayMode currentPlayMode = m_playIterator.getState().m_directionMode;
    m_playIterator.setDirectionMode( PlayerStepIterator::EPlayMode::REVERSE );

    //
    bool res = true;
    if( m_playIterator.goNextStep() ){
        playOneStep();
    }
    else{
        res = false;
    }

    // restore mode
    m_playIterator.setDirectionMode( currentPlayMode );

    return res;
}

bool PlayerWorker::setRange( const TTimeRangeMillisec & _range ){

    if( ! m_dataForPlayingExist ){
        return false;
    }

    // TODO: do
}

void PlayerWorker::switchReverseMode( bool _reverse ){    

    if( _reverse ){
        m_playIterator.setDirectionMode( PlayerStepIterator::EPlayMode::REVERSE );
    }
    else{
        m_playIterator.setDirectionMode( PlayerStepIterator::EPlayMode::NORMAL );
    }
}

void PlayerWorker::switchLoopMode( bool _loop ){

    m_playIterator.setLoopMode( _loop );
}

bool PlayerWorker::playFromPosition( int64_t _stepMillisec ){

    if( ! m_dataForPlayingExist ){
        return false;
    }

    m_state.playingStatus = EPlayerStatus::PLAY_FROM_POSITION;

    // astro time -> global logic time
    const DatasourceMixer::SState & mixerState = m_state.mixer->getState();
    const int64_t timeDifferenceMillisec = _stepMillisec - mixerState.globalDataRangeMillisec.first;
    const TLogicStep logStep = timeDifferenceMillisec / mixerState.globalStepUpdateMillisec;

    //
    if( ! m_playIterator.setStep(logStep) ){
        m_state.playingStatus = EPlayerStatus::PLAYING;
        m_cvPlayStartEvent.notify_one();
        return false;
    }

    emitStepInstant( m_playIterator.getState().m_currentPlayStep );

    VS_LOG_TRACE << PRINT_HEADER
                 << " 'playFromPosition' astro step: " << _stepMillisec
                 << " calc step " << logStep
                 << " iter step " << m_playIterator.getState().m_currentPlayStep
                 << endl;

    return true;
}

bool PlayerWorker::increasePlayingSpeed(){

    if( m_playIterator.increaseSpeed() ){
        return true;
    }
    else{
        m_state.lastError = m_playIterator.getState().m_lastError;
        return false;
    }
}

bool PlayerWorker::decreasePlayingSpeed(){

    if( m_playIterator.decreaseSpeed() ){
        return true;
    }
    else{
        m_state.lastError = m_playIterator.getState().m_lastError;
        return false;
    }
}

void PlayerWorker::normalizePlayingSpeed(){

    m_playIterator.normalizeSpeed();
}

void PlayerWorker::hideFutureObjects(){

    for( auto & valuePair : m_playingObjects ){
        SPlayableObject & obj = valuePair.second;

#ifdef OBJREPR_LIBRARY_EXIST
        obj.objreprObject->setTemporalState( objrepr::SpatialObject::TemporalState::TS_Absent );
        obj.objreprObject->push();
#endif
    }
}






