#ifndef _IMMERSIVE_H
#define _IMMERSIVE_H

#include "Platform/Define.h"

#include <string>
#include <map>

class Creature;
class Player;
class Quest;
class Unit;
class WorldObject;

struct FactionEntry;
struct GossipMenuItemData;
struct PlayerInfo;
struct PlayerLevelInfo;

enum Stats;

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

enum ImmersiveGossipOptions
{
    // UNIT_NPC_FLAG_TRAINER             (16) (bonus option for GOSSIP_OPTION_TRAINER)
    GOSSIP_OPTION_IMMERSIVE = 19
};

enum ImmpersiveGossipMenuOptions
{
    IMMERSIVE_GOSSIP_OPTION_HELP = 0,
    IMMERSIVE_GOSSIP_OPTION_INCREASE_STRENGTH,
    IMMERSIVE_GOSSIP_OPTION_INCREASE_AGILITY,
    IMMERSIVE_GOSSIP_OPTION_INCREASE_STAMINA,
    IMMERSIVE_GOSSIP_OPTION_INCREASE_INTELLECT,
    IMMERSIVE_GOSSIP_OPTION_INCREASE_SPIRIT,
    IMMERSIVE_GOSSIP_OPTION_RESET_STATS,
    IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_0 = 11,
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
    LANG_IMMERSIVE_MANUAL_ATTR_DISABLED     = 12100,
    LANG_IMMERSIVE_MANUAL_ATTR_LOST         = 12101,
    LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH     = 12102,
    LANG_IMMERSIVE_MANUAL_ATTR_AGILITY      = 12103,
    LANG_IMMERSIVE_MANUAL_ATTR_STAMINA      = 12104,
    LANG_IMMERSIVE_MANUAL_ATTR_INTELLECT    = 12105,
    LANG_IMMERSIVE_MANUAL_ATTR_SPIRIT       = 12106,
    LANG_IMMERSIVE_MANUAL_ATTR_AVAILABLE    = 12107,
    LANG_IMMERSIVE_MANUAL_ATTR_ASSIGNED     = 12108,
    LANG_IMMERSIVE_MANUAL_ATTR_MODIFIER     = 12109,
    LANG_IMMERSIVE_MANUAL_ATTR_SUGGESTED    = 12110,
    LANG_IMMERSIVE_MANUAL_ATTR_MOD_CHANGED  = 12111,
    LANG_IMMERSIVE_MANUAL_ATTR_MOD_DISABLED = 12112,
    LANG_IMMERSIVE_MANUAL_ATTR_MISSING      = 12113,
    LANG_IMMERSIVE_MANUAL_ATTR_MISSING_GOLD = 12114,
    LANG_IMMERSIVE_MANUAL_ATTR_GAINED       = 12115,
    LANG_IMMERSIVE_MANUAL_ATTR_RESET        = 12116,
    LANG_IMMERSIVE_EXP_GAINED               = 12117,
    LANG_IMMERSIVE_MANUAL_ATTR_POINTS_ADDED = 12118,
    LANG_IMMERSIVE_MONEY_GAINED             = 12119,
    LANG_IMMERSIVE_REPUTATION_GAINED        = 12120,
    LANG_IMMERSIVE_QUEST_COMPLETED          = 12121,
};

class ImmersiveAction
{
public:
    ImmersiveAction() {}

public:
    virtual bool Run(Player* player, Player* bot) = 0;
    virtual std::string GetActionMessage(Player* player) = 0;

protected:
    virtual bool CheckSharedPercentReqs(Player* player, Player* bot);
    virtual bool CheckSharedPercentReqsSingle(Player* player, Player* bot);
};

class Immersive
{
public:
    Immersive();

    void Init();
    void Update(uint32 elapsed);

    static std::string FormatString(const char* format, ...);

    // Fall damage
    float GetFallThreshold(const float default);
    float GetFallDamage(Player* player, float zdist, float defaultVal);

    // Respawn
    bool CanCreatureRespawn(Creature* creature) const;
    void DisableOfflineRespawn();

    // Player methods
    void OnPlayerResurrect(Player *player);
    void OnPlayerGiveXP(Player *player, uint32 xp, Unit* victim);
    void OnPlayerGiveLevel(Player* player);
    void OnPlayerModifyMoney(Player *player, int32 delta);
    void OnPlayerSetReputation(Player* player, FactionEntry const* factionEntry, int32& standing, bool incremental);
    void OnPlayerRewardQuest(Player* player, Quest const* quest);
    bool OnPlayerUseFishingNode(Player* player, bool success);

    // Manual stats
    bool OnPlayerPrepareUnitGossipMenu(uint32 optionId);
    void OnPlayerGossipSelect(Player* player, WorldObject* source, uint32 gossipOptionId, uint32 gossipListId, GossipMenuItemData* menuData);
    void OnPlayerGossipHello(Player* player, Creature* creature);

    PlayerInfo const* GetPlayerInfo(uint32 race, uint32 class_);
    void GetPlayerLevelInfo(Player* player, PlayerLevelInfo* info);
    int32 CalculateEffectiveChance(int32 difference, const Unit* attacker, const Unit* victim, ImmersiveEffectiveChance type);

private:
    uint32 CalculateEffectiveChanceDelta(const Unit* unit);
    uint32 GetModifierValue(uint32 owner);
    uint32 GetStatsValue(uint32 owner, Stats type);
    uint32 GetStatsValue(uint32 owner, const std::string& type);
    void SetStatsValue(uint32 owner, Stats type, uint32 value);
    void SetStatsValue(uint32 owner, const std::string& type, uint32 value);
    uint32 GetTotalStats(Player* player, uint8 level = 0);
    uint32 GetUsedStats(Player* player);
    uint32 GetStatCost(Player* player, uint8 level = 0, uint32 usedStats = 0);
    void IncreaseStat(Player* player, uint32 type);
    void ResetStats(Player* player);
    void ChangeModifier(Player* player, uint32 type);
    void CheckScaleChange(Player* player);

    void PrintHelp(Player *player, bool detailed = false, bool help = false);
    void PrintUsedStats(Player* player);
    void PrintSuggestedStats(Player* player);
    
    void SendSysMessage(Player *player, const std::string& message);
    void RunAction(Player* player, ImmersiveAction* action);

private:
    static std::map<Stats, std::string> statValues;
    std::map< uint32, std::map<std::string, uint32> > valueCache;
    uint32 updateDelay;
};

#define sImmersiveMgr MaNGOS::Singleton<Immersive>::Instance()

#endif