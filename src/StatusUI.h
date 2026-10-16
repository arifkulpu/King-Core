#pragma once

#include <RE/Skyrim.h>
#include "PrismaUI_API.h"

namespace KingCore
{
    class StatusUI final : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        static StatusUI* GetSingleton();
        bool Initialize();
        void Open();
        void SendState();

        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* event,
            RE::BSTEventSource<RE::InputEvent*>* source) override;

    private:
        static void OnClose(const char*);
        void Close();

        PRISMA_UI_API::IVPrismaUI1* _api{ nullptr };
        PrismaView _view{ 0 };
        bool _viewReady{ false };
        bool _inputRegistered{ false };
        bool _leftShiftDown{ false };
        bool _rightShiftDown{ false };
    };
}
