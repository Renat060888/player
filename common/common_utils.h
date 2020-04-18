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


}

}

#endif // COMMON_UTILS_H
