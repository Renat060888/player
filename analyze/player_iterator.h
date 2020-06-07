#ifndef PLAYER_ITERATOR_H
#define PLAYER_ITERATOR_H

#include "common/common_types.h"

class PlayerStepIterator{
public:
    enum class EPlayMode {
        NORMAL,
        REVERSE,
        UNDEFINED
    };

    enum class ESpeedPolicy {
        TIME_STEP_CHANGE,
        LOGIC_STEP_THINNING,
        UNDEFINED
    };

    struct SInitSettings {
        SInitSettings()
            : _stepUpdateMillisec(0)
            , _totalSimulSteps(0)
            , speedPolicy(ESpeedPolicy::TIME_STEP_CHANGE)
        {}
        int32_t _stepUpdateMillisec;
        int64_t _totalSimulSteps;
        common_types::TTimeRangeMillisec _globalRange;
        ESpeedPolicy speedPolicy;
    };

    struct SState {
        SState()
            : m_loopMode(false)
            , m_directionMode(EPlayMode::NORMAL)
            , m_currentPlayStep(0)
            , m_totalStepsCount(0)
            , m_updateGenerationMillisec(0)
        {}

        bool m_loopMode;
        EPlayMode m_directionMode;

        int64_t m_currentPlayStep;
        int64_t m_totalStepsCount;
        std::pair<int64_t, int64_t> m_stepBound;

        int64_t m_updateGenerationMillisec;
        int64_t m_updateDelayMillisec;
        common_types::TTimeRangeMillisec m_globalTimeRangeMillisec;
        common_types::TTimeRangeMillisec m_currentTimeRangeMillisec;

        SInitSettings m_settings;
        std::string m_lastError;
    };

    PlayerStepIterator();
    ~PlayerStepIterator();

    // TODO: remove
    void setDelayTime( int64_t _delay ){ m_state.m_updateDelayMillisec = _delay; }

    // system
    bool init( SInitSettings _settings );
    const SState & getState() const { return m_state; }

    // time range
    bool changeRange( const common_types::TTimeRangeMillisec & _range );
    void resetRange();

    // step
    bool goNextStep();
    bool setStep( int64_t _stepNum );
    void resetStep();

    // mode
    void setDirectionMode( const EPlayMode _playMode );
    void setLoopMode( bool _loop );

    // speed
    bool increaseSpeed();
    bool decreaseSpeed();
    void normalizeSpeed();


private:
    inline bool tryNextStep();
    inline bool tryPrevStep();

    inline bool isNextStepExist();
    inline bool isPrevStepExist();

    void printState();

    // data
    SState m_state;
    int64_t m_stepSize;
    int m_currentUpdateTimeStepIdx;
    int m_originalUpdateTimeStepIdx;
};

#endif // PLAYER_ITERATOR_H
