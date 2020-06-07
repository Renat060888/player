
#include <microservice_common/system/logger.h>

#include "datasource_mixer.h"

using namespace std;

// functors >
struct FLessDatasourceReader {
    bool operator()( DatasourceReader * _d1, DatasourceReader * _d2 ){
        return _d1->getState().globalTimeRangeMillisec.first < _d2->getState().globalTimeRangeMillisec.first;
    }
};
// functors <

DatasourceMixer::DatasourceMixer()
{

}

DatasourceMixer::~DatasourceMixer()
{

}

bool DatasourceMixer::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

    if( _settings.datasourceReaders.empty() ){
        m_state.inited = true;
        return true;
    }

    if( ! mixDatasources(_settings.datasourceReaders) ){
        VS_LOG_ERROR << "TODO: print" << endl;
        return false;
    }

    fillState( m_state );

    m_state.inited = true;
    return true;
}

void DatasourceMixer::fillState( DatasourceMixer::SState & _state ){

    // empty context
    if( _state.settings.datasourceReaders.empty() ){
        return;
    }

    // just reflect state of single datasrc
    if( 1 == _state.settings.datasourceReaders.size() ){
        DatasourceReader * firstDatasource = _state.settings.datasourceReaders[ 0 ];
        const DatasourceReader::SState & state = firstDatasource->getState();

        _state.globalDataRangeMillisec = state.globalTimeRangeMillisec;
        _state.globalStepCount = state.stepsCount;
        _state.globalStepUpdateMillisec = state.settings.updateStepMillisec;
    }
    // composite state
    else{
        std::sort( _state.settings.datasourceReaders.begin(), _state.settings.datasourceReaders.end(), FLessDatasourceReader() );

        int64_t totalUpdateStepMillisec = 0;

        for( std::size_t i = 0; i < _state.settings.datasourceReaders.size(); i++ ){
            DatasourceReader * datasource = _state.settings.datasourceReaders[ i ];

            const DatasourceReader::SState & state = datasource->getState();

            if( 0 == i ){
                _state.globalDataRangeMillisec.first = state.globalTimeRangeMillisec.first;
            }

            if( i == (_state.settings.datasourceReaders.size() - 1) ){
                _state.globalDataRangeMillisec.second = state.globalTimeRangeMillisec.second;
            }

            totalUpdateStepMillisec += state.settings.updateStepMillisec;
        }

        _state.globalStepUpdateMillisec = ( totalUpdateStepMillisec / _state.settings.datasourceReaders.size() ); // avg
        _state.globalStepCount = ( _state.globalDataRangeMillisec.second - _state.globalDataRangeMillisec.first )
                                  / _state.globalStepUpdateMillisec;
    }
}

const DatasourceMixer::SState & DatasourceMixer::getState() const {

    return m_state;
}

bool DatasourceMixer::mixDatasources( std::vector<DatasourceReader *> _datasources ){

    if( 1 == _datasources.size() ){
        DatasourceReader * datasource = _datasources[ 0 ];

        SMixerTrack track;
        track.logicStepOffset = 0;
        track.datasrcReader = datasource;
        m_mixerTracks.push_back( track );

        return true;
    }

    // sort by the most left bound
    std::sort( _datasources.begin(), _datasources.end(), FLessDatasourceReader() );

    // push off from 1st sorted datasource
    DatasourceReader * firstDatasource = _datasources[ 0 ];

    SMixerTrack track;
    track.logicStepOffset = 0;
    track.datasrcReader = firstDatasource;
    m_mixerTracks.push_back( track );

    //
    const DatasourceReader::SState & state = firstDatasource->getState();
    const int64_t firstDatasrcLeftBoundMillisec = state.globalTimeRangeMillisec.first;
    const int64_t updateStepMillisec = state.settings.updateStepMillisec;

    // mix
    for( std::size_t i = 1; i < _datasources.size(); i++ ){
        DatasourceReader * datasource = _datasources[ i ];

        const DatasourceReader::SState & state = datasource->getState();
        const int64_t leftBoundMillisec = state.globalTimeRangeMillisec.first;

        SMixerTrack track;
        track.logicStepOffset = - ( leftBoundMillisec - firstDatasrcLeftBoundMillisec ) / updateStepMillisec;
        track.datasrcReader = datasource;
        m_mixerTracks.push_back( track );
    }

    return true;
}

bool DatasourceMixer::read( common_types::TLogicStep _step ){

    m_currentStepFromDatasources.clear();

    for( const SMixerTrack & track : m_mixerTracks ){

        const common_types::TLogicStep correctedStep = _step + track.logicStepOffset;
        if( correctedStep < 0 ){
            continue;
        }

        if( ! track.datasrcReader->read( correctedStep ) ){
            continue;
        }

        const DatasourceReader::TObjectsAtOneStep & data = track.datasrcReader->getCurrentStep();
        m_currentStepFromDatasources.insert( m_currentStepFromDatasources.end(), data.begin(), data.end() );
    }

    return true;
}

bool DatasourceMixer::readInstant( common_types::TLogicStep _step ){

    m_currentStepFromDatasources.clear();

    for( const SMixerTrack & track : m_mixerTracks ){

        const common_types::TLogicStep correctedStep = _step + track.logicStepOffset;
        if( correctedStep < 0 ){
            continue;
        }

        if( ! track.datasrcReader->readInstant( correctedStep ) ){
            continue;
        }

        const DatasourceReader::TObjectsAtOneStep & data = track.datasrcReader->getCurrentStep();
        m_currentStepFromDatasources.insert( m_currentStepFromDatasources.end(), data.begin(), data.end() );
    }

    return true;
}

const DatasourceReader::TObjectsAtOneStep & DatasourceMixer::getCurrentStep() const {
    return m_currentStepFromDatasources;
}







