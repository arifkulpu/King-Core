#pragma once

#include <cstdint>
#include <string_view>

namespace RE
{
    class Actor;
}

namespace KingCore
{
    // API Versioning
    inline constexpr uint32_t API_VERSION_MAJOR = 1;
    inline constexpr uint32_t API_VERSION_MINOR = 0;
    inline constexpr uint32_t API_VERSION = (API_VERSION_MAJOR << 16) | API_VERSION_MINOR;

    // SKSE Messaging Protocol constants
    inline constexpr const char* PLUGIN_NAME = "KingCore";
    inline constexpr uint32_t MESSAGE_TYPE_REQUEST_API = 0x4B430001; // 'KC' 0001
    inline constexpr uint32_t MESSAGE_TYPE_PROVIDE_API = 0x4B430002; // 'KC' 0002
    inline constexpr uint32_t MESSAGE_TYPE_CLASS_EVENT = 0x4B430003; // 'KC' 0003

    // Treatment registration (e.g. Life Again): a mod announces items it uses as
    // injury/medical indicators (bandages). King Core relays the registration to
    // consumer mods (e.g. Follower Unbound), which must NOT sell, remove or
    // unequip these items — the treatment provider removes them itself upon healing.
    inline constexpr uint32_t MESSAGE_TYPE_REGISTER_TREATMENT = 0x4B430004; // 'KC' 0004
    inline constexpr uint32_t TREATMENT_MAX_ITEMS = 16;

    // One protected treatment item. formID is the provider's already-resolved
    // runtime FormID (valid for the current session's load order).
    struct TreatmentItemEntry
    {
        uint32_t formID{ 0 };
        uint32_t flags{ 0 }; // reserved, must be 0
    };

    // Fixed-size payload for MESSAGE_TYPE_REGISTER_TREATMENT.
    struct TreatmentRegistration
    {
        uint32_t apiVersion{ API_VERSION };
        uint32_t itemCount{ 0 }; // number of valid entries in items[]
        TreatmentItemEntry items[TREATMENT_MAX_ITEMS]{};
    };

    // Soul management registration (e.g. SoulKeeper): the provider announces that it
    // owns the follower soul-gem economy (purchasing, stock, recharge). While active,
    // consumers (e.g. Follower Unbound) must stand down their own soul-gem buying and
    // collecting and let the provider's systems run.
    inline constexpr uint32_t MESSAGE_TYPE_REGISTER_SOUL_MANAGER = 0x4B430005; // 'KC' 0005

    // Follower Unbound rest notification relayed to Life Again.
    inline constexpr uint32_t MESSAGE_TYPE_FOLLOWER_RESTED = 0x4B430006; // 'KC' 0006

    enum class RestEventType : uint32_t { Wait = 1, Sleep = 2 };

    struct FollowerRestEvent
    {
        uint32_t apiVersion{ API_VERSION };
        RestEventType type{ RestEventType::Wait };
        float gameDaysPassed{ 0.0f };
    };

    struct SoulManagerRegistration
    {
        uint32_t apiVersion{ API_VERSION };
        uint32_t active{ 1 }; // 1 = provider manages soul gems
    };

    // Stable Class Identifiers (Independent of localized names)
    enum class ClassID : uint32_t
    {
        Unknown = 0,

        // Melee
        Warrior = 1,
        Knight = 2,
        Paladin = 3,
        Thief = 4,
        Assassin = 5,

        // Ranged
        Archer = 10,
        Hunter = 11,
        Marksman = 12,
        Gunslinger = 13,
        Crossbowman = 14,

        // Magic
        Mage = 20,
        Necromancer = 21,
        Battlemage = 22,
        Druid = 23,
        Elementalist = 24,

        // Social / Economy / Lifestyle
        Merchant = 30,
        Bard = 31,
        Alchemist = 32,
        Blacksmith = 33,
        Explorer = 34,

        // Extensibility for custom / future classes
        Custom = 100
    };

    // Primary Archetype Category
    enum class ClassCategory : uint32_t
    {
        None = 0,
        Melee = 1,
        Ranged = 2,
        Magic = 3,
        Social = 4
    };

    // Preferred Armor Weight Class
    enum class ArmorPreference : uint8_t
    {
        None = 0,
        Cloth = 1,
        Light = 2,
        Heavy = 3,
        Any = 4
    };

    // Weapon Combat Style Preference
    enum class WeaponPreference : uint8_t
    {
        Any = 0,
        OneHandedSword = 1,
        OneHandedDagger = 2,
        OneHandedAxeMace = 3,
        TwoHandedGreatsword = 4,
        TwoHandedBattleaxeHammer = 5,
        Bow = 6,
        Crossbow = 7,
        DestructionStaff = 8,
        PureMagic = 9,
        ShieldBearer = 10
    };

    // Derived behavioral profile for AI consumers (e.g. Follower Unbound)
    // Lightweight, zero-allocation struct with 0-100 priority weights
    struct BehaviorProfile
    {
        ClassID classID{ ClassID::Unknown };
        ClassCategory category{ ClassCategory::None };
        ArmorPreference preferredArmor{ ArmorPreference::Any };
        WeaponPreference preferredWeapon{ WeaponPreference::Any };

        // Item / Loot & Inventory Evaluation Weights (0 - 100)
        uint8_t weaponWeight{ 50 };
        uint8_t armorWeight{ 50 };
        uint8_t ammoWeight{ 20 };
        uint8_t healthPotionWeight{ 50 };
        uint8_t magickaPotionWeight{ 20 };
        uint8_t staminaPotionWeight{ 40 };
        uint8_t poisonWeight{ 10 };
        uint8_t goldWeight{ 50 };
        uint8_t soulGemWeight{ 10 };
        uint8_t spellTomeWeight{ 10 };
        uint8_t craftingMaterialWeight{ 20 };
        uint8_t alchemyIngredientWeight{ 20 };

        // Target Stock / Quotas for autonomous restock & shopping
        uint16_t targetAmmoCount{ 0 };
        uint8_t targetHealthPotions{ 2 };
        uint8_t targetMagickaPotions{ 0 };
        uint8_t targetStaminaPotions{ 1 };

        // Economic & Survival traits
        bool isMerchantMinded{ false };
        bool prefersStealth{ false };
        bool canCraftAmmo{ false };
        bool canCraftPotions{ false };
    };

    // Basic Class Metadata structure for public consumption
    struct ClassInfo
    {
        ClassID id{ ClassID::Unknown };
        ClassCategory category{ ClassCategory::None };
        const char* identifierName{ "" };
        const char* displayName{ "" };
        uint32_t level{ 1 };
        uint32_t xp{ 0 };
        const char* activeSpecialization{ "" };
    };

    // Change Event Types dispatched across the King Core event bus
    enum class ClassEventType : uint32_t
    {
        ClassChanged = 1,
        LevelChanged = 2,
        SpecializationChanged = 3,
        FollowerRecruited = 4,
        FollowerDismissed = 5
    };

    struct ClassEvent
    {
        ClassEventType type;
        uint32_t actorFormID;
        RE::Actor* actor;
        ClassID oldClass;
        ClassID newClass;
        uint32_t newLevel;
    };

    // Event Listener interface that consumer mods can implement
    class IClassEventListener
    {
    public:
        virtual ~IClassEventListener() = default;
        virtual void OnClassEvent(const ClassEvent& ev) = 0;
    };

    // Public ClassBound Provider API Interface
    // Lifetime contract: a provider must keep this interface object and its DLL alive
    // for the rest of the game process after registration. SKSE plugins are not
    // unloadable during normal play; runtime hot-unload is not supported by this API.
    class IClassBoundAPI
    {
    public:
        virtual ~IClassBoundAPI() = default;

        // Versioning check
        [[nodiscard]] virtual uint32_t GetVersion() const noexcept = 0;

        // Follower eligibility (filters out non-humanoids, summons, creatures, horses, dogs)
        [[nodiscard]] virtual bool IsActorEligible(RE::Actor* actor) const noexcept = 0;

        // Follower Queries
        [[nodiscard]] virtual ClassID GetFollowerClass(RE::Actor* actor) const noexcept = 0;
        [[nodiscard]] virtual uint32_t GetFollowerClassLevel(RE::Actor* actor) const noexcept = 0;
        [[nodiscard]] virtual std::string_view GetFollowerSpecialization(RE::Actor* actor) const noexcept = 0;
        [[nodiscard]] virtual bool GetFollowerClassInfo(RE::Actor* actor, ClassInfo& outInfo) const noexcept = 0;

        // AI Behavioral Profile Query (generated/cached with zero polling overhead)
        [[nodiscard]] virtual bool GetBehaviorProfile(RE::Actor* actor, BehaviorProfile& outProfile) const noexcept = 0;

        // Event Subscription
        virtual void RegisterEventListener(IClassEventListener* listener) noexcept = 0;
        virtual void UnregisterEventListener(IClassEventListener* listener) noexcept = 0;

        // Direct Class Assignment (if permitted)
        virtual bool AssignFollowerClass(RE::Actor* actor, ClassID newClass) noexcept = 0;
    };

    // Message payload sent during SKSE Message handshake
    struct KingCoreInterfaceExchange
    {
        uint32_t apiVersion{ API_VERSION };
        IClassBoundAPI* classAPI{ nullptr };
    };
}
