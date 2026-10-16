#include "CoreManager.h"
#include <spdlog/spdlog.h>

namespace KingCore
{
    void CoreManager::GenerateFallbackProfile(RE::Actor* actor, BehaviorProfile& outProfile)
    {
        outProfile = BehaviorProfile{};
        outProfile.classID = ClassID::Warrior;
        outProfile.category = ClassCategory::Melee;
        outProfile.preferredArmor = ArmorPreference::Heavy;
        outProfile.preferredWeapon = WeaponPreference::OneHandedSword;

        if (!actor) return;

        auto* avOwner = actor->AsActorValueOwner();
        if (avOwner) {
            float marksman = avOwner->GetActorValue(RE::ActorValue::kArchery);
            float destruction = avOwner->GetActorValue(RE::ActorValue::kDestruction);
            float sneak = avOwner->GetActorValue(RE::ActorValue::kSneak);
            float oneHanded = avOwner->GetActorValue(RE::ActorValue::kOneHanded);
            float twoHanded = avOwner->GetActorValue(RE::ActorValue::kTwoHanded);

            if (marksman > oneHanded && marksman > twoHanded && marksman > destruction) {
                outProfile.classID = ClassID::Archer;
                outProfile.category = ClassCategory::Ranged;
                outProfile.preferredArmor = ArmorPreference::Light;
                outProfile.preferredWeapon = WeaponPreference::Bow;
                outProfile.ammoWeight = 90;
                outProfile.targetAmmoCount = 60;
                outProfile.staminaPotionWeight = 70;
            } else if (destruction > oneHanded && destruction > twoHanded && destruction > marksman) {
                outProfile.classID = ClassID::Mage;
                outProfile.category = ClassCategory::Magic;
                outProfile.preferredArmor = ArmorPreference::Cloth;
                outProfile.preferredWeapon = WeaponPreference::PureMagic;
                outProfile.magickaPotionWeight = 90;
                outProfile.targetMagickaPotions = 5;
                outProfile.soulGemWeight = 70;
                outProfile.spellTomeWeight = 80;
            } else if (sneak > 45.0f && sneak > oneHanded) {
                outProfile.classID = ClassID::Assassin;
                outProfile.category = ClassCategory::Melee;
                outProfile.preferredArmor = ArmorPreference::Light;
                outProfile.preferredWeapon = WeaponPreference::OneHandedDagger;
                outProfile.prefersStealth = true;
                outProfile.poisonWeight = 80;
            } else {
                outProfile.classID = ClassID::Warrior;
                outProfile.category = ClassCategory::Melee;
                outProfile.preferredArmor = (twoHanded > oneHanded) ? ArmorPreference::Heavy : ArmorPreference::Light;
                outProfile.preferredWeapon = (twoHanded > oneHanded) ? WeaponPreference::TwoHandedGreatsword : WeaponPreference::OneHandedSword;
                outProfile.healthPotionWeight = 80;
                outProfile.targetHealthPotions = 3;
            }
        }
    }
}
