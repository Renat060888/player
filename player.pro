ROOT_DIR=../

TEMPLATE = app
TARGET = player

include($${ROOT_DIR}pri/common.pri)

CONFIG -= qt
#CONFIG += release

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-variable

# TODO: add defines to logger, system monitor, restbed webserver, database, etc...
DEFINES += \
    SWITCH_LOGGER_ASTRA \

LIBS += \
    -lmongoc-1.0 \
    -lbson-1.0 \
    -lpthread \
    -lboost_regex \
    -lboost_system \
    -lboost_filesystem \
    -lboost_program_options \ # TODO: wtf?
    -ljsoncpp \
    -lunilog \  # TODO: wtf?
    -lobjrepr \  # TODO: wtf?
    -lmicroservice_common \

# NOTE: paths for dev environment ( all projects sources in one dir )
INCLUDEPATH +=  \
    /usr/include/libmongoc-1.0 \
    /usr/include/libbson-1.0 \
    $${ROOT_DIR}/microservice_common/ \

SOURCES += \
    analyze/datasource_mixer.cpp \
    analyze/dispatcher_player_contoller.cpp \
    analyze/dispatcher_user.cpp \
    analyze/mirror_player_controller.cpp \
    analyze/player_controller.cpp \
    analyze/player_iterator.cpp \
    analyze/player_worker.cpp \
    analyze/playing_datasource.cpp \
    communication/communication_gateway_facade_player.cpp \
    communication/unified_command_convertor_player.cpp \
    main.cpp \
    analyze/analytic_manager_facade.cpp \
    communication/command_factory.cpp \
    communication/commands/cmd_context_close.cpp \
    communication/commands/cmd_context_open.cpp \
    communication/commands/cmd_player_controller_ping.cpp \
    communication/commands/cmd_user_ping.cpp \
    communication/commands/cmd_user_register.cpp \
    player_agent.cpp \
    storage/database_manager.cpp \
    storage/storage_engine_facade.cpp \
    system/args_parser.cpp \
    system/config_reader.cpp \
    system/path_locator.cpp

HEADERS += \
    analyze/analytic_manager_facade.h \
    analyze/datasource_mixer.h \
    analyze/dispatcher_player_contoller.h \
    analyze/dispatcher_user.h \
    analyze/mirror_player_controller.h \
    analyze/player_controller.h \
    analyze/player_iterator.h \
    analyze/player_worker.h \
    analyze/playing_datasource.h \
    common/common_types.h \
    common/common_utils.h \
    common/common_vars.h \
    communication/command_factory.h \
    communication/commands/cmd_context_close.h \
    communication/commands/cmd_context_open.h \
    communication/commands/cmd_player_controller_ping.h \
    communication/commands/cmd_user_ping.h \
    communication/commands/cmd_user_register.h \
    communication/communication_gateway_facade_player.h \
    communication/unified_command_convertor_player.h \
    datasource/dummy.h \
    player_agent.h \
    storage/database_manager.h \
    storage/storage_engine_facade.h \
    system/args_parser.h \
    system/config_reader.h \
    system/path_locator.h


