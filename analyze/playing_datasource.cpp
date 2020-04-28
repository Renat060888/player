
#include <algorithm>

#include <microservice_common/system/logger.h>

#include "common/common_vars.h"
#include "system/config_reader.h"
#include "playing_datasource.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "DataSrc:";
static constexpr int16_t PACKAGE_SIZE = 200;

PlayingDatasource::PlayingDatasource()
    : m_currentPackHeadStep(0)
    , m_currentReadStep(0)
    , m_database(nullptr)
{
    m_currentPlayingFrame.resize( PACKAGE_SIZE );
}

PlayingDatasource::~PlayingDatasource()
{
    DatabaseManagerBase::destroyInstance( m_database );

    destroyBeacons( m_timelineBeacons );
}

bool PlayingDatasource::init( const SInitSettings & _settings ){

    m_settings = _settings;
    m_state.settings = & m_settings;

    //
    m_database = DatabaseManagerBase::getInstance();

    DatabaseManagerBase::SInitSettings settings;
    settings.host = CONFIG_PARAMS.baseParams.MONGO_DB_ADDRESS;
    settings.databaseName = CONFIG_PARAMS.baseParams.MONGO_DB_NAME;
    if( ! m_database->init(settings) ){
        return false;
    }

    // in order to update knowledge of the content in database
    m_database->getPersistenceSetMetadata( _settings.ctxId );

    //
    if( ! createBeacons(m_timelineBeacons) ){
        VS_LOG_ERROR << PRINT_HEADER
                     << " failed to create beacons for pers id: " << _settings.persistenceSetId
                     << endl;
        return false;
    }

    // initial read
    if( ! loadPackage( m_currentPackHeadStep, m_currentPlayingFrame ) ){
        VS_LOG_ERROR << PRINT_HEADER
                     << " cannot load package for pers id: " << _settings.persistenceSetId
                     << " May be no data"
                     << endl;
        return false;
    }

    // fill state
    fillState( m_timelineBeacons, m_state );

    getState();

    return true;
}

void PlayingDatasource::fillState( const std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons, SState & _state ){

    //
    vector<TLogicStep> sortedHeadSteps;
    sortedHeadSteps.reserve( _beacons.size() );
    for( auto & valuePair : _beacons ){
        sortedHeadSteps.push_back( valuePair.first );
    }

    std::sort( sortedHeadSteps.begin(), sortedHeadSteps.end() );

    //
    for( const common_types::TLogicStep step : sortedHeadSteps ){
        const SBeacon & beacon = _beacons.find( step )->second;
        _state.dataRangesInfo.insert( _state.dataRangesInfo.end(), beacon.dataBlocks.begin(), beacon.dataBlocks.end() );
    }
}

const PlayingDatasource::SState & PlayingDatasource::getState(){

    //
    m_state.stepsCount = 0;
    m_state.payloadDataRangesInfo.clear();
    for( SBeacon::SDataBlock * block : m_state.dataRangesInfo ){
        if( block->empty ){
            m_state.stepsCount += block->emptyStepsCount;
        }
        else{
            m_state.stepsCount += ( block->logicStepRange.second - block->logicStepRange.first ) + 1;

            //
            m_state.payloadDataRangesInfo.push_back( block );
        }        
    }

    //
    if( ! m_state.dataRangesInfo.empty() ){
        m_state.globalTimeRangeMillisec.first = m_state.dataRangesInfo.front()->timestampRangeMillisec.first;
        m_state.globalTimeRangeMillisec.second = m_state.dataRangesInfo.back()->timestampRangeMillisec.second;
    }

    return m_state;
}

bool PlayingDatasource::createBeacons( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons ){

    destroyBeacons( _beacons );

    const std::vector<SEventsSessionInfo> sessionsInfo = m_database->getPersistenceSetSessions( m_settings.persistenceSetId );
    const std::vector<SEventsSessionInfo> correctedSessions = checkSessionsForEmptyFrames( sessionsInfo );

    std::vector<SEventsSessionInfo>::size_type currentSessionIdx = 0;
    int32_t emptyStepCount = 0;
    SStepCounter currentPassedPoint;

    TLogicStep frameStep = 0;
    while( true ){
        // sections for beacon
        const vector<SBeacon::SDataBlock *> blocks = createBlocks(  correctedSessions,
                                                                    currentPassedPoint,
                                                                    currentSessionIdx,
                                                                    emptyStepCount );
        if( blocks.empty() ){
            break;
        }

        // create beacon
        SBeacon beacon;
        beacon.dataBlocks = blocks;

        // move formal step
        _beacons[ frameStep ] = beacon;
        frameStep += PACKAGE_SIZE;
    }

    //
    setTimestampToEmptyAreas( _beacons );

    return true;
}

void PlayingDatasource::destroyBeacons( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons ){

    for( auto & valuePair : _beacons ){
        const SBeacon & beacon = valuePair.second;

        for( SBeacon::SDataBlock * block : beacon.dataBlocks ){
            delete block;
        }
    }

    _beacons.clear();
}

void PlayingDatasource::setTimestampToEmptyAreas( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons ){

    for( auto & valuePair : _beacons ){
        SBeacon & beacon = valuePair.second;

        for( SBeacon::SDataBlock * block : beacon.dataBlocks ){
            if( block->empty ){
                // TODO: do ?
            }
        }
    }
}

std::vector<PlayingDatasource::SBeacon::SDataBlock *> PlayingDatasource::createBlocks( const std::vector<SEventsSessionInfo> & _sessionsInfo,
                                                                                     SStepCounter & _currentPassedPoint,
                                                                                     std::vector<SEventsSessionInfo>::size_type & _currentSessionIdx,
                                                                                     int32_t & _emptyStepCount ){
    vector<SBeacon::SDataBlock *> dataBlocks;
    if( _sessionsInfo.empty() ){
        return dataBlocks;
    }

    // create blocks
    int32_t collectedSteps = 0;
    while( collectedSteps < PACKAGE_SIZE ){

        int32_t neededSteps = PACKAGE_SIZE - collectedSteps;

        // empty area
        if( _emptyStepCount > 0 ){
            if( _emptyStepCount > neededSteps ){
                SBeacon::SDataBlock * block = new SBeacon::SDataBlock();
                block->empty = true;
                block->emptyStepsCount = neededSteps;
                dataBlocks.push_back( block );

                _emptyStepCount -= neededSteps;
                break;
            }
            else{
                SBeacon::SDataBlock * block = new SBeacon::SDataBlock();
                block->empty = true;
                block->emptyStepsCount = _emptyStepCount;
                dataBlocks.push_back( block );

                collectedSteps += _emptyStepCount;
                neededSteps = PACKAGE_SIZE - collectedSteps;

                _emptyStepCount = 0;
            }
        }

        // available steps
        const SEventsSessionInfo * session = & _sessionsInfo[ _currentSessionIdx ];
        common_types::TLogicStep availableSteps = ( session->maxLogicStep - session->minLogicStep + 1 )
                                    - _currentPassedPoint.logicStepCounter;

        // current session is exhausted - get next session
        if( availableSteps <= 0 ){
            if( (_currentSessionIdx + 1 ) < _sessionsInfo.size() ){
                session = & _sessionsInfo[ ++_currentSessionIdx ];
                availableSteps = ( session->maxLogicStep - session->minLogicStep + 1 );
                _currentPassedPoint.logicStepCounter = 0;
                _currentPassedPoint.sesNum = session->number;
            }
            else{
                // sessions is out
                return dataBlocks;
            }
        }

        // steps enough for beacon "closing"
        if( (availableSteps - neededSteps) > 0 ){
            SBeacon::SDataBlock * block = new SBeacon::SDataBlock();
            block->empty = false;
            block->sesNum = session->number;
            block->logicStepRange.first = session->minLogicStep + _currentPassedPoint.logicStepCounter;
            block->logicStepRange.second = (block->logicStepRange.first - 1) + neededSteps;
            block->timestampRangeMillisec.first = getTimestampByLogicStep( session, block->logicStepRange.first );
            block->timestampRangeMillisec.second = getTimestampByLogicStep( session, block->logicStepRange.second );
            dataBlocks.push_back( block );

            _currentPassedPoint.logicStepCounter += neededSteps;
            _currentPassedPoint.sesNum = session->number;
            break;
        }
        // session steps NOT enough for complete package
        else{
            SBeacon::SDataBlock * block = new SBeacon::SDataBlock();
            block->empty = false;
            block->sesNum = session->number;
            block->logicStepRange.first = session->minLogicStep + _currentPassedPoint.logicStepCounter;
            block->logicStepRange.second = (block->logicStepRange.first - 1) + availableSteps;
            block->timestampRangeMillisec.first = getTimestampByLogicStep( session, block->logicStepRange.first );
            block->timestampRangeMillisec.second = getTimestampByLogicStep( session, block->logicStepRange.second );
            dataBlocks.push_back( block );

            _currentPassedPoint.logicStepCounter += availableSteps;
            _currentPassedPoint.sesNum = session->number;

            collectedSteps += availableSteps;

            // transition to new session
            if( (_currentSessionIdx + 1) < _sessionsInfo.size() ){

                // watch for empty area between two sessions ( will be processed on the next iteration )
                const SEventsSessionInfo * prevSession = session;
                const SEventsSessionInfo * nextSession = & _sessionsInfo[ ++_currentSessionIdx ];

                _currentPassedPoint.logicStepCounter = 0;
                _currentPassedPoint.sesNum = nextSession->number;

                const int64_t differenceBetweenAdjacentTimestamps = m_settings.updateStepMillisec;
                const int64_t timeBetweenSessions = nextSession->minTimestampMillisec
                                                    - prevSession->maxTimestampMillisec
                                                    - differenceBetweenAdjacentTimestamps;

                _emptyStepCount = timeBetweenSessions / m_settings.updateStepMillisec;
            }
            else{
                // sessions is out
                return dataBlocks;
            }
        }
    }

    return dataBlocks;
}

int64_t PlayingDatasource::getTimestampByLogicStep( const SEventsSessionInfo * _session, common_types::TLogicStep _logicStep ){

    auto iter = std::find_if( _session->steps.begin(), _session->steps.end(), FunctorObjectStep(_logicStep) );
    if( iter != _session->steps.end() ){
        return ( * iter ).timestampMillisec;
    }

    return -1;
}

std::vector<SEventsSessionInfo> PlayingDatasource::checkSessionsForEmptyFrames( const std::vector<SEventsSessionInfo> & _sessionInfo ){

    std::vector<SEventsSessionInfo> out;

    if( _sessionInfo.empty() ){
        return out;
    }

    common_types::TSessionNum currentSessionLastNumber = 0;

    // for each session
    for( auto iter = _sessionInfo.begin(); iter != _sessionInfo.end(); ++iter ){
        const SEventsSessionInfo & info = ( * iter );

        vector<SEventsSessionInfo> slicedSession;

        // init first session
        SEventsSessionInfo currentNewSession;
        currentNewSession.number = info.number;
        currentNewSession.minLogicStep = info.steps[ 0 ].logicStep;
        currentNewSession.minTimestampMillisec = info.steps[ 0 ].timestampMillisec;
        currentNewSession.steps.push_back( info.steps[ 0 ] );

        // find the gaps inside it
        for( std::size_t i = 0; i < info.steps.size(); i++ ){

            if( (info.steps[ i ].logicStep + 1) != info.steps[ i+1 ].logicStep ){
                // close current session
                currentNewSession.maxLogicStep = info.steps[ i ].logicStep;
                currentNewSession.maxTimestampMillisec = info.steps[ i ].timestampMillisec;
                if( currentNewSession.minLogicStep != currentNewSession.maxLogicStep ){
                    currentNewSession.steps.push_back( info.steps[ i ] );
                }
                slicedSession.push_back( currentNewSession );
                currentNewSession.clear();

                // open new one
                currentNewSession.number = info.number;
                currentNewSession.minLogicStep = info.steps[ i+1 ].logicStep;
                currentNewSession.minTimestampMillisec = info.steps[ i+1 ].timestampMillisec;
                currentNewSession.steps.push_back( info.steps[ i+1 ] );
            }
            else{
                // continue to accumulate current session
                currentNewSession.steps.push_back( info.steps[ i ] );
            }
        }

        // insert new sessions in position of sliced session ( instead it )
        if( ! slicedSession.empty() ){
            out.insert( out.end(), slicedSession.begin(), slicedSession.end() );
        }
        else{
            out.push_back( info );
        }
    }

    return out;
}

bool PlayingDatasource::moveStepWindow( int64_t _stepFormal, int64_t & _currentPackHeadStep ){

    if( _stepFormal < _currentPackHeadStep ||
        _stepFormal >= (_currentPackHeadStep + PACKAGE_SIZE)
         ){

        _currentPackHeadStep = ( _stepFormal / PACKAGE_SIZE ) * PACKAGE_SIZE;

        return true;
    }

    return false;
}

bool PlayingDatasource::read( common_types::TLogicStep _step ){

    if( moveStepWindow( _step, m_currentPackHeadStep ) && _step < m_state.stepsCount ){
        if( ! loadPackage( m_currentPackHeadStep, m_currentPlayingFrame ) ){
            return false;
        }
    }

    m_currentReadStep = _step;
    return true;
}

bool PlayingDatasource::readInstant( common_types::TLogicStep _step ){

    if( ! loadSingleFrame(_step, m_currentInstantPlayingFrame) ){
        return false;
    }

    return true;
}

bool PlayingDatasource::loadSingleFrame( common_types::TLogicStep _logicStep, TObjectsAtOneStep & _step ){

    _step.clear();

    const int64_t _currentPackHeadStep = ( _logicStep / PACKAGE_SIZE ) * PACKAGE_SIZE;

    auto iter = m_timelineBeacons.find( _currentPackHeadStep );
    if( iter != m_timelineBeacons.end() ){
        const SBeacon & beacon = iter->second;

        SPersistenceSetFilter filter;
        filter.persistenceSetId = m_settings.persistenceSetId;

        getActualData( beacon, _logicStep, filter.sessionNum, filter.minLogicStep );
        if( common_vars::INVALID_SESSION_NUM == filter.sessionNum ){
            return false; // empty area
        }

        filter.maxLogicStep = filter.minLogicStep;

        TObjectsAtOneStep data = m_database->readTrajectoryData( filter );
        _step.swap( data );

        return true;
    }

    return false;
}

bool PlayingDatasource::loadPackage( int64_t _currentPackHeadStep, std::vector<TObjectsAtOneStep> & _steps ){

    auto iter = m_timelineBeacons.find( _currentPackHeadStep );
    if( iter != m_timelineBeacons.end() ){
        const SBeacon & beacon = iter->second;

        // clear previous frame
        for( TObjectsAtOneStep & objStep : _steps ){
            objStep.clear();
        }

        int32_t positionCounter = 0;

        for( const SBeacon::SDataBlock * block : beacon.dataBlocks ){

            // payload area
            if( ! block->empty ){
                SPersistenceSetFilter filter;
                filter.persistenceSetId = m_settings.persistenceSetId;
                filter.sessionNum = block->sesNum;
                filter.minLogicStep = block->logicStepRange.first;
                filter.maxLogicStep = block->logicStepRange.second;

                assert( (filter.maxLogicStep - filter.minLogicStep) <= PACKAGE_SIZE );

                const TObjectsAtOneStep data = m_database->readTrajectoryData( filter );
                TLogicStep currentLogicStep = data.front().logicTime;

                for( const common_types::SPersistenceTrajectory & object : data ){

                    if( object.logicTime == currentLogicStep ){
                        TObjectsAtOneStep & readCell = _steps[ positionCounter ];
                        readCell.push_back( object );
                    }
                    else{
                        currentLogicStep = object.logicTime;
                        positionCounter++;
                        TObjectsAtOneStep & readCell = _steps[ positionCounter ];
                        readCell.push_back( object );
                    }
                }
            }
            // empty area
            else{
                positionCounter += block->emptyStepsCount;
            }
        }
    }
    else{
        return false;
    }

    return true;
}

inline void PlayingDatasource::getActualData(   const SBeacon & _beacon,
                                                const common_types::TLogicStep _logicFormalStep,
                                                common_types::TSessionNum & _sesNum,
                                                common_types::TLogicStep & _logicActualStep ){

    int64_t offset = _logicFormalStep % PACKAGE_SIZE;

    for( SBeacon::SDataBlock * block : _beacon.dataBlocks ){

        if( ! block->empty ){
            // enough - block greater than offset
            if( (block->logicStepRange.second - block->logicStepRange.first) > offset ){
                _sesNum = block->sesNum;
                _logicActualStep = block->logicStepRange.first + offset;
                return;
            }
            // NOT enough - offset greater than block
            else{
                offset -= ( block->logicStepRange.second - block->logicStepRange.first );
            }
        }
        else{
            if( block->emptyStepsCount > offset ){
                _sesNum = common_vars::INVALID_SESSION_NUM;
                _logicActualStep = common_vars::INVALID_LOGIC_STEP;
                return;
            }
            else{
                offset -= ( block->logicStepRange.second - block->logicStepRange.first );
            }
        }
    }
}

const PlayingDatasource::TObjectsAtOneStep & PlayingDatasource::getCurrentStep(){
    return m_currentPlayingFrame[ m_currentReadStep % PACKAGE_SIZE ];
}






















