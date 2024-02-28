#include "Commands.h"

using namespace C3;

void Commands::Load()
{
	// TODO: parse config files
}

bool Commands::Parse(const std::string& a_command, RE::TESObjectREFR* a_ref)
{
	// yes, i indeed have no idea how to write a good parser

	auto formId = a_ref ? a_ref->GetFormID() : 0;

	logger::info("command registered = {}, target = {}", a_command, formId);

	std::istringstream iss(a_command);
	std::vector<std::string> tokens;
	std::string token;

	while (iss >> std::quoted(token)) {
		tokens.push_back(token);
		logger::info("token: {}", token);
	}

	if (tokens.size() < 2)
		return false;

	if (auto cmd = GetCmd(tokens[0]))
	{
		if (auto sub = cmd->GetSub(tokens[1]))
		{

			// parse arguments

			// todo: check selected argument
			
			std::vector<std::pair<Arg*, std::string_view>> matches;
			std::vector<std::string> invalid;

			for (int i = 3; i < tokens.size(); i++)
			{
				if (!invalid.empty())
					break;

				token = tokens[i];
				if (token.starts_with("-") || token.starts_with("--"))
				{
					// named argument
					if (auto arg = sub->GetArg(token))
					{
						if (arg->action == Arg::Action::StoreTrue)
						{
							matches.push_back({ arg, "true" });
						}
						else
						{
							std::string val;
							if ((i + 1) < tokens.size())
							{
								val = tokens[i];
							}
							matches.push_back({ arg, val });

							i++;
						}
					} 
					else
					{
						invalid.push_back(token);
					}
				}
				else
				{
					if (matches.size() > sub->args.size()) {
						invalid.push_back(token);
					}
					else
					{
						matches.push_back({ &sub->args[i - 3], token });
					}
				}

			}

			if (invalid.empty())
			{
				// parse values and check types
			}
			else
			{
				std::string err{ "unrecognized arguments: " };
				for (auto& arg : invalid)
				{
					err += arg;
					err += " ";
				}
				PrintErr(cmd, sub, err);
			}
		}
		else
		{
			PrintErr(cmd, std::format("unrecognized subcommand {}", tokens[1]));
		}

		return true;
	}

	return false;
}

void Commands::Print(const std::string& a_str)
{
	std::string str{ stl::safe_string(a_str.c_str()) };
	auto task = SKSE::GetTaskInterface();
	task->AddTask([=]() {
		auto console = RE::ConsoleLog::GetSingleton();
		if (console) {
			console->Print(str.c_str());
		}
	});
}

void Commands::PrintErr(Command* a_cmd, std::string a_str)
{
	Print(std::format("ERROR {}: {}.\nUse help for usage information.", a_cmd->name, a_str));
}

void Commands::PrintErr(Command* a_cmd, SubCommand* a_sub, std::string a_str)
{
	auto cmb = std::format("{} {}", a_cmd->name, a_sub->name);
	Print(std::format("ERROR {}: {}.\nUse {} help for usage information.", cmb, a_str, cmb));
}