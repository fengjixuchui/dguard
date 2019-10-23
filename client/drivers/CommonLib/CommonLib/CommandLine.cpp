#include "CommandLine.h"

namespace szi
{
	CommandLine::CommandLine() {
	}


	CommandLine::~CommandLine() {
	}

	void CommandLine::Parse(int argc, wchar_t* argv[]) 
	{
		m_command.clear();
		for(int i = 1; argc > i; )
		{
			const std::wstring command = argv[i];
			if (!command.empty())
			{
				if(i + 1 < argc)
				{
					if (argv[i + 1][0] != L'-')
					{
						m_command[command] = argv[i + 1];
						++i;
					} else
						m_command[command] = L"";
				} else
					m_command[command].clear();
			}
			++i;
		}
	}

	std::wstring CommandLine::GetCommand(const std::wstring& command) const
	{
		auto it = m_command.find(command);		
		return it != m_command.end() ? it->second : L"";
	}

	bool CommandLine::IsCommand(const std::wstring& command) const
	{
		return m_command.find(command) != m_command.end();
	}

	int CommandLine::GetCommandAsInt(const std::wstring& command) const
	{
		wchar_t* lEnd = nullptr;
		const std::wstring lValue = GetCommand(command);
		if (!lValue.empty())
		{
			return std::wcstoul(lValue.c_str(), &lEnd, 10);
		}
		return 0;
	}

	unsigned long CommandLine::GetCommandAsUlong(const std::wstring& command) const 
	{
		wchar_t* lEnd = nullptr;
		const std::wstring lValue = GetCommand(command);
		if(!lValue.empty()) 
		{
			return std::wcstoul(lValue.c_str(), &lEnd, 10);
		}
		return 0;
	}
}
