#include "net/receivepacket.h"
#include "common/net/netdefs.h"
#include "common/logger.h"

using namespace std;

/**
 * Constructor. Parses incoming packet
 * @param buf 
 */
CReceivePacket::CReceivePacket(const Buffer& buf)
{
	SetBufferAndParse(buf);
}

/**
 * Checks if packet is valid
 * @return True if signature is TCP_PACKET_SIGNATURE
 */
bool CReceivePacket::IsValid()
{
	return m_nSignature == TCP_PACKET_SIGNATURE;
}

/**
*  Gets read buffer
*  @return Reference to read buffer
*/
Buffer& CReceivePacket::GetData()
{
	return m_Buffer;
}

/**
 * Gets packet ID
 * @return ID of packet
 */
int CReceivePacket::GetID()
{
	return m_nPacketID;
}

/**
 * Gets length of packet
 * @return length of packet
 */
int CReceivePacket::GetLength()
{
	return m_nLength;
}

/**
 * Gets sequence of packet
 * @return packet sequence 
 */
int CReceivePacket::GetSequence()
{
	return m_nSequence;
}

/**
 * Reads int8
 * @return read data or 0 if failed
 */
int8_t CReceivePacket::ReadInt8()
{
	if (!CanReadBytes(1))
	{
		Logger().Error("ReadInt8: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return m_Buffer.readInt8();
}

/**
 * Reads int16
 * @param bigEndian
 * @return number or 0 if failed
 */
int16_t CReceivePacket::ReadInt16(bool bigEndian)
{
	if (!CanReadBytes(2))
	{
		Logger().Error("ReadInt16: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return bigEndian ? m_Buffer.readInt16_BE() : m_Buffer.readInt16_LE();
}

/**
 * Reads int32
 * @param bigEndian
 * @return number or 0 if failed
 */
int32_t CReceivePacket::ReadInt32(bool bigEndian)
{
	if (!CanReadBytes(4))
	{
		Logger().Error("ReadInt32: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return bigEndian ? m_Buffer.readInt32_BE() : m_Buffer.readInt32_LE();
}

/**
 * Reads int64
 * @param bigEndian
 * @return number or 0 if failed
 */
int64_t CReceivePacket::ReadInt64(bool bigEndian)
{
	if (!CanReadBytes(8))
	{
		Logger().Error("ReadInt64: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return bigEndian ? m_Buffer.readInt64_BE() : m_Buffer.readInt64_LE();
}

/**
 * Reads uint8
 * @return number or 0 if failed
 */
uint8_t CReceivePacket::ReadUInt8()
{
	if (!CanReadBytes(1))
	{
		Logger().Error("ReadUInt8: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return m_Buffer.readUInt8();
}

/**
 * Reads uint16
 * @param bigEndian
 * @return number or 0 if failed
 */
uint16_t CReceivePacket::ReadUInt16(bool bigEndian)
{
	if (!CanReadBytes(2))
	{
		Logger().Error("ReadUInt16: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return bigEndian ? m_Buffer.readUInt16_BE() : m_Buffer.readUInt16_LE();
}

/**
 * Reads uint32
 * @param bigEndian
 * @return number or 0 if failed
 */
uint32_t CReceivePacket::ReadUInt32(bool bigEndian)
{
	if (!CanReadBytes(4))
	{
		Logger().Error("ReadUInt32: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return bigEndian ? m_Buffer.readUInt32_BE() : m_Buffer.readUInt32_LE();
}

/**
 * Reads uint64
 * @param bigEndian
 * @return number or 0 if failed
 */
uint64_t CReceivePacket::ReadUInt64(bool bigEndian)
{
	if (!CanReadBytes(8))
	{
		Logger().Error("ReadUInt64: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return bigEndian ? m_Buffer.readUInt64_BE() : m_Buffer.readUInt64_LE();
}

/**
 * Reads float
 * @param bigEndian
 * @return number or 0 if failed
 */
float CReceivePacket::ReadFloat(bool bigEndian)
{
	if (!CanReadBytes(4))
	{
		Logger().Error("ReadFloat: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return 0;
	}

	return bigEndian ? m_Buffer.readFloat_BE() : m_Buffer.readFloat_LE();
}

/**
 * Reads string
 * @return string or "" if failed
 */
string CReceivePacket::ReadString()
{
	if (!CanReadBytes(1))
	{
		Logger().Error("ReadString: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return "";
	}

	string str = m_Buffer.readStr();
	return str;
}

/**
 * Reads vector of unsigned char
 * @param length Length of vector to be read
 * @return vector or vector<unsigned char>() if failed
 */
vector<unsigned char> CReceivePacket::ReadArray(int length)
{
	if (!CanReadBytes(length))
	{
		Logger().Error("ReadArray: out of buffer, packet id: %d, len: %d, offset: %d\n", m_nPacketID, m_nLength, m_Buffer.getReadOffset());
		return vector<unsigned char>();
	}

	return m_Buffer.readArr(length);
}

/**
 * Reads packet header
 */
void CReceivePacket::ParseHeader()
{
	m_nSignature = m_Buffer.readUInt8();

	if (!IsValid())
	{
		return;
	}

	m_nSequence = m_Buffer.readUInt8();
	m_nLength = m_Buffer.readUInt16_LE();

	// check the buf size because packet ID is not part of header?
	if (m_Buffer.getBuffer().size() > 4)
	{
		m_nPacketID = m_Buffer.readUInt8();

		Logger().Debug("ReceivePacket::Parse() sequence: %d, length: %d, id: %d\n", m_nSequence, m_nLength, m_nPacketID);
	}
}

/**
 * Sets buffer object and parses packet header
 * @param buf
 */
void CReceivePacket::SetBufferAndParse(const Buffer& buf)
{
	m_Buffer = buf;
	ParseHeader();
}

/**
 * Checks if bytes can be read and there no buffer overrun 
 * @param len
 * @return True if can be read
 */
bool CReceivePacket::CanReadBytes(int len)
{
	return (m_nLength + 4) >= m_Buffer.getReadOffset() + len;
}