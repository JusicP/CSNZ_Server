#pragma once

#include "common/buffer.h"

/**
 * Class that represents incoming packets.
 */
class CReceivePacket
{
public:
	CReceivePacket(const Buffer& buf);

	bool IsValid();
	int GetID();
	int GetLength();
	int GetSequence();
	Buffer& GetData();
	int8_t ReadInt8();
	int16_t ReadInt16(bool bigEndian = false);
	int32_t ReadInt32(bool bigEndian = false);
	int64_t ReadInt64(bool bigEndian = false);
	uint8_t ReadUInt8();
	uint16_t ReadUInt16(bool bigEndian = false);
	uint32_t ReadUInt32(bool bigEndian = false);
	uint64_t ReadUInt64(bool bigEndian = false);
	float ReadFloat(bool bigEndian = false);
	std::string ReadString();
	std::vector<unsigned char> ReadArray(int length);
	bool CanReadBytes(int len);
	void ParseHeader();
	void SetBufferAndParse(const Buffer& buf);

private:
	// packet header
	int m_nSignature;
	int m_nSequence;
	int m_nLength;
	int m_nPacketID;

	Buffer m_Buffer;
};
