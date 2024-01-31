#pragma once

#include "Config/Config.h"

class ImmersiveConfig
{
public:
    ImmersiveConfig();

    static ImmersiveConfig& instance()
    {
        static ImmersiveConfig instance;
        return instance;
    }

    bool Initialize();

public:
    bool enabled;
    bool manualAttributes;
    float manualAttributesPercent;
    uint32 manualAttributesIncrease;
    uint32 manualAttributesCostMult;
    uint32 manualAttributesMaxPoints;
    uint32 sharedXpPercent;
    uint32 sharedMoneyPercent;
    uint32 sharedRepPercent;
    bool sharedQuests;
    bool fishingBaubles;
    uint32 sharedPercentRaceRestiction;
    uint32 sharedPercentClassRestiction;
    bool sharedPercentGuildRestiction;
    bool sharedPercentFactionRestiction;
    uint32 sharedPercentMinLevel;
    uint32 attributeLossPerDeath;
    float fallDamageMultiplier;
    uint32 sharedXpPercentLevelDiff;
    bool scaleModifierWorkaround;
    uint32 sharedRandomPercent;
    bool disableOfflineRespawn;
    bool disableInstanceRespawn;

private:
    Config config;
};

#define sImmersiveConfig MaNGOS::Singleton<ImmersiveConfig>::Instance()

