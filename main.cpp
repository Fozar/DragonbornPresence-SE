#include "skse64/PluginAPI.h"		// super
#include "skse64_common/skse_version.h"	// What version of SKSE is running?
#include <shlobj.h>				// CSIDL_MYCODUMENTS

#include "DragonbornPresence.h"


const char * log_path = R"(\My Games\Skyrim Special Edition\SKSE\DragonbornPresence.log)";

const char * plugin_name = "DragonbornPresence";
const UInt32 kVersionMajor = 1;
const UInt32 kVersionMinor = 1;
const UInt32 kVersionPatch = 0;

static PluginHandle g_pluginHandle = kPluginHandle_Invalid;


SKSEScaleformInterface * g_scaleform = nullptr;
SKSESerializationInterface * g_serialization = nullptr;
SKSEMessagingInterface * g_messageInterface = nullptr;
SKSEPapyrusInterface * g_papyrus = nullptr;


extern "C" {

bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info) {
  // Called by SKSE to learn about this plugin and check that it's safe to load it
  IDebugLog::OpenRelative(CSIDL_MYDOCUMENTS, log_path);
  IDebugLog::SetPrintLevel(IDebugLog::kLevel_Error);
#ifdef _DEBUG
  IDebugLog::SetLogLevel(IDebugLog::kLevel_DebugMessage);
#else
  IDebugLog::SetLogLevel(IDebugLog::kLevel_Message);
#endif

  _MESSAGE("%s %i.%i.%i", plugin_name, kVersionMajor, kVersionMinor,
           kVersionPatch);

  // populate info structure
  info->infoVersion = PluginInfo::kInfoVersion;
  info->name = plugin_name;
  info->version = kVersionMajor;

  // store plugin handle so we can identify ourselves later
  g_pluginHandle = skse->GetPluginHandle();

  if (skse->isEditor) {
    _MESSAGE("loaded in editor, marking as incompatible");

    return false;
  }
  if (skse->runtimeVersion != RUNTIME_VERSION_1_6_323) {
    _MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);

    return false;
  }


  // ### do not do anything else in this callback
  // ### only fill out PluginInfo and return true/false

  // supported runtime version
  return true;
}

bool SKSEPlugin_Load(const SKSEInterface * skse) {
  // Called by SKSE to load this plugin
  _MESSAGE("DragonbornPresence loaded");

  dragonborn_presence_namespace::SetLocale();

  g_papyrus = static_cast<SKSEPapyrusInterface *>(skse->QueryInterface(
    kInterface_Papyrus));

  //Check if the function registration was a success...
  const auto reg_check = g_papyrus->Register(
    dragonborn_presence_namespace::RegisterFuncs);

  if (reg_check) { _MESSAGE("Papyrus Register Succeeded"); }

  dragonborn_presence_namespace::RegisterGameEventHandlers();

  return true;
}

};
