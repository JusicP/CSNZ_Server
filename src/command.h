#pragma once

#include <string>
#include <vector>
#include <functional>

/**
 * Class that represents command. Contains a name and function that will be executed by calling the Exec() method.
 * When creating a command object, it's added to list of commands and you can find it by its name (CCommandList::GetInstance().GetCommand(...)).
 */
class CCommand
{
public:
	CCommand(const std::string& name, const std::string& desc, const std::string& usage, const std::function<void(CCommand*, const std::vector<std::string>&)>& func);
	~CCommand();

	std::string GetName();
	std::string GetDescription();
	std::string GetUsage();

	void Exec(const std::vector<std::string>& args);

private:
	std::string m_Name;
	std::string m_Description;
	std::string m_Usage;
	std::function<void(CCommand*, const std::vector<std::string>&)> m_Func;
};

/**
 * Class for managing commands (add, remove, get)
 */
class CCommandList
{
private:
	CCommandList() = default;
	CCommandList(const CCommandList&) = delete;
	CCommandList(CCommandList&&) = delete;
	CCommandList& operator=(const CCommandList&) = delete;
	CCommandList& operator=(CCommandList&&) = delete;

public:
	static CCommandList& GetInstance();

	std::vector<std::string> GetCommandList();
	void AddCommand(CCommand* cmd);
	void RemoveCommand(CCommand* cmd);
	CCommand* GetCommand(const std::string& name);

private:
	std::vector<CCommand*> m_Commands;
};

/**
 * Singleton
 */
static CCommandList& CmdList()
{
	return CCommandList::GetInstance();
}
