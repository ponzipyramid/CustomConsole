#pragma

#include "Command.h"
#include "Script.h"

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
		default: 
			return "none";
		}
	}

	inline std::string BoolToString(bool b)
	{
		return b ? "true" : "false";
	}

	class VmCallback : public RE::BSScript::IStackCallbackFunctor
	{
	public:
		using OnResult = std::function<void(const RE::BSScript::Variable& result)>;

		static auto New(const OnResult& onResult_)
		{
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> res;
			res.reset(new VmCallback(onResult_));
			return res;
		}

		VmCallback(const OnResult& onResult_) :
			onResult(onResult_) {}

	private:
		void operator()(RE::BSScript::Variable result) override
		{
			onResult(result);
		}

		bool CanSave() const override { return false; }

		void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

		const OnResult onResult;
	};


	inline bool InvokeFuncWithArgs(std::string a_scr, std::string a_func, ArgValues a_vals)
	{
		logger::info("invoking {} in {} with {} arguments", a_func, a_scr, a_vals.size());

		std::string str{ "test" };

		auto args = RE::MakeFunctionArguments(std::move(str));
		auto onResult = [](const RE::BSScript::Variable& a_var) {
			logger::info("received callback value = {}", a_var.GetString());
		};

		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
		callback.reset(new VmCallback(onResult));

		auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();

		return vm->DispatchStaticCall("zadCommands", "LockDevice", args, callback);
	}
}