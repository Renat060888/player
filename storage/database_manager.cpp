
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <microservice_common/system/logger.h>
#include <microservice_common/common/ms_common_vars.h>
#include <microservice_common/common/ms_common_utils.h>

#include "database_manager.h"

using namespace std;
using namespace common_vars;
using namespace common_types;

static constexpr const char * PRINT_HEADER = "DatabaseMgr:";
static const string ARGS_DELIMETER = "$";

bool DatabaseManager::m_systemInited = false;
int DatabaseManager::m_instanceCounter = 0;
const std::string DatabaseManager::ALL_CLIENT_OPERATIONS = "";
const common_types::TPid DatabaseManager::ALL_PROCESS_EVENTS = 0;
const std::string DatabaseManager::ALL_REGISTRATION_IDS = "";

DatabaseManager::DatabaseManager()
    : m_mongoClient(nullptr)
    , m_database(nullptr)
{

}

DatabaseManager::~DatabaseManager()
{    
    mongoc_cleanup();

    for( mongoc_collection_t * collect : m_allCollections ){
        mongoc_collection_destroy( collect );
    }
    mongoc_database_destroy( m_database );
    mongoc_client_destroy( m_mongoClient );
}

void DatabaseManager::systemInit(){

    mongoc_init();    
    VS_LOG_INFO << PRINT_HEADER << " init success" << endl;
    m_systemInited = true;
}

// -------------------------------------------------------------------------------------
// service
// -------------------------------------------------------------------------------------
bool DatabaseManager::init( SInitSettings _settings ){

    m_settings = _settings;

    // init mongo
    const mongoc_uri_t * uri = mongoc_uri_new_for_host_port( _settings.host.c_str(), _settings.port );
    if( ! uri ){
        VS_LOG_ERROR << PRINT_HEADER << " mongo URI creation failed by host: " << _settings.host << endl;
        return false;
    }

    m_mongoClient = mongoc_client_new_from_uri( uri );
    if( ! m_mongoClient ){
        VS_LOG_ERROR << PRINT_HEADER << " mongo connect failed to: " << _settings.host << endl;
        return false;
    }

    m_database = mongoc_client_get_database( m_mongoClient, _settings.databaseName.c_str() );

    m_tableWALClientOperations = mongoc_client_get_collection( m_mongoClient,
        _settings.databaseName.c_str(),
        (string("video_server_") + mongo_fields::wal_client_operations::COLLECTION_NAME).c_str() );

    m_tableWALProcessEvents = mongoc_client_get_collection( m_mongoClient,
        _settings.databaseName.c_str(),
        (string("video_server_") + mongo_fields::wal_process_events::COLLECTION_NAME).c_str() );

    m_allCollections.push_back( m_tableWALClientOperations );
    m_allCollections.push_back( m_tableWALProcessEvents );

    VS_LOG_INFO << PRINT_HEADER << " instance connected to [" << _settings.host << "]" << endl;
    return true;
}

inline bool DatabaseManager::createIndex( const std::string & _tableName, const std::vector<std::string> & _fieldNames ){

    //
    bson_t keys;
    bson_init( & keys );

    for( const string & key : _fieldNames ){
        BSON_APPEND_INT32( & keys, key.c_str(), 1 );
    }

    //
    char * indexName = mongoc_collection_keys_to_index_string( & keys );
    bson_t * createIndex = BCON_NEW( "createIndexes",
                                     BCON_UTF8(_tableName.c_str()),
                                     "indexes", "[",
                                         "{", "key", BCON_DOCUMENT(& keys),
                                              "name", BCON_UTF8(indexName),
                                         "}",
                                     "]"
                                );

    //
    bson_t reply;
    bson_error_t error;
    const bool rt = mongoc_database_command_simple( m_database,
                                                    createIndex,
                                                    NULL,
                                                    & reply,
                                                    & error );


    if( ! rt ){
        VS_LOG_ERROR << PRINT_HEADER << " index creation failed, reason: " << error.message << endl;
        bson_destroy( createIndex );

        // TODO: remove later
        assert( rt );

        return false;
    }

    bson_destroy( createIndex );
    return false;
}









