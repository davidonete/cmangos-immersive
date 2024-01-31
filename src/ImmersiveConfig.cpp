#include "ImmersiveConfig.h"

#include "Log.h"
#include "Policies/Singleton.h"
#include "SystemConfig.h"

ImmersiveConfig::ImmersiveConfig()
: enabled(false)
, manualAttributes(false)
, manualAttributesPercent(0)
, manualAttributesIncrease(0)
, manualAttributesCostMult(0)
, manualAttributesMaxPoints(0)
, sharedXpPercent(0)
, sharedMoneyPercent(0)
, sharedRepPercent(0)
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
{

}

INSTANTIATE_SINGLETON_1(ImmersiveConfig);

bool ImmersiveConfig::Initialize()
{
    sLog.outString("Initializing Immersive by ike3");

    if (!config.SetSource(SYSCONFDIR"immersive.conf"))
    {
        sLog.outError("Failed to open configuration file immersive.conf");
        return false;
    }

    enabled = config.GetBoolDefault("Immersive.Enable", false);
    manualAttributes = config.GetBoolDefault("Immersive.ManualAttributes", false);
    manualAttributesPercent = config.GetIntDefault("Immersive.ManualAttributesPercent", 100);
    manualAttributesIncrease = config.GetIntDefault("Immersive.ManualAttributesIncrease", 5);
    manualAttributesCostMult = config.GetIntDefault("Immersive.ManualAttributesCostMult", 5);
    manualAttributesMaxPoints = config.GetIntDefault("Immersive.ManualAttributesMaxPoints", 0);
    sharedXpPercent = config.GetIntDefault("Immersive.SharedXpPercent", 0);
    sharedMoneyPercent = config.GetIntDefault("Immersive.SharedMoneyPercent", 0);
    sharedRepPercent = config.GetIntDefault("Immersive.SharedRepPercent", 0);
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

    sLog.outString("Immersive configuration loaded");
    return true;
}
