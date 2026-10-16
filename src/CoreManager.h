#pragma once

#include <RE/Skyrim.h>
#include "KingCore/KingCoreAPI.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <chrono>

namespace KingCore
{
    class CoreManager
    {
    public:
        struct IntegrationStatus
        {
            bool classBound{ false };
            bool followerUnbound{ false };
            bool soulKeeper{ false };
            bool lifeAgain{ false };
        };

        static CoreManager* GetSingleton()
        {
            static CoreManager instance;
            return &instance;
        }

        // Provider Registration (ClassBound)
        // Returns true only when the provider pointer actually changed; ClassBound
        // announces repeatedly (kPostLoad + kDataLoaded, directed + broadcast).
        bool SetClassBoundAPI(IClassBoundAPI* api)
        {
            IClassBoundAPI* previous = nullptr;
            {
                std::unique_lock lock(_mutex);
                if (api == _classBoundAPI) {
                    return false;
                }
                previous = _classBoundAPI;
                _classBoundAPI = api;
                ++_providerGeneration;
                _cachedProfiles.clear();
            }

            // Provider callbacks may synchronously re-enter KingCore. Never invoke
            // them while holding _mutex.
            if (previous) previous->UnregisterEventListener(&_internalEventListener);
            if (api) api->RegisterEventListener(&_internalEventListener);
            return true;
        }

        [[nodiscard]] IClassBoundAPI* GetClassBoundAPI() const
        {
            std::shared_lock lock(_mutex);
            return _classBoundAPI;
        }

        [[nodiscard]] bool HasClassBound() const
        {
            std::shared_lock lock(_mutex);
            return _classBoundAPI != nullptr;
        }

        [[nodiscard]] IntegrationStatus GetIntegrationStatus() const
        {
            std::shared_lock lock(_mutex);
            return { _classBoundAPI != nullptr, _followerUnboundActive,
                     _soulManagerKnown && _soulManagerActive, _lifeAgainConnected };
        }

        void SetFollowerUnboundActive(bool active)
        {
            std::unique_lock lock(_mutex);
            _followerUnboundActive = active;
        }

        [[nodiscard]] bool HasFollowerUnbound() const
        {
            std::shared_lock lock(_mutex);
            return _followerUnboundActive || !_listeners.empty();
        }

        // Treatment Item Registry (e.g. Life Again bandages).
        // Returns true only when the registered set actually changed.
        bool SetTreatmentItems(const TreatmentRegistration* reg)
        {
            if (!reg) return false;

            std::unordered_set<uint32_t> incoming;
            incoming.reserve(reg->itemCount);
            for (uint32_t i = 0; i < reg->itemCount && i < TREATMENT_MAX_ITEMS; ++i) {
                if (reg->items[i].formID != 0) {
                    incoming.insert(reg->items[i].formID);
                }
            }

            std::unique_lock lock(_mutex);
            if (_treatmentProviderActive && incoming == _treatmentItems) {
                return false;
            }
            _treatmentItems = std::move(incoming);
            _treatmentProviderActive = true;
            _lifeAgainConnected = true;
            _treatmentRegistration = *reg;
            return true;
        }

        [[nodiscard]] bool GetTreatmentRegistration(TreatmentRegistration& out) const
        {
            std::shared_lock lock(_mutex);
            if (!_treatmentProviderActive) return false;
            out = _treatmentRegistration;
            return true;
        }

        [[nodiscard]] bool IsTreatmentItem(uint32_t formID) const
        {
            if (formID == 0) return false;
            std::shared_lock lock(_mutex);
            return _treatmentProviderActive && _treatmentItems.contains(formID);
        }

        [[nodiscard]] bool HasTreatmentProvider() const
        {
            std::shared_lock lock(_mutex);
            return _treatmentProviderActive;
        }

        // Soul manager registration (e.g. SoulKeeper). Returns true on change.
        bool SetSoulManagerActive(bool active)
        {
            std::unique_lock lock(_mutex);
            if (_soulManagerKnown && _soulManagerActive == active) {
                return false;
            }
            _soulManagerActive = active;
            _soulManagerKnown = true;
            _soulKeeperConnected = true;
            _soulManagerRegistration.active = active ? 1u : 0u;
            return true;
        }

        [[nodiscard]] bool GetSoulManagerRegistration(SoulManagerRegistration& out) const
        {
            std::shared_lock lock(_mutex);
            if (!_soulManagerKnown) return false;
            out = _soulManagerRegistration;
            return true;
        }

        [[nodiscard]] bool HasSoulManager() const
        {
            std::shared_lock lock(_mutex);
            return _soulManagerActive;
        }

        // Consumer Event Subscription
        void RegisterEventListener(IClassEventListener* listener)
        {
            std::unique_lock lock(_mutex);
            if (!listener) return;
            for (auto* l : _listeners) {
                if (l == listener) return;
            }
            _listeners.push_back(listener);
        }

        void UnregisterEventListener(IClassEventListener* listener)
        {
            std::unique_lock lock(_mutex);
            std::erase(_listeners, listener);
        }

        // Broadcast event to all registered consumer mods (Follower Unbound, etc.)
        void BroadcastEvent(const ClassEvent& ev)
        {
            // Take a snapshot of listeners under unique_lock (we also write to the map).
            // Releasing the lock before calling out prevents reentrancy deadlocks:
            // a consumer that calls QueryBehaviorProfile() inside OnClassEvent() would
            // otherwise try to re-acquire the same non-recursive shared_mutex → deadlock.
            std::vector<IClassEventListener*> listenersCopy;
            {
                std::unique_lock lock(_mutex);
                _cachedProfiles.erase(ev.actorFormID);
                listenersCopy = _listeners;
            }
            for (auto* listener : listenersCopy) {
                if (listener) {
                    listener->OnClassEvent(ev);
                }
            }
        }

        // High-Performance Query with Dirty Flag / Caching (Zero-Polling)
        bool QueryBehaviorProfile(RE::Actor* actor, BehaviorProfile& outProfile)
        {
            if (!actor) return false;

            uint32_t formID = actor->GetFormID();

            for (;;) {
                IClassBoundAPI* api = nullptr;
                uint64_t generation = 0;
                {
                    std::shared_lock lock(_mutex);
                    auto it = _cachedProfiles.find(formID);
                    if (it != _cachedProfiles.end()) {
                        outProfile = it->second;
                        return true;
                    }
                    api = _classBoundAPI;
                    generation = _providerGeneration;
                }

                // Call provider code outside _mutex: it may re-enter KingCore.
                bool fromProvider = false;
                if (api && api->IsActorEligible(actor)) {
                    fromProvider = api->GetBehaviorProfile(actor, outProfile);
                }

                if (!fromProvider) {
                    GenerateFallbackProfile(actor, outProfile);
                }

                {
                    std::unique_lock lock(_mutex);
                    // A provider swap/event may have invalidated this computation.
                    // Retry against the current provider instead of caching stale data.
                    if (generation != _providerGeneration) continue;
                    _cachedProfiles[formID] = outProfile;
                    return true;
                }
            }
        }

        void InvalidateActorCache(uint32_t formID)
        {
            std::unique_lock lock(_mutex);
            _cachedProfiles.erase(formID);
        }

        void ClearAllCache()
        {
            std::unique_lock lock(_mutex);
            _cachedProfiles.clear();
        }

    private:
        CoreManager() = default;
        ~CoreManager() = default;

        // Internal listener to bridge ClassBound notifications into King Core
        class InternalEventListener : public IClassEventListener
        {
        public:
            void OnClassEvent(const ClassEvent& ev) override
            {
                CoreManager::GetSingleton()->BroadcastEvent(ev);
            }
        };

        void GenerateFallbackProfile(RE::Actor* actor, BehaviorProfile& outProfile);

        mutable std::shared_mutex _mutex;
        IClassBoundAPI* _classBoundAPI{ nullptr };
        uint64_t _providerGeneration{ 0 };
        bool _followerUnboundActive{ false };
        std::vector<IClassEventListener*> _listeners;
        std::unordered_map<uint32_t, BehaviorProfile> _cachedProfiles;
        std::unordered_set<uint32_t> _treatmentItems;
        bool _treatmentProviderActive{ false };
        bool _soulManagerActive{ false };
        bool _soulManagerKnown{ false };
        bool _soulKeeperConnected{ false };
        bool _lifeAgainConnected{ false };
        TreatmentRegistration _treatmentRegistration{};
        SoulManagerRegistration _soulManagerRegistration{};
        InternalEventListener _internalEventListener;
    };
}
