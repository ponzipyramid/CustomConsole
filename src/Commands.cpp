#include "Commands.h"
#include "Util.h"

using namespace C3;
namespace fs = std::filesystem;

void Commands::Load()
{
	// TODO: parse config files
	std::string dir{ "Data/SKSE/CustomConsole" };

	logger::info("loading commands");

	for (const auto& entry : fs::directory_iterator(dir)) {
		if (entry.is_directory())
			continue;

		auto path = entry.path();
		
		logger::info("parsing {}", path.string());
		
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
	std::string split;

	while (iss >> std::quoted(split)) {
		tokens.push_back(split);
		logger::info("token: {}", split);
	}

	if (auto cmd = GetCmd(tokens[0])) {

		logger::info("command {} recognized", cmd->name);
		
		if (tokens.size() == 1) {
			PrintErr("subcommand required");
			return true;
		}

		if (auto sub = cmd->GetSub(tokens[1])) {
			logger::info("subcommand {} recognized", sub->name);

			std::unordered_map<std::string_view, std::string> flags;
			std::vector<std::string> positional;
			std::string invalid;

			// TODO: add support for --flag=value pattern
			for (int i = 2; i < tokens.size(); i++) {
				const auto& token = tokens[i];

				if (token.starts_with("-") || token.starts_with("--")) {
					if (auto arg = sub->GetFlag(token)) {
						if (arg->flag) {
							flags[arg->name] = "true";
						} else {
							if ((i + 1) < tokens.size() && !tokens[i + 1].starts_with("--") && !tokens[i + 1].starts_with("-")) {
								flags[arg->name] = tokens[i + 1];
							}
						}
					} else {
						invalid += token;
						invalid += " ";
					}
				} else {
					logger::info("adding {} to positional", token);
					positional.push_back(token);	
				}
			}

			if (!invalid.empty()) {
				PrintErr(cmd, std::format("unrecognized flag arguments: {}", invalid));
				return true;
			}

			std::vector<std::string> values;
			values.resize(sub->args.size());

			std::string missing;

			logger::info("Keys: {}", sub->all.size());
			for (auto& [key, _] : sub->all) {
				logger::info("key: |{}|", key);
			}

			int pos = 0;
			for (const auto& arg : sub->args) {
				logger::info("{} index found |{}|", arg.name, sub->all.count(arg.name));
				int index = sub->all[arg.name];

				if (arg.positional) {
					if (pos == positional.size()) {
						missing += arg.name;
						missing += " ";
					} else {
						logger::info("setting index {} to {}", index, positional[pos]);
						values[index] = positional[pos];
						pos++;
					}
				} else {
					if (flags.count(arg.name)) {
						values[index] = flags[arg.name];
					} else if (!arg.required) {
						values[index] = Util::GetDefault(arg.type);
					} else {
						missing += arg.name;
						missing += " ";
					}
				}
			}

			if (!missing.empty()) {
				PrintErr(cmd, std::format("missing arguments {}", missing));
				return true;
			}		
			
			auto onResult = [](const RE::BSScript::Variable& a_var) {
				logger::info("received callback value = {}", a_var.GetString());
				// TODO: check return type and format appropriately w/ user-supplied template if available
				Print(std::string{a_var.GetString()});
			};

			Util::InvokeFuncWithArgs(cmd->script, sub->func, sub->args, values, onResult);

		} else {
			PrintErr(cmd, std::format("invalid subcommand {}", tokens[1]));
			return true;
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

void Commands::PrintErr(std::string a_str)
{
	logger::error("{}", a_str);
	Print(std::format("ERROR {}", a_str));
}

void Commands::PrintErr(Command* a_cmd, std::string a_str)
{
	logger::error("{}", a_str);
	Print(std::format("ERROR {}", a_str));
	Print(a_cmd->Help());
}