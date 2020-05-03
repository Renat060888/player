#ifndef DATASOURCE_MIXER_H
#define DATASOURCE_MIXER_H

#include "datasource_reader.h"

class DatasourceMixer
{
public:
    struct SInitSettings {
        SInitSettings()
        {}
        std::vector<DatasourceReader *> datasources;
    };

    struct SState {
        SState()
            : inited(false)
            , globalStepCount(0)
            , globalStepUpdateMillisec(0)
        {}
        //
        bool inited;
        // derivative
        common_types::TTimeRangeMillisec globalDataRangeMillisec;
        int64_t globalStepCount;
        int64_t globalStepUpdateMillisec;
        // original
        SInitSettings settings;
    };

    struct SMixerTrack {
        SMixerTrack()
            : logicStepOffset(0)
            , src(nullptr)
            , active(false)
        {}
        common_types::TLogicStep logicStepOffset;
        DatasourceReader * src;
        bool active;
    };

    DatasourceMixer();
    ~DatasourceMixer();

//    DatasourceMixer( const DatasourceMixer & _rhs );

    bool init( const SInitSettings & _settings );
    const SState & getState();

    bool read( common_types::TLogicStep _step );
    bool readInstant( common_types::TLogicStep _step );
    const DatasourceReader::TObjectsAtOneStep & getCurrentStep() const;

private:
    bool mixDatasources( std::vector<DatasourceReader *> _datasources );


    // data
    SState m_state;
    std::vector<SMixerTrack> m_mixerTracks;
    DatasourceReader::TObjectsAtOneStep m_currentStepFromDatasources;

    // service


};

#endif // DATASOURCE_MIXER_H
