# ItemBox.json
### Fields
```
{
	"Version": <int>, // config version (def. 0)
	// Decoder list
	"<int>": { // Decoder item ID
		"Rate": { // array of rates
			"<double>": { // probability (from 0 to 100)
				"Grade": <int>, // item grade 1 - default, 2 - normal, 3 - advanced, 4 - premium (def. 0)
				"Duration": [<int>], // array of durations (in days) (def. empty)
				"Items": [<int>], // array of items (itemID) (def. empty)
			}
		}
	}
}
```
The sum of all rates for each decoder must be less or equal to 100.
def. means that if the field is undefined, the default value will be used.