#ifndef PLAYER_H
#define PLAYER_H

#include <thread>
#include <condition_variable>

#ifdef OBJREPR_LIBRARY_EXIST
#include <objrepr/reprServer.h>
#endif

#include "common/common_types.h"
#include "player_iterator.h"
#include "datasource_descriptor.h"
#include "datasource_reader.h"
#include "datasource_editor.h"
#include "datasource_mixer.h"

class PlayerWorker
{
public:
    struct SDatasourceServices {
        SDatasourceServices()
            : descriptor(nullptr)
            , reader(nullptr)
            , editor(nullptr)
            , databaseMgr(nullptr)
            , firstDatabaseCurrent(true)
        {}
        DatasourceDescriptor * descriptor; // no need to recreate ( self updating )
        DatasourceReader * reader; // recreate is needed due to high complexity ( and therefore sophisticated state )
        DatasourceEditor * editor; // TODO: recreate ?
        DatabaseManagerBase * databaseMgr; // no need to recreate ( stable connection )
        DatabaseManagerBase * databaseMgrSecond; // for new reader creation in parallel thread
        bool firstDatabaseCurrent;
    };
    using PDatasourceServices = std::shared_ptr<SDatasourceServices>;

    struct SInitSettings {
        SInitSettings()
            : ctxId(common_vars::INVALID_CONTEXT_ID)
            , datasourcesUpdateIntervalMilllisec(1000 * 10)
        {}
        common_types::TContextId ctxId;
        int64_t datasourcesUpdateIntervalMilllisec;
    };

    struct SState {
        SState()
            : playingStatus(common_types::EPlayerStatus::UNDEFINED)
            , playIterator(nullptr)
            , mixer(nullptr)
        {}
        common_types::EPlayerStatus playingStatus;
        SInitSettings settings;
        PlayerStepIterator * playIterator;
        DatasourceMixer * mixer;
        std::string lastError;
    };

    struct SPlayableObject {
#ifdef OBJREPR_LIBRARY_EXIST
        objrepr::SpatialObjectPtr objreprObject;
#endif
    };

    PlayerWorker();
    ~PlayerWorker();

    bool init( const SInitSettings & _settings );
    const SState & getState();

    void start();
    void pause();
    void stop();
    bool stepForward();
    bool stepBackward();

    bool setRange( const common_types::TTimeRangeMillisec & _range );
    void switchReverseMode( bool _reverse );
    void switchLoopMode( bool _loop );
    bool playFromPosition( int64_t _stepMillisec );

    bool increasePlayingSpeed();
    bool decreasePlayingSpeed();
    void normalizePlayingSpeed();


private:
    void threadPlaying();
    void threadDatasourceMonitoring();

    bool isDataForPlayingExist();
    void checkForChangedDatasources();
    void checkForNewDatasources();

    bool preparePlayingInfrastructure();
    bool createDatasourceServices( const SInitSettings & _settings, std::vector<DatasourceReader *> & _datasrc );
    bool createDatasource( common_types::TPersistenceSetId _persId, int64_t _timeStepIntervalMillisec, PlayerWorker::PDatasourceServices & _datasrc );

    inline void playObjects( const DatasourceReader::TObjectsAtOneStep & _objectsStep );
    inline void emitStep( common_types::TLogicStep _step );
    inline void emitStepInstant( common_types::TLogicStep _step );
    void playingLoop();
    bool playOneStep();

    void hideFutureObjects();

    // data    
    SState m_state;
    std::unordered_map<common_types::TObjectId, SPlayableObject> m_playingObjects;
    std::unordered_map<common_types::TPersistenceSetId, PDatasourceServices> m_datasrcByPersId;
    bool m_dataForPlayingExist;

    // service
    PlayerStepIterator m_playIterator;
    DatasourceMixer m_datasourcesMixer;
    std::thread * m_threadPlaying;
    std::thread * m_threadDatasourceMonitoring;
    std::condition_variable m_cvDatasourceMonitoringSleep;
    std::condition_variable m_cvPlayStartEvent;
    std::mutex m_muEmitStepProtect;
    DatabaseManagerBase * m_database;
#ifdef OBJREPR_LIBRARY_EXIST
    objrepr::SpatialObjectManager * m_objectManager;
#endif
};

#endif // PLAYER_H
