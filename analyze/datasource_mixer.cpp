
#include <microservice_common/system/logger.h>

#include "datasource_mixer.h"

using namespace std;

DatasourceMixer::DatasourceMixer()
{

}

DatasourceMixer::~DatasourceMixer()
{

}

bool DatasourceMixer::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

    if( _settings.datasources.empty() ){
        m_state.inited = true;
        return true;
    }

    if( ! mixDatasources(_settings.datasources) ){
        VS_LOG_ERROR << "TODO: print" << endl;
        return false;
    }

    m_state.inited = true;
    return true;
}

struct FunctorCompareDatasources {
    bool operator()( DatasourceReader * _d1, DatasourceReader * _d2 ){
        return _d1->getState().globalTimeRangeMillisec.first < _d2->getState().globalTimeRangeMillisec.first;
    }
};

const DatasourceMixer::SState & DatasourceMixer::getState(){

    if( m_state.settings.datasources.empty() ){
        return m_state;
    }

    if( 1 == m_state.settings.datasources.size() ){
        DatasourceReader * firstDatasource = m_state.settings.datasources[ 0 ];
        const DatasourceReader::SState & state = firstDatasource->getState();

        m_state.globalDataRangeMillisec = state.globalTimeRangeMillisec;
        m_state.globalStepCount = state.stepsCount;
        m_state.globalStepUpdateMillisec = state.settings->updateStepMillisec;
    }
    else{
        std::sort( m_state.settings.datasources.begin(), m_state.settings.datasources.end(), FunctorCompareDatasources() );

        int64_t totalUpdateStepMillisec = 0;

        for( std::size_t i = 0; i < m_state.settings.datasources.size(); i++ ){
            DatasourceReader * datasource = m_state.settings.datasources[ i ];

            const DatasourceReader::SState & state = datasource->getState();

            if( 0 == i ){
                m_state.globalDataRangeMillisec.first = state.globalTimeRangeMillisec.first;
            }

            if( i == (m_state.settings.datasources.size() - 1) ){
                m_state.globalDataRangeMillisec.second = state.globalTimeRangeMillisec.second;
            }

            totalUpdateStepMillisec += state.settings->updateStepMillisec;
        }

        m_state.globalStepUpdateMillisec = ( totalUpdateStepMillisec / m_state.settings.datasources.size() ); // avg
        m_state.globalStepCount = ( m_state.globalDataRangeMillisec.second - m_state.globalDataRangeMillisec.first )
                                  / m_state.globalStepUpdateMillisec;
    }

    return m_state;
}

bool DatasourceMixer::mixDatasources( std::vector<DatasourceReader *> _datasources ){

    if( 1 == _datasources.size() ){
        DatasourceReader * datasource = _datasources[ 0 ];

        SMixerTrack track;
        track.logicStepOffset = 0;
        track.src = datasource;
        m_mixerTracks.push_back( track );

        return true;
    }

    // sort by the most left bound
    std::sort( _datasources.begin(), _datasources.end(), FunctorCompareDatasources() );

    // push off from 1st sorted datasource
    DatasourceReader * firstDatasource = _datasources[ 0 ];

    SMixerTrack track;
    track.logicStepOffset = 0;
    track.src = firstDatasource;
    m_mixerTracks.push_back( track );

    //
    const DatasourceReader::SState & state = firstDatasource->getState();
    const int64_t firstDatasrcLeftBoundMillisec = state.globalTimeRangeMillisec.first;
    const int64_t updateStepMillisec = state.settings->updateStepMillisec;

    // mix
    for( std::size_t i = 1; i < _datasources.size(); i++ ){
        DatasourceReader * datasource = _datasources[ i ];

        const DatasourceReader::SState & state = datasource->getState();
        const int64_t leftBoundMillisec = state.globalTimeRangeMillisec.first;

        SMixerTrack track;
        track.logicStepOffset = - ( leftBoundMillisec - firstDatasrcLeftBoundMillisec ) / updateStepMillisec;
        track.src = datasource;
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

        if( ! track.src->read( correctedStep ) ){
            continue;
        }

        const DatasourceReader::TObjectsAtOneStep & data = track.src->getCurrentStep();
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

        if( ! track.src->readInstant( correctedStep ) ){
            continue;
        }

        const DatasourceReader::TObjectsAtOneStep & data = track.src->getCurrentStep();
        m_currentStepFromDatasources.insert( m_currentStepFromDatasources.end(), data.begin(), data.end() );
    }

    return true;
}

const DatasourceReader::TObjectsAtOneStep & DatasourceMixer::getCurrentStep() const {
    return m_currentStepFromDatasources;
}
