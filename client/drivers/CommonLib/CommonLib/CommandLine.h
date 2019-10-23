#pragma once

#include <string>
#include <map>

namespace szi
{
	class CommandLine 
	{
		typedef std::map<std::wstring, std::wstring> CommandParam;
	public:
		CommandLine();
		~CommandLine();
		
		void Parse(int argc, wchar_t* argv[]);
		std::wstring GetCommand(const std::wstring&) const;
		int GetCommandAsInt(const std::wstring&) const;
		unsigned long GetCommandAsUlong(const std::wstring& command) const;
		bool IsCommand(const std::wstring& command) const;
	private:
		CommandParam m_command;
	};

}

