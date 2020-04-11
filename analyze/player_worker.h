#ifndef PLAYER_H
#define PLAYER_H

//#define OBJREPR_LIBRARY_EXIST

#include <thread>
#include <condition_variable>

#ifdef OBJREPR_LIBRARY_EXIST
#include <objrepr/reprServer.h>
#endif

#include "common/common_types.h"
#include "player_iterator.h"
#include "playing_datasource.h"
#include "datasource_mixer.h"

class PlayerWorker
{
public:    
    struct SInitSettings {
        common_types::TContextId ctxId;
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
        std::string m_lastError;
    };

    struct SPlayableObject {
#ifdef OBJREPR_LIBRARY_EXIST
        objrepr::SpatialObjectPtr objreprObject;
#endif
    };

    PlayerWorker();
    ~PlayerWorker();

    bool init( const SInitSettings & _settings );
    void * getState();

    void start();
    void pause();
    void stop();
    bool stepForward();
    bool stepBackward();

    bool setRange( const common_types::TTimeRangeMillisec & _range );
    void switchReverseMode( bool _reverse );
    void switchLoopMode( bool _loop );
    bool playFromPosition( int64_t _stepMillisec );
    bool updatePlayingData();

    bool increasePlayingSpeed();
    bool decreasePlayingSpeed();
    void normalizePlayingSpeed();


private:
    void threadPlaying();

    bool createDatasources( const SInitSettings & _settings, std::vector<PlayingDatasource *> & _datasrc );

    void playingLoop();
    bool playOneStep();
    inline void emitStep( common_types::TLogicStep _step );
    inline void emitStepInstant( common_types::TLogicStep _step );
    void hideFutureObjects();

    inline void playObjects( const PlayingDatasource::TObjectsAtOneStep & _objectsStep );

    // data    
    SState m_state;
    std::unordered_map<common_types::TObjectId, SPlayableObject> m_playingObjects;

    // service
    PlayerStepIterator m_playIterator;
    DatasourceMixer m_datasourcesMixer;
    std::thread * m_threadPlaying;
    std::condition_variable m_cvPlayStartEvent;
    std::mutex m_muEmitStepProtect;
    DatabaseManagerBase * m_database;
#ifdef OBJREPR_LIBRARY_EXIST
    objrepr::SpatialObjectManager * m_objectManager;
#endif
};

#endif // PLAYER_H
