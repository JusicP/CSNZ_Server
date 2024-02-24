# ItemRewards.json
### Fields
```
{
	"Version": <int>, // config version (def. 0)
	// Reward list
	"<int>": { // Reward ID
		"Select": <bool>, // allow user to select item (def. false)
		"LvlRestriction": <int>, // level restriction (def. 0)
		"Points": [<int>], // random number of point that will be given to user (array) (def. empty)
		"Exp": [<int>], // the same as points (array) (def. empty)
		"HonorPoints": [<int>], // the same as points (array) (def. empty)
		"Title": <string>, // reward title (def. "")
		"Description": <string>, // reward description (def. "")
		"Items": { // array of items
			"<int>": { // Item ID
				"Count": <int>, // count (def. 1)
				"Duration": <int> // duration (in days) (def. 0)
			}
		},
		"RandomItems": { // array of random items with drop probability
			"<int>": { // probability (array)
				"<int>": { // Item ID
					"Count": <int>, // count (def. 1)
					"Duration": <int> // duration (in days) (def. 0)
				}
			}
		}
	}
}
```
def. means that if the field is undefined, the default value will be used.