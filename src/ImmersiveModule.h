#ifndef MANGOS_IMMERSIVE_MODULE_H
#define MANGOS_IMMERSIVE_MODULE_H

#include "Module.h"
#include "ImmersiveModuleConfig.h"

struct PlayerInfo;

class ImmersiveModule;

class ImmersiveAction
{
public:
    ImmersiveAction(ImmersiveModuleConfig* config) : config(config) {}

public:
    virtual bool Run(Player* player, Player* bot) = 0;
    virtual std::string GetActionMessage(Player* player) = 0;

protected:
    virtual bool CheckSharedPercentReqs(Player* player, Player* bot);
    virtual bool CheckSharedPercentReqsSingle(Player* player, Player* bot);

protected:
    ImmersiveModuleConfig* config;
};

class ImmersiveModule : public Module
{
public:
    ImmersiveModule();
    ImmersiveModuleConfig* CreateConfig() override { return new ImmersiveModuleConfig(); }
    const ImmersiveModuleConfig* GetConfig() const override { return (ImmersiveModuleConfig*)GetConfigInternal(); }

    // Module Hooks
    void OnInitialize() override;
    void OnUpdate(uint32 elapsed) override;

    // Player Hooks
    bool OnHandleFall(Player* player, const MovementInfo& movementInfo, float lastFallZ) override;
    void OnResurrect(Player* player) override;
    void OnGiveXP(Player* player, uint32 xp, Creature* victim) override;
    void OnGiveLevel(Player* player, uint32 level) override;
    void OnModifyMoney(Player* player, int32 diff) override;
    void OnSetReputation(Player* player, const FactionEntry* factionEntry, int32 standing, bool incremental) override;
    void OnRewardQuest(Player* player, const Quest* quest) override;
    void OnGossipHello(Player* player, Creature* creature) override;
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId) override;
    void OnGetPlayerLevelInfo(Player* player, PlayerLevelInfo& info) override;

    // Creature Hooks
    bool OnRespawn(Creature* creature, time_t& respawnTime) override;
    void OnRespawnRequest(Creature* creature) override;

    // Game Object Hooks
    bool OnUseFishingNode(GameObject* gameObject, Player* player) override;

    // Unit Hooks
    bool OnCalculateEffectiveDodgeChance(const Unit* unit, const Unit* attacker, uint8 attType, const SpellEntry* ability, float& outChance) override;
    bool OnCalculateEffectiveBlockChance(const Unit* unit, const Unit* attacker, uint8 attType, const SpellEntry* ability, float& outChance) override;
    bool OnCalculateEffectiveParryChance(const Unit* unit, const Unit* attacker, uint8 attType, const SpellEntry* ability, float& outChance) override;
    bool OnCalculateEffectiveCritChance(const Unit* unit, const Unit* victim, uint8 attType, const SpellEntry* ability, float& outChance) override;
    bool OnCalculateEffectiveMissChance(const Unit* unit, const Unit* victim, uint8 attType, const SpellEntry* ability, const Spell* const* currentSpells, const SpellPartialResistDistribution& spellPartialResistDistribution, float& outChance) override;
    bool OnCalculateSpellMissChance(const Unit* unit, const Unit* victim, uint32 schoolMask, const SpellEntry* spell, float& outChance) override;
    bool OnGetAttackDistance(const Unit* unit, const Unit* target, float& outDistance) override;

private:
    float GetFallDamage(Player* player, float zdist, float defaultVal);
    bool HasFishingBauble(Player* player);
    void DisableOfflineRespawn();
    const PlayerInfo* GetPlayerInfo(uint32 race, uint32 class_);
    int32 CalculateEffectiveChance(int32 difference, const Unit* attacker, const Unit* victim, ImmersiveEffectiveChance type);

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
    std::map<uint8, std::string> statValues;
    std::map< uint32, std::map<std::string, uint32> > valueCache;
    std::unordered_set<uint32> creatureRespawnScheduled;
    uint32 updateDelay;
};

static ImmersiveModule immersiveModule;
#endif