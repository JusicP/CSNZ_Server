#include "PacketHelper_RoomUpdateSettings.h"

CPacketHelper_RoomUpdateSettings::CPacketHelper_RoomUpdateSettings()
{
	Init();
}

CPacketHelper_RoomUpdateSettings::CPacketHelper_RoomUpdateSettings(Buffer& inPacket) // unfinished
{
	Init();

	// TODO: implement 128-bit bitwise operations...
	lowFlag = inPacket.readUInt32_LE();
	lowMidFlag = inPacket.readUInt32_LE();
	highMidFlag = inPacket.readUInt32_LE();
	highFlag = inPacket.readUInt32_LE();

	if (lowFlag & ROOM_LOW_NAME) {
		roomName = inPacket.readStr().c_str();
	}
	if (lowFlag & ROOM_LOW_UNK) {
		unk00 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK2) {
		unk01 = inPacket.readUInt8();
		unk02 = inPacket.readUInt8();
		unk03 = inPacket.readUInt8();
		unk04 = inPacket.readUInt32_LE();
	}
	if (lowFlag & ROOM_LOW_PASSWORD) {
		unk05 = inPacket.readStr().c_str();
	}
	if (lowFlag & ROOM_LOW_LEVELLIMIT) {
		unk06 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK7) {
		unk07 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_GAMEMODE) {
		unk08 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_MAPID) {
		unk09 = inPacket.readUInt16_LE();
	}
	if (lowFlag & ROOM_LOW_MAXPLAYERS) {
		unk10 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_WINLIMIT) {
		unk11 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_NEEDEDKILLS) {
		unk12 = inPacket.readUInt16_LE();
	}
	if (lowFlag & ROOM_LOW_GAMETIME) {
		unk13 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_ROUNDTIME) {
		unk14 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_ARMSRESTRICTION) {
		unk15 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_HOSTAGEKILLLIMIT) {
		unk16 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_FREEZETIME) {
		unk17 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_BUYTIME) {
		unk18 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_DISPLAYGAMENAME) {
		unk19 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_TEAMBALANCE) {
		unk20 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK21) {
		unk21 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_FRIENDLYFIRE) {
		unk22 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_FLASHLIGHT) {
		unk23 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_FOOTSTEPS) {
		unk24 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK25) {
		unk25 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK26) {
		unk26 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK27) {
		unk27 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK28) {
		unk28 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK29) {
		unk29 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_VIEWFLAG) {
		unk30 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_VOICECHAT) {
		unk31 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_STATUS) {
		unk32 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK33) {
		unk33 = inPacket.readUInt16_LE();
	}

	if (lowMidFlag & ROOM_LOWMID_UNK) {
		unk34 = inPacket.readStr();
		unk35 = inPacket.readUInt8();
		unk36 = inPacket.readUInt8();
		unk37 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_C4TIMER) {
		unk38 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_BOT) {
		unk39 = inPacket.readUInt8();
		unk40 = inPacket.readUInt8();
		unk41 = inPacket.readUInt8();
		unk42 = inPacket.readUInt8();
		unk43 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_KDRULE) {
		unk44 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_STARTINGCASH) {
		unk45 = inPacket.readUInt16_LE();
	}
	if (lowMidFlag & ROOM_LOWMID_MOVINGSHOT) {
		unk46 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK47) {
		unk47 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_STATUSSYMBOL) {
		unk48 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_RANDOMMAP) {
		unk49 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_MULTIPLEMAPS) {
		unk50 = inPacket.readUInt8();
		for (int i = 0; i < unk50; i++)
		{
			unk50_data dat;
			dat.unk1 = inPacket.readUInt8();
			dat.unk2 = inPacket.readUInt16_LE();

			unk50_vec.push_back(dat);

			g_pConsole->Log("Unknown data read(unk50)\n");
		}
	}
	if (lowMidFlag & ROOM_LOWMID_UNK51) {
		unk51 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_WPNENHANCERESTRICT) {
		unk52 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_SD) {
		unk53 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_ZSDIFFICULTY) {
		unk54 = inPacket.readUInt8();
		unk55 = inPacket.readUInt32_LE();
		unk56 = inPacket.readUInt32_LE();
	}
	if (lowMidFlag & ROOM_LOWMID_LEAGUERULE) {
		unk57 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_MANNERLIMIT) {
		unk58 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK59) {
		unk59 = inPacket.readUInt16_LE();
	}
	if (lowMidFlag & ROOM_LOWMID_ZBLIMIT) {
		unk60 = inPacket.readUInt8(); // "limited use of normal zombies"
		for (int i = 0; i < 4; i++)
		{
			zbLimit.push_back(inPacket.readUInt32_LE());
		}
	}
	if (lowMidFlag & ROOM_LOWMID_UNK61) {
		unk61 = inPacket.readUInt32_LE(); // user char flag
	}
	if (lowMidFlag & ROOM_LOWMID_UNK62) {
		unk62 = inPacket.readUInt8();
		for (int i = 0; i < unk62; i++)
		{
			unk62_vec.push_back(inPacket.readUInt16_LE());
			g_pConsole->Log("Unknown data read(unk62)\n");
		}
	}
	if (lowMidFlag & ROOM_LOWMID_UNK63) {
		unk63 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_TEAMSWITCH) {
		teamSwitch = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK65) {
		unk65 = inPacket.readUInt8(); // zombie respawn
	}
	if (lowMidFlag & ROOM_LOWMID_UNK66) {
		unk66 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK67) {
		unk67 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK68) {
		unk68 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_ISZBCOMPETITIVE) {
		isZbCompetitive = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK70) {
		unk70 = inPacket.readUInt8(); // zombie auto hunting
	}
	if (lowMidFlag & ROOM_LOWMID_UNK71) {
		unk71 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK72) {
		unk72 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK73) {
		printf("shit!\n");
	}

	if (highMidFlag & ROOM_HIGHMID_UNK73)
	{
		unk73 = inPacket.readUInt8();
	}
	if (highMidFlag & ROOM_HIGHMID_UNK74)
	{
		unk74 = inPacket.readUInt8();
		for (int i = 0; i < 4; i++)
		{
			unk74_vec.push_back(inPacket.readInt8());
		}
	}
	if (highMidFlag & ROOM_HIGHMID_UNK75)
	{
		unk75 = inPacket.readUInt8();
	}
	if (highMidFlag & ROOM_HIGHMID_UNK76)
	{
		unk76 = inPacket.readUInt8();
	}

	if (highFlag)
	{
		g_pConsole->Warn("CPacketHelper_RoomUpdateSettings: got high flag!\n");
	}
}

void CPacketHelper_RoomUpdateSettings::Init()
{
	lowFlag = 0;
	lowMidFlag = 0;
	highMidFlag = 0;
	highFlag = 0;

	unk00 = 0;
	unk01 = 0;
	unk02 = 0;
	unk03 = 0;
	unk04 = 0;
	unk05 = "";
	unk06 = 0;
	unk07 = 0;
	unk08 = 0;
	unk09 = 0;
	unk10 = 0;
	unk11 = 0;
	unk12 = 0;
	unk13 = 0;
	unk14 = 0;
	unk15 = 0;
	unk16 = 0;
	unk17 = 0;
	unk18 = 0;
	unk19 = 0;
	unk20 = 0;
	unk21 = 0;
	unk22 = 0;
	unk23 = 0;
	unk24 = 0;
	unk25 = 0;
	unk26 = 0;
	unk27 = 0;
	unk28 = 0;
	unk29 = 0;
	unk30 = 0;
	unk31 = 0;
	unk32 = 0;
	unk33 = 0;
	unk34 = "";
	unk35 = 0;
	unk36 = 0;
	unk37 = 0;
	unk38 = 0;
	unk39 = 0;
	unk40 = 0;
	unk41 = 0;
	unk42 = 0;
	unk43 = 0;
	unk44 = 0;
	unk45 = 0;
	unk46 = 0;
	unk47 = 0;
	unk48 = 0;
	unk49 = 0;
	unk50 = 0;
	unk51 = 0;
	unk52 = 0;
	unk53 = 0;
	unk54 = 0;
	unk55 = 0;
	unk56 = 0;
	unk57 = 0;
	unk58 = 0;
	unk59 = 0;
	unk60 = 0;
	unk61 = 0;
	unk62 = 0;
	unk63 = 0;
	teamSwitch = 0;
	unk65 = 0;
	unk66 = 0;
	unk67 = 0;
	unk68 = 0;
	isZbCompetitive = 0;
	unk70 = 0;
	unk71 = 0;
	unk72 = 0;
	unk73 = 0;
	unk74 = 0;
	unk75 = 0;
	unk76 = 0;
}