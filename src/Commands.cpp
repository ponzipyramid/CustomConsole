#include "Commands.h"
#include "Util.h"

using namespace C3;
namespace fs = std::filesystem;

void Commands::Load()
{
	// TODO: parse config files
	std::string dir{ "Data/SKSE/ConsoleExtender" };

	for (const auto& entry : fs::directory_iterator(dir)) {
		if (entry.is_directory())
			continue;

		auto path = entry.path();
		if (path.extension() == ".yaml" || path.extension() == ".yaml") {
			try {
				YAML::Node node = YAML::LoadFile(path.string());
				auto command = node.as<Command>();
				logger::info("registering command {} {} w/ {} subcommands", command.name, command.alias, command.subs.size());

				if (_commands.count(command.name)) {
					logger::error("{} already registered as a command - skipping", command.name);
					continue;
				}
				else
					_commands[command.name] = command;
				
				if (_commands.count(command.alias))
					logger::error("{} command alias already registered as a command - skipping", command.alias);
				else
					_commands[command.alias] = command;

				logger::info("registered command {}", command.name);

			} catch (std::exception& e) {
				logger::error("failed to create command from file: {} due to {}", path.string(), e.what());
			} catch (...) {
				logger::error("failed to create command from file: {}", path.string());
			}
		}
	}

}

bool Commands::Parse(const std::string& a_command, RE::TESObjectREFR* a_ref)
{
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

	if (auto cmd = GetCmd(tokens[0])) {

		Print(std::format("command {}, recognized", cmd->name));

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
	Print(std::format("ERROR {}: {}.", a_cmd->name, a_str));
}