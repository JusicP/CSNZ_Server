# Coding style
The code is written using Hungarian notation.

### Variable naming:
- g_tX for global variables;
- m_tX for class members;
- s_tX for static variables,
where "t" is type name prefix and "X" is qualifier. Qualifiers starts with a capital if type and scope present. For local scope you don't need to specify scope and type prefix
Example:

```
bool g_bButtonClicked;
bool g_bRunning;

void Function()
{
	bool var;
	...
}
```

### Type naming:
- "i" or "n": integer
- "b": boolean
- "sz": zero terminated string
- "p": pointer
- "f": float
- "d": double
- "dw": double word

If there is no type from list you need, you can leave type empty.

Example:
```
SomeStructure g_Structure;
```

For class:
```
class CName;
class IName;
```