#include "serverconfig.h"
#include "common/buildnum.h"

using namespace std;

#define DEFAULT_PORT "30002"

#define DEFAULTUSER_LEVEL 1
#define DEFAULTUSER_EXP 0
#define DEFAULTUSER_POINTS 100000

CServerConfig::CServerConfig()
{
	tcpSendBufferSize = 0;
	maxPlayers = 0;
	restartOnCrash = false;
	inventorySlotMax = 0;
	checkClientBuild = false;
	defUser.gameMaster = false;
	defUser.level = 0;
	defUser.exp = 0;
	defUser.points = 0;
	defUser.honorPoints = 0;
	defUser.prefixID = 0;
	defUser.kills = 0;
	defUser.deaths = 0;
	defUser.battles = 0;
	defUser.win = 0;
	defUser.passwordBoxes = 0;
	defUser.mileagePoints = 0;
	room.connectingMethod = 0;
	room.validateSettings = false;
	activeMiniGamesFlag = 0;
	metadataToSend = 0;
	flockingFlyerType = 0;
	allowedLauncherVersion = 0;
	allowedClientTimestamp = 0;
	maxRegistrationsPerIP = 0;
	crypt = false;
	mainMenuSkinEvent = 0;
}

CServerConfig::~CServerConfig()
{
}

const char* defaultServerConfig = R"(
{
	"HostName": "CSO Server",
	"Description": "",
	"Port": "30002",
	"TCPSendBufferSize": 131072,
	"MaxPlayers": 100,
	"WelcomeMessage": "https://discord.gg/EvUAY6D",
	"RestartOnCrash": false,
	"InventorySlotMax": 3000,
	"CheckClientBuild": false,
	"AllowedClientTimestamp": 0,
	"AllowedLauncherVersion": 67,
	"MaxRegistrationsPerIP": 3,
	"Crypt": false,
	"MainMenuSkinEvent": 0,
	"Metadata": {
		"Maplist": true,
		"ClientTable": true,
		"Unk3": true,
		"ItemBox": true,
		"Unk8": true,
		"MatchOption": true,
		"Unk15": true,
		"WeaponParts": true,
		"Unk20": true,
		"Encyclopedia": false,
		"GameModeList": true,
		"ProgressUnlock": true,
		"WeaponPaint": true,
		"ReinforceMaxLvl": true,
		"ReinforceMaxEXP": true,
		"ReinforceItemsExp": true,
		"Unk31": true,
		"HonorMoneyShop": false,
		"ItemExpireTime": true,
		"ScenarioTX_Common": true,
		"ScenarioTX_Dedi": true,
		"ShopItemList_Dedi": true,
		"ZBCompetitive": true,
		"Unk43": true,
		"Unk49": true,
		"PPSystem": true,
		"CodisData": true,
		"Item": false,
		"WeaponProp": true,
		"Hash": false,
		"RandomWeaponList": true
	},
	"DefaultUser": {
		"GameMaster": true,
		"Level": 1,
		"Exp": 0,
		"Points": 100000,
		"HonorPoints": 0,
		"PrefixID": 0,
		"Kills": 0,
		"Deaths": 0,
		"Battles": 0,
		"Win": 0,
		"PasswordBoxes": 1,
		"MileagePoints": 0,
		"DefaultItems": [
			25,
			26,
			27,
			28,
			29,
			30,
			31,
			113,
			114,
			40,
			41,
			42,
			43,
			44,
			49,
			50,
			51,
			52,
			53,
			136,
			137,
			138,
			163,
			175,
			200,
			201,
			211,
			214,
			215,
			216,
			252,
			305,
			306,
			358,
			388,
			390,
			391,
			394,
			395,
			459,
			460,
			674,
			675,
			8079,
			8080,
			8115,
			8137,
			8138,
			8222,
			8415
		],
		"PseudoDefaultItems": [
			1,
			2,
			3,
			4,
			5,
			6,
			7,
			8,
			9,
			10,
			11,
			12,
			13,
			14,
			15,
			16,
			17,
			18,
			19,
			20,
			21,
			22,
			23,
			24,
			161,
			164,
			176,
			525
		],
		"Loadouts": {
			"0": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"1": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"2": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"3": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"4": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"5": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"6": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"7": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"8": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"9": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"10": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			},
			"11": {
				"1": 12,
				"2": 2,
				"3": 161,
				"4": 31
			}
		},
		"BuyMenu": {
			"0": {
				"0": 3,
				"1": 6,
				"2": 2,
				"3": 4,
				"4": 1,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"1": {
				"0": 7,
				"1": 8,
				"2": 38,
				"3": 0,
				"4": 0,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"2": {
				"0": 10,
				"1": 12,
				"2": 13,
				"3": 11,
				"4": 37,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"3": {
				"0": 23,
				"1": 21,
				"2": 17,
				"3": 19,
				"4": 14,
				"5": 22,
				"6": 34,
				"7": 39,
				"8": 114
			},
			"4": {
				"0": 24,
				"1": 32,
				"2": 525,
				"3": 0,
				"4": 0,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"5": {
				"0": 27,
				"1": 28,
				"2": 30,
				"3": 31,
				"4": 26,
				"5": 25,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"6": {
				"0": 41,
				"1": 42,
				"2": 43,
				"3": 40,
				"4": 44,
				"5": 45,
				"6": 46,
				"7": 47,
				"8": 48
			},
			"7": {
				"0": 3,
				"1": 6,
				"2": 2,
				"3": 4,
				"4": 5,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"8": {
				"0": 7,
				"1": 8,
				"2": 0,
				"3": 0,
				"4": 0,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"9": {
				"0": 9,
				"1": 12,
				"2": 13,
				"3": 11,
				"4": 36,
				"5": 37,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"10": {
				"0": 20,
				"1": 17,
				"2": 15,
				"3": 16,
				"4": 18,
				"5": 14,
				"6": 33,
				"7": 35,
				"8": 113
			},
			"11": {
				"0": 24,
				"1": 32,
				"2": 525,
				"3": 0,
				"4": 0,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"12": {
				"0": 27,
				"1": 28,
				"2": 30,
				"3": 31,
				"4": 26,
				"5": 25,
				"6": 29,
				"7": 0,
				"8": 0
			},
			"13": {
				"0": 49,
				"1": 50,
				"2": 52,
				"3": 51,
				"4": 53,
				"5": 54,
				"6": 55,
				"7": 56,
				"8": 57
			},
			"14": {
				"0": 161,
				"1": 0,
				"2": 0,
				"3": 0,
				"4": 0,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"15": {
				"0": 161,
				"1": 0,
				"2": 0,
				"3": 0,
				"4": 0,
				"5": 0,
				"6": 0,
				"7": 0,
				"8": 0
			},
			"16": {
				"0": 1,
				"1": 163,
				"2": 460,
				"3": 175,
				"4": 8079,
				"5": 8080,
				"6": 390,
				"7": 391,
				"8": 459
			}
		}
	},
	"Notices": {
		"1": {
			"Type": 1,
			"Name": "Server information",
			"Description": "Welcome to the CSO server. Report about bugs in vk group: vk.com/csnz_server"
		}
	},
	"GameMatch": {
		"CalcResults": {
			"GameModeCoefficient": {
				"0": {
					"Exp": 230,
					"Points": 200
				},
				"1": {
					"Exp": 230,
					"Points": 200
				},
				"2": {
					"Exp": 230,
					"Points": 200
				},
				"3": {
					"Exp": 230,
					"Points": 200
				},
				"4": {
					"Exp": 230,
					"Points": 200
				},
				"5": {
					"Exp": 230,
					"Points": 200
				},
				"8": {
					"Exp": 300,
					"Points": 270
				},
				"9": {
					"Exp": 300,
					"Points": 270
				},
				"10": {
					"Exp": 210,
					"Points": 180
				},
				"12": {
					"Exp": 55,
					"Points": 50
				},
				"14": {
					"Exp": 300,
					"Points": 270
				},
				"15": {
					"Exp": 5,
					"Points": 2
				},
				"16": {
					"Exp": 60,
					"Points": 30
				},
				"17": {
					"Exp": 10,
					"Points": 4
				},
				"19": {
					"Exp": 210,
					"Points": 180
				},
				"20": {
					"Exp": 320,
					"Points": 290
				},
				"21": {
					"Exp": 210,
					"Points": 180
				},
				"22": {
					"Exp": 240,
					"Points": 210
				},
				"23": {
					"Exp": 230,
					"Points": 200
				},
				"24": {
					"Exp": 300,
					"Points": 270
				},
				"25": {
					"Exp": 230,
					"Points": 200
				},
				"26": {
					"Exp": 20,
					"Points": 8
				},
				"27": {
					"Exp": 240,
					"Points": 210
				},
				"28": {
					"Exp": 20,
					"Points": 8
				},
				"29": {
					"Exp": 310,
					"Points": 260
				}
			},
			"BonusPercentage": {
				"Items": {
					"73": {
						"Exp": 50,
						"Points": 50
					},
					"59": {
						"Exp": 50
					},
					"108": {
						"Points": 50
					},
					"180": {
						"Exp": 50,
						"Points": 50
					},
					"181": {
						"Points": 50
					},
					"182": {
						"Exp": 50
					}
				},
				"Classes": {
					"55": {
						"Exp": 20,
						"Points": 20
					},
					"47": {
						"Exp": 5,
						"Points": 5
					},
					"48": {
						"Exp": 20
					},
					"154": {
						"Exp": 10,
						"Points": 10
					},
					"155": {
						"Exp": 10,
						"Points": 10
					},
					"362": {
						"Exp": 20,
						"Points": 10
					},
					"376": {
						"Exp": 10
					},
					"377": {
						"Exp": 10
					}
				}
			}
		}
	},
	"Room": {
		"HostConnectingMethod": 0,
		"ValidateSettings": false
	},
	"MiniGames": {
		"Bingo": {
			"Active": false,
			"Items": {
				"564": {
					"Count": 1,
					"Duration": 0
				},
				"457": {
					"Count": 1,
					"Duration": 0
				},
				"458": {
					"Count": 1,
					"Duration": 0
				},
				"565": {
					"Count": 1,
					"Duration": 0
				},
				"363": {
					"Count": 1,
					"Duration": 0
				},
				"483": {
					"Count": 1,
					"Duration": 0
				},
				"355": {
					"Count": 1,
					"Duration": 0
				},
				"242": {
					"Count": 1,
					"Duration": 0
				},
				"416": {
					"Count": 1,
					"Duration": 0
				},
				"365": {
					"Count": 1,
					"Duration": 0
				},
				"38": {
					"Count": 1,
					"Duration": 0
				},
				"187": {
					"Count": 1,
					"Duration": 0
				},
				"304": {
					"Count": 1,
					"Duration": 0
				},
				"463": {
					"Count": 1,
					"Duration": 0
				},
				"261": {
					"Count": 1,
					"Duration": 0
				},
				"372": {
					"Count": 1,
					"Duration": 0
				},
				"589": {
					"Count": 1,
					"Duration": 0
				},
				"237": {
					"Count": 1,
					"Duration": 0
				},
				"233": {
					"Count": 1,
					"Duration": 0
				},
				"677": {
					"Count": 1,
					"Duration": 0
				},
				"531": {
					"Count": 1,
					"Duration": 0
				},
				"158": {
					"Count": 1,
					"Duration": 30
				},
				"164": {
					"Count": 1,
					"Duration": 30
				},
				"163": {
					"Count": 1,
					"Duration": 30
				},
				"21": {
					"Count": 1,
					"Duration": 30
				},
				"4": {
					"Count": 1,
					"Duration": 30
				},
				"306": {
					"Count": 1,
					"Duration": 30
				},
				"8": {
					"Count": 1,
					"Duration": 30
				},
				"15": {
					"Count": 1,
					"Duration": 30
				},
				"14": {
					"Count": 1,
					"Duration": 30
				},
				"471": {
					"Count": 1,
					"Duration": 30
				},
				"472": {
					"Count": 1,
					"Duration": 30
				},
				"473": {
					"Count": 1,
					"Duration": 30
				},
				"557": {
					"Count": 1,
					"Duration": 30
				},
				"558": {
					"Count": 1,
					"Duration": 30
				},
				"559": {
					"Count": 1,
					"Duration": 30
				},
				"561": {
					"Count": 1,
					"Duration": 30
				},
				"676": {
					"Count": 30,
					"Duration": 0
				},
				"65": {
					"Count": 1,
					"Duration": 0
				},
				"418": {
					"Count": 1,
					"Duration": 30
				},
				"419": {
					"Count": 1,
					"Duration": 30
				},
				"388": {
					"Count": 1,
					"Duration": 30
				},
				"390": {
					"Count": 1,
					"Duration": 30
				},
				"391": {
					"Count": 1,
					"Duration": 30
				},
				"1035": {
					"Count": 5,
					"Duration": 0
				},
				"510": {
					"Count": 1,
					"Duration": 30
				},
				"511": {
					"Count": 1,
					"Duration": 30
				},
				"512": {
					"Count": 1,
					"Duration": 30
				},
				"73": {
					"Count": 1,
					"Duration": 15
				},
				"59": {
					"Count": 1,
					"Duration": 15
				},
				"108": {
					"Count": 1,
					"Duration": 15
				},
				"46": {
					"Count": 1,
					"Duration": 30
				},
				"48": {
					"Count": 1,
					"Duration": 30
				},
				"56": {
					"Count": 1,
					"Duration": 30
				},
				"362": {
					"Count": 1,
					"Duration": 30
				},
				"376": {
					"Count": 1,
					"Duration": 30
				},
				"377": {
					"Count": 1,
					"Duration": 30
				}
			}
		},
		"WeaponRelease": {
			"Active": false,
			"Items": {
				"566": {
					"Name": "DAGGER",
					"Duration": 0
				},
				"567": {
					"Name": "GALIL",
					"Duration": 30
				},
				"568": {
					"Name": "FAMAS",
					"Duration": 30
				},
				"569": {
					"Name": "QBB95",
					"Duration": 30
				},
				"570": {
					"Name": "GLOCK",
					"Duration": 30
				},
				"571": {
					"Name": "USP45",
					"Duration": 30
				},
				"173": {
					"Name": "DEC",
					"Count": 30
				}
			},
			"Characters": [
				"4",
				"5",
				"9",
				"A",
				"B",
				"C",
				"D",
				"E",
				"F",
				"G",
				"U",
				"K",
				"L",
				"M",
				"O",
				"P",
				"Q",
				"R",
				"S",
				"U",
				"T",
				"I",
				"~"
			]
		}
	},
	"FlockingFlyerType": 0,
	"NameBlacklist": [],
	"Surveys": {
		"1": {
			"Title": "User survey",
			"Questions": {
				"1": {
					"Question": "Do you like CSN:S?",
					"AnswerType": 0, // 0 - check box, 1 - text entry (16-bit unk field added), 2 - unknown
					"AnswerCheckBoxType": 1, // 0 - can't choose answer, 1 - choose only one, 2 - multiple choose
					"Answers": {
						"1": {
							"Answer": "Yes"
						},
						"2": {
							"Answer": "No"
						}
					}
				}
			}
		}
	}
}
)";

bool CServerConfig::Load()
{ 
	try
	{
		ordered_json cfg;
		ifstream f("ServerConfig.json");

		if (!f.is_open())
		{
			Console().Warn("CServerConfig::Load: couldn't load ServerConfig.json. Loading default config\n");

			LoadDefaultConfig(cfg);
		}
		else
		{
			cfg = ordered_json::parse(f, nullptr, true, true);
		}

		hostName = cfg.value("HostName", "CSO Server");
		description = cfg.value("Description", "");
		tcpPort = udpPort = cfg.value("Port", DEFAULT_PORT);
		tcpSendBufferSize = cfg.value("TCPSendBufferSize", 131072);
		maxPlayers = cfg.value("MaxPlayers", 100);
		welcomeMessage = cfg.value("WelcomeMessage", "");
		restartOnCrash = cfg.value("RestartOnCrash", false);
		inventorySlotMax = cfg.value("InventorySlotMax", 3000);
		checkClientBuild = cfg.value("CheckClientBuild", false);
		allowedClientTimestamp = cfg.value("AllowedClientTimestamp", 0);
		allowedLauncherVersion = cfg.value("AllowedLauncherVersion", 67);
		maxRegistrationsPerIP = cfg.value("MaxRegistrationsPerIP", 1);
		crypt = cfg.value("Crypt", false);
		mainMenuSkinEvent = cfg.value("MainMenuSkinEvent", 0);
		if (cfg.contains("Metadata"))
		{
			ordered_json jMetadata = cfg["Metadata"];
			if (jMetadata.value("Maplist", false))
				metadataToSend |= kMetadataFlag_MapList;
			if (jMetadata.value("ClientTable", false))
				metadataToSend |= kMetadataFlag_ClientTable;
			if (jMetadata.value("Unk3", false))
				metadataToSend |= kMetadataFlag_Unk3;
			if (jMetadata.value("ItemBox", false))
				metadataToSend |= kMetadataFlag_ItemBox;
			if (jMetadata.value("Unk8", false))
				metadataToSend |= kMetadataFlag_Unk8;
			if (jMetadata.value("MatchOption", false))
				metadataToSend |= kMetadataFlag_MatchOption;
			if (jMetadata.value("Unk15", false))
				metadataToSend |= kMetadataFlag_Unk15;
			if (jMetadata.value("WeaponParts", false))
				metadataToSend |= kMetadataFlag_WeaponParts;
			if (jMetadata.value("Unk20", false))
				metadataToSend |= kMetadataFlag_Unk20;
			if (jMetadata.value("Encyclopedia", false))
				metadataToSend |= kMetadataFlag_Encyclopedia;
			if (jMetadata.value("GameModeList", false))
				metadataToSend |= kMetadataFlag_GameModeList;
			if (jMetadata.value("ProgressUnlock", false))
				metadataToSend |= kMetadataFlag_ProgressUnlock;
			if (jMetadata.value("WeaponPaint", false))
				metadataToSend |= kMetadataFlag_WeaponPaint;
			if (jMetadata.value("ReinforceMaxLvl", false))
				metadataToSend |= kMetadataFlag_ReinforceMaxLvl;
			if (jMetadata.value("ReinforceMaxEXP", false))
				metadataToSend |= kMetadataFlag_ReinforceMaxEXP;
			if (jMetadata.value("ReinforceItemsExp", false))
				metadataToSend |= kMetadataFlag_ReinforceItemsExp;
			if (jMetadata.value("Unk31", false))
				metadataToSend |= kMetadataFlag_Unk31;
			if (jMetadata.value("HonorMoneyShop", false))
				metadataToSend |= kMetadataFlag_HonorMoneyShop;
			if (jMetadata.value("ItemExpireTime", false))
				metadataToSend |= kMetadataFlag_ItemExpireTime;
			if (jMetadata.value("ScenarioTX_Common", false))
				metadataToSend |= kMetadataFlag_ScenarioTX_Common;
			if (jMetadata.value("ScenarioTX_Dedi", false))
				metadataToSend |= kMetadataFlag_ScenarioTX_Dedi;
			if (jMetadata.value("ShopItemList_Dedi", false))
				metadataToSend |= kMetadataFlag_ShopItemList_Dedi;
			if (jMetadata.value("ZBCompetitive", false))
				metadataToSend |= kMetadataFlag_ZBCompetitive;
			if (jMetadata.value("Unk43", false))
				metadataToSend |= kMetadataFlag_Unk43;
			if (jMetadata.value("Unk49", false))
				metadataToSend |= kMetadataFlag_Unk49;
			if (jMetadata.value("PPSystem", false))
				metadataToSend |= kMetadataFlag_PPSystem;
			if (jMetadata.value("CodisData", false))
				metadataToSend |= kMetadataFlag_CodisData;
			if (jMetadata.value("Item", false))
				metadataToSend |= kMetadataFlag_Item;
			if (jMetadata.value("WeaponProp", false))
				metadataToSend |= kMetadataFlag_WeaponProp;
			if (jMetadata.value("Hash", false))
				metadataToSend |= kMetadataFlag_Hash;
			if (jMetadata.value("RandomWeaponList", false))
				metadataToSend |= kMetadataFlag_RandomWeaponList;
		}
		if (cfg.contains("DefaultUser"))
		{
			ordered_json jDefUser = cfg["DefaultUser"];
			defUser.gameMaster = jDefUser.value("GameMaster", false);
			defUser.level = jDefUser.value("Level", 1);
			defUser.exp = jDefUser.value("Exp", 0);
			defUser.points = jDefUser.value("Points", 0);
			defUser.honorPoints = jDefUser.value("HonorPoints", 0);
			defUser.prefixID = jDefUser.value("PrefixID", 0);
			defUser.kills = jDefUser.value("Kills", 0);
			defUser.deaths = jDefUser.value("Deaths", 0);
			defUser.battles = jDefUser.value("Battles", 0);
			defUser.win = jDefUser.value("Win", 0);
			defUser.passwordBoxes = jDefUser.value("PasswordBoxes", 1);
			defUser.mileagePoints = jDefUser.value("MileagePoints", 0);
			defUser.defaultItems = jDefUser.value("DefaultItems", defUser.defaultItems);
			defUser.pseudoDefaultItems = jDefUser.value("PseudoDefaultItems", defUser.pseudoDefaultItems);

			if (jDefUser.contains("Loadouts"))
			{
				ordered_json jLoadouts = jDefUser["Loadouts"];
				for (auto& jItem : jLoadouts)
				{
					vector<int> items;
					items.push_back(jItem.value("1", 0));
					items.push_back(jItem.value("2", 0));
					items.push_back(jItem.value("3", 0));
					items.push_back(jItem.value("4", 0));

					defUser.loadouts.m_Loadouts.push_back(items);
				}

				int loadoutCount = defUser.loadouts.m_Loadouts.size();
				if (loadoutCount < LOADOUT_COUNT)
				{
					for (int i = 0; i < (LOADOUT_COUNT - loadoutCount); i++)
					{
						vector<int> items;
						items.push_back(12);
						items.push_back(2);
						items.push_back(161);
						items.push_back(31);

						defUser.loadouts.m_Loadouts.push_back(items);
					}
				}
			}
			else
			{
				Console().Warn("CServerInstance::ParseServerConfig: kvLoadouts == NULL\n");
			}

			if (jDefUser.contains("BuyMenu"))
			{
				ordered_json jBuyMenu = jDefUser["BuyMenu"];
				for (auto& jItem : jBuyMenu)
				{
					vector<int> items;
					for (int i = 0; i < BUYMENU_SLOT_COUNT; i++)
					{
						items.push_back(jItem.value(to_string(i), 0));
					}
					defUser.buyMenu.push_back(CUserBuyMenu(items));
				}

				int buyMenuCount = defUser.buyMenu.size();
				if (buyMenuCount < BUYMENU_COUNT)
				{
					for (int i = 0; i < (BUYMENU_COUNT - buyMenuCount); i++)
					{
						vector<int> items;
						for (int _i = 0; _i < BUYMENU_SLOT_COUNT; _i++)
						{
							items.push_back(0);
						}

						defUser.buyMenu.push_back(CUserBuyMenu(items));
					}
				}
			}
			else
			{
				Console().Warn("CServerInstance::ParseServerConfig: kvBuyMenu == NULL\n");
			}
		}
		else
		{
			Console().Warn("CServerInstance::ParseServerConfig: kvDefaultUser == NULL\n");
		}

		if (cfg.contains("Notices"))
		{
			json jNotices = cfg["Notices"];
			// should we check for object?
			//if (jNotices.is_object())
			auto obj = jNotices.get<json::object_t>();
			for (auto& kvp : obj)
			{
				Notice_s notice;
				notice.id = stoi(kvp.first);
				notice.type = (NoticeType)kvp.second.value("Type", 0);
				notice.name = kvp.second.value("Name", "");
				notice.description = kvp.second.value("Description", "");

				notices.push_back(notice);
			}
		}

		if (cfg.contains("GameMatch"))
		{
			json jGameMatch = cfg["GameMatch"];
			if (jGameMatch.contains("CalcResults"))
			{
				json jResult = jGameMatch["CalcResults"];
				if (jResult.contains("GameModeCoefficient"))
				{
					json jGameModeCoefficient = jResult["GameModeCoefficient"];
					auto obj = jGameModeCoefficient.get<json::object_t>();
					for (auto& kvp : obj)
					{
						GameMatchCoefficients_s result;
						result.gameMode = stoi(kvp.first);
						result.exp = kvp.second.value("Exp", 0);
						result.points = kvp.second.value("Points", 0);

						gameMatch.gameModeCoefficients.push_back(result);
					}
				}
				if (jResult.contains("BonusPercentage"))
				{
					json jBonusPercentage = jResult["BonusPercentage"];
					if (jBonusPercentage.contains("Items"))
					{
						json jItems = jBonusPercentage["Items"];
						auto obj = jItems.get<json::object_t>();
						for (auto& kvp : obj)
						{
							BonusPercentage_s result;
							result.itemID = stoi(kvp.first);
							result.exp = kvp.second.value("Exp", 0);
							result.points = kvp.second.value("Points", 0);

							gameMatch.bonusPercentageItems.push_back(result);
						}
					}
					if (jBonusPercentage.contains("Classes"))
					{
						json jClasses = jBonusPercentage["Classes"];
						auto obj = jClasses.get<json::object_t>();
						for (auto& kvp : obj)
						{
							BonusPercentage_s result;
							result.itemID = stoi(kvp.first);
							result.exp = kvp.second.value("Exp", 0);
							result.points = kvp.second.value("Points", 0);

							gameMatch.bonusPercentageClasses.push_back(result);
						}
					}
				}
			}
		}

		if (cfg.contains("Room"))
		{
			json jRoom = cfg["Room"];
			room.connectingMethod = jRoom.value("HostConnectingMethod", 0);
			room.validateSettings = jRoom.value("ValidateSettings", false);
		}

		if (cfg.contains("MiniGames"))
		{
			json jMiniGames = cfg["MiniGames"];
			if (jMiniGames.contains("Bingo"))
			{
				json jBingo = jMiniGames["Bingo"];
				if (jBingo.value("Active", 0))
					activeMiniGamesFlag |= kEventFlag_Bingo;

				if (jBingo.contains("Items"))
				{
					json jItems = jBingo["Items"];
					for (auto iItem : jItems.items())
					{
						json jItem = iItem.value();

						RewardItem rewardItem;
						rewardItem.itemID = stoi(iItem.key());
						rewardItem.count = jItem.value("Count", 1);
						rewardItem.duration = jItem.value("Duration", 0);

						bingo.prizeItems.push_back(rewardItem);
					}
				}
			}

			if (jMiniGames.contains("WeaponRelease"))
			{
				json jWeaponRelease = jMiniGames["WeaponRelease"];
				if (jWeaponRelease.value("Active", 0))
					activeMiniGamesFlag |= kEventFlag_WeaponRelease;

				if (jWeaponRelease.contains("Items"))
				{
					json jItems = jWeaponRelease["Items"];
					for (auto iItem : jItems.items())
					{
						json jItem = iItem.value();

						WeaponReleaseConfigRow row;
						row.item.itemID = stoi(iItem.key());
						row.item.count = jItem.value("Count", 1);
						row.item.duration = jItem.value("Duration", 0);
						row.rowName = jItem.value("Name", "");

						weaponRelease.rows.push_back(row);
					}
				}

				vector<string> chars = jWeaponRelease.value("Characters", vector<string>{});
				for (auto& str : chars)
				{
					weaponRelease.characters.push_back(str[0]);
				}
			}
		}

		flockingFlyerType = cfg.value("FlockingFlyerType", 0);

		if (cfg.contains("NameBlacklist"))
		{
			json jNameBlacklist = cfg["NameBlacklist"];
			for (auto& name : jNameBlacklist)
			{
				nameBlacklist.push_back(name);
			}
		}

		if (cfg.contains("Surveys"))
		{
			ordered_json jSurveys = cfg["Surveys"];
			for (auto& iSurvey : jSurveys.items())
			{
				ordered_json jSurvey = iSurvey.value();

				Survey survey = {};
				survey.id = stoi(iSurvey.key());
				survey.title = jSurvey.value("Title", "");
				if (jSurvey.contains("Questions"))
				{
					ordered_json jQuestions = jSurvey["Questions"];
					for (auto& iQuestion : jQuestions.items())
					{
						ordered_json jQuestion = iQuestion.value();

						SurveyQuestion question = {};
						question.id = stoi(iQuestion.key());
						question.question = jQuestion.value("Question", "");
						question.answerType = jQuestion.value("AnswerType", 0);
						question.answerCheckBoxType = jQuestion.value("AnswerCheckBoxType", 0);

						if (jQuestion.contains("Answers"))
						{
							ordered_json jAnswers = jQuestion["Answers"];
							if (question.answerType == 1)
							{
								question.answerTextEntry.unk = jAnswers.value("Unk", 0);
							}
							else
							{
								for (auto& iAnswer : jAnswers.items())
								{
									ordered_json jAnswer = iAnswer.value();

									SurveyQuestionAnswerCheckBox answer = {};
									answer.id = stoi(iAnswer.key());
									answer.answer = jAnswer.value("Answer", "");
									question.answersCheckBox.push_back(answer);
								}
							}
						}
						survey.questions.push_back(question);
					}

				}	
				surveys.push_back(survey);
			}
		}
	}
	catch (exception& ex)
	{
		Console().FatalError("CServerConfig::Load: an error occured while parsing ServerConfig.json: %s\n", ex.what());
		return false;
	}

	return true;
}

void CServerConfig::LoadDefaultConfig(ordered_json& cfg)
{
	// или просто вьебать целый файл без ебли
	cfg = ordered_json::parse(defaultServerConfig, nullptr, true, true);

	//cfg = ordered_json();

	//cfg["HostName"] = "CSO Server";
	//cfg["Description"] = "123";
	//cfg["Port"] = DEFAULT_PORT;
	//cfg["TCPSendBufferSize"] = 131072;
	//cfg["MaxPlayers"] = 100;
	//cfg["WelcomeMessage"] = "vk.com/csnz_server";
	//cfg["RestartOnCrash"] = false;
	//cfg["InventorySlotMax"] = 3000;
	//cfg["CheckClientBuild"] = false;
	//cfg["MaxRegistrationsPerIP"] = 3;
	//cfg["Crypt"] = false;

	//static vector<int> defaultItems{ 175, 459, 460, 8079, 8080, 8138, 358, 136, 137, 25, 305, 138, 26, 306, 27,
	//	28, 252, 29, 30, 31, 200, 201, 214, 8222, 215, 216, 163, 8115, 390, 391, 40, 41, 42, 43, 44, 49, 50, 51, 52, 53 };

	//static vector<int> pseudoDefaultItems{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 161, 525}; // pseudo default item list

	//cfg["Metadata"] = {
	//	{"Maplist", true},
	//	{"ClientTable", true},
	//	{"Unk3", false},
	//	{"ItemBox", true},
	//	{"WeaponPaint", true},
	//	{"Unk8", true},
	//	{"MatchOption", true},
	//	{"Unk15", true},
	//	{"WeaponParts", true},
	//	{"Unk20", false},
	//	{"Encyclopedia", false},
	//	{"GameModeList", true},
	//	{"ProgressUnlock", true},
	//	{"ReinforceMaxLvl", true},
	//	{"ReinforceMaxEXP", true},
	//	{"ReinforceItemsExp", true},
	//	{"Unk31", true},
	//	{"HonorMoneyShop", false},
	//	{"ItemExpireTime", true},
	//	{"ScenarioTX_Common", true},
	//	{"ScenarioTX_Dedi", true},
	//	{"ShopItemList_Dedi", true},
	//	{"ZBCompetitive", true},
	//	//{"Hash", false},
	//};

	//cfg["DefaultUser"] = {
	//	{"GameMaster", true},
	//	{"Level", DEFAULTUSER_LEVEL},
	//	{"Exp", DEFAULTUSER_EXP},
	//	{"Points", DEFAULTUSER_POINTS},
	//	{"HonorPoints", 0},
	//	{"PrefixID", 0},
	//	{"Kills", 0},
	//	{"Deaths", 0},
	//	{"Battles", 0},
	//	{"Win", 0},
	//	{"PasswordBoxes", 1},
	//	{"MileagePoints", 0},
	//	{"DefaultItems", defaultItems},
	//	{"PseudoDefaultItems", pseudoDefaultItems},
	//};

	//for (int i = 0; i < LOADOUT_COUNT; i++)
	//{
	//	cfg["DefaultUser"]["Loadouts"][to_string(i)] = {
	//		{"1", 12}, // MP5
	//		{"2", 2}, // P228
	//		{"3", 161}, // Default knife
	//		{"4", 31}, // HE grenade
	//	};
	//}

	//static const int buyMenuDefault[BUYMENU_COUNT][BUYMENU_SLOT_COUNT] = {
	//	{3, 6, 2, 4, 1, 0, 0, 0, 0}, // t Secondary
	//	{7, 8, 38, 0, 0, 0, 0, 0, 0}, // t Shotgun
	//	{10, 12, 13, 11, 37, 0, 0, 0, 0}, // t SMG
	//	{23, 21, 17, 19, 14, 22, 34, 39, 114}, // t Rifle
	//	{24, 32, 525, 0, 0, 0, 0, 0, 0}, // t MG
	//	{27, 28, 30, 31, 26, 25, 0, 0, 0}, // t Special
	//	{41, 42, 43, 40, 44, 45, 46, 47, 48}, // t Class
	//	{3, 6, 2, 4, 5, 0, 0, 0, 0}, // ct Secondary
	//	{7, 8, 0, 0, 0, 0, 0, 0, 0}, // ct Shotgun
	//	{9, 12, 13, 11, 36, 37, 0, 0, 0}, // ct SMG
	//	{20, 17, 15, 16, 18, 14, 33, 35, 113}, // ct Rifle
	//	{24, 32, 525, 0, 0, 0, 0, 0, 0}, // ct MG
	//	{27, 28, 30, 31, 26, 25, 29, 0, 0}, // ct Special
	//	{49, 50, 52, 51, 53, 54, 55, 56, 57}, // ct Class
	//	{161, 0, 0, 0, 0, 0, 0, 0, 0}, // t Melee
	//	{161, 0, 0, 0, 0, 0, 0, 0, 0}, // ct Melee
	//	{1, 163, 460, 175, 8079, 8080, 390, 391, 459}, // zb
	//};
	//
	//ordered_json buyMenu = ordered_json({});
	//for (int i = 0; i < BUYMENU_COUNT; i++)
	//{
	//	for (int j = 0; j < BUYMENU_SLOT_COUNT; j++)
	//	{
	//		buyMenu[to_string(i)][to_string(j)] = buyMenuDefault[i][j];
	//	}
	//}

	//cfg["DefaultUser"]["BuyMenu"] = buyMenu;

	//// add default notice
	//cfg["Notices"]["1"]["Type"] = 1;
	//cfg["Notices"]["1"]["Name"] = "Server information";
	//cfg["Notices"]["1"]["Description"] = "Welcome to the CSO server. Report about bugs in vk group: vk.com/csnz_server";
	//cfg["Notices"]["1"]["StartDate"] = GetCurrentTime();
	//cfg["Notices"]["1"]["EndDate"] = GetCurrentTime();

	ofstream f("ServerConfig.json");
	f << setfill('\t') << setw(1) << cfg << endl;
}