#ifndef STORAGE_ENGINE_FACADE_H
#define STORAGE_ENGINE_FACADE_H


class StorageEngineFacade
{
public:
    struct SServiceLocator {

    };

    struct SInitSettings {
        SServiceLocator services;
    };

    StorageEngineFacade();

    bool init( const SInitSettings & _settings );
    void shutdown();


private:




};

#endif // STORAGE_ENGINE_FACADE_H
