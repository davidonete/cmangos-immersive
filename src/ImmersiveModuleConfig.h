#pragma once
#include "ModuleConfig.h"

namespace cmangos_module
{
    enum ImmersiveEffectiveChance
    {
        IMMERSIVE_EFFECTIVE_CHANCE_MISS,
        IMMERSIVE_EFFECTIVE_CHANCE_DODGE,
        IMMERSIVE_EFFECTIVE_CHANCE_PARRY,
        IMMERSIVE_EFFECTIVE_CHANCE_BLOCK,
        IMMERSIVE_EFFECTIVE_CHANCE_CRIT,
        IMMERSIVE_EFFECTIVE_CHANCE_SPELL_MISS,
        IMMERSIVE_EFFECTIVE_ATTACK_DISTANCE
    };

    #define IMMERSIVE_NPC_TEXT 50800
    #define IMMERSIVE_RESET_ATTR_TEXT 50801
    #define IMMERSIVE_REDUCE_ATTR_TEXT 50802

    enum ImmpersiveGossipMenuOptions
    {
        IMMERSIVE_GOSSIP_OPTION_MENU = 1500,
        IMMERSIVE_GOSSIP_OPTION_CHECK_CURRENT,
        IMMERSIVE_GOSSIP_OPTION_IMPROVE_STRENGTH,
        IMMERSIVE_GOSSIP_OPTION_IMPROVE_AGILITY,
        IMMERSIVE_GOSSIP_OPTION_IMPROVE_STAMINA,
        IMMERSIVE_GOSSIP_OPTION_IMPROVE_INTELLECT,
        IMMERSIVE_GOSSIP_OPTION_IMPROVE_SPIRIT,
        IMMERSIVE_GOSSIP_OPTION_RESET_STATS_MENU,
        IMMERSIVE_GOSSIP_OPTION_RESET_STATS_CONFIRM,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_MENU,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_0,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_10,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_20,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_30,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_40,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_50,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_60,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_70,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_80,
        IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_90
    };

    enum ImmersiveMangosStrings
    {
        LANG_IMMERSIVE_MANUAL_ATTR_DISABLED = 12100,
        LANG_IMMERSIVE_MANUAL_ATTR_LOST,
        LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH,
        LANG_IMMERSIVE_MANUAL_ATTR_AGILITY,
        LANG_IMMERSIVE_MANUAL_ATTR_STAMINA,
        LANG_IMMERSIVE_MANUAL_ATTR_INTELLECT,
        LANG_IMMERSIVE_MANUAL_ATTR_SPIRIT,
        LANG_IMMERSIVE_MANUAL_ATTR_AVAILABLE,
        LANG_IMMERSIVE_MANUAL_ATTR_ASSIGNED,
        LANG_IMMERSIVE_MANUAL_ATTR_MODIFIER,
        LANG_IMMERSIVE_MANUAL_ATTR_SUGGESTED,
        LANG_IMMERSIVE_MANUAL_ATTR_MOD_CHANGED,
        LANG_IMMERSIVE_MANUAL_ATTR_MOD_DISABLED,
        LANG_IMMERSIVE_MANUAL_ATTR_MISSING,
        LANG_IMMERSIVE_MANUAL_ATTR_MISSING_GOLD,
        LANG_IMMERSIVE_MANUAL_ATTR_GAINED,
        LANG_IMMERSIVE_MANUAL_ATTR_RESET,
        LANG_IMMERSIVE_EXP_GAINED,
        LANG_IMMERSIVE_MANUAL_ATTR_POINTS_ADDED,
        LANG_IMMERSIVE_MONEY_GAINED,
        LANG_IMMERSIVE_REPUTATION_GAINED,
        LANG_IMMERSIVE_QUEST_COMPLETED,
        LANG_IMMERSIVE_MANUAL_ATTR_MANAGE,
        LANG_IMMERSIVE_MANUAL_ATTR_CHECK_CURRENT,
        LANG_IMMERSIVE_MANUAL_ATTR_IMPROVE,
        LANG_IMMERSIVE_MANUAL_ATTR_UNLEARN,
        LANG_IMMERSIVE_MANUAL_ATTR_UNLEARN_SURE,
        LANG_IMMERSIVE_MANUAL_ATTR_REDUCE,
        LANG_IMMERSIVE_MANUAL_ATTR_REDUCE_REMOVE,
        LANG_IMMERSIVE_MANUAL_ATTR_REDUCE_PCT
    };

    class ImmersiveModuleConfig : public ModuleConfig
    {
    public:
        ImmersiveModuleConfig();
        bool OnLoad() override;

    public:
        bool enabled;
        bool manualAttributes;
        float manualAttributesPercent;
        uint32 manualAttributesIncrease;
        uint32 manualAttributesCostMult;
        uint32 manualAttributesMaxPoints;
        uint32 manualAttributesBaseAttributes;
        bool accountReputation;
        uint32 sharedXpPercent;
        uint32 sharedMoneyPercent;
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
        bool infiniteLeveling;
        uint32 infiniteLevelingMax;
        float infiniteLevelingRaidBossMult;
        float infiniteLevelingRaidEliteMult;
        float infiniteLevelingRaidNonEliteMult;
        float infiniteLevelingDungeonBossMult;
        float infiniteLevelingDungeonEliteMult;
        float infiniteLevelingDungeonNonEliteMult;
        float infiniteLevelingWorldEliteMult;
        float infiniteLevelingWorldNonEliteMult;
        bool xpOnPvPKill;
        float xpOnPvPKillBgMult;
        float xpOnPvPKillArenaMult;
        float xpOnPvPKillWorldMult;
        bool xpOnGathering;
        float xpOnGatheringOrangePct;
        float xpOnGatheringYellowPct;
        float xpOnGatheringGreenPct;
    };
}