#ifndef DATASOURCE_MIXER_H
#define DATASOURCE_MIXER_H

#include "playing_datasource.h"

class DatasourceMixer
{
public:
    struct SInitSettings {
        SInitSettings()
        {}
        std::vector<PlayingDatasource *> datasources;
    };

    struct SState {
        SState()
            : inited(false)
            , globalStepCount(0)
            , globalStepUpdateMillisec(0)
            , settings(nullptr)
        {}
        //
        bool inited;
        // derivative
        common_types::TTimeRangeMillisec globalDataRangeMillisec;
        int64_t globalStepCount;
        int64_t globalStepUpdateMillisec;
        // refs
        SInitSettings * settings;
    };

    struct SMixerTrack {
        SMixerTrack()
            : logicStepOffset(0)
            , src(nullptr)
            , active(false)
        {}
        common_types::TLogicStep logicStepOffset;
        PlayingDatasource * src;
        bool active;
    };

    DatasourceMixer();
    ~DatasourceMixer();

//    DatasourceMixer( const DatasourceMixer & _rhs );

    bool init( const SInitSettings & _settings );
    const SState & getState();

    bool read( common_types::TLogicStep _step );
    bool readInstant( common_types::TLogicStep _step );
    const PlayingDatasource::TObjectsAtOneStep & getCurrentStep() const;

private:
    bool mixDatasources( std::vector<PlayingDatasource *> _datasources );


    // data
    SState m_state;
    SInitSettings m_settings;
    std::vector<SMixerTrack> m_mixerTracks;
    PlayingDatasource::TObjectsAtOneStep m_currentStepFromDatasources;

    // service


};

#endif // DATASOURCE_MIXER_H
