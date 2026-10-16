#include <spdlog/sinks/basic_file_sink.h>
#include <SKSE/SKSE.h>
#include "KingCore/KingCoreAPI.h"
#include "CoreManager.h"
#include "StatusUI.h"

#include <shlobj_core.h>
#include <filesystem>
#include <system_error>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cmath>

using namespace std::literals;

namespace
{
    void InitializeLog()
    {
        // NG 3.7.0's log_directory() reads the My-Games subfolder from an engine global
        // whose RELOCATION_ID no longer matches current AE runtimes (resolves to a wrong
        // string, e.g. "Skyrim.INI"). Resolve the standard path ourselves instead:
        // Documents\My Games\Skyrim Special Edition\SKSE
        std::filesystem::path logDir;
        PWSTR docPath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &docPath))) {
            logDir = std::filesystem::path(docPath) / "My Games" / "Skyrim Special Edition" / "SKSE";
            CoTaskMemFree(docPath);
        } else if (auto fallback = SKSE::log::log_directory()) {
            logDir = *fallback;
        } else {
            return;
        }

        std::error_code ec;
        std::filesystem::create_directories(logDir, ec);

        const std::string logPath = (logDir / "KingCore.log").string();
        try {
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath, true);
            auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

            log->set_level(spdlog::level::info);
            log->flush_on(spdlog::level::info);

            spdlog::set_default_logger(std::move(log));
            spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
        } catch (const std::exception& e) {
            printf("KingCore: failed to initialize log at %s (%s)\n", logPath.c_str(), e.what());
        }
    }

    void HandleSKSEMessage(SKSE::MessagingInterface::Message* a_msg)
    {
        if (!a_msg) return;

        switch (a_msg->type) {
        case SKSE::MessagingInterface::kPostLoad:
            {
                KingCore::StatusUI::GetSingleton()->Initialize();
                SKSE::log::info("King Core: Listening for provider registrations...");
            }
            break;

        case SKSE::MessagingInterface::kDataLoaded:
            {
                // The input manager may not exist yet during kPostLoad.
                KingCore::StatusUI::GetSingleton()->Initialize();
            }
            break;

        case KingCore::MESSAGE_TYPE_PROVIDE_API:
            {
                // SKSE delivers broadcast Dispatch back to the dispatching plugin as well.
                // The relay below is itself a broadcast, so without this guard it would
                // re-enter this handler and re-dispatch forever (stack overflow).
                if (a_msg->sender && _stricmp(a_msg->sender, KingCore::PLUGIN_NAME) == 0) {
                    break;
                }

                if (a_msg->data && a_msg->dataLen >= sizeof(KingCore::KingCoreInterfaceExchange)) {
                    auto* exchange = static_cast<KingCore::KingCoreInterfaceExchange*>(a_msg->data);
                    if (exchange && exchange->classAPI) {
                        uint32_t providerVer   = exchange->classAPI->GetVersion();
                        uint32_t providerMajor = (providerVer >> 16) & 0xFFFF;

                        SKSE::log::info("King Core: Class API provider registered! Version: 0x{:X}", providerVer);

                        // Reject ABI-incompatible providers: a major version mismatch means
                        // virtual table layouts may differ → accepting would cause crashes.
                        if (providerMajor != KingCore::API_VERSION_MAJOR) {
                            SKSE::log::error(
                                "King Core: Version mismatch! Provider major={}, expected={}. Rejecting API.",
                                providerMajor, KingCore::API_VERSION_MAJOR);
                            break;
                        }

                        // Relay only when the provider actually changed; ClassBound announces
                        // repeatedly (kPostLoad + kDataLoaded, directed + broadcast) and each
                        // relay reaches us too.
                        if (KingCore::CoreManager::GetSingleton()->SetClassBoundAPI(exchange->classAPI)) {
                            KingCore::StatusUI::GetSingleton()->SendState();
                            // Broadcast to any consumers (e.g. Follower Unbound)
                            auto* messaging = SKSE::GetMessagingInterface();
                            if (messaging) {
                                messaging->Dispatch(
                                    KingCore::MESSAGE_TYPE_PROVIDE_API,
                                    exchange,
                                    sizeof(*exchange),
                                    nullptr // Broadcast to all listeners
                                );
                                SKSE::log::info("King Core: Broadcasted API provider to all listeners.");
                            }
                        }
                    }
                }
            }
            break;

        case KingCore::MESSAGE_TYPE_REQUEST_API:
            {
                SKSE::log::info("King Core: Received API request from sender: {}", (a_msg->sender ? a_msg->sender : "unknown"));

                // Follower Unbound announces itself implicitly: it dispatches REQUEST_API
                // (broadcast) at kPostLoad/kDataLoaded, so receiving one from it proves the
                // consumer plugin is running. This drives the in-game status notification.
                if (a_msg->sender) {
                    std::string senderLower = a_msg->sender;
                    std::transform(senderLower.begin(), senderLower.end(), senderLower.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (senderLower.find("followerunbound") != std::string::npos) {
                        KingCore::CoreManager::GetSingleton()->SetFollowerUnboundActive(true);
                        KingCore::StatusUI::GetSingleton()->SendState();
                    }
                }

                auto* classAPI = KingCore::CoreManager::GetSingleton()->GetClassBoundAPI();
                if (classAPI) {
                    // Do NOT use 'static' here: the active provider may re-register
                    // a different interface during startup. Always send the current one.
                    KingCore::KingCoreInterfaceExchange exchange{
                        .apiVersion = KingCore::API_VERSION,
                        .classAPI   = classAPI
                    };
                    auto* messaging = SKSE::GetMessagingInterface();
                    if (messaging) {
                        messaging->Dispatch(
                            KingCore::MESSAGE_TYPE_PROVIDE_API,
                            &exchange,
                            sizeof(exchange),
                            a_msg->sender
                        );
                        SKSE::log::info("King Core: Sent API to {}", (a_msg->sender ? a_msg->sender : "requester"));
                    }
                } else {
                    SKSE::log::warn("King Core: API requested by '{}' but no provider registered yet.",
                        (a_msg->sender ? a_msg->sender : "unknown"));
                }

                // Registrations are state, not one-shot events. A consumer may load
                // after its provider, so replay the latest snapshots on every request.
                if (a_msg->sender) {
                    auto* messaging = SKSE::GetMessagingInterface();
                    if (messaging) {
                        KingCore::TreatmentRegistration treatment{};
                        if (KingCore::CoreManager::GetSingleton()->GetTreatmentRegistration(treatment)) {
                            messaging->Dispatch(KingCore::MESSAGE_TYPE_REGISTER_TREATMENT,
                                                &treatment, sizeof(treatment), a_msg->sender);
                        }

                        KingCore::SoulManagerRegistration soulManager{};
                        if (KingCore::CoreManager::GetSingleton()->GetSoulManagerRegistration(soulManager)) {
                            messaging->Dispatch(KingCore::MESSAGE_TYPE_REGISTER_SOUL_MANAGER,
                                                &soulManager, sizeof(soulManager), a_msg->sender);
                        }
                    }
                }
            }
            break;

        case KingCore::MESSAGE_TYPE_REGISTER_TREATMENT:
            {
                // Same self-guard as PROVIDE_API: our relay below is a broadcast and
                // SKSE delivers broadcasts back to the dispatching plugin as well.
                if (a_msg->sender && _stricmp(a_msg->sender, KingCore::PLUGIN_NAME) == 0) {
                    break;
                }

                if (a_msg->data && a_msg->dataLen >= sizeof(KingCore::TreatmentRegistration)) {
                    auto* reg = static_cast<const KingCore::TreatmentRegistration*>(a_msg->data);
                    uint32_t providerMajor = (reg->apiVersion >> 16) & 0xFFFF;
                    if (providerMajor != KingCore::API_VERSION_MAJOR || reg->itemCount > KingCore::TREATMENT_MAX_ITEMS) {
                        SKSE::log::warn(
                            "King Core: Treatment registration rejected (major={}, itemCount={}).",
                            providerMajor, reg->itemCount);
                        break;
                    }

                    if (KingCore::CoreManager::GetSingleton()->SetTreatmentItems(reg)) {
                        KingCore::StatusUI::GetSingleton()->SendState();
                        SKSE::log::info("King Core: Treatment provider registered {} item(s). Relaying to consumers.",
                                        reg->itemCount);

                        // Relay to all consumers (e.g. Follower Unbound)
                        auto* messaging = SKSE::GetMessagingInterface();
                        if (messaging) {
                            messaging->Dispatch(
                                KingCore::MESSAGE_TYPE_REGISTER_TREATMENT,
                                a_msg->data,
                                sizeof(*reg),
                                nullptr // Broadcast to all listeners
                            );
                        }
                    }
                }
            }
            break;

        case KingCore::MESSAGE_TYPE_REGISTER_SOUL_MANAGER:
            {
                // Same self-guard as the other relays: broadcast Dispatch reaches us too.
                if (a_msg->sender && _stricmp(a_msg->sender, KingCore::PLUGIN_NAME) == 0) {
                    break;
                }

                if (a_msg->data && a_msg->dataLen >= sizeof(KingCore::SoulManagerRegistration)) {
                    auto* reg = static_cast<const KingCore::SoulManagerRegistration*>(a_msg->data);
                    uint32_t providerMajor = (reg->apiVersion >> 16) & 0xFFFF;
                    if (providerMajor != KingCore::API_VERSION_MAJOR) {
                        SKSE::log::warn("King Core: Soul manager registration rejected (major={}).", providerMajor);
                        break;
                    }

                    if (KingCore::CoreManager::GetSingleton()->SetSoulManagerActive(reg->active != 0)) {
                        KingCore::StatusUI::GetSingleton()->SendState();
                        SKSE::log::info("King Core: Soul manager provider {}! Relaying to consumers.",
                                        reg->active ? "registered" : "unregistered");

                        // Relay to all consumers (e.g. Follower Unbound)
                        auto* messaging = SKSE::GetMessagingInterface();
                        if (messaging) {
                            messaging->Dispatch(
                                KingCore::MESSAGE_TYPE_REGISTER_SOUL_MANAGER,
                                a_msg->data,
                                sizeof(*reg),
                                nullptr // Broadcast to all listeners
                            );
                        }
                    }
                }
            }
            break;

        case KingCore::MESSAGE_TYPE_FOLLOWER_RESTED:
            {
                if (!a_msg->sender || _stricmp(a_msg->sender, "FollowerUnbound") != 0) break;
                if (!a_msg->data || a_msg->dataLen < sizeof(KingCore::FollowerRestEvent)) break;
                const auto* event = static_cast<const KingCore::FollowerRestEvent*>(a_msg->data);
                const auto providerMajor = (event->apiVersion >> 16) & 0xFFFF;
                if (providerMajor != KingCore::API_VERSION_MAJOR ||
                    (event->type != KingCore::RestEventType::Wait && event->type != KingCore::RestEventType::Sleep) ||
                    !std::isfinite(event->gameDaysPassed)) break;
                if (auto* messaging = SKSE::GetMessagingInterface()) {
                    messaging->Dispatch(KingCore::MESSAGE_TYPE_FOLLOWER_RESTED,
                        const_cast<KingCore::FollowerRestEvent*>(event), sizeof(*event), "LifeAgain");
                }
            }
            break;

        case SKSE::MessagingInterface::kPreLoadGame:
            {
                // Invalidate follower profile caches on save reload
                KingCore::CoreManager::GetSingleton()->ClearAllCache();
                SKSE::log::info("King Core: Session reset, cleared profile cache.");
            }
            break;

        case SKSE::MessagingInterface::kPostLoadGame:
        case SKSE::MessagingInterface::kNewGame:
            {
                // Retry once the game session is active; this is the reliable point
                // for registering the keyboard event sink on runtimes that initialize
                // BSInputDeviceManager after kPostLoad/kDataLoaded.
                KingCore::StatusUI::GetSingleton()->Initialize();
                KingCore::CoreManager::GetSingleton()->ClearAllCache();
                SKSE::log::info("King Core: Session loaded/started.");

                // Notify in-game on screen
                auto* coreMgr = KingCore::CoreManager::GetSingleton();
                bool hasClassBound = coreMgr->HasClassBound();
                bool hasFollowerUnbound = coreMgr->HasFollowerUnbound();

                std::string notifMsg = std::format(
                    "King Core | ClassBound: {} | Follower Unbound: {} | LifeAgain: {} | SoulKeeper: {}",
                    hasClassBound ? "Connected" : "Not Found",
                    hasFollowerUnbound ? "Connected" : "Not Found",
                    coreMgr->HasTreatmentProvider() ? "Connected" : "Not Found",
                    coreMgr->HasSoulManager() ? "Connected" : "Not Found"
                );

                RE::DebugNotification(notifMsg.c_str());
                SKSE::log::info("In-Game Notification: {}", notifMsg);
            }
            break;

        default:
            break;
        }
    }
}

SKSEPluginInfo(
    .Version = REL::Version{ KingCore::API_VERSION_MAJOR, KingCore::API_VERSION_MINOR, 0, 0 },
    .Name = "KingCore"sv,
    .Author = "King Modding Team"sv,
    .SupportEmail = ""sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
    .MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }
)

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    InitializeLog();

    auto* plugin = SKSE::PluginDeclaration::GetSingleton();
    SKSE::log::info("{} initializing...", plugin->GetName());

    SKSE::Init(skse);

    auto messaging = SKSE::GetMessagingInterface();
    if (messaging) {
        // SKSE 2.0+ mesaj sistemi gönderici bazlıdır: her gönderici için ayrı kayıt gerekir.
        // "SKSE"            → SKSE yaşam döngüsü mesajları (kPostLoad, kDataLoaded, ...)
        // "Classbound"      → ClassBound'un PROVIDE_API kayıt/yanıt mesajları
        // "LifeAgain"       → tedavi (bandaj) kayıt duyuruları
        // "Soulkeeper"      → soul gem yöneticisi kayıt duyuruları
        // "FollowerUnbound" → Follower Unbound'un REQUEST_API istekleri
        if (!messaging->RegisterListener(HandleSKSEMessage)) {
            SKSE::log::error("Failed to register SKSE messaging listener.");
        }
        if (!messaging->RegisterListener("Classbound", HandleSKSEMessage)) {
            SKSE::log::warn("Classbound listener registration failed (ClassBound not loaded?).");
        }
        if (!messaging->RegisterListener("LifeAgain", HandleSKSEMessage)) {
            SKSE::log::warn("LifeAgain listener registration failed (Life Again not loaded?).");
        }
        if (!messaging->RegisterListener("Soulkeeper", HandleSKSEMessage)) {
            SKSE::log::warn("Soulkeeper listener registration failed (SoulKeeper not loaded?).");
        }
        if (!messaging->RegisterListener("FollowerUnbound", HandleSKSEMessage)) {
            SKSE::log::warn("FollowerUnbound listener registration failed (Follower Unbound not loaded?).");
        }
        SKSE::log::info("Registered SKSE Messaging listeners (SKSE + Classbound + LifeAgain + Soulkeeper + FollowerUnbound).");
    }

    SKSE::log::info("{} initialized successfully.", plugin->GetName());
    return true;
}
