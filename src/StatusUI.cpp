#include "StatusUI.h"

#include "CoreManager.h"
#include <SKSE/SKSE.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace KingCore
{
    StatusUI* StatusUI::GetSingleton()
    {
        static StatusUI instance;
        return &instance;
    }

    bool StatusUI::Initialize()
    {
        if (!_inputRegistered) {
            if (auto* input = RE::BSInputDeviceManager::GetSingleton()) {
                input->AddEventSink(this);
                _inputRegistered = true;
                SKSE::log::info("King Core: Shift+K input listener registered.");
            } else {
                SKSE::log::warn("King Core: Input manager unavailable; Shift+K hotkey was not registered.");
            }
        }

        // kPostLoad can arrive before Skyrim creates its input manager. Retry the
        // input registration on later lifecycle messages even if the view exists.
        if (_view != 0 && _api) return true;

        _api = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI1>();
        if (!_api) {
            SKSE::log::info("King Core: PrismaUI not found; Shift+K status panel unavailable.");
            return false;
        }

        _view = _api->CreateView("KingCore/index.html", [](PrismaView view) {
            if (auto* tasks = SKSE::GetTaskInterface()) {
                tasks->AddTask([view]() {
                    auto* self = GetSingleton();
                    if (!self->_api || self->_view != view || !self->_api->IsValid(view)) return;
                    self->_viewReady = true;
                    self->_api->Hide(view);
                    self->SendState();
                    SKSE::log::info("King Core: PrismaUI status view is ready.");
                });
            }
        });
        if (_view == 0 || !_api->IsValid(_view)) {
            SKSE::log::error("King Core: Failed to create PrismaUI status view.");
            _api = nullptr;
            return false;
        }
        _api->RegisterJSListener(_view, "kcClose", OnClose);
        _api->Hide(_view);
        SKSE::log::info("King Core: PrismaUI status panel initialized. Shortcut: Shift+K.");
        return true;
    }

    RE::BSEventNotifyControl StatusUI::ProcessEvent(
        RE::InputEvent* const* event,
        RE::BSTEventSource<RE::InputEvent*>*)
    {
        bool openRequested = false;
        for (auto* current = event ? *event : nullptr; current; current = current->next) {
            if (current->GetDevice() != RE::INPUT_DEVICE::kKeyboard) continue;
            auto* button = current->AsButtonEvent();
            if (!button) continue;

            const auto key = button->GetIDCode();
            if (key == RE::BSKeyboardDevice::Keys::kLeftShift) {
                if (button->IsUp()) _leftShiftDown = false;
                else if (button->IsDown()) _leftShiftDown = true;
            } else if (key == RE::BSKeyboardDevice::Keys::kRightShift) {
                if (button->IsUp()) _rightShiftDown = false;
                else if (button->IsDown()) _rightShiftDown = true;
            } else if (key == RE::BSKeyboardDevice::Keys::kK && button->IsDown() &&
                       (_leftShiftDown || _rightShiftDown)) {
                openRequested = true;
            }
        }

        if (openRequested) {
            if (auto* tasks = SKSE::GetTaskInterface()) {
                tasks->AddTask([]() { GetSingleton()->Open(); });
                return RE::BSEventNotifyControl::kStop;
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    void StatusUI::Open()
    {
        if (!_api || !_viewReady || !_api->IsValid(_view)) return;
        if (!_api->IsHidden(_view)) {
            Close();
            return;
        }
        SendState();
        _api->Show(_view);
        _api->Focus(_view, true, false);
    }

    void StatusUI::Close()
    {
        if (!_api || !_view || !_api->IsValid(_view)) return;
        _api->Unfocus(_view);
        _api->Hide(_view);
    }

    void StatusUI::OnClose(const char*)
    {
        if (auto* tasks = SKSE::GetTaskInterface()) {
            tasks->AddTask([]() { GetSingleton()->Close(); });
        }
    }

    void StatusUI::SendState()
    {
        if (!_api || !_viewReady || !_api->IsValid(_view)) return;
        const auto status = CoreManager::GetSingleton()->GetIntegrationStatus();
        const auto script = fmt::format(
            "window.kcUpdateState && window.kcUpdateState({{\"classBound\":{},\"followerUnbound\":{},\"soulKeeper\":{},\"lifeAgain\":{}}});",
            status.classBound ? "true" : "false",
            status.followerUnbound ? "true" : "false",
            status.soulKeeper ? "true" : "false",
            status.lifeAgain ? "true" : "false");
        _api->Invoke(_view, script.c_str());
    }
}
