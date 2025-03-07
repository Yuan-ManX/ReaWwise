#define REAPERAPI_IMPLEMENT

#include "ExtensionWindow.h"
#include "ReaperContext.h"

#include "Core/WaapiClient.h"
#include "Theme/CustomLookAndFeel.h"

#include <JSONHelpers.h>
#include <juce_events/juce_events.h>
#include <memory>
#include <reaper_plugin_functions.h>
#include <tuple>
#include <variant>

#ifdef __APPLE__
#include "MacHelpers.h"
#endif

namespace AK::ReaWwise
{
	static bool juceInitialised = false;
	static std::unique_ptr<ExtensionWindow> mainWindow;
	static std::unique_ptr<ReaperContext> reaperContext;
	static std::string returnString;
	static std::string emptyReturnString;
	static double returnDouble;
	static int openReaperWwiseTransferCommandId = 0;

	static void onHookCustomMenu(const char* menuName, HMENU menuHandle, int flag)
	{
		if(flag == 0 && juce::String(menuName) == "Main extensions" && openReaperWwiseTransferCommandId)
		{
			MENUITEMINFO mi = {sizeof(MENUITEMINFO)};
			mi.fMask = MIIM_TYPE | MIIM_ID;
			mi.fType = MFT_STRING;
			mi.wID = openReaperWwiseTransferCommandId;
			mi.dwTypeData = (char*)"ReaWwise";
			InsertMenuItem(menuHandle, 0, true, &mi);
		}
	}

	static bool onHookCommand(int command, int flag)
	{
		if(command == openReaperWwiseTransferCommandId)
		{
			if(!juceInitialised)
			{
				juce::initialiseJuce_GUI();
				juceInitialised = true;
			}

			if(!mainWindow)
			{
				mainWindow = std::make_unique<ExtensionWindow>(*reaperContext);
#ifdef WIN32
				mainWindow->addToDesktop(mainWindow->getDesktopWindowStyleFlags(), reaperContext->getMainHwnd());
#else
				mainWindow->addToDesktop(mainWindow->getDesktopWindowStyleFlags(), 0);
				MacHelpers::makeWindowFloatingPanel(dynamic_cast<juce::Component*>(mainWindow.get()));
#endif
			}

			// TODO: There might be more stuff that needs to be done on visibility toggle, i.e. re-read app properties, keeping the extension alive
			mainWindow->setVisible(!mainWindow->isVisible());

			return true;
		}

		return false;
	}

	namespace Scripting
	{
		using namespace AK::WwiseAuthoringAPI;

		static std::unique_ptr<WwiseTransfer::WaapiClient> waapiClient;

		template <typename Type>
		static bool isObjectValid(Type* object, const std::set<Type>& objectSet)
		{
			return object != nullptr && objectSet.find(*object) != objectSet.end();
		}

		static bool isWaapiClientConnected()
		{
			return waapiClient != nullptr && waapiClient->isConnected();
		}

		template <class T>
		static T argCast(void* arg)
		{
			return (T)arg;
		}

		template <>
		double argCast<double>(void* arg)
		{
			return *(double*)arg;
		}

		template <>
		int argCast<int>(void* arg)
		{
			return (int)(intptr_t)arg;
		}

		template <class... Types>
		struct Arguments : public std::tuple<Types...>
		{
			Arguments(void** args)
			{
				populate(args);
			}

			template <size_t I>
			auto get()
			{
				return std::get<I>(*this);
			}

		private:
			template <size_t I = 0>
			constexpr void populate(void** args)
			{
				if constexpr(I == sizeof...(Types))
				{
					return;
				}
				else
				{
					using Type = typename std::tuple_element<I, std::tuple<Types...>>::type;

					std::get<I, Types...>(*this) = argCast<Type>(args[I]);
					populate<I + 1>(args);
				}
			}
		};

		struct AkJsonRef
		{
			typedef std::map<std::string, std::shared_ptr<AkJsonRef>> Map;
			typedef std::vector<std::shared_ptr<AkJsonRef>> Array;

			std::variant<AkVariant, Map, Array> data;

			AkJsonRef() = default;

			AkJsonRef(const std::variant<AkVariant, Map, Array>& data)
				: data(data)
			{
			}

			virtual ~AkJsonRef()
			{
			}

			bool isMap() const
			{
				return std::holds_alternative<Map>(data);
			}

			bool isArray() const
			{
				return std::holds_alternative<Array>(data);
			}

			bool isVariant() const
			{
				return std::holds_alternative<AkVariant>(data);
			}
		};

		static AkJsonRef AkJsonToAkJsonRef(const AkJson& akJson)
		{
			if(akJson.IsMap())
			{
				AkJsonRef::Map map{};

				for(auto& [key, value] : akJson.GetMap())
				{
					map[key] = std::make_shared<AkJsonRef>(AkJsonToAkJsonRef(value));
				}

				return AkJsonRef{map};
			}
			else if(akJson.IsArray())
			{
				AkJsonRef::Array array{};

				for(auto& value : akJson.GetArray())
				{
					array.emplace_back(std::make_shared<AkJsonRef>(AkJsonToAkJsonRef(value)));
				}

				return AkJsonRef{array};
			}
			else if(akJson.IsVariant())
			{
				return AkJsonRef{akJson.GetVariant()};
			}

			return AkJsonRef();
		}

		static AkJson AkJsonRefToAkJson(std::shared_ptr<AkJsonRef> akJsonRef)
		{
			if(akJsonRef->isMap())
			{
				AkJson::Map mapAsAkJson{};

				AkJsonRef::Map& map = std::get<AkJsonRef::Map>(akJsonRef->data);
				for(auto& [key, value] : map)
				{
					mapAsAkJson.emplace(key, AkJsonRefToAkJson(value));
				}

				return AkJson(mapAsAkJson);
			}
			else if(akJsonRef->isArray())
			{
				AkJson::Array arrayAsAkJson{};

				AkJsonRef::Array& arr = std::get<AkJsonRef::Array>(akJsonRef->data);
				for(auto& element : arr)
				{
					arrayAsAkJson.emplace_back(AkJsonRefToAkJson(element));
				}

				return AkJson(arrayAsAkJson);
			}
			else if(akJsonRef->isVariant())
			{
				return AkJson(std::get<AkVariant>(akJsonRef->data));
			}

			return AkJson();
		}

		struct AkJsonRefWithStatus : public AkJsonRef
		{
			bool status;
		};

		static std::set<std::shared_ptr<AkJsonRef>> objects;

		///////// Waapi /////////

		static bool Waapi_Connect(const char* ipAddress, intptr_t port)
		{
			if(waapiClient == nullptr)
				waapiClient.reset(new WwiseTransfer::WaapiClient());

			if(!waapiClient->isConnected())
				return waapiClient->connect(ipAddress, port);

			return true;
		}

		static void* Waapi_ConnectVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			return (void*)Waapi_Connect((const char*)argv[0], (intptr_t)argv[1]);
		}

		static void Waapi_Disconnect()
		{
			if(waapiClient != nullptr)
			{
				if(waapiClient->isConnected())
					waapiClient->disconnect();

				waapiClient.reset(nullptr);
			}
		}

		static void Waapi_DisconnectVarArg(void** argv, int argc)
		{
			Waapi_Disconnect();
		}

		static bool Waapi_IsConnected()
		{
			return waapiClient != nullptr && waapiClient->isConnected();
		}

		static void* Waapi_IsConnectedVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			return (void*)Waapi_IsConnected();
		}

		static void* Waapi_Call(const char* uri, std::shared_ptr<AkJsonRef>* args, std::shared_ptr<AkJsonRef>* options)
		{
			if(isWaapiClientConnected() && isObjectValid(args, objects) && isObjectValid(options, objects))
			{
				AkJson result;

				AkJsonRefWithStatus akJsonResult;
				akJsonResult.status = waapiClient->call(uri, AkJsonRefToAkJson(*args), AkJsonRefToAkJson(*options), result);

				akJsonResult.data = AkJsonToAkJsonRef(result).data;

				auto insert = objects.insert(std::make_shared<AkJsonRefWithStatus>(akJsonResult));

				if(insert.second)
					return (void*)&(*insert.first);
			}

			return nullptr;
		}

		static void* Waapi_CallVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<const char*, std::shared_ptr<AkJsonRef>*, std::shared_ptr<AkJsonRef>*> arguments(argv);

			return Waapi_Call(arguments.get<0>(), arguments.get<1>(), arguments.get<2>());
		}

		///////// AkJson_Any /////////

		template <typename T>
		static void* AkJson_Any()
		{
			auto result = objects.insert(std::make_shared<AkJsonRef>(T{}));

			if(result.second)
				return (void*)&(*result.first);

			return nullptr;
		}

		///////// AkJson_Map /////////
		static void* AkJson_Map()
		{
			return AkJson_Any<AkJsonRef::Map>();
		}

		static void* AkJson_MapVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			return AkJson_Map();
		}

		static bool AkJson_Map_Set(std::shared_ptr<AkJsonRef>* akJsonRef, const char* key, std::shared_ptr<AkJsonRef>* value)
		{
			if(isObjectValid(akJsonRef, objects) && isObjectValid(value, objects) && (**akJsonRef).isMap())
			{
				AkJsonRef::Map& map = std::get<AkJsonRef::Map>((**akJsonRef).data);
				map[key] = *value;

				return true;
			}

			return false;
		}

		static void* AkJson_Map_SetVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*, const char*, std::shared_ptr<AkJsonRef>*> arguments(argv);

			return (void*)AkJson_Map_Set(arguments.get<0>(), arguments.get<1>(), arguments.get<2>());
		}

		static void* AkJson_Map_Get(std::shared_ptr<AkJsonRef>* akJsonRef, const char* key)
		{
			if(isObjectValid(akJsonRef, objects) && (**akJsonRef).isMap())
			{
				AkJsonRef::Map& map = std::get<AkJsonRef::Map>((**akJsonRef).data);

				if(map.find(key) != map.end())
				{
					if(objects.find(map[key]) == objects.end())
						objects.insert(map[key]);

					return (void*)&map[key];
				}
			}

			return nullptr;
		}

		static void* AkJson_Map_GetVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*, const char*> arguments(argv);

			return AkJson_Map_Get(arguments.get<0>(), arguments.get<1>());
		}

		///////// AkJson_Array /////////
		static void* AkJson_Array()
		{
			return AkJson_Any<AkJsonRef::Array>();
		}

		static void* AkJson_ArrayVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			return AkJson_Array();
		}

		static bool AkJson_Array_Add(std::shared_ptr<AkJsonRef>* akJsonRef, std::shared_ptr<AkJsonRef>* value)
		{
			if(isObjectValid(akJsonRef, objects) && isObjectValid(value, objects) && (**akJsonRef).isArray())
			{
				AkJsonRef::Array& array = std::get<AkJsonRef::Array>((**akJsonRef).data);
				array.emplace_back(*value);

				return true;
			}

			return false;
		}

		static void* AkJson_Array_AddVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*, std::shared_ptr<AkJsonRef>*> arguments(argv);

			return (void*)AkJson_Array_Add(arguments.get<0>(), arguments.get<1>());
		}

		static void* AkJson_Array_Get(std::shared_ptr<AkJsonRef>* akJsonRef, int index)
		{
			if(isObjectValid(akJsonRef, objects) && (**akJsonRef).isArray())
			{
				AkJsonRef::Array& array = std::get<AkJsonRef::Array>((**akJsonRef).data);

				if(index < array.size())
				{
					if(objects.find(array[index]) == objects.end())
						objects.insert(array[index]);

					return (void*)&array[index];
				}
			}

			return nullptr;
		}

		static void* AkJson_Array_GetVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*, int> arguments(argv);

			return AkJson_Array_Get(arguments.get<0>(), arguments.get<1>());
		}

		static int AkJson_Array_Size(std::shared_ptr<AkJsonRef>* akJsonRef)
		{
			if(isObjectValid(akJsonRef, objects) && (**akJsonRef).isArray())
			{
				AkJsonRef::Array& array = std::get<AkJsonRef::Array>((**akJsonRef).data);
				return array.size();
			}

			return 0;
		}

		static void* AkJson_Array_SizeVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*> arguments(argv);

			return (void*)(intptr_t)AkJson_Array_Size(arguments.get<0>());
		}

		///////// AkVariant /////////

		template <typename T>
		static void* AkVariant_Any(T value)
		{
			auto result = objects.insert(std::make_shared<AkJsonRef>(AkVariant(value)));

			if(result.second)
				return (void*)&(*result.first);

			return nullptr;
		}

		static void* AkVariant_Bool(bool value)
		{
			return AkVariant_Any(value);
		}

		static void* AkVariant_BoolVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<bool> arguments(argv);

			return AkVariant_Bool(arguments.get<0>());
		}

		static void* AkVariant_Int(int value)
		{
			return AkVariant_Any(value);
		}

		static void* AkVariant_IntVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<int> arguments(argv);

			return AkVariant_Int(arguments.get<0>());
		}

		static void* AkVariant_Double(double value)
		{
			return AkVariant_Any(value);
		}

		static void* AkVariant_DoubleVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<double> arguments(argv);

			return AkVariant_Double(arguments.get<0>());
		}

		static void* AkVariant_String(const char* value)
		{
			return AkVariant_Any(value);
		}

		static void* AkVariant_StringVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<const char*> arguments(argv);

			return AkVariant_String(arguments.get<0>());
		}

		template <typename T>
		static T AkVariant_GetAny(std::shared_ptr<AkJsonRef>* akJsonRef)
		{
			if(isObjectValid(akJsonRef, objects) && (**akJsonRef).isVariant())
			{
				AkVariant& variant = std::get<AkVariant>((**akJsonRef).data);
				return (T)variant;
			}

			return 0;
		}

		template <>
		const char* AkVariant_GetAny<const char*>(std::shared_ptr<AkJsonRef>* akJsonRef)
		{
			if(isObjectValid(akJsonRef, objects) && (**akJsonRef).isVariant())
			{
				AkVariant& variant = std::get<AkVariant>((**akJsonRef).data);
				returnString = variant.GetString();

				return returnString.c_str();
			}

			return emptyReturnString.c_str();
		}

		static bool AkVariant_GetBool(std::shared_ptr<AkJsonRef>* akJsonRef)
		{
			return AkVariant_GetAny<bool>(akJsonRef);
		}

		static void* AkVariant_GetBoolVarArg(void** argv, int argc)
		{
			Arguments<std::shared_ptr<AkJsonRef>*> arguments(argv);

			return (void*)AkVariant_GetBool(arguments.get<0>());
		}

		static int AkVariant_GetInt(std::shared_ptr<AkJsonRef>* akJsonRef)
		{
			return AkVariant_GetAny<int>(akJsonRef);
		}

		static void* AkVariant_GetIntVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*> arguments(argv);

			return (void*)(intptr_t)AkVariant_GetInt(arguments.get<0>());
		}

		static double AkVariant_GetDouble(std::shared_ptr<AkJsonRef>* akJsonRef)
		{
			return AkVariant_GetAny<double>(akJsonRef);
		}

		static void* AkVariant_GetDoubleVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*> arguments(argv);

			returnDouble = AkVariant_GetDouble(arguments.get<0>());
			return reinterpret_cast<void*>(&returnDouble);
		}

		static const char* AkVariant_GetString(std::shared_ptr<AkJsonRef>* akJsonRef)
		{
			return AkVariant_GetAny<const char*>(akJsonRef);
		}

		static void* AkVariant_GetStringVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*> arguments(argv);

			return (void*)AkVariant_GetString(arguments.get<0>());
		}

		///////// AkJsonRefWithStatus /////////

		static const bool AkJson_GetStatus(std::shared_ptr<AkJsonRef>* akJsonRef)
		{
			if(isObjectValid(akJsonRef, objects))
			{
				if(auto akJsonResult = std::dynamic_pointer_cast<AkJsonRefWithStatus>(*akJsonRef))
				{
					return akJsonResult->status;
				};
			}

			return false;
		}

		static void* AkJson_GetStatusVarArg(void** argv, int argc)
		{
			juce::ignoreUnused(argc);

			Arguments<std::shared_ptr<AkJsonRef>*> arguments(argv);

			return (void*)AkJson_GetStatus(arguments.get<0>());
		}

		struct ApiFunctionDefinition
		{
			const char* Api;
			const char* ApiVarArg;
			const char* ApiDef;
			const char* FunctionSignature;
			void* FunctionPointer;
			void* FunctionPointerVarArg;
		};

#define AK_RWT_GENERATE_API_FUNC_DEF(functionName, output, inputs, inputNames, description) \
	ApiFunctionDefinition                                                                   \
	{                                                                                       \
		"API_AK_" #functionName,                                                            \
			"APIvararg_AK_" #functionName,                                                  \
			"APIdef_AK_" #functionName,                                                     \
			output "\0" inputs "\0" inputNames "\0" description,                            \
			(void*)&functionName,                                                           \
			(void*)&functionName##VarArg                                                    \
	}

		static auto apiFunctionDefinitions = {
			AK_RWT_GENERATE_API_FUNC_DEF(Waapi_Connect, "bool", "const char*,int", "ipAddress,port", "Ak: Connect to waapi (Returns connection status as bool)"),
			AK_RWT_GENERATE_API_FUNC_DEF(Waapi_Disconnect, "void", "", "", "Ak: Disconnect from waapi"),
			AK_RWT_GENERATE_API_FUNC_DEF(Waapi_Call, "*", "const char*,*,*", "uri,*,*", "Ak: Make a call to Waapi"),

			AK_RWT_GENERATE_API_FUNC_DEF(AkJson_Map, "*", "", "", "Ak: Create a map object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkJson_Map_Get, "*", "*,const char*", "*,key", "Ak: Get a map object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkJson_Map_Set, "bool", "*,const char*,*", "*,key,*", "Ak: Set a property on a map object"),

			AK_RWT_GENERATE_API_FUNC_DEF(AkJson_Array, "*", "", "", "Ak: Create an array object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkJson_Array_Add, "bool", "*,*", "*,*", "Ak: Add element to an array object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkJson_Array_Get, "*", "*,int", "*,index", "Ak: Get element at index of array object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkJson_Array_Size, "int", "*", "*", "Ak: Get count of elements in array object"),

			AK_RWT_GENERATE_API_FUNC_DEF(AkJson_GetStatus, "bool", "*", "*", "Ak: Get the status of a result from a call to waapi"),

			AK_RWT_GENERATE_API_FUNC_DEF(AkVariant_Bool, "*", "bool", "bool", "Ak: Create a bool object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkVariant_GetBool, "bool", "*", "*", "Ak: Extract raw boolean value from bool object"),

			AK_RWT_GENERATE_API_FUNC_DEF(AkVariant_String, "*", "const char*", "string", "Ak: Create a string object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkVariant_GetString, "const char*", "*", "*", "Ak: Extract raw string value from string object"),

			AK_RWT_GENERATE_API_FUNC_DEF(AkVariant_Int, "*", "int", "int", "Ak: Create an int object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkVariant_GetInt, "int", "*", "*", "Ak: Extract raw int value from int object"),

			AK_RWT_GENERATE_API_FUNC_DEF(AkVariant_Double, "*", "double", "double", "Ak: Create a double object"),
			AK_RWT_GENERATE_API_FUNC_DEF(AkVariant_GetDouble, "double", "*", "*", "Ak: Extract raw double value from double object"),
		};

#undef AK_RWT_GENERATE_API_FUNC_DEF
	} // namespace Scripting

	static int initialize(reaper_plugin_info_t* pluginInfo)
	{
		reaperContext = std::make_unique<ReaperContext>(pluginInfo);

		// Should actually report errors to the user somehow
		if(reaperContext->callerVersion() != REAPER_PLUGIN_VERSION)
		{
			return 0;
		}

		// Checks that all function pointers needed from reaper are valid
		if(!reaperContext->isValid())
		{
			return 0;
		}

		openReaperWwiseTransferCommandId = reaperContext->registerFunction("command_id", (void*)"openReaperWwiseTransferCommand");
		if(!openReaperWwiseTransferCommandId)
		{
			return 0;
		}

		if(!reaperContext->registerFunction("hookcommand", (void*)onHookCommand))
		{
			return 0;
		}

		if(!reaperContext->registerFunction("hookcustommenu", (void*)onHookCustomMenu))
		{
			return 0;
		}

		reaperContext->addExtensionsMainMenu();

		for(const auto& apiFunctionDefinition : Scripting::apiFunctionDefinitions)
		{
			reaperContext->registerFunction(apiFunctionDefinition.Api, (void*)apiFunctionDefinition.FunctionPointer);
			reaperContext->registerFunction(apiFunctionDefinition.ApiVarArg, (void*)apiFunctionDefinition.FunctionPointerVarArg);
			reaperContext->registerFunction(apiFunctionDefinition.ApiDef, (void*)apiFunctionDefinition.FunctionSignature);
		}

		return 1;
	}

	static int cleanup()
	{
		if(juceInitialised)
		{
			mainWindow.reset(nullptr);
			reaperContext.reset(nullptr);

			juce::shutdownJuce_GUI();
			juceInitialised = false;
		}

		return 0;
	}
} // namespace AK::ReaWwise

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
	REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t* pluginInfo)
{
	using namespace AK::ReaWwise;

	return pluginInfo ? initialize(pluginInfo) : cleanup();
}
