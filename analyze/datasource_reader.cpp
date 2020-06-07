
#include <algorithm>

#include <microservice_common/system/logger.h>

#include "common/common_vars.h"
#include "system/config_reader.h"
#include "datasource_descriptor.h"
#include "datasource_reader.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "DatasrcReader:";
static constexpr int16_t PACKAGE_SIZE = 10;

DatasourceReader::DatasourceReader()
    : m_currentPackHeadStep(0)
    , m_currentReadStep(0)
{
    m_currentPlayingFrame.resize( PACKAGE_SIZE );
}

DatasourceReader::~DatasourceReader()
{
    destroyBeacons( m_timelineBeacons );
}

bool DatasourceReader::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

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

void DatasourceReader::fillState( const std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons, SState & _state ){

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

    //
    m_state.stepsCount = 0;
    m_state.payloadDataRangesInfo.clear();

    if( m_state.dataRangesInfo.empty() ){
        return;
    }

    // payload raw blocks
    for( SBeacon::SDataBlock * block : m_state.dataRangesInfo ){
        if( block->empty ){
            m_state.stepsCount += block->emptyStepsCount;
        }
        else{
            m_state.stepsCount += ( block->logicStepRange.second - block->logicStepRange.first ) + 1;
            m_state.payloadDataRangesInfo.push_back( block );            
        }
    }

    // payload sessions
    TTimeRangeMillisec currentSessionRange;
    TSessionNum currentSessionNum = m_state.payloadDataRangesInfo.front()->sesNum;
    currentSessionRange.first = m_state.payloadDataRangesInfo.front()->timestampRangeMillisec.first;

    for( int i = 0; i < m_state.payloadDataRangesInfo.size(); i++ ){
        SBeacon::SDataBlock * block = m_state.payloadDataRangesInfo[ i ];

        if( block->sesNum != currentSessionNum ){
            SBeacon::SDataBlock * prevBlock = m_state.payloadDataRangesInfo[ i-1 ];

            // close previous
            currentSessionRange.second = prevBlock->timestampRangeMillisec.second;
            m_state.sessionsTimeRangeMillisec.push_back( currentSessionRange );

            // open next
            currentSessionRange.first = block->timestampRangeMillisec.first;
            currentSessionNum = block->sesNum;
        }
    }

    currentSessionRange.second = m_state.payloadDataRangesInfo.back()->timestampRangeMillisec.second;
    m_state.sessionsTimeRangeMillisec.push_back( currentSessionRange );

    // global range
    m_state.globalTimeRangeMillisec.first = m_state.dataRangesInfo.front()->timestampRangeMillisec.first;
    m_state.globalTimeRangeMillisec.second = m_state.dataRangesInfo.back()->timestampRangeMillisec.second;
}

const DatasourceReader::SState & DatasourceReader::getState() const {

    return m_state;
}

bool DatasourceReader::createBeacons( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons ){

    destroyBeacons( _beacons );

    const std::vector<SEventsSessionInfo> sessionsInfo = m_state.settings.descriptor->getSessionsDescription();

    std::vector<SEventsSessionInfo>::size_type currentSessionIdx = 0;
    int32_t emptyStepCount = 0;
    SStepCounter currentPassedPoint;

    TLogicStep frameStep = 0;
    while( true ){
        // sections for beacon
        const vector<SBeacon::SDataBlock *> blocks = createBlocks(  sessionsInfo,
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

void DatasourceReader::destroyBeacons( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons ){

    for( auto & valuePair : _beacons ){
        const SBeacon & beacon = valuePair.second;

        for( SBeacon::SDataBlock * block : beacon.dataBlocks ){
            delete block;
        }
    }

    _beacons.clear();
}

void DatasourceReader::setTimestampToEmptyAreas( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons ){

    for( auto & valuePair : _beacons ){
        SBeacon & beacon = valuePair.second;

        for( SBeacon::SDataBlock * block : beacon.dataBlocks ){
            if( block->empty ){
                // TODO: do ?
            }
        }
    }
}

std::vector<DatasourceReader::SBeacon::SDataBlock *> DatasourceReader::createBlocks( const std::vector<SEventsSessionInfo> & _sessionsInfo,
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
            if( _emptyStepCount >= neededSteps ){
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
            block->timestampRangeMillisec.first = m_state.settings.descriptor->getTimestampByLogicStep( session->number, block->logicStepRange.first );
            block->timestampRangeMillisec.second = m_state.settings.descriptor->getTimestampByLogicStep( session->number, block->logicStepRange.second );
//            block->timestampRangeMillisec.first = getTimestampByLogicStep( session, block->logicStepRange.first );
//            block->timestampRangeMillisec.second = getTimestampByLogicStep( session, block->logicStepRange.second );
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
            block->timestampRangeMillisec.first = m_state.settings.descriptor->getTimestampByLogicStep( session->number, block->logicStepRange.first );
            block->timestampRangeMillisec.second = m_state.settings.descriptor->getTimestampByLogicStep( session->number, block->logicStepRange.second );
//            block->timestampRangeMillisec.first = getTimestampByLogicStep( session, block->logicStepRange.first );
//            block->timestampRangeMillisec.second = getTimestampByLogicStep( session, block->logicStepRange.second );
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

                const int64_t differenceBetweenAdjacentTimestamps = m_state.settings.updateStepMillisec;
                const int64_t timeBetweenSessions = nextSession->minTimestampMillisec
                                                    - prevSession->maxTimestampMillisec
                                                    - differenceBetweenAdjacentTimestamps;

                _emptyStepCount = timeBetweenSessions / m_state.settings.updateStepMillisec;
            }
            else{
                // sessions is out
                return dataBlocks;
            }
        }
    }

    return dataBlocks;
}

int64_t DatasourceReader::getTimestampByLogicStep( const SEventsSessionInfo * _session, common_types::TLogicStep _logicStep ){

    auto iter = std::find_if( _session->steps.begin(), _session->steps.end(), FEqualSObjectStep(_logicStep) );
    if( iter != _session->steps.end() ){
        return ( * iter ).timestampMillisec;
    }

    return -1;
}

bool DatasourceReader::moveStepWindow( int64_t _stepFormal, int64_t & _currentPackHeadStep ){

    if( _stepFormal < _currentPackHeadStep ||
        _stepFormal >= (_currentPackHeadStep + PACKAGE_SIZE)
         ){

        _currentPackHeadStep = ( _stepFormal / PACKAGE_SIZE ) * PACKAGE_SIZE;

        return true;
    }

    return false;
}

bool DatasourceReader::read( common_types::TLogicStep _step ){

    if( moveStepWindow( _step, m_currentPackHeadStep ) && _step < m_state.stepsCount ){
        if( ! loadPackage( m_currentPackHeadStep, m_currentPlayingFrame ) ){
            return false;
        }
    }

    m_currentReadStep = _step;
    return true;
}

bool DatasourceReader::readInstant( common_types::TLogicStep _step ){

    if( ! loadSingleFrame(_step, m_currentInstantPlayingFrame) ){
        return false;
    }

    return true;
}

bool DatasourceReader::loadSingleFrame( common_types::TLogicStep _logicStep, TObjectsAtOneStep & _step ){

    _step.clear();

    const int64_t _currentPackHeadStep = ( _logicStep / PACKAGE_SIZE ) * PACKAGE_SIZE;

    auto iter = m_timelineBeacons.find( _currentPackHeadStep );
    if( iter != m_timelineBeacons.end() ){
        const SBeacon & beacon = iter->second;

        SPersistenceSetFilter filter( m_state.settings.persistenceSetId );

        getActualData( beacon, _logicStep, filter.sessionNum, filter.minLogicStep );
        if( common_vars::INVALID_SESSION_NUM == filter.sessionNum ){
            return false; // empty area
        }

        filter.maxLogicStep = filter.minLogicStep;

        TObjectsAtOneStep data = m_state.settings.databaseMgr->readTrajectoryData( filter );
        _step.swap( data );

        return true;
    }

    return false;
}

bool DatasourceReader::loadPackage( int64_t _currentPackHeadStep, std::vector<TObjectsAtOneStep> & _steps ){

    auto iter = m_timelineBeacons.find( _currentPackHeadStep );
    if( iter != m_timelineBeacons.end() ){
        const SBeacon & beacon = iter->second;

        // clear previous frame
        for( TObjectsAtOneStep & objStep : _steps ){
            objStep.clear();
        }

        int32_t positionCounter = 0;
        bool firstBlock = true; // NOTE: at the beginning '0' index grab ONE position from empty area ( [0] + 4 = 4 -> [0][1][2][3] and [4] for next data )

        for( const SBeacon::SDataBlock * block : beacon.dataBlocks ){

            // payload area
            if( ! block->empty ){
                SPersistenceSetFilter filter( m_state.settings.persistenceSetId );
                filter.sessionNum = block->sesNum;
                filter.minLogicStep = block->logicStepRange.first;
                filter.maxLogicStep = block->logicStepRange.second;

                assert( (filter.maxLogicStep - filter.minLogicStep) <= PACKAGE_SIZE );

                const TObjectsAtOneStep data = m_state.settings.databaseMgr->readTrajectoryData( filter );
                TLogicStep currentLogicStep = data.front().logicTime;

                for( const common_types::SPersistenceTrajectory & object : data ){

                    if( object.logicTime == currentLogicStep ){
                        TObjectsAtOneStep & readCell = _steps[ positionCounter ];
                        readCell.push_back( object );
                    }
                    else{
                        positionCounter += object.logicTime - currentLogicStep;
                        currentLogicStep = object.logicTime;
                        TObjectsAtOneStep & readCell = _steps[ positionCounter ];
                        readCell.push_back( object );
                    }
                }
            }
            // empty area
            else{
                positionCounter += block->emptyStepsCount;
                if( ! firstBlock ){
                    // NOTE: on first block '0' index gives additional position, further we should ourself get this 'additional' pos
                    positionCounter++;
                }
            }

            firstBlock = false;
        }
    }
    else{
        return false;
    }

    return true;
}

inline void DatasourceReader::getActualData(   const SBeacon & _beacon,
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

const DatasourceReader::TObjectsAtOneStep & DatasourceReader::getCurrentStep(){
    return m_currentPlayingFrame[ m_currentReadStep % PACKAGE_SIZE ];
}






















