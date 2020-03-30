
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <microservice_common/system/logger.h>

#include "path_locator.h"

using namespace std;

static constexpr const char * PID_FILE_NAME = "/tmp/player_agent_unique_lock_file.pid";
static constexpr const char * SHELL_IMITATION_DOMAIN_SOCKET_PATH = "/tmp/player_agent_shell_interface.socket";
static constexpr const char * PRINT_HEADER = "PathLocator:";

// TODO: remove
static const string g_tempPFN = PID_FILE_NAME;
static const string g_tempDSP = SHELL_IMITATION_DOMAIN_SOCKET_PATH;

PathLocator::PathLocator()
{

}

PathLocator::~PathLocator()
{

}

void PathLocator::removePreviousSessionSocketFiles(){

    // TODO: not best place for this operation
    VS_LOG_INFO << PRINT_HEADER <<" remove all previous session socket files..." << endl;

    try {
        // single file
        boost::filesystem::path filePath( SHELL_IMITATION_DOMAIN_SOCKET_PATH );
        if( boost::filesystem::exists(filePath) ){
            VS_LOG_INFO << PRINT_HEADER << " to remove [" << SHELL_IMITATION_DOMAIN_SOCKET_PATH << "]" << endl;
            boost::filesystem::remove( filePath );
        }

        // by pattern - analyzer
        {
            // by pattern
            const string targetPath = "/tmp/";
            const boost::regex filter1( "video_server_analyzer_.*\." );

            boost::filesystem::directory_iterator endIter;
            for( boost::filesystem::directory_iterator iter( targetPath ); iter != endIter; ++iter ){

                boost::smatch what;
                if( ! boost::regex_match( iter->path().filename().string(), what, filter1) ){
                    continue;
                }

                VS_LOG_INFO << PRINT_HEADER << " to remove [" << iter->path().filename().string() << "]" << endl;
                boost::filesystem::remove( iter->path() );
            }
        }

        // by pattern - video receiver
        {
            const string targetPath = "/tmp/";
            const boost::regex filter1( "video_server_vr_.*\." );

            boost::filesystem::directory_iterator endIter;
            for( boost::filesystem::directory_iterator iter( targetPath ); iter != endIter; ++iter ){

                boost::smatch what;
                if( ! boost::regex_match( iter->path().filename().string(), what, filter1) ){
                    continue;
                }

                VS_LOG_INFO << PRINT_HEADER << " to remove [" << iter->path().filename().string() << "]" << endl;
                boost::filesystem::remove( iter->path() );
            }
        }
    } catch( const boost::filesystem::filesystem_error & _err ){
        VS_LOG_WARN << PRINT_HEADER << " boost exception, WHAT: " << _err.what() << " CODE MSG: " << _err.code().message() << endl;
    }

    VS_LOG_INFO << PRINT_HEADER << " ...removing complete" << endl;
}

const std::string PathLocator::getUniqueLockFile(){
    return g_tempPFN;
}

const std::string PathLocator::getShellImitationDomainSocket(){
    return g_tempDSP;
}
