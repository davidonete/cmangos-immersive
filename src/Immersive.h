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

    float GetFallThreshold();
    PlayerInfo const* GetPlayerInfo(uint32 race, uint32 class_);
    void GetPlayerLevelInfo(Player *player, PlayerLevelInfo* info);
    void OnGossipSelect(Player *player, WorldObject* source, uint32 gossipListId, GossipMenuItemData *menuData);
    float GetFallDamage(Player* player, float zdist, float defaultVal);
    void OnDeath(Player *player);
    void OnGiveXP(Player *player, uint32 xp, Unit* victim);
    void OnGiveLevel(Player* player);
    void OnModifyMoney(Player *player, int32 delta);
    void OnReputationChange(Player* player, FactionEntry const* factionEntry, int32& standing, bool incremental);
    void OnRewardQuest(Player* player, Quest const* quest);
    bool OnFishing(Player* player, bool success);
    bool CanCreatureRespawn(Creature* creature) const;
    int32 CalculateEffectiveChance(int32 difference, const Unit* attacker, const Unit* victim, ImmersiveEffectiveChance type);
    uint32 GetModifierValue(uint32 owner);
    uint32 GetStatsValue(uint32 owner, Stats type);
    void SetStatsValue(uint32 owner, Stats type, uint32 value);
    uint32 GetTotalStats(Player *player, uint8 level = 0);
    void OnGossipHello(Player* player, Creature* creature);
    void CheckScaleChange(Player* player);
    static std::string FormatString(const char* format, ...);
    void Update(uint32 elapsed);
    void Init();

private:
    void PrintHelp(Player *player, bool detailed = false, bool help = false);
    void PrintUsedStats(Player* player);
    void PrintSuggestedStats(Player* player);
    void IncreaseStat(Player *player, uint32 type);
    void ChangeModifier(Player *player, uint32 type);
    void ResetStats(Player *player);
    void SendSysMessage(Player *player, const std::string& message);
    uint32 CalculateEffectiveChanceDelta(const Unit* unit);
    void DisableOfflineRespawn();

    uint32 GetUsedStats(Player *player);
    uint32 GetStatCost(Player *player, uint8 level = 0, uint32 usedStats = 0);
    void RunAction(Player* player, ImmersiveAction* action);
    uint32 GetValue(uint32 owner, const std::string& type);
    void SetValue(uint32 owner, const std::string& type, uint32 value);

private:
    static std::map<Stats, std::string> statValues;
    std::map< uint32, std::map<std::string, uint32> > valueCache;
    uint32 updateDelay;
};

#define sImmersive MaNGOS::Singleton<Immersive>::Instance()

#endif