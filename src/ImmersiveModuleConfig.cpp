#include "ImmersiveModuleConfig.h"

namespace cmangos_module
{
    ImmersiveModuleConfig::ImmersiveModuleConfig()
    : ModuleConfig("immersive.conf")
    , enabled(false)
    , manualAttributes(false)
    , manualAttributesPercent(0)
    , manualAttributesIncrease(0)
    , manualAttributesCostMult(0)
    , manualAttributesMaxPoints(0)
    , manualAttributesBaseAttributes(0)
    , sharedXpPercent(0)
    , sharedMoneyPercent(0)
    , accountReputation(false)
    , sharedQuests(false)
    , fishingBaubles(false)
    , sharedPercentRaceRestiction(0)
    , sharedPercentClassRestiction(0)
    , sharedPercentGuildRestiction(false)
    , sharedPercentFactionRestiction(false)
    , sharedPercentMinLevel(0)
    , attributeLossPerDeath(0)
    , fallDamageMultiplier(0.0f)
    , sharedXpPercentLevelDiff(0)
    , scaleModifierWorkaround(false)
    , sharedRandomPercent(0)
    , disableOfflineRespawn(false)
    , disableInstanceRespawn(false)
    , infiniteLeveling(false)
    , infiniteLevelingMax(0)
    , infiniteLevelingRaidBossMult(0.0f)
    , infiniteLevelingRaidEliteMult(0.0f)
    , infiniteLevelingRaidNonEliteMult(0.0f)
    , infiniteLevelingDungeonBossMult(0.0f)
    , infiniteLevelingDungeonEliteMult(0.0f)
    , infiniteLevelingDungeonNonEliteMult(0.0f)
    , infiniteLevelingWorldEliteMult(0.0f)
    , infiniteLevelingWorldNonEliteMult(0.0f)
    , xpOnPvPKill(false)
    , xpOnPvPKillBgMult(0.0f)
    , xpOnPvPKillArenaMult(0.0f)
    , xpOnPvPKillWorldMult(0.0f)
    {

    }

    bool ImmersiveModuleConfig::OnLoad()
    {
        enabled = config.GetBoolDefault("Immersive.Enable", false);
        manualAttributes = config.GetBoolDefault("Immersive.ManualAttributes", false);
        manualAttributesPercent = config.GetIntDefault("Immersive.ManualAttributesPercent", 100);
        manualAttributesIncrease = config.GetIntDefault("Immersive.ManualAttributesIncrease", 5);
        manualAttributesCostMult = config.GetIntDefault("Immersive.ManualAttributesCostMult", 5);
        manualAttributesMaxPoints = config.GetIntDefault("Immersive.ManualAttributesMaxPoints", 0);
        manualAttributesBaseAttributes = config.GetIntDefault("Immersive.ManualAttributesBaseAttributes", 0);
        accountReputation = config.GetBoolDefault("Immersive.AccountReputation", false);
        sharedXpPercent = config.GetIntDefault("Immersive.SharedXpPercent", 0);
        sharedMoneyPercent = config.GetIntDefault("Immersive.SharedMoneyPercent", 0);
        sharedQuests = config.GetBoolDefault("Immersive.SharedQuests", false);
        fishingBaubles = config.GetBoolDefault("Immersive.FishingBaubles", false);
        sharedPercentRaceRestiction = config.GetIntDefault("Immersive.SharedPercentRaceRestriction", 0);
        sharedPercentClassRestiction = config.GetIntDefault("Immersive.SharedPercentClassRestriction", 0);
        sharedPercentGuildRestiction = config.GetBoolDefault("Immersive.SharedPercentGuildRestriction", false);
        sharedPercentFactionRestiction = config.GetBoolDefault("Immersive.SharedPercentFactionRestriction", false);
        sharedPercentMinLevel = config.GetIntDefault("Immersive.SharedPercentMinLevel", 1);
        attributeLossPerDeath = config.GetIntDefault("Immersive.AttributeLossPerDeath", 1);
        fallDamageMultiplier = config.GetFloatDefault("Immersive.FallDamageMultiplier", 1.0f);
        sharedXpPercentLevelDiff = config.GetIntDefault("Immersive.SharedXpPercentLevelDiff", 0);
        scaleModifierWorkaround = config.GetBoolDefault("Immersive.ScaleModifierWorkaround", false);
        sharedRandomPercent = config.GetIntDefault("Immersive.SharedRandomPercent", 0);
        disableOfflineRespawn = config.GetBoolDefault("Immersive.DisableOfflineRespawn", false);
        disableInstanceRespawn = config.GetBoolDefault("Immersive.DisableInstanceRespawn", false);
        infiniteLeveling = config.GetBoolDefault("Immersive.InfiniteLeveling", false);
        infiniteLevelingMax = config.GetIntDefault("Immersive.InfiniteLevelingMax", 100);
        infiniteLevelingRaidBossMult = config.GetFloatDefault("Immersive.InfiniteLevelingRaidBossMult", 0.0f);
        infiniteLevelingRaidEliteMult = config.GetFloatDefault("Immersive.InfiniteLevelingRaidEliteMult", 0.0f);
        infiniteLevelingRaidNonEliteMult = config.GetFloatDefault("Immersive.InfiniteLevelingRaidNonEliteMult", 0.0f);
        infiniteLevelingDungeonBossMult = config.GetFloatDefault("Immersive.InfiniteLevelingDungeonBossMult", 0.0f);
        infiniteLevelingDungeonEliteMult = config.GetFloatDefault("Immersive.InfiniteLevelingDungeonEliteMult", 0.0f);
        infiniteLevelingDungeonNonEliteMult = config.GetFloatDefault("Immersive.InfiniteLevelingDungeonNonEliteMult", 0.0f);
        infiniteLevelingWorldEliteMult = config.GetFloatDefault("Immersive.InfiniteLevelingWorldEliteMult", 0.0f);
        infiniteLevelingWorldNonEliteMult = config.GetFloatDefault("Immersive.InfiniteLevelingWorldNonEliteMult", 0.0f);
        xpOnPvPKill = config.GetBoolDefault("Immersive.GiveXPOnPvp", false);
        xpOnPvPKillBgMult = config.GetFloatDefault("Immersive.GiveXPOnPvpBgMult", 0.0f);
        xpOnPvPKillArenaMult = config.GetFloatDefault("Immersive.GiveXPOnPvpArenaMult", 0.0f);
        xpOnPvPKillWorldMult = config.GetFloatDefault("Immersive.GiveXPOnPvpWorldMult", 0.0f);
        return true;
    }
}