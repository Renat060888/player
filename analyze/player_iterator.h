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

    struct SPlayerIteratorState {
        SPlayerIteratorState()
            : loop(false)
            , m_playMode(EPlayMode::NORMAL)
            , m_currentPlayStep(0)
            , m_totalStepsCount(0)
            , m_updateGenerationMillisec(0)
        {}

        bool loop;
        EPlayMode m_playMode;

        int64_t m_currentPlayStep;
        int64_t m_totalStepsCount;
        std::pair<int64_t, int64_t> m_stepBound;

        int64_t m_updateGenerationMillisec;
        int64_t m_updateDelayMillisec;
        common_types::TTimeRangeMillisec m_globalTimeRangeMillisec;
        common_types::TTimeRangeMillisec m_currentTimeRangeMillisec;
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

    PlayerStepIterator();
    ~PlayerStepIterator();

    // system
    bool init( SInitSettings _settings );
    const SPlayerIteratorState & getState() const { return m_state; }
    std::string & getLastError(){ return m_lastError; }

    // time range
    bool changeRange( const common_types::TTimeRangeMillisec & _range );
    void resetRange();

    // step
    bool goNextStep();
    int64_t getCurrentStep();
    bool setStep( int64_t _stepNum );
    void resetStep();

    // mode
    void setPlayMode( const EPlayMode _playMode );
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

    SInitSettings m_settings;
    std::string m_lastError;
    int m_currentUpdateTimeStepIdx;
    int m_originalUpdateTimeStepIdx;
    SPlayerIteratorState m_state;
    int64_t m_stepSize;
};

#endif // PLAYER_ITERATOR_H
