#pragma once

#define MAX_SEQUENCE 255

#define PACKET_MAX_SIZE 0x10000
#define PACKET_HEADER_SIZE 4 // without packet ID

#define TCP_PACKET_SIGNATURE 'U'

#define UDP_CLIENT_PACKET_SIGNATURE 'W'
#define UDP_RELAY_PACKET_SIGNATURE 'Y'
#define UDP_PING_PACKET_SIGNATURE '`'

#define UDP_PACKET_MAX_SIZE 8206
#define UDP_CLIENT_HEARTBEAT_TIMEOUT 15

#define TCP_CONNECTED_MESSAGE "~SERVERCONNECTED\n"

enum class ClientPacketType {
	SocketInfo = 0,
	Heartbeat = 1
};