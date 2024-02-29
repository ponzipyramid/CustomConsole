#pragma

#include "Command.h"


namespace C3::Util
{
	using ArgValues = std::vector<std::pair<Arg::Type, std::string>>;

	inline std::string GetDefault(Arg::Type a_type)
	{
		switch (a_type) {
		case Arg::Type::Int:
			return "0";
		case Arg::Type::Bool:
			return "false";
		case Arg::Type::Float:
			return "0.0";
		case Arg::Type::String:
			return "";
		case Arg::Type::Actor:
		case Arg::Type::ObjectRef:
		case Arg::Type::Armor:
		case Arg::Type::Form:
			return "none";
		}
	}

	inline std::string BoolToString(bool b)
	{
		return b ? "true" : "false";
	}

	inline bool InvokeFuncWithArgs(std::string a_scr, std::string a_func, ArgValues a_vals)
	{

	}
}