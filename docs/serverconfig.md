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
	"InventorySlotMax": <int>, // the maximum number of user inventory slots user can use (def. 3000)
	"CheckClientBuild": <bool>, // check client timestamp and launcher version (def. false)
	"AllowedClientTimestamp": <int>, // allowed timestamp of client libraries: hw.dll, client.dll, mp.dll, cstrike-online.exe, gameui.dll, vgui2.dll (CheckClientBuild must be true to work)
	"AllowedLauncherVersion": <int>, // allowed launcher version (def. 67) (CheckClientBuild must be true to work)
	"MaxRegistrationsPerIP": <int>, // the maximum number of users that can be registered on the same IP (def. 1)
	"MainMenuSkinEvent": <int>, // main menu skin event (5 - christmas, 6 - halloween, 7 - halloween + snow, 8 - easter bunny) (def. 0)
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
			"Type": <int>, // type of the notice (1 - hot, 2 - notice, 3 - event, 4 - etc) (def. 0)
			"Name":	<string>, // notice name (def. "")
			"Description": <string> // notice description (def. "")
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
		"HostConnectingMethod": <int>, // method that will be used to connect to the game host, 0 - direct connection/dedicated server (host restart functionality is unavailable), 1 - peer to peer connection (def. 0)
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
	}
}
```
def. means that if the field is undefined, the default value will be used