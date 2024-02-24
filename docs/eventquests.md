# EventQuests.json
### Fields
```
{
	"Version": <int>, // config version (def. 0)
	// Quest list
	"<int>": { // Quest ID
		"Active": <bool>, // is quest active (def. false)
		"Tasks": { // tasks (array)
			"Goal": <int>, // number of goal points needed to finish task (def. 1)
			"RewardID": <int>, // reward ID from ItemRewards.json (def. 0)
			"Notice": { // notice message shown on client
				"Goal": <int>, // show progress message on client every X points (def. 0)
				"UserMsg": <string>, // progress message (def. "%d/%d");
			},
			"Conditions": { // conditions needed to earn goal points (array)
				"<int>": { // conditition ID
					"GameModes": [<int>], // game mode conditition (array) (def. empty)
					"Maps": [<int>], // maps condition (array) (def. empty)
					"PlayerCount": <int>, // minimum number of players needed to finish task (def. 0)
					"GoalPoints": <int>, // number of goal points that will be given to user (def. 1)
					"EventType": <int>, // type of event (check QuestTaskEventType structure) (def. 0)
					// Variables for event type
					// EVENT_KILL (2):
					"KillerTeam": <int>, // -1 - ignore, 1 - tr, 2 - ct (def. -1)
					"VictimKillType": <int>, // -1 - ignore, 1 - headshot, 2 - knife, 16 - grenade, 32 - sentry gun (human scenario), 144 - zombie bomb, 512 - air strike (def. -1) 
					"VictimTeam": <int>, // -1 - ignore, 1 - tr, 2 - ct (def. -1)
					"Continuous": <int>, // 1 - reset condition progress if user didn't follow them (def. 0)
					"Gun": <int>, // -1 - ignore 0 - melee, 1 - primary, 2 - secondary, 3 - he (def. -1)
					"GunID": <int>, // item id of the gun (def. -1)
					"CheckForGun": <bool>, // check for gun id (def. false)
					"Bot": <int>, // victim is bot, -1 - ignore, 0 - not bot, 1 - bot (def. -1)
					// EVENT_KILLMONSTER (10):
					"MonsterType": <int>, // monster(zombie) ID
				}
			}
		}
	}
}
```
def. means that if the field is undefined, the default value will be used.