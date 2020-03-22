
#include <microservice_common/system/logger.h>

#include "common/common_utils.h"

#include "player_iterator.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "PlayIter:";
static constexpr std::array<int64_t, 8> UPDATE_TIME_STEPS_MILLISEC = { 8000, 4000, 2000, 1000, 500, 250, 100, 50 };

PlayerStepIterator::PlayerStepIterator()
    : m_currentUpdateTimeStepIdx(0)
    , m_originalUpdateTimeStepIdx(0)
    , m_stepSize(1)
{

}

PlayerStepIterator::~PlayerStepIterator(){

}

void PlayerStepIterator::printState(){

    VS_LOG_DBG << PRINT_HEADER << " =============== state ===============" << endl;
    VS_LOG_DBG << PRINT_HEADER
               << " begin step " << m_state.m_stepBound.first
               << " end step " << m_state.m_stepBound.second
               << " update step millisec " <<  m_state.m_updateGenerationMillisec
               << endl;
    VS_LOG_DBG << PRINT_HEADER << " current step " << m_state.m_currentPlayStep << endl;
    VS_LOG_DBG << PRINT_HEADER
               << " current begin range " << common_utils::timeMillisecToStr( m_state.m_currentTimeRangeMillisec.first )
               << " end range " << common_utils::timeMillisecToStr( m_state.m_currentTimeRangeMillisec.second )
               << endl;
    VS_LOG_DBG << PRINT_HEADER
               << " global begin range " << common_utils::timeMillisecToStr( m_state.m_globalTimeRangeMillisec.first )
               << " end range " << common_utils::timeMillisecToStr( m_state.m_globalTimeRangeMillisec.second )
               << endl;
    VS_LOG_DBG << PRINT_HEADER << " =============== state ===============" << endl;
}

bool PlayerStepIterator::init( SInitSettings _settings ){

    m_settings = _settings;

    m_state.m_globalTimeRangeMillisec = _settings._globalRange;
    m_state.m_currentTimeRangeMillisec = m_state.m_globalTimeRangeMillisec;

    // TODO: what for ?
    if( -1 == _settings._totalSimulSteps ){
        m_state.m_totalStepsCount = std::numeric_limits<int64_t>::max();
    }
    else{
        m_state.m_totalStepsCount = _settings._totalSimulSteps;
    }

    m_state.m_stepBound.first = 0;
    m_state.m_stepBound.second = m_state.m_totalStepsCount - 1;

//    string error;
//    if( ! common_utils::checkUpdateTimeValue(_stepUpdateMillisec, error) && _stepUpdateMillisec != 0 ){
//        stringstream ss;
//        ss << " set quant interval fail [" << error << "]";
//        VS_LOG_INFO << PRINT_HEADER << ss.str() << endl;
//        m_lastError = ss.str();
//        return false;
//    }

    m_state.m_updateGenerationMillisec     = _settings._stepUpdateMillisec;
    m_state.m_updateDelayMillisec     = _settings._stepUpdateMillisec;

    for( int i = 0; i < UPDATE_TIME_STEPS_MILLISEC.size(); i++ ){
        if( UPDATE_TIME_STEPS_MILLISEC[ i ] == m_state.m_updateGenerationMillisec ){
            m_currentUpdateTimeStepIdx = i;
            m_originalUpdateTimeStepIdx = i;
            break;
        }
    }

    printState();
    VS_LOG_INFO << PRINT_HEADER << " init success" << endl;

    return true;
}

bool PlayerStepIterator::increaseSpeed(){

    if( ESpeedPolicy::TIME_STEP_CHANGE == m_settings.speedPolicy ){
        if( (m_currentUpdateTimeStepIdx+1) == UPDATE_TIME_STEPS_MILLISEC.size() ){
            m_lastError = "min step value (millisec): " + to_string( UPDATE_TIME_STEPS_MILLISEC[ m_currentUpdateTimeStepIdx ] );
            return false;
        }
        else{
            m_currentUpdateTimeStepIdx++;
            m_state.m_updateDelayMillisec = UPDATE_TIME_STEPS_MILLISEC[ m_currentUpdateTimeStepIdx ];
            return true;
        }
    }
    else if( ESpeedPolicy::LOGIC_STEP_THINNING == m_settings.speedPolicy ){
        m_stepSize *= 2;
    }
    else{
        assert( false && "WTF?" );
    }

    return true;
}

bool PlayerStepIterator::decreaseSpeed(){

    if( ESpeedPolicy::TIME_STEP_CHANGE == m_settings.speedPolicy ){
        if( 0 == m_currentUpdateTimeStepIdx ){
            m_lastError = "max step value (millisec): " + to_string( UPDATE_TIME_STEPS_MILLISEC[ m_currentUpdateTimeStepIdx ] );
            return false;
        }
        else{
            m_currentUpdateTimeStepIdx--;
            m_state.m_updateDelayMillisec = UPDATE_TIME_STEPS_MILLISEC[ m_currentUpdateTimeStepIdx ];
            return true;
        }
    }
    else if( ESpeedPolicy::LOGIC_STEP_THINNING == m_settings.speedPolicy ){
        if( m_stepSize != 1 ){
            m_stepSize /= 2;
        }
    }
    else{
        assert( false && "WTF?" );
    }

    return true;
}

void PlayerStepIterator::normalizeSpeed(){

    m_currentUpdateTimeStepIdx = m_originalUpdateTimeStepIdx;
    m_state.m_updateDelayMillisec = UPDATE_TIME_STEPS_MILLISEC[ m_currentUpdateTimeStepIdx ];
}

bool PlayerStepIterator::changeRange( const common_types::TTimeRangeMillisec & _range ){

    const int64_t globalRangeTime = m_state.m_globalTimeRangeMillisec.second - m_state.m_globalTimeRangeMillisec.first;
    const int64_t oneStepDurationMillisec = globalRangeTime / m_state.m_totalStepsCount;

    const int64_t timeUntilBeginMillisec = _range.first - m_state.m_globalTimeRangeMillisec.first;
    const int64_t timeUntilEndMillisec = _range.second - m_state.m_globalTimeRangeMillisec.first;

    const int64_t stepsUntilBegin = timeUntilBeginMillisec / oneStepDurationMillisec;
    const int64_t stepsUntilEnd = timeUntilEndMillisec / oneStepDurationMillisec;

    m_state.m_stepBound.first = stepsUntilBegin;
    m_state.m_stepBound.second = stepsUntilEnd;
    m_state.m_currentTimeRangeMillisec = _range;
    m_state.m_currentPlayStep = m_state.m_stepBound.first;

    VS_LOG_INFO << PRINT_HEADER << " range changed" << endl;
    printState();

    return true;
}

void PlayerStepIterator::resetRange(){

    m_state.m_stepBound.first = 0;
    m_state.m_stepBound.second = m_state.m_totalStepsCount;
    m_state.m_currentTimeRangeMillisec = m_state.m_globalTimeRangeMillisec;
    m_state.m_currentPlayStep = m_state.m_stepBound.first;
}

void PlayerStepIterator::resetStep(){

    m_state.m_currentPlayStep = m_state.m_stepBound.first;
}

void PlayerStepIterator::setPlayMode( const EPlayMode _playMode ){

    m_state.m_playMode = _playMode;
}

void PlayerStepIterator::setLoopMode( bool _loop ){
    m_state.loop = _loop;
}

bool PlayerStepIterator::setStep( int64_t _stepNum ){

    if( _stepNum >= m_state.m_stepBound.first && _stepNum < m_state.m_stepBound.second ){
        m_state.m_currentPlayStep = _stepNum;
        return true;
    }
    else{
        VS_LOG_ERROR << PRINT_HEADER
                     << " out of range. Begin: " << m_state.m_stepBound.first << " "
                     << " End: " << m_state.m_stepBound.second << " "
                     << " Requested: " << _stepNum
                     << endl;

        return false;
    }
}

bool PlayerStepIterator::isNextStepExist(){

    if( ! m_state.loop ){
        return ( (m_state.m_currentPlayStep + m_stepSize ) < m_state.m_stepBound.second );
    }
    else{
        if( (m_state.m_currentPlayStep + m_stepSize ) < m_state.m_stepBound.second ){
            return true;
        }
        else{
            m_state.m_currentPlayStep = m_state.m_stepBound.first;
            return true;
        }
    }
}

bool PlayerStepIterator::isPrevStepExist(){

    if( ! m_state.loop ){
        return ( m_state.m_currentPlayStep > m_state.m_stepBound.first );
    }
    else{
        if( m_state.m_currentPlayStep > m_state.m_stepBound.first ){
            return true;
        }
        else{
            m_state.m_currentPlayStep = m_state.m_stepBound.second;
            return true;
        }
    }
}

bool PlayerStepIterator::tryNextStep(){

    if( ! isNextStepExist() ){
        return false;
    }

    m_state.m_currentPlayStep += m_stepSize; // TODO почему увеличение на 1 если выкачивается пачкой
    return true;
}

bool PlayerStepIterator::tryPrevStep(){

    if( ! isPrevStepExist() ){
        return false;
    }

    m_state.m_currentPlayStep -= m_stepSize;

    return true;
}

int64_t PlayerStepIterator::getCurrentStep(){
    return m_state.m_currentPlayStep;
}

bool PlayerStepIterator::goNextStep(){

    switch( m_state.m_playMode ){
    case EPlayMode::NORMAL: {
        return tryNextStep();
    }
    case EPlayMode::REVERSE: {
        return tryPrevStep();
    }
    default: {
        VS_LOG_ERROR << PRINT_HEADER << " go next step - unknown play mode " << (int)m_state.m_playMode << endl;
        return false;
    }
    }
}
