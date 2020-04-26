#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <microservice_common/common/ms_common_utils.h>

#include "common_types.h"

namespace common_utils {

// --------------------------------------------------------------
// converters
// --------------------------------------------------------------
static std::string printPlayingStatus( common_types::EPlayerStatus _stat ){

    switch( _stat ){
    case common_types::EPlayerStatus::INITED : { return "INITED"; }
    case common_types::EPlayerStatus::PREPARING : { return "PREPARING"; }
    case common_types::EPlayerStatus::READY : { return "READY"; }

    case common_types::EPlayerStatus::PLAYING : { return "PLAYING"; }
    case common_types::EPlayerStatus::PAUSED : { return "PAUSED"; }
    case common_types::EPlayerStatus::STOPPED : { return "STOPPED"; }
    case common_types::EPlayerStatus::PLAY_FROM_POSITION : { return "PLAY_FROM_POSITION"; }
    case common_types::EPlayerStatus::ALL_STEPS_PASSED : { return "ALL_STEPS_PASSED"; }
    case common_types::EPlayerStatus::NOT_ENOUGH_STEPS : { return "NOT_ENOUGH_STEPS"; }
    case common_types::EPlayerStatus::CLOSE : { return "CLOSE"; }

    case common_types::EPlayerStatus::ACTIVE : { return "ACTIVE"; }
    case common_types::EPlayerStatus::IDLE : { return "IDLE"; }

    case common_types::EPlayerStatus::CRASHED : { return "CRASHED"; }
    case common_types::EPlayerStatus::UNDEFINED : { return "UNDEFINED"; }
    }
    return "shit";
}

static common_types::EPlayerStatus convertPlayingStatusFromStr( const std::string & _str ){

    if( "INITED" == _str ){ return common_types::EPlayerStatus::INITED; }
    else if( "PREPARING" == _str ){ return common_types::EPlayerStatus::PREPARING; }
    else if( "READY" == _str ){ return common_types::EPlayerStatus::READY; }

    else if( "PLAYING" == _str ){ return common_types::EPlayerStatus::PLAYING; }
    else if( "PAUSED" == _str ){ return common_types::EPlayerStatus::PAUSED; }
    else if( "STOPPED" == _str ){ return common_types::EPlayerStatus::STOPPED; }
    else if( "PLAY_FROM_POSITION" == _str ){ return common_types::EPlayerStatus::PLAY_FROM_POSITION; }
    else if( "ALL_STEPS_PASSED" == _str ){ return common_types::EPlayerStatus::ALL_STEPS_PASSED; }
    else if( "NOT_ENOUGH_STEPS" == _str ){ return common_types::EPlayerStatus::NOT_ENOUGH_STEPS; }
    else if( "CLOSE" == _str ){ return common_types::EPlayerStatus::CLOSE; }

    else if( "ACTIVE" == _str ){ return common_types::EPlayerStatus::ACTIVE; }
    else if( "IDLE" == _str ){ return common_types::EPlayerStatus::IDLE; }

    else if( "CRASHED" == _str ){ return common_types::EPlayerStatus::CRASHED; }
    else if( "UNDEFINED" == _str ){ return common_types::EPlayerStatus::UNDEFINED; }
    else{ assert( false && "unknown playing status str: " && _str.c_str() ); }
}

}

#endif // COMMON_UTILS_H
