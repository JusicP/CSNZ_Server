# VoxelConfigList.json
### Fields
```
{
	"Version": <int>, // config version (def. 0)
	// Voxel Config List
	"VoxelConfigList": [ // voxel configs (array) (def. empty)
		{
			"Unk": <int>, // unknown (def. 0)
			"VxlURL": <string>, // studio vxl url, used by client to download studio maps (def. "")
			"VmgURL": <string>, // studio vmg url, used by client to download studio images (def. "")
			"HTTPIPList": [ // studio http ips (array), used by client to get studio map info (def. empty)
				{
					"IP": <string>, // ip or hostname (def. "")
					"Ports": [<int>] // ports (def. empty)
				}
			]
		}
	]
}
```
def. means that if the field is undefined, the default value will be used.

