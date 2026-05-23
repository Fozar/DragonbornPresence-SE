#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <Windows.h>
#include "DragonbornPresence.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {
    void InitializeLogging() {
        auto logPath = SKSE::log::log_directory().value_or(
            []() {
                wchar_t buf[MAX_PATH] = {};
                GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), buf, MAX_PATH);
                return std::filesystem::path(buf).parent_path();
            }()) / "DragonbornPresence.log";

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
        auto log  = std::make_shared<spdlog::logger>("global", std::move(sink));
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(log));
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    InitializeLogging();
    const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
    const auto& ver    = plugin->GetVersion();
    SKSE::log::info("DragonbornPresence {}.{}.{} — plugin loaded", ver.major(), ver.minor(), ver.patch());

    DragonbornPresence::SetLocale();
    DragonbornPresence::LoadConfig();

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* msg) {
        using MI = SKSE::MessagingInterface;
        switch (msg->type) {
        case MI::kDataLoaded:
            // Game data and singletons (UI, ScriptEventSourceHolder) are ready
            DragonbornPresence::RegisterGameEventHandlers();
            break;
        case MI::kNewGame:
        case MI::kPostLoadGame:
            DragonbornPresence::OnGameLoaded();
            break;
        }
    });

    return true;
}
