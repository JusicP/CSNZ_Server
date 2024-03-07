#include "packetin_udp.h"

CPacketIn_UDP::CPacketIn_UDP(Buffer& buf)
{
	buffer = buf;
}

void CPacketIn_UDP::Parse()
{
	m_nSignature = buffer.readUInt8();
	if (m_nSignature != 'W')
	{
		Console().Error("CPacketIn_UDP::Parse: signature error\n");
	}
	m_nUserID = buffer.readUInt32_LE();
	m_nPortID = buffer.readUInt16_LE();
	long longAddr = buffer.readUInt32_BE();
	m_IpAddress = ip_to_string(longAddr);
	m_nPort = buffer.readUInt16_LE();
}