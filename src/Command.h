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
			Form,
			Actor,
			ObjectRef,
			Armor,
			None
		};

		std::string name;
		std::string help;
		std::vector<std::string> aliases;
		Type type;		
		bool selected = false;
		bool flag = false;
		bool required = false;
	};

	struct SubCommand
	{
		std::string name;
		std::string func;
		std::string help;
		std::string alias;
		std::vector<Arg> args;
	};

	struct Command
	{
		std::string name;
		std::string help;
		std::string alias;
		std::string script;
		std::vector<SubCommand> subs;
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
			rhs.aliases = node["aliases"].as<std::vector<std::string>>(std::vector<std::string>{});
			rhs.selected = node["selected"].as<boolean>(false);
			rhs.flag = node["flag"].as<boolean>(false);
			rhs.required = node["required"].as<boolean>(false);

			auto type = node["type"].as<std::string>("");
			rhs.type = magic_enum::enum_cast<C3::Arg::Type>(type, magic_enum::case_insensitive).value_or(C3::Arg::Type::None);

			return !rhs.name.empty() && rhs.type != C3::Arg::Type::None;
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
			
			rhs.args = node["args"].as<std::vector<C3::Arg>>(std::vector<C3::Arg>{});

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

			rhs.subs = node["subs"].as<std::vector<C3::SubCommand>>(std::vector<C3::SubCommand>{});

			return !rhs.name.empty() && !rhs.script.empty();
		}
	};
}