
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
{

}

PlayerWorker::~PlayerWorker()
{
    common_utils::threadShutdown( m_threadPlaying );

    DatabaseManagerBase::destroyInstance( m_database );

    // TODO: destroy datasrc
}

void * PlayerWorker::getState(){

    m_state.mixer = & m_datasourcesMixer;
    m_state.playIterator = & m_playIterator;

    return (void *)( & m_state );
}

bool PlayerWorker::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

    //
#ifdef OBJREPR_LIBRARY_EXIST
    m_objectManager = objrepr::RepresentationServer::instance()->objectManager();
#endif

    //
    m_database = DatabaseManagerBase::getInstance();

    DatabaseManagerBase::SInitSettings settings;
    settings.host = CONFIG_PARAMS.baseParams.MONGO_DB_ADDRESS;
    settings.databaseName = CONFIG_PARAMS.baseParams.MONGO_DB_NAME;

    if( ! m_database->init(settings) ){
        m_state.playingStatus = EPlayerStatus::CRASHED;
        return false;
    }

    //
    if( ! updatePlayingData() ){
        m_state.playingStatus = EPlayerStatus::CRASHED;
        return false;
    }

    //
    m_threadPlaying = new thread( & PlayerWorker::threadPlaying, this );

    m_state.playingStatus = EPlayerStatus::INITED;

    return true;
}

bool PlayerWorker::createDatasources( const SInitSettings & _settings, std::vector<DatasourceReader *> & _datasrc ){

    const vector<common_types::SPersistenceMetadata> ctxMetadatas = m_database->getPersistenceSetMetadata( _settings.ctxId );
    const common_types::SPersistenceMetadata & ctxMetadata = ctxMetadatas[ 0 ]; // TODO: segfault ?

    // TODO: check 'update time' equality of all datasources

    // datasources ( video )
    for( const SPersistenceMetadataVideo & processedSensor : ctxMetadata.persistenceFromVideo ){

        DatasourceReader * datasource = new DatasourceReader();

        DatasourceReader::SInitSettings settings;
        settings.persistenceSetId = processedSensor.persistenceSetId;
        settings.updateStepMillisec = processedSensor.timeStepIntervalMillisec;
        settings.ctxId = _settings.ctxId;
        if( ! datasource->init(settings) ){
            VS_LOG_ERROR << PRINT_HEADER << " failed to create datasource for sensor: " << processedSensor.recordedFromSensorId << endl;
            continue;
        }

        const DatasourceReader::SState & state = datasource->getState();

        VS_LOG_TRACE << PRINT_HEADER
                     << " video datasrc on sensor " << processedSensor.recordedFromSensorId
                     << " time begin ms " << state.globalTimeRangeMillisec.first
                     << " time end ms " << state.globalTimeRangeMillisec.second
                     << " steps " << state.stepsCount
                     << endl;

        _datasrc.push_back( datasource );
    }

    // datasources ( dss )
    for( const SPersistenceMetadataDSS & processedSituation : ctxMetadata.persistenceFromDSS ){

        DatasourceReader * datasource = new DatasourceReader();

        DatasourceReader::SInitSettings settings;
        settings.persistenceSetId = processedSituation.persistenceSetId;
        settings.updateStepMillisec = processedSituation.timeStepIntervalMillisec;
        if( ! datasource->init(settings) ){
            VS_LOG_ERROR << PRINT_HEADER << " failed to create datasource for data: " << ( processedSituation.realData ? "REAL" : "SIMULATE" ) << endl;
            continue;
        }

        const DatasourceReader::SState & state = datasource->getState();

        VS_LOG_TRACE << PRINT_HEADER
                     << " dss datasrc on data " << ( processedSituation.realData ? "REAL" : "SIMULATE" )
                     << " mission " << processedSituation.missionId
                     << " time begin ms " << state.globalTimeRangeMillisec.first
                     << " time end ms " << state.globalTimeRangeMillisec.second
                     << " steps " << state.stepsCount
                     << endl;

        _datasrc.push_back( datasource );
    }

    // datasources ( raw )
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

    return true;
}

bool PlayerWorker::updatePlayingData(){

    VS_LOG_DBG << PRINT_HEADER << " update playing data >>" << endl;

    // 0 level - datasource
    std::vector<DatasourceReader *> datasources;
    if( ! createDatasources(m_state.settings, datasources) ){
        VS_LOG_ERROR << PRINT_HEADER << " datasources creation failed" << endl;
        return false;
    }

    std::vector<DatasourceReader *> oldDatasources;
    if( m_datasourcesMixer.getState().inited ){
        oldDatasources = m_datasourcesMixer.getState().settings.datasources;
    }

    // 1 level - mixer
    DatasourceMixer mixer;

    DatasourceMixer::SInitSettings settings2;
    settings2.datasources = datasources;
    if( ! mixer.init(settings2) ){
        VS_LOG_ERROR << PRINT_HEADER << " datasources mixing failed" << endl;
        return false;
    }

    // 2 level - iterator
    PlayerStepIterator playIterator;

    const DatasourceMixer::SState & state = mixer.getState();
    PlayerStepIterator::SInitSettings settings3;
    settings3._stepUpdateMillisec = state.globalStepUpdateMillisec;
    settings3._globalRange = state.globalDataRangeMillisec;
    settings3._totalSimulSteps = state.globalStepCount;
    if( ! playIterator.init(settings3) ){
        VS_LOG_ERROR << PRINT_HEADER << " play iterator init failed" << endl;
        return false;
    }

    // transparent substitution
    m_muEmitStepProtect.lock();
    m_datasourcesMixer = mixer;
    playIterator.setStep( m_playIterator.getCurrentStep() );
    m_playIterator = playIterator;
    m_muEmitStepProtect.unlock();

    // remore out-dated resources
    if( ! oldDatasources.empty() ){
        for( DatasourceReader * datasrc : oldDatasources ){
            delete datasrc;
        }
    }

    VS_LOG_DBG << PRINT_HEADER << " update playing data <<" << endl;
    return true;
}

void PlayerWorker::threadPlaying(){

    // TODO: may be call this from Analytic Manager's maintenance thread ?

    while( EPlayerStatus::CLOSE != m_state.playingStatus ){

        if( EPlayerStatus::PLAYING == m_state.playingStatus ){
            playingLoop();
        }

        std::mutex cvMutex;
        std::unique_lock<std::mutex> lockCV( cvMutex );
        m_cvPlayStartEvent.wait( lockCV, [this](){ return   EPlayerStatus::PLAYING == m_state.playingStatus ||
                                                            EPlayerStatus::CLOSE == m_state.playingStatus; } );
    }
}

void PlayerWorker::playingLoop(){

    while( EPlayerStatus::PLAYING == m_state.playingStatus ){

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

    for( const common_types::SPersistenceTrajectory & object : _objectsStep ){

        auto iter = m_playingObjects.find( object.objId );
        if( iter != m_playingObjects.end() ){
            SPlayableObject & playObj = iter->second;
#ifdef OBJREPR_LIBRARY_EXIST
            playObj.objreprObject->changePoint( 0, 0, objrepr::GeoCoord(object.lonDeg, object.latDeg, 0) );
            playObj.objreprObject->setTemporalState( convertObjectStateToObjrepr(object.state) );
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

            playObj.objreprObject->changePoint( 0, 0, objrepr::GeoCoord(object.lonDeg, object.latDeg, 0) );
            playObj.objreprObject->setTemporalState( convertObjectStateToObjrepr(object.state) );
            playObj.objreprObject->setOrientationHeading( object.yawDeg );

            playObj.objreprObject->push();
#endif

            m_playingObjects.insert( {object.objId, playObj} );
        }
    }
}

void PlayerWorker::start(){
    m_state.playingStatus = EPlayerStatus::PLAYING;
    m_cvPlayStartEvent.notify_one();
}

void PlayerWorker::pause(){
    m_state.playingStatus = EPlayerStatus::PAUSED;
}

void PlayerWorker::stop(){
    m_state.playingStatus = EPlayerStatus::STOPPED;
    m_playIterator.resetStep();
    hideFutureObjects();
    playOneStep(); // чтобы объекты появились на карте в начальной позиции
}

bool PlayerWorker::stepForward(){

    if( EPlayerStatus::PLAYING == m_state.playingStatus ){
        m_state.m_lastError = "stop or pause playing before make a step";
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

    if( EPlayerStatus::PLAYING == m_state.playingStatus ){
        m_state.m_lastError = "stop or pause playing before make a step";
        return false;
    }

    // save current mode
    const PlayerStepIterator::EPlayMode currentPlayMode = m_playIterator.getState().m_playMode;
    m_playIterator.setPlayMode( PlayerStepIterator::EPlayMode::REVERSE );

    //
    bool res = true;
    if( m_playIterator.goNextStep() ){
        playOneStep();
    }
    else{
        res = false;
    }

    // restore mode
    m_playIterator.setPlayMode( currentPlayMode );

    return res;
}

bool PlayerWorker::setRange( const TTimeRangeMillisec & _range ){
    // TODO: do
}

void PlayerWorker::switchReverseMode( bool _reverse ){

    if( _reverse ){
        m_playIterator.setPlayMode( PlayerStepIterator::EPlayMode::REVERSE );
    }
    else{
        m_playIterator.setPlayMode( PlayerStepIterator::EPlayMode::NORMAL );
    }
}

void PlayerWorker::switchLoopMode( bool _loop ){

    m_playIterator.setLoopMode( _loop );
}

bool PlayerWorker::playFromPosition( int64_t _stepMillisec ){

    m_state.playingStatus = EPlayerStatus::PLAY_FROM_POSITION;

    // astro time -> logic time
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
        m_state.m_lastError = m_playIterator.getLastError();
        return false;
    }
}

bool PlayerWorker::decreasePlayingSpeed(){

    if( m_playIterator.decreaseSpeed() ){
        return true;
    }
    else{
        m_state.m_lastError = m_playIterator.getLastError();
        return false;
    }
}

void PlayerWorker::normalizePlayingSpeed(){
    // TODO: do
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





