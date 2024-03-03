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
	std::istringstream iss(a_command);
	std::vector<std::string> tokens;
	std::string split;

	while (iss >> std::quoted(split)) {
		tokens.push_back(split);
	}

	if (auto cmd = GetCmd(tokens[0])) {

		logger::info("command {} recognized", cmd->name);
		
		if (tokens.size() == 1) {
			PrintErr("subcommand required");
			return true;
		}

		if (auto sub = cmd->GetSub(tokens[1])) {
			logger::info("subcommand {} recognized", sub->name);

			std::unordered_map<std::string, std::string> flags;
			std::vector<std::string> positional;
			std::string unrecognized;
			std::string invalid;

			if (auto selected = sub->GetSelected()) {
				if (a_ref) {
					auto edid = a_ref->GetFormID() == 20 ? "player" : Util::GetEditorID(a_ref);
					if (!edid.empty()) {
						if (selected->positional)
							positional.emplace_back(edid);
						else
							flags[selected->name] = edid;
					}
				}
			}

			// TODO: add support for --flag=value pattern
			// TODO: add support for arrays
			for (int i = 2; i < tokens.size(); i++) {
				const auto token = tokens[i];

				if (token.starts_with("-")) {
					if (auto arg = sub->GetFlag(token)) {
						if (arg->flag) {
							flags[arg->name] = "true";
						} else if ((i + 1) < tokens.size() && !tokens[i + 1].starts_with("--") && !tokens[i + 1].starts_with("-")) {
							flags[arg->name] = tokens[i + 1];
							logger::info("adding {} to flags", tokens[i + 1]);
							i++;
						} else {
							invalid += arg->name;
							invalid += " ";
						}
					} else {
						unrecognized += token;
						unrecognized += " ";
					}
				} else {
					logger::info("adding {} to positional", token);
					positional.push_back(token);	
				}
			}

			if (!unrecognized.empty()) {
				PrintErr(cmd, std::format("unrecognized flag arguments: {}", unrecognized));
			}

			if (!invalid.empty()) {
				PrintErr(cmd, std::format("invalid flag arguments - was expecting value: {}", invalid));
			}

			if (!unrecognized.empty() || !invalid.empty()) {
				return true;
			}

			std::vector<std::string> values;
			values.resize(sub->args.size());

			std::string missing;

			int pos = 0;
			for (const auto& arg : sub->args) {
				int index = sub->all[arg.name];

				if (arg.positional && pos < positional.size()) {
					values[index] = positional[pos];
					logger::info("setting {} to {}", index, positional[pos]);
					pos++;
				} else if (!arg.positional && flags.count(arg.name)) {
					logger::info("setting {} to {}", index, flags[arg.name]);
					values[index] = flags[arg.name];
				} else if (!arg.required) {
					logger::info("setting {} to {}", index, Util::GetDefault(arg));
					values[index] = Util::GetDefault(arg);
				} else {
					logger::info("{} is missing", index);
					missing += arg.name;
					missing += " ";
				}
			}

			if (!missing.empty()) {
				PrintErr(cmd, std::format("missing arguments {}", missing));
				return true;
			}		
			
			auto onResult = [](const RE::BSScript::Variable& a_var) {
				using RawType = RE::BSScript::TypeInfo::RawType;
				std::string ret;
				
				switch (a_var.GetType().GetRawType())
				{
				case RawType::kNone:
					ret = "none";
					break;
				case RawType::kObject:
					// TODO: implement
					// maybe try to get the object, and then invoke GetFormID() on it?
					ret = "completed";
					break;
				case RawType::kString:
					ret = std::string{ a_var.GetString() };
					break;
				case RawType::kInt:
					ret = std::to_string(a_var.GetSInt());
					break;
				case RawType::kFloat:
					ret = std::to_string(a_var.GetFloat());
					break;
				case RawType::kBool:
					ret = Util::BoolToString(a_var.GetBool());
					break;
				default:
					// TODO: handle arrays
					ret = "completed";
					break;
				}
				logger::info("received callback value = {}", ret);
				Print(ret);
			};

			if (sub->close) {
				if (const auto queue = RE::UIMessageQueue::GetSingleton()) {
					queue->AddMessage(RE::Console::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
				}
			}

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