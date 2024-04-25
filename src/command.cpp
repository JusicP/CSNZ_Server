#include "command.h"
#include "common/utils.h"
#include "common/logger.h"

using namespace std;

CCommandList& CCommandList::GetInstance()
{
	static CCommandList cmdList;
	return cmdList;
}

/**
 * Gets all command names
 * @return std::string vector of command names
 */
vector<string> CCommandList::GetCommandList()
{
	vector<string> cmdlist;
	for (auto cmd : m_Commands)
	{
		cmdlist.push_back(cmd->GetName());
	}

	return cmdlist;
}

/**
 * Adds command to command list
 * @param cmd Pointer to command object
 */
void CCommandList::AddCommand(CCommand* cmd)
{
	if (GetCommand(cmd->GetName()))
	{
		Logger().Warn("CCommandList::AddCommand: command %s duplicate!\n", cmd->GetName().c_str());
		return;
	}

	m_Commands.push_back(cmd);
}

/**
 * Removes command from command list
 * @param cmd Pointer to command object
 */
void CCommandList::RemoveCommand(CCommand* cmd)
{
	m_Commands.erase(remove(begin(m_Commands), end(m_Commands), cmd), end(m_Commands));
}

/**
 * Gets command object by its name
 * @param name Command name
 * @return Pointer to command object, NULL if not found
 */
CCommand* CCommandList::GetCommand(const std::string& name)
{
	for (auto cmd : m_Commands)
	{
		if (cmd->GetName() == name)
		{
			return cmd;
		}
	}

	return NULL;
}

/**
 * Constructor. Adds command to command list
 * @param name Command name
 * @param desc Command description
 * @param usage Command usage (arguments)
 * @param func Command function
 */
CCommand::CCommand(const std::string& name, const std::string& desc, const std::string& usage, const function<void(CCommand*, const vector<string>&)>& func)
{
	m_Name = name;
	m_Description = desc;
	m_Usage = usage;
	m_Func = func;

	CCommandList::GetInstance().AddCommand(this);
}

/**
 * Destructor. Removes command from command list
 */
CCommand::~CCommand()
{
	CCommandList::GetInstance().RemoveCommand(this);
}

string CCommand::GetName()
{
	return m_Name;
}

string CCommand::GetDescription()
{
	return m_Description;
}

/**
 * Gets command usage, its arguments
 * @return Usage std::string
 */
string CCommand::GetUsage()
{
	return m_Usage;
}

/**
 * Executes command function with given arguments
 * @param args std::string vector of command arguments
 */
void CCommand::Exec(const vector<string>& args)
{
	m_Func(this, args);
}