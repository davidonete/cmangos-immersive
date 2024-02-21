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

class ImmersiveModule : public Module
{
public:
    ImmersiveModule();

    void Init();
    void Update(uint32 elapsed);

    static std::string FormatString(const char* format, ...);

    // Fall damage
    float GetFallThreshold(const float defaultVal);
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
    uint32 GetStatsValue(uint32 owner, uint8 type);
    uint32 GetStatsValue(uint32 owner, const std::string& type);
    void SetStatsValue(uint32 owner, uint8 type, uint32 value);
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
    static std::map<uint8, std::string> statValues;
    std::map< uint32, std::map<std::string, uint32> > valueCache;
    uint32 updateDelay;
};

#define sImmersiveMgr MaNGOS::Singleton<ImmersiveMgr>::Instance()

#endif