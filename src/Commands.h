#pragma once

#include "Command.h"

namespace C3
{
	class Commands
	{
	public:
		static void Load();
		static bool Parse(const std::string& a_command, RE::TESObjectREFR* a_ref);
	private:
		static void Print(const std::string& a_str);
		static void PrintErr(std::string a_str);
		static inline Command* GetCmd(std::string a_str) { return _commands.count(a_str) ? &_commands[a_str] : nullptr; }

		static inline std::unordered_map<std::string, Command> _commands;
	};
}