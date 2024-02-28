#pragma once

namespace C3
{
	struct ArgValue
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

		int intVal;
		bool bVal;
		float fltVal;
		std::string strVal;
		RE::TESForm* formVal;
		RE::Actor* actorVal;
		RE::TESObjectREFR* objectRefVal;
		RE::TESObjectARMO* armVal;
		Type type;
	};

	struct Arg
	{
		enum Action
		{
			StoreTrue
		};

		ArgValue& CreateValue(std::string_view a_val)
		{
			ArgValue val;

			val.type = type;
			val.strVal = std::string{ a_val };

			return val;
		}

		std::string name;
		std::string flag;
		bool selected = false;
		ArgValue::Type type;
		Action action;
	};

	struct SubCommand
	{

		inline Arg* GetArg(std::string_view a_str) { return argsByFlag[a_str]; }
		std::string name;
		std::string flag;
		std::vector<Arg> args;
		std::unordered_map<std::string_view, Arg*> argsByFlag;
	};

	struct Command
	{
		inline SubCommand* GetSub(std::string_view a_str) { return subs.count(a_str) ? &subs[a_str] : nullptr; }
		std::string name;
		std::string flag;
		std::string script;
		std::unordered_map<std::string_view, SubCommand> subs;
	};
}