
#include <microservice_common/system/logger.h>

#include "datasource_descriptor.h"

using namespace std;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "DatasrcDescriptor:";

// NOTE: decriptor acts only as MIRROR, it does not change description by itself - only updates based on payload

// functors >
struct FEqualSEventsSessionInfo {
    FEqualSEventsSessionInfo( TSessionNum _sessionNum )
        : sessionNum(_sessionNum)
    {}

    bool operator()( const SEventsSessionInfo & _rhs ) const {
        return ( this->sessionNum == _rhs.number );
    }

    TSessionNum sessionNum;
};


// functors <

DatasourceDescriptor::DatasourceDescriptor()
{

}

bool DatasourceDescriptor::init( const SInitSettings & _settings ){

    m_state.settings = _settings;




    return true;
}

bool DatasourceDescriptor::isDescriptionChanged(){

    DatabaseManagerBase * db = m_state.settings.databaseMgr;
    const vector<SEventsSessionInfo> storedSessionInfo = db->selectSessionDescriptions( m_state.settings.persId );
    if( storedSessionInfo.empty() ){
        return false;
    }

    // check HEAD of payload with current descr ( & update if necessary )
    const SEventsSessionInfo scannedHeadSession = db->scanPayloadHeadForSessions( m_state.settings.persId );
    const SEventsSessionInfo & storedBeginSession = storedSessionInfo.front();

    if( (scannedHeadSession.number != storedBeginSession.number)
            || (scannedHeadSession.minLogicStep != storedBeginSession.minLogicStep)
            || (scannedHeadSession.maxLogicStep != storedBeginSession.maxLogicStep) ){

        return true;
    }

    // check TAIL of payload with current descr ( & update if necessary )
    const SEventsSessionInfo scannedTailSession = db->scanPayloadTailForSessions( m_state.settings.persId );
    const SEventsSessionInfo & storedEndSession = storedSessionInfo.back();

    if( (scannedTailSession.number != storedEndSession.number)
            || (scannedTailSession.minLogicStep != storedEndSession.minLogicStep)
            || (scannedTailSession.maxLogicStep != storedEndSession.maxLogicStep) ){

        return true;
    }

    return false;
}

std::vector<common_types::SEventsSessionInfo> DatasourceDescriptor::getSessionsDescription(){

    DatabaseManagerBase * db = m_state.settings.databaseMgr;
    vector<SEventsSessionInfo> out;

    // NOTE: changes always only to RIGHT (->) direction
    const vector<SEventsSessionInfo> storedSessionInfo = db->selectSessionDescriptions( m_state.settings.persId );

    // description just created
    if( storedSessionInfo.empty() ){
        const TSessionNum from = std::numeric_limits<TSessionNum>::min();
        const TSessionNum to = std::numeric_limits<TSessionNum>::max();
        out = db->scanPayloadRangeForSessions( m_state.settings.persId, {from, to} );

        for( const SEventsSessionInfo & info : out ){
            assert( db->insertSessionDescription( m_state.settings.persId, info ) );
        }

        return out;
    }

    // check HEAD of payload with current descr ( & update if necessary )
    const SEventsSessionInfo scannedHeadSession = db->scanPayloadHeadForSessions( m_state.settings.persId );
    const SEventsSessionInfo & storedBeginSession = storedSessionInfo.front();

    if( (scannedHeadSession.number != storedBeginSession.number)
            || (scannedHeadSession.minLogicStep != storedBeginSession.minLogicStep)
            || (scannedHeadSession.maxLogicStep != storedBeginSession.maxLogicStep) ){

        const vector<SEventsSessionInfo> freshScannedHeadSessionInfo = db->scanPayloadRangeForSessions( m_state.settings.persId, {scannedHeadSession.number, storedBeginSession.number} );

        // TODO: do
    }

    // check TAIL of payload with current descr ( & update if necessary )
    const SEventsSessionInfo scannedTailSession = db->scanPayloadTailForSessions( m_state.settings.persId );
    const SEventsSessionInfo & storedEndSession = storedSessionInfo.back();

    if( (scannedTailSession.number != storedEndSession.number)
            || (scannedTailSession.minLogicStep != storedEndSession.minLogicStep)
            || (scannedTailSession.maxLogicStep != storedEndSession.maxLogicStep) ){

        if( scannedTailSession.number == storedEndSession.number){
            // updated only current session
            assert( db->updateSessionDescription( m_state.settings.persId, scannedTailSession ) );
        }
        else{
            // there is new sessions created besides current one
            const vector<SEventsSessionInfo> freshScannedTailSessionInfo = db->scanPayloadRangeForSessions( m_state.settings.persId,
                    {storedEndSession.number, scannedTailSession.number} );

            for( const SEventsSessionInfo & info : freshScannedTailSessionInfo ){
                if( info.number != storedEndSession.number ){
                    assert( db->insertSessionDescription( m_state.settings.persId, info ) );
                }
                else{
                    assert( db->updateSessionDescription( m_state.settings.persId, info ) );
                }
            }
        }
    }

    // return descr
    return db->selectSessionDescriptions( m_state.settings.persId );
}

void DatasourceDescriptor::clearDescription(){

    m_state.settings.databaseMgr->deleteSessionDescription( m_state.settings.persId );
}

int64_t DatasourceDescriptor::getTimestampByLogicStep( common_types::TSessionNum _sessionNum, common_types::TLogicStep _logicStep ){

    const vector<SEventsSessionInfo> storedSessionInfo = m_state.settings.databaseMgr->selectSessionDescriptions( m_state.settings.persId );
    auto iter = std::find_if( storedSessionInfo.begin(), storedSessionInfo.end(), FEqualSEventsSessionInfo(_sessionNum) );
    if( iter != storedSessionInfo.end() ){
        const SEventsSessionInfo & sessionInfo = ( * iter );
        if( _logicStep >= sessionInfo.minLogicStep && _logicStep <= sessionInfo.maxLogicStep ){
            return ( sessionInfo.minTimestampMillisec + ((_logicStep - sessionInfo.minLogicStep) * m_state.settings.updateStepMillisec) );
        }
        else{
            VS_LOG_WARN << PRINT_HEADER << " logic step: " << _logicStep
                        << " not hit in range: " << sessionInfo.minLogicStep << " " << sessionInfo.maxLogicStep
                        << endl;
            return -1;
        }
    }
    else{
        VS_LOG_WARN << PRINT_HEADER << " such session number: " << _sessionNum
                    << " and logic step: " << _logicStep
                    << " not found for pers id: " << m_state.settings.persId
                    << " total count in db: " << storedSessionInfo.size()
                    << endl;

        return -1;
    }
}

void DatasourceDescriptor::getLogicStepByTimestamp( int64_t _timestamp, common_types::TSessionNum & _sessionNum, common_types::TLogicStep & _logicStep ){

    // TODO: do
}

std::vector<SEventsSessionInfo> DatasourceDescriptor::checkSessionsForEmptyFrames( const std::vector<SEventsSessionInfo> & _sessionInfo ){

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





