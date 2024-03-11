cmake_minimum_required(VERSION 3.20)
set(PROJECTNAME "CSO Server")

project(PROJECTNAME CXX)

option(CSO_SERVER_USE_GUI "Use QT GUI" OFF)
option(CSO_SERVER_PUBLIC "Public build" OFF)
option(CSO_SERVER_PROTECTION "Enable protection" OFF)
option(CSO_SERVER_DBSQLITE "SQLite database" ON)

add_subdirectory(thirdparty)
add_subdirectory(net)

if (CSO_SERVER_USE_GUI)
	add_subdirectory(GUI)
endif()

# if test build enabled
add_subdirectory(test)

add_executable(PROJECTNAME)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)

target_link_libraries(PROJECTNAME PRIVATE keyvalues)
target_link_libraries(PROJECTNAME PRIVATE nlohmann_json)
target_link_libraries(PROJECTNAME PRIVATE wolfssl)
target_link_libraries(PROJECTNAME PRIVATE zip)

target_link_libraries(PROJECTNAME PRIVATE net)
if (CSO_SERVER_USE_GUI)
	target_link_libraries(PROJECTNAME PRIVATE gui)
	qt6_disable_unicode_defines(PROJECTNAME)
	add_definitions(-DUSE_GUI)
endif()

target_sources(PROJECTNAME PRIVATE "main.cpp")
target_sources(PROJECTNAME PRIVATE "serverinstance.cpp")
target_sources(PROJECTNAME PRIVATE "serverconfig.cpp")
target_sources(PROJECTNAME PRIVATE "consolecommands.cpp")
target_sources(PROJECTNAME PRIVATE "common/buffer.cpp")
target_sources(PROJECTNAME PRIVATE "common/buildnum.cpp")
target_sources(PROJECTNAME PRIVATE "user/user.cpp")
target_sources(PROJECTNAME PRIVATE "user/userinventoryitem.cpp")
target_sources(PROJECTNAME PRIVATE "user/userloadout.cpp")
target_sources(PROJECTNAME PRIVATE "room/room.cpp")
target_sources(PROJECTNAME PRIVATE "room/roomsettings.cpp")
target_sources(PROJECTNAME PRIVATE "room/gamematch.cpp")
target_sources(PROJECTNAME PRIVATE "channel/channel.cpp")
target_sources(PROJECTNAME PRIVATE "channel/channelserver.cpp")
target_sources(PROJECTNAME PRIVATE "csvtable.cpp")
target_sources(PROJECTNAME PRIVATE "quest/quest.cpp")
target_sources(PROJECTNAME PRIVATE "quest/questevent.cpp")

target_sources(PROJECTNAME PRIVATE "manager/manager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/usermanager.cpp")
if (CSO_SERVER_DBSQLITE)
	target_sources(PROJECTNAME PRIVATE "manager/userdatabase_sqlite.cpp")
endif()
target_sources(PROJECTNAME PRIVATE "manager/channelmanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/packetmanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/shopmanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/itemmanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/luckyitemmanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/hostmanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/dedicatedservermanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/questmanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/minigamemanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/clanmanager.cpp")
target_sources(PROJECTNAME PRIVATE "manager/rankmanager.cpp")

target_sources(PROJECTNAME PRIVATE "packet/packethelper_fulluserinfo.cpp")
if (WIN32)
	target_sources(PROJECTNAME PRIVATE "crashdump.cpp")
endif()

target_sources(PROJECTNAME PRIVATE "common/console.cpp")
target_sources(PROJECTNAME PRIVATE "common/thread.cpp")
target_sources(PROJECTNAME PRIVATE "common/utils.cpp")

target_include_directories(PROJECTNAME PUBLIC
	"."
	"public"
	"thirdparty"
	"thirdparty/json/include"
	"thirdparty/SQLiteCpp/include"
	"thirdparty/KeyValues/include"
	"thirdparty/wolfssl"
	"thirdparty/zip/src"
	"thirdparty/rapidcsv/src"
)

target_precompile_headers(PROJECTNAME PRIVATE main.h)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

if (CSO_SERVER_PROTECTION)
	add_definitions(-DPROTECTION)
endif()

if (CSO_SERVER_DBSQLITE)
	add_definitions(-DDB_SQLITE)
	target_link_libraries(PROJECTNAME PRIVATE SQLiteCpp sqlite3)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED True)
endif()

set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset Default Build Config")

set_target_properties(PROJECTNAME PROPERTIES OUTPUT_NAME "CSNZ_Server")

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/../bin)

# define max numbers of connected users by changing FD_SETSIZE
if (CSO_SERVER_PUBLIC)
	add_definitions(-DFD_SETSIZE=40)
	add_definitions(-DPUBLIC_RELEASE)
else()
	add_definitions(-DFD_SETSIZE=1024)
endif()

if (NOT WIN32)
	add_definitions(-DPOSIX=1)
endif()

if (MSVC)
	add_definitions(-DWIN32_LEAN_AND_MEAN)

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")

	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Od")
	set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O1")
		
	set_target_properties(PROJECTNAME PROPERTIES LINK_FLAG_RELEASE "/WHOLEARCHIVE")
	
	target_link_libraries(PROJECTNAME PRIVATE ws2_32)
	target_link_libraries(PROJECTNAME PRIVATE Shlwapi)

	target_compile_options(PROJECTNAME PRIVATE /W3)
else()
	# don't treat some warnings as errors
	target_compile_options(PROJECTNAME PRIVATE -Wno-error=sign-compare -Wno-error=unused-variable -Wno-error=delete-non-virtual-dtor -Wno-error=unused-parameter -Wno-error=conversion-null -Wno-error=register -Wno-error=unused-value -Wno-error=reorder -Wno-error=attributes)
endif()