# ServerConfig.json
### Fields
```
{
	"HostName": <string>, // unused (def. "CSO Server")
	"Description": <string>, // unused (def. "")
	"Port": <string>, // TCP and UDP ports used to connect to the server (def. "30002")
	"TCPSendBufferSize": <int>, // send buffer size for TCP (def. 131072)
	"MaxPlayers": <int>, // the maximum number of users that can login the server (def. 100)
	"WelcomeMessage": <string>, // the message that is displayed to user after logging in (def. "")
	"RestartOnCrash": <bool>, // restart the server on crash (def. false)
	"InventorySlotMax": <int>, // the maximum number of user inventory slots user can use (def. 4000)
	"CheckClientBuild": <bool>, // check client timestamp and launcher version (def. false)
	"AllowedClientTimestamp": <int>, // allowed timestamp of client libraries: hw.dll, client.dll, mp.dll, cstrike-online.exe, gameui.dll, vgui2.dll (CheckClientBuild must be true to work)
	"AllowedLauncherVersion": <int>, // allowed launcher version (def. 67) (CheckClientBuild must be true to work)
	"MaxRegistrationsPerIP": <int>, // the maximum number of users that can be registered on the same IP (def. 1)
	"SSL": <bool>, // enable/disable ssl encryption. requires a certificate file (Data/Certs/server-cert.pem) and a private key file (Data/Certs/server-key.pem) (def. false)
	"Crypt": <bool>, // enable/disable evp encryption (def. false)
	"MainMenuSkinEvent": <int>, // main menu skin event (5 - christmas, 6 - halloween, 7 - halloween + snow, 8 - easter bunny) (def. 0)
	"BanListMaxSize": <int>, // the maximum number of names a user can have on its ban list (def. 300)
	"Metadata": { // metadata configuration, enables/disables the sending of a given metadata to the client
		"Maplist": <bool>, // (def. true)
		"ClientTable": <bool>, // (def. true)
		"Unk3": <bool>, // (def. true)
		"ItemBox": <bool>, // (def. true)
		"Unk8": <bool>, // (def. true)
		"MatchOption": <bool>, // (def. true)
		"ZombieWarWeaponList": <bool>, // (def. true)
		"WeaponParts": <bool>, // (def. true)
		"Unk20": <bool>, // (def. true)
		"GameModeList": <bool>, // (def. true)
		"ProgressUnlock": <bool>, // (def. true)
		"WeaponPaints": <bool>, // (def. true)
		"ReinforceMaxLvl": <bool>, // (def. true)
		"ReinforceMaxEXP": <bool>, // (def. true)
		"ReinforceItemsExp": <bool>, // (def. true)
		"Unk31": <bool>, // (def. true)
		"HonorMoneyShop": <bool>, // (def. false)
		"ItemExpireTime": <bool>, // (def. true)
		"ScenarioTX_Common": <bool>, // (def. true)
		"ScenarioTX_Dedi": <bool>, // (def. true)
		"ShopItemList_Dedi": <bool>, // (def. true)
		"ZBCompetitive": <bool>, // (def. true)
		"Unk43": <bool>, // (def. true)
		"Unk49": <bool>, // (def. true)
		"PPSystem": <bool>, // (def. true)
		"CodisData": <bool>, // (def. true)
		"Item": <bool>, // (def. true)
		"WeaponProp": <bool>, // (def. true)
		"RandomWeaponList": <bool>, // (def. true)
		"ModeEvent": <bool>, // (def. true)
		"MileageShop": <bool>, // (def. false)
		"EventShop": <bool>, // (def. false)
		"FamilyTotalWarMap": <bool>, // (def. true)
		"FamilyTotalWar": <bool>, // (def. true)
		"Unk54": <bool>, // (def. true)
		"Unk55": <bool>, // (def. true)
		"WeaponAscend": <bool>, // (def. true)
		"Unk57": <bool>, // (def. true)
		"PerkParam": <bool>, // (def. true)
		"Synthesis": <bool>, // (def. true)
		"VoxelList": <bool>, // (def. true)
		"VoxelItem": <bool>, // (def. true)
		"Unk64": <bool>, // (def. true)
		"VoxelConfigList": <bool> // (def. true)
	},
	"DefaultUser": { // DefaultUser (default values that apply to the user after registration and character creation)
		"GameMaster": <bool>, // game master (admin) privilege (def. false)
		"Level": <int>, // (def. 1)
		"Exp": <int>, // (def. 0)
		"Points": <int>, // (def. 0)
		"HonorPoints": <int>, // (def. 0)
		"PrefixID": <int>, // (def. 0)
		"Kills": <int>, // (def. 0)
		"Deaths": <int>, // (def. 0)
		"Battles": <int>, // (def. 0)
		"Win": <int>, // (def. 0)
		"PasswordBoxes": <int>, // (def. 1)
		"MileagePoints": <int>, (// def. 0)
		"DefaultItems": [<int>], // (array) items that cannot be edited by user. (def. {175, 459, 460, 8079, 8080, 8138, 358, 136, 137, 25, 305, 138, 26, 306, 27,
				// 28, 252, 29, 30, 31, 200, 201, 214, 8222, 215, 216, 163, 8115, 390, 391, /*characters*/ 40, 41, 42, 43, 44, 49, 50, 51, 52, 53})
		"PseudoDefaultItems": [<int>], // (array) the same as DefaultItems but user can do whatever they want with these items (except remove). (def. {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 161, 525})
		// !!! Changing PseudoDefaultItems only affects new users!
		"Loadouts": { // loadout list, max. 12 loadout (array) (def. empty)
			"<int>": { // loadout slot
				"1": <int>, // itemID for 1 slot (def. 0)
				"2": <int>, // itemID for 2 slot (def. 0)
				"3": <int>, // itemID for 3 slot (def. 0) 
				"4": <int> // itemID for 4 slot (def. 0)
			}
		},
		"BuyMenu: { // buymenu (array) (def. empty)
		}
	},
	
	"Notices": { // notice information in main menu (array)
		"<int>": { // notice ID
			"Type": <int>, // type of the notice (0 - new, else not new) (def. 0)
			"Name":	<string>, // notice name (def. "")
			"Description": <string>, // notice description (def. "")
			"StartDate": <int>, // notice start date unix timestamp (def. 0)
			"EndDate": <int> // notice end date unix timestamp (def. 0)
		}
	},

	"GameMatch": { // game match configuration
		"CalcResults": { // game match results calculation cfg
			"GameModeCoefficient": { // multiplier settings for each game mode (array)
				"<int>": { // game mode ID
					"Exp": <int>, // exp coefficient (def. 0)
					"Points": <int> // points coefficient (def. 0)
				}
			},
			"BonusPercentage": { // bonus percentage for using specific items (classes and other items)
				"Items": { // array
					"<int>": { // itemID 
						"Exp": <int>, // exp percentage bonus (def. 0)
						"Points": <int> // points percentage bonus (def. 0)
					}
				},
				"Classes": { // array
					"<int>": { // class itemID 
						"Exp": <int>, // exp percentage bonus (def. 0)
						"Points": <int> // points percentage bonus (def. 0)
					}
				}
			}
		}
	},

	"Room": { // room configuration
		"HostConnectingMethod": <int>, // method that will be used to connect to the game host, 0 - peer to peer only, 1 - dedicated server only, 2 - peer to peer or dedicated server (def. 2)
		"ValidateSettings": <bool> // validate room settings (def. false)
	},

	"MiniGames": { // mini games configuration
		"Bingo": { // bingo minigame (currently broken)
			"Active": <bool>, // is mini game active (def. false)
			"Items": { // items (array)
				"<int>": { // itemID
					"Count": <int>, // count (def. 1)
					"Duration": <int> // duration (def. 0)
				}
			}
		},
		"WeaponRelease": { // weapon release minigame
			"Active": <bool>, // is mini game active (def. false)
			"Items": { // items (array)
				"<int>": { // itemID
					"Count": <int>, // count (def. 1)
					"Duration": <int>, // duration (def. 0)
					"Name": <string> // row name (def. "")
				}
			},
			"Characters": [ // character list (array) (def. empty)
				<string>
			]
		}
	},

	"FlockingFlyerType": <int>, // FlockingFlyer event, 0 - none, 1 - mosquito, 2 - kite (def. 0) 

	"NameBlacklist": [ // nickname blacklist (array) (def. empty)
		<string>
	],

	"Surveys": { // array
		"<int>": { // survey ID 
			"Title": <string>, // title (def. "")
			"Questions": { // questions (array)
				"<int>": { // question ID
					"Question": <string>, // question (def. "")
					"AnswerType": <int>, // answer type, 0 - checkboxes, 1 - textentry (def. 0)
					"AnswerCheckBoxType": <int>, // answer checkbox type, 0 - can't choose, 1 - choose only one, 2 - multiple choose (def. 0)
					"Answers": { // array
						"<int>":{	// answer ID
							"Answer": <string> // answer string (def. "")
						}
					}
				}
			}
		}
	},

	"DedicatedServerWhitelist": [ // dedicated server IP whitelist (array) (def. [ "127.0.0.1" ])
		<string>
	]
}
```
def. means that if the field is undefined, the default value will be used