#include "net/sendpacket.h"
#include "common/net/netdefs.h"

using namespace std;

/**
 * Constructor
 * @param sequence Current send sequence
 * @param packetID ID of the packet
 */
CSendPacket::CSendPacket(int sequence, int packetID)
{
	m_nPacketID = packetID;
	m_nSequence = sequence;
}

/**
 * Destructor
 */
CSendPacket::~CSendPacket()
{
}

/**
 * Writes packet length to buffer. Called after packet structure is filled. 
 * @param socket
 * @param packetID ID of the packet
 * @return Packet buffer
 */
vector<unsigned char> CSendPacket::SetPacketLength()
{
	/// @todo rewrite
	vector<unsigned char> v = m_OutStream.getBuffer();

	uint16_t size = (uint16_t)v.size() - 4;
	if (size < 255)
	{
		v[2] = (uint8_t)size;
	}
	else
	{
		v[2] = (uint8_t)(size & 0xff);
		v[3] = (uint8_t)((size >> 8));
	}

	return v;
}

/**
 * Gets data buffer
 * @return buffer
 */
Buffer CSendPacket::GetData()
{
	return m_OutStream;
}

/**
 * Writes int8 to buffer
 * @param number
 */
void CSendPacket::WriteInt8(int number)
{
	m_OutStream.writeInt8(number);
}

/**
 * Writes int16 to buffer
 * @param number
 * @param littleEndian Write number in little endian order
 */
void CSendPacket::WriteInt16(int number, bool littleEndian)
{
	littleEndian ? m_OutStream.writeInt16_LE(number) : m_OutStream.writeInt16_BE(number);
}

/**
 * Writes int32 to buffer
 * @param number
 * @param littleEndian Write number in little endian order
 */
void CSendPacket::WriteInt32(int number, bool littleEndian)
{
	littleEndian ? m_OutStream.writeInt32_LE(number) : m_OutStream.writeInt32_BE(number);
}

/**
 * Writes int64 to buffer
 * @param number
 * @param littleEndian Write number in little endian order
 */
void CSendPacket::WriteInt64(long long number, bool littleEndian)
{
	littleEndian ? m_OutStream.writeInt64_LE(number) : m_OutStream.writeInt64_BE(number);
}

/**
 * Writes uint8 to buffer
 * @param number
 */
void CSendPacket::WriteUInt8(unsigned int number)
{
	m_OutStream.writeUInt8(number);
}

/**
 * Writes uint16 to buffer
 * @param number
 * @param littleEndian Write number in little endian order
 */
void CSendPacket::WriteUInt16(unsigned int number, bool littleEndian)
{
	littleEndian ? m_OutStream.writeUInt16_LE(number) : m_OutStream.writeUInt16_BE(number);
}

/**
 * Writes uint32 to buffer
 * @param number
 * @param littleEndian Write number in little endian order
 */
void CSendPacket::WriteUInt32(unsigned int number, bool littleEndian)
{
	littleEndian ? m_OutStream.writeUInt32_LE(number) : m_OutStream.writeUInt32_BE(number);
}

/**
 * Writes uint64 to buffer
 * @param number
 * @param littleEndian Write number in little endian order
 */
void CSendPacket::WriteUInt64(unsigned long long number, bool littleEndian)
{
	littleEndian ? m_OutStream.writeUInt64_LE(number) : m_OutStream.writeUInt64_BE(number);
}

/**
 * Writes string to buffer
 * @param str
 */
void CSendPacket::WriteString(const string& str)
{
	m_OutStream.writeStr(str);
}

/**
 * Writes wide string to buffer
 * @param str
 */
void CSendPacket::WriteWString(const wstring& str)
{
	m_OutStream.writeWStr(str);
}

/**
 * Writes data to buffer by pointer and data len
 * @param data
 * @param len
 */
void CSendPacket::WriteData(void* data, int len)
{
	m_OutStream.writeData(data, len);
}

/**
 * Writes vector of chars to buffer
 * @param arr
 */
void CSendPacket::WriteArray(const vector<unsigned char>& arr)
{
	m_OutStream.writeArray(arr);
}

/**
 * Sets write offset for buffer
 * @param offset
 */
void CSendPacket::SetWriteOffset(int offset)
{
	m_OutStream.setWriteOffset(offset);
}

/**
 * Sets override flag for buffer. Any data written to buffer will overwrite already written
 * @param override
 */
void CSendPacket::SetOverride(bool override)
{
	m_OutStream.setOverride(override);
}

/**
 * Checks if buffer is full. Buffer is full when its size exceeds PACKET_MAX_SIZE (0x10000)
 * @return True if size of written data exceeds PACKET_MAX_SIZE (0x10000)
 */
bool CSendPacket::IsBufferFull()
{
	return m_OutStream.getBuffer().size() > PACKET_MAX_SIZE;
}

/**
 * Writes packet header. Should be called before anything was written there.
 * @todo Avoid code duplicating by calling function in CPacketManager::CreatePacket()
 * rather than anywhere else
 */
void CSendPacket::BuildHeader()
{
	WriteUInt8(TCP_PACKET_SIGNATURE);
	WriteUInt8(m_nSequence);
	WriteUInt16(0); // packet size
	WriteUInt8(m_nPacketID);
}