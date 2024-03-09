cmake_minimum_required(VERSION 3.20)

project(net CXX)

add_library(net STATIC)

target_link_libraries(net PRIVATE wolfssl)

target_sources(net PRIVATE "extendedsocket.cpp")
target_sources(net PRIVATE "receivepacket.cpp")
target_sources(net PRIVATE "sendpacket.cpp")
target_sources(net PRIVATE "socketshared.cpp")
target_sources(net PRIVATE "tcpclient.cpp")
target_sources(net PRIVATE "tcpserver.cpp")
target_sources(net PRIVATE "udpserver.cpp")

target_sources(net PRIVATE "../common/utils.cpp")
target_sources(net PRIVATE "../common/thread.cpp")
target_sources(net PRIVATE "../common/buffer.cpp")
target_sources(net PRIVATE "../common/console.cpp")

target_include_directories(net PUBLIC
	"../"
	"../public"
	"../thirdparty/wolfssl"
)