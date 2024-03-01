#pragma

#include "Command.h"
#include "Script.h"

namespace C3::Util
{
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
		case Arg::Type::Object:
		default: 
			return "none";
		}
	}

	inline bool IsEditorID(const std::string_view identifier) { return std::strchr(identifier.data(), '|') == nullptr; }
	inline std::pair<RE::FormID, std::string> GetFormIDAndPluginName(const std::string_view a_str)
	{
		if (const auto tilde{ std::strchr(a_str.data(), '|') }) {
			const auto tilde_pos{ static_cast<int>(tilde - a_str.data()) };
			return { std::stoi(std::string{ a_str.substr(0, tilde_pos) }, 0, 16), a_str.substr(tilde_pos + 1).data() };
		}
		logger::error("ERROR: Failed to get FormID and plugin name for {}", a_str);

		return { 0, "" };
	}

	inline std::string BoolToString(bool b)
	{
		return b ? "true" : "false";
	}

	inline RE::TESForm* StringToForm(std::string_view a_str)
	{
		if (IsEditorID(a_str)) {
			return RE::TESForm::LookupByEditorID(a_str);
		} else {
			const auto& [formId, modName] = GetFormIDAndPluginName(a_str);
			return RE::TESDataHandler::GetSingleton()->LookupForm(formId, modName);
		}
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

	class FunctionArguments : public RE::BSScript::IFunctionArguments
	{
	private:
		RE::BSScrapArray<RE::BSScript::Variable> _variables;

	public:
		FunctionArguments() noexcept = default;
		FunctionArguments(std::size_t capacity)
		{
			_variables.reserve((RE::BSTArrayBase::size_type) capacity);
		}
		FunctionArguments(const std::vector<Arg>& args, const std::vector<std::string>& values)
		{
			assert(args.size() == values.size());

			_variables.reserve((RE::BSTArrayBase::size_type) values.size());
			for (size_t i = 0; i < args.size(); i++) {
				const auto& arg = args[i];
				const auto& val = values[i];

				logger::info("Argument: {} {}", (int) arg.type, val);

				std::optional<RE::BSScript::Variable> scriptVariable;

				if (val == "none") {
					scriptVariable.emplace();
					scriptVariable->SetNone();
				} else {
					switch (arg.type) {
					case Arg::Type::Object:
						{
							const auto& objType = arg.rawType;

							RE::TESForm* form = nullptr;
							if (objType == "actor" && val == "player") {
								form = RE::PlayerCharacter::GetSingleton();
							} else {
								form = StringToForm(val);
							}

							if (!form)
								break;

							auto ptr = Script::GetObjectPtr(form, objType.c_str());

							if (!ptr)
								break;

							scriptVariable.emplace();
							scriptVariable->SetObject(std::move(ptr));

							break;
						}
					case Arg::Type::String:
						{
							scriptVariable.emplace();
							scriptVariable->SetString(val);

							break;
						}
					case Arg::Type::Int:
						{
							scriptVariable.emplace();
							scriptVariable->SetSInt(std::stoi(val));

							break;
						}
					case Arg::Type::Float:
						{
							scriptVariable.emplace();
							scriptVariable->SetFloat(std::stof(val));

							break;
						}
					case Arg::Type::Bool:
						{
							scriptVariable.emplace();
							scriptVariable->SetBool(val == "1" || val == "true" || val == "TRUE");

							break;
						}
					};
				}

				if (scriptVariable) {
					_variables.emplace_back(std::move(*scriptVariable));
				}
			}
		}
		~FunctionArguments() noexcept = default;

	public:
		void PushVariable(RE::BSScript::Variable variable)
		{
			_variables.emplace_back(std::move(variable));
		}

	public:
		bool operator()(RE::BSScrapArray<RE::BSScript::Variable>& destination) const override
		{
			destination = _variables;
			return true;
		}
	};

	inline bool InvokeFuncWithArgs(std::string a_scr, std::string a_func, std::vector<Arg>& a_args, std::vector<std::string>& a_vals, std::function<void(const RE::BSScript::Variable& a_var)> a_onResult)
	{
		logger::info("invoking {} in {} with {} arguments", a_func, a_scr, a_vals.size());


		//std::vector<std::string> forced{ "player", "none" };
		auto args = new FunctionArguments(a_args, a_vals);

		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
		callback.reset(new VmCallback(a_onResult));


		bool result = false;
		if (auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton()) {
			result = vm->DispatchStaticCall(a_scr, a_func, args, callback);
		} 

		return result;
	}
}