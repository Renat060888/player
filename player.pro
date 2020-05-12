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
#    SWITCH_LOGGER_SIMPLE \
    SWITCH_LOGGER_ASTRA \
    OBJREPR_LIBRARY_EXIST \
#    UNIT_TESTS_GOOGLE \

LIBS += \
    -lprotobuf \
    -lmongoc-1.0 \
    -lbson-1.0 \
    -lpthread \
    -lboost_regex \
    -lboost_system \
    -lboost_filesystem \
    -lboost_program_options \ # TODO: wtf?
    -ljsoncpp \    
    -lmicroservice_common \

contains( DEFINES, OBJREPR_LIBRARY_EXIST ){
    message("connect 'unilog' and 'objrepr' libraries")
LIBS += \
    -lunilog \
    -lobjrepr
}

contains( DEFINES, UNIT_TESTS_GOOGLE ){
    message("connect 'gtests' library")
LIBS += \
    -lgtest
}

# NOTE: paths for dev environment ( all projects sources in one dir )
INCLUDEPATH +=  \
    /usr/include/libmongoc-1.0 \
    /usr/include/libbson-1.0 \
    $${ROOT_DIR}/microservice_common/ \

SOURCES += \
    analyze/datasource_editor.cpp \
    analyze/datasource_mixer.cpp \
    analyze/datasource_descriptor.cpp \
    analyze/datasource_reader.cpp \
    analyze/dispatcher_player_contoller.cpp \
    analyze/dispatcher_user.cpp \
    analyze/proxy_player_controller.cpp \
    communication/commands/cmd_player_from_pos.cpp \
    communication/commands/cmd_player_live.cpp \
    communication/commands/cmd_player_loop.cpp \
    communication/commands/cmd_player_pause.cpp \
    communication/commands/cmd_player_reverse.cpp \
    communication/commands/cmd_player_speed.cpp \
    communication/commands/cmd_player_start.cpp \
    communication/commands/cmd_player_step.cpp \
    communication/commands/cmd_player_stop.cpp \
    communication/protocols/protocol_player_agent_to_controller.pb.cpp \
    player_controller.cpp \
    analyze/player_iterator.cpp \
    analyze/player_worker.cpp \
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
    system/path_locator.cpp \
    system/objrepr_bus_player.cpp \
    system/system_environment_facade_player.cpp \

contains( DEFINES, UNIT_TESTS_GOOGLE ){
    message("connect 'gtests' library")
SOURCES += \
    unit_tests/unit_tests.cpp \
    unit_tests/datasource_reader_test.cpp
}

HEADERS += \
    analyze/analytic_manager_facade.h \
    analyze/datasource_editor.h \
    analyze/datasource_mixer.h \
    analyze/datasource_descriptor.h \
    analyze/datasource_reader.h \
    analyze/dispatcher_player_contoller.h \
    analyze/dispatcher_user.h \
    analyze/proxy_player_controller.h \
    communication/commands/cmd_player_from_pos.h \
    communication/commands/cmd_player_live.h \
    communication/commands/cmd_player_loop.h \
    communication/commands/cmd_player_pause.h \
    communication/commands/cmd_player_reverse.h \
    communication/commands/cmd_player_speed.h \
    communication/commands/cmd_player_start.h \
    communication/commands/cmd_player_step.h \
    communication/commands/cmd_player_stop.h \
    communication/protocols/protocol_player_agent_to_controller.pb.h \
    player_controller.h \
    analyze/player_iterator.h \
    analyze/player_worker.h \
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
    system/path_locator.h \
    system/objrepr_bus_player.h \
    system/system_environment_facade_player.h \
    unit_tests/unit_tests.h \

contains( DEFINES, UNIT_TESTS_GOOGLE ){
    message("connect 'gtests' library")
HEADERS += \
    unit_tests/unit_tests.h \
    unit_tests/datasource_reader_test.h
}

