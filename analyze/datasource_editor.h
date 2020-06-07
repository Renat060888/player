#ifndef DATASOURCE_EDITOR_H
#define DATASOURCE_EDITOR_H

#include <microservice_common/storage/database_manager_base.h>

#include "common/common_types.h"

class DatasourceEditor
{
public:
    struct SInitSettings {
        SInitSettings()
            : persistenceSetId(common_vars::INVALID_PERS_ID)
            , databaseMgr(nullptr)
            , descriptor(nullptr)
        {}

        common_types::TPersistenceSetId persistenceSetId;
        DatabaseManagerBase * databaseMgr;
        class DatasourceDescriptor * descriptor;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    DatasourceEditor();

    bool init( const SInitSettings & _settings );
    const SState & getState(){ return m_state; }

    void eraseRangeFromHead( const common_types::TTimeRangeMillisec & _timeRangeMillisec );
    void eraseRangeFromTail( const common_types::TTimeRangeMillisec & _timeRangeMillisec );

    // TODO: total erase


private:
    // data
    SState m_state;

    // service



};

#endif // DATASOURCE_EDITOR_H
