# Shop.json
### Fields
```
{
	"Version": <int>, // config version (def. 0)
	"Recommended": { // recommended items(itemID) (array)
		"<int>": [ // page ID
			<int> // product ID
		]
	},
	"Popular": [<int>], // popular items(itemID) (array)
	// Shop list
	"Products": { // products (array)
		"<int>": { // itemID
			"IsPoints": <bool>, // product is costs points (def. 0)
			"SubProducts": { // array of subproducts (def. empty)
				"<int>": { // sub product ID
					"Count": <int>, // item count (def. 1)
					"Duration": <int>, // item duration (def. 0)
					"Price": <int>, // item price (points or cash) (def. 0)
					"AdditionalPoints": <int>, // number of points that will be given after purchase (def. 0)
					"AdditionalClanPoints": <int>, // number of clan points that will be given after purchase (def. 0)
					"AdType": <int> // advertisement type (def. 0)
				}
			}
		}
	}
}
```
def. means that if the field is undefined, the default value will be used.