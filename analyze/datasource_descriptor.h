#ifndef DATASOURCE_DESCRIPTOR_H
#define DATASOURCE_DESCRIPTOR_H

#include <microservice_common/storage/database_manager_base.h>

#include "common/common_types.h"

class DatasourceDescriptor
{
public:
    struct SInitSettings {
        SInitSettings()
            : databaseMgr(nullptr)
            , persId(common_vars::INVALID_PERS_ID)
            , sessionGapsThreshold(0)
            , updateStepMillisec(0)
        {}
        DatabaseManagerBase * databaseMgr;
        common_types::TPersistenceSetId persId;
        int sessionGapsThreshold;
        int64_t updateStepMillisec;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    DatasourceDescriptor();

    bool init( const SInitSettings & _settings );
    const SState & getState(){ return m_state; }

    bool isDescriptionChanged();
    std::vector<common_types::SEventsSessionInfo> getSessionsDescription();
    void clearDescription();

    int64_t getTimestampByLogicStep( common_types::TSessionNum _sessionNum, common_types::TLogicStep _logicStep );
    void getLogicStepByTimestamp( int64_t _timestamp, common_types::TSessionNum & _sessionNum, common_types::TLogicStep & _logicStep );


private:
    std::vector<common_types::SEventsSessionInfo> checkSessionsForEmptyFrames( const std::vector<common_types::SEventsSessionInfo> & _sessionInfo );


    // data
    SState m_state;

    // service

};

#endif // DATASOURCE_DESCRIPTOR_H
