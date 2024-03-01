#pragma once

namespace C3
{

	struct Arg
	{
		enum Type
		{
			Int,
			Bool,
			Float,
			String,
			Object,
		};

		std::string Help() {
			return help;
		}

		std::string name;
		std::string help;
		std::string defaultVal;
		std::string alias;
		Type type;
		std::string rawType;
		bool positional = false;
		bool selected = false;
		bool flag = false;
		bool required = false;
	};

	struct SubCommand
	{
		std::string Help()
		{
			std::string msg{ help };
			for (auto& arg : args) {
				msg += arg.Help();
				msg += "\n";
			}
			return msg;
		}

		Arg* GetFlag(std::string a_name) { return flags.count(a_name) ? &args[flags[a_name]] : nullptr; }
		
		std::string name;
		std::string func;
		std::string help;
		std::string alias;
		std::vector<Arg> args;
		std::unordered_map<std::string, int> flags;
		std::unordered_map<std::string, int> all;
	};

	struct Command
	{
		std::string Help() 
		{
			return help;
		}

		std::string Help(SubCommand* a_sub)
		{
			return Help() + "\n" + a_sub->Help();
		}

		SubCommand* GetSub(std::string a_name) { return subs.count(a_name) ? &subs[a_name] : nullptr; } 

		std::string name;
		std::string help;
		std::string alias;
		std::string script;
		std::unordered_map<std::string, SubCommand> subs;

	};
}

namespace YAML
{
	template <>
	struct convert<C3::Arg>
	{
		static bool decode(const Node& node, C3::Arg& rhs)
		{
			rhs.name = node["name"].as<std::string>("");
			rhs.help = node["help"].as<std::string>("");
			rhs.defaultVal = node["default"].as<std::string>("");
			rhs.alias = node["aliases"].as<std::string>("");
			rhs.selected = node["selected"].as<boolean>(false);
			rhs.flag = node["flag"].as<boolean>(false);
			rhs.required = node["required"].as<boolean>(false);

			rhs.positional = !rhs.name.starts_with("-");

			rhs.rawType = node["type"].as<std::string>("");
			rhs.type = magic_enum::enum_cast<C3::Arg::Type>(rhs.rawType, magic_enum::case_insensitive).value_or(C3::Arg::Type::Object);

			return !rhs.name.empty();
		}
	};

	template <>
	struct convert<C3::SubCommand>
	{
		static bool decode(const Node& node, C3::SubCommand& rhs)
		{
			rhs.name = node["name"].as<std::string>("");
			rhs.help = node["help"].as<std::string>("");
			rhs.alias = node["alias"].as<std::string>("");
			rhs.func = node["func"].as<std::string>("");
			
			// TODO: arg name/alias collision checks
			rhs.args = node["args"].as<std::vector<C3::Arg>>(std::vector<C3::Arg>{});
			for (int i = 0; i < rhs.args.size(); i++) {
				auto& arg = rhs.args[i];

				if (!arg.positional) {
					rhs.flags[arg.name] = i;
					if (!rhs.alias.empty())
						rhs.flags[arg.alias] = i;
				}

				logger::info("assigning {} to {}", arg.name, i);
				rhs.all[arg.name] = i;
			}

			return !rhs.name.empty() && !rhs.func.empty();
		}
	};

	template <>
	struct convert<C3::Command>
	{
		static bool decode(const Node& node, C3::Command& rhs)
		{
			rhs.name = node["name"].as<std::string>("");
			rhs.help = node["help"].as<std::string>("");
			rhs.alias = node["alias"].as<std::string>("");
			rhs.script = node["script"].as<std::string>("");

			// TODO: sub name/alias collision checks
			auto subs = node["subs"].as<std::vector<C3::SubCommand>>(std::vector<C3::SubCommand>{});
			for (auto& sub : subs) {
				rhs.subs[sub.name] = sub;
				rhs.subs[sub.alias] = sub;
			}

			return !rhs.name.empty() && !rhs.script.empty();
		}
	};
}