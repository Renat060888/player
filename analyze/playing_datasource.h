#ifndef PLAYING_DATASOURCE_H
#define PLAYING_DATASOURCE_H

#include <microservice_common/storage/database_manager_base.h>

#include "common/common_types.h"

// TODO: template ( what if objects kinds is many ? ) - trajectory, weather, etc...

class PlayingDatasource
{
public:
    // TODO ^
    using TObjectsAtOneStep = std::vector<common_types::SPersistenceTrajectory>;

    struct SBeacon {
        struct SDataBlock {
            SDataBlock()
                : empty(false)
                , emptyStepsCount(0)
                , sesNum(0)
            {}
            bool empty;
            int32_t emptyStepsCount;
            common_types::TSessionNum sesNum;
            std::pair<common_types::TLogicStep, common_types::TLogicStep> logicStepRange;
            std::pair<int64_t, int64_t> timestampRangeMillisec;
        };        

        std::vector<SDataBlock *> dataBlocks;
    };

    struct SStepCounter {
        SStepCounter()
            : logicStepCounter(0)
            , sesNum(0)
        {}
        common_types::TLogicStep logicStepCounter;
        common_types::TSessionNum sesNum;
    };

    struct SInitSettings {
        SInitSettings()
            : persistenceSetId(-1)
            , updateStepMillisec( 1000 / 24 ) // 24 fps ~ 41 ms
        {}

        common_types::TContextId ctxId;
        common_types::TPersistenceSetId persistenceSetId;
        int64_t updateStepMillisec;
    };

    struct SState {
        SState()
            : stepsCount(0)
            , settings(nullptr)
        {}
        // original data
        std::vector<SBeacon::SDataBlock *> dataRangesInfo;
        // derivative data
        std::vector<SBeacon::SDataBlock *> payloadDataRangesInfo;
        common_types::TTimeRangeMillisec globalTimeRangeMillisec;
        int64_t stepsCount;
        // refs
        SInitSettings * settings;
    };

    PlayingDatasource();
    ~PlayingDatasource();

    // TODO: may be very slow & work or not ?
    bool operator>( PlayingDatasource & _rhs ){
        return ( this->getState().globalTimeRangeMillisec.first
                 > _rhs.getState().globalTimeRangeMillisec.first );
    }

    bool init( const SInitSettings & _settings );
    const SState & getState();

    bool read( common_types::TLogicStep _step );
    bool readInstant( common_types::TLogicStep _step );
    const TObjectsAtOneStep & getCurrentStep();

private:
    // reading
    bool moveStepWindow( int64_t _stepFormal, int64_t & _currentPackHeadStep );

    // data
    bool loadPackage( int64_t _currentPackHeadStep, std::vector<TObjectsAtOneStep> & _steps );

    // metadata
    bool createBeacons( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons );
    std::vector<common_types::SEventsSessionInfo> checkSessionsForEmptyFrames( const std::vector<common_types::SEventsSessionInfo> & _sessionInfo );
    int64_t getTimestampByLogicStep( const common_types::SEventsSessionInfo * _session, common_types::TLogicStep _logicStep );
    void setTimestampToEmptyAreas( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons );
    std::vector<SBeacon::SDataBlock *> createBlocks( const std::vector<common_types::SEventsSessionInfo> & _sessionsInfo,
                                                   SStepCounter & _currentPassedPoint,
                                                   std::vector<common_types::SEventsSessionInfo>::size_type &_currentSessionIdx,
                                                   int32_t & _emptyStepCount );
    bool loadSingleFrame( common_types::TLogicStep _logicStep, TObjectsAtOneStep & _step );
    inline void getActualData(  const SBeacon & _beacon,
                                const common_types::TLogicStep _logicFormalStep,
                                common_types::TSessionNum & _sesNum,
                                common_types::TLogicStep & _logicActualStep );
    void fillState( const std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons, SState & _state );
    void destroyBeacons( std::unordered_map<common_types::TLogicStep, SBeacon> & _beacons );



    // data
    SInitSettings m_settings;
    SState m_state;
    std::vector<TObjectsAtOneStep> m_currentPlayingFrame;
    TObjectsAtOneStep m_currentInstantPlayingFrame;
    std::unordered_map<common_types::TLogicStep, SBeacon> m_timelineBeacons;
    common_types::TLogicStep m_currentPackHeadStep;
    int64_t m_currentReadStep;

    // service
    DatabaseManagerBase * m_database;
};

#endif // PLAYING_DATASOURCE_H

