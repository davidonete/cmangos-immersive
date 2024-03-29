#include "ImmersiveModule.h"

#include "AI/ScriptDevAI/include/sc_gossip.h"
#include "Entities/Creature.h"
#include "Entities/GossipDef.h"
#include "Entities/Player.h"
#include "Globals/SharedDefines.h"
#include "Globals/ObjectMgr.h"
#include "Log/Log.h"
#include "Spells/SpellMgr.h"
#include "Tools/Formulas.h"
#include "Tools/Language.h"
#include "World/World.h"

#ifdef ENABLE_PLAYERBOTS
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/PlayerbotAI.h"
#include "playerbot/ChatHelper.h"
#endif

std::string formatMoney(uint32 copper)
{
    std::ostringstream out;
    if (!copper)
    {
        out << "0";
        return out.str();
    }

    uint32 gold = uint32(copper / 10000);
    copper -= (gold * 10000);
    uint32 silver = uint32(copper / 100);
    copper -= (silver * 100);

    bool space = false;
    if (gold > 0)
    {
        out << gold << "g";
        space = true;
    }

    if (silver > 0 && gold < 50)
    {
        if (space) out << " ";
        out << silver << "s";
        space = true;
    }

    if (copper > 0 && gold < 10)
    {
        if (space) out << " ";
        out << copper << "c";
    }

    return out.str();
}

std::string percent(Player* player)
{
#ifdef ENABLE_PLAYERBOTS
    return player->GetPlayerbotAI() ? "%" : "%%";
#else
    return "%%";
#endif
}

#ifdef ENABLE_PLAYERBOTS
bool IsRandomBot(const Unit* unit)
{
    if (unit && unit->IsPlayer())
    {
        Player* player = (Player*)unit;
        return sRandomPlayerbotMgr.IsFreeBot(player);
    }

    return false;
}
#endif

namespace cmangos_module
{
    ImmersiveModule::ImmersiveModule()
    : Module("Immersive", new ImmersiveModuleConfig())
    , updateDelay(0U)
    {
        statValues[STAT_STRENGTH] = "Strength";
        statValues[STAT_AGILITY] = "Agility";
        statValues[STAT_STAMINA] = "Stamina";
        statValues[STAT_INTELLECT] = "Intellect";
        statValues[STAT_SPIRIT] = "Spirit";
    }

    const ImmersiveModuleConfig* ImmersiveModule::GetConfig() const
    {
        return (ImmersiveModuleConfig*)Module::GetConfig();
    }

    void ImmersiveModule::OnInitialize()
    {
        if (GetConfig()->enabled)
        {
            if (GetConfig()->disableOfflineRespawn || GetConfig()->disableInstanceRespawn)
            {
                updateDelay = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);
                DisableOfflineRespawn();
            }
        }
    }

    void ImmersiveModule::OnUpdate(uint32 elapsed)
    {
        if (GetConfig()->enabled)
        {
            if (GetConfig()->disableOfflineRespawn || GetConfig()->disableInstanceRespawn)
            {
                if (updateDelay > elapsed)
                {
                    updateDelay -= elapsed;
                }
                else
                {
                    updateDelay = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);
                    SetStatsValue(0, "last_ping", sWorld.GetGameTime());
                }
            }
        }
    }

    void ImmersiveModule::OnWorldPreInitialized()
    {
        if (GetConfig()->enabled)
        {
            if (GetConfig()->infiniteLeveling)
            {
                uint32 newMaxLevel = GetConfig()->infiniteLevelingMax;
                if (newMaxLevel < DEFAULT_MAX_LEVEL || newMaxLevel > MAX_LEVEL)
                {
                    newMaxLevel = newMaxLevel > MAX_LEVEL ? MAX_LEVEL : newMaxLevel;
                    newMaxLevel = newMaxLevel < DEFAULT_MAX_LEVEL ? DEFAULT_MAX_LEVEL : newMaxLevel;
                    sLog.outError("Immersive.InfiniteLevelingMax must be between %u and %u. Using %u instead.", DEFAULT_MAX_LEVEL, MAX_LEVEL, newMaxLevel);
                }

                sWorld.setConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL, newMaxLevel);

                WorldDatabase.PExecute("UPDATE `battleground_template` SET `MaxLvl`='%u';", newMaxLevel);
            }
        }
    }

    bool ImmersiveModule::OnPreHandleFall(Player* player, const MovementInfo& movementInfo, float lastFallZ, uint32& outDamage)
    {
        if (GetConfig()->enabled)
        {
            if (player)
            {
                // Don't get extra fall damage on battlegrounds or arenas
                if (helper::InPvpMap(player))
                    return false;

                // Don't get extra fall damage on dungeons or raids
                if (helper::InDungeon(player) || helper::InRaid(player))
                    return false;

    #ifdef ENABLE_PLAYERBOTS
                // Don't apply extra fall damage on bots
                if (!player->isRealPlayer())
                    return false;
    #endif

                // calculate total z distance of the fall
                const Position & position = movementInfo.GetPos();
                float z_diff = lastFallZ - position.z;
                DEBUG_LOG("zDiff = %f", z_diff);

                // Players with low fall distance, Feather Fall or physical immunity (charges used) are ignored
                // 14.57 can be calculated by resolving damageperc formula below to 0
                if (z_diff >= 4.57f && !player->IsDead() && !player->IsGameMaster() && !player->HasMovementFlag(MOVEFLAG_ONTRANSPORT) &&
                    !player->HasAuraType(SPELL_AURA_HOVER) && !player->HasAuraType(SPELL_AURA_FEATHER_FALL) &&
                    !player->IsImmuneToDamage(SPELL_SCHOOL_MASK_NORMAL))
                {
                    // Safe fall, fall height reduction
                    float safe_fall = player->GetTotalAuraModifier(SPELL_AURA_SAFE_FALL) * 0.5f;
                    z_diff = z_diff > safe_fall ? z_diff - safe_fall : 0;

                    float damageperc = 0.018f * z_diff - 0.2426f;
                    damageperc = GetFallDamage(player, z_diff, damageperc);
                    if (damageperc > 0)
                    {
                        uint32 damage = (uint32)(damageperc * player->GetMaxHealth() * sWorld.getConfig(CONFIG_FLOAT_RATE_DAMAGE_FALL));

                        float height = position.z;
                        player->UpdateAllowedPositionZ(position.x, position.y, height);

                        if (damage > 0)
                        {
                            // Prevent fall damage from being more than the player maximum health
                            if (damage > player->GetMaxHealth())
                            {
                                damage = player->GetMaxHealth();
                            }

                            // Gust of Wind
                            if (player->GetDummyAura(43621))
                            {
                                damage = player->GetMaxHealth() / 2;
                            }

                            outDamage = player->EnvironmentalDamage(DAMAGE_FALL, damage);
                        }

                        // Z given by moveinfo, LastZ, FallTime, WaterZ, MapZ, Damage, Safefall reduction
                        DEBUG_LOG("FALLDAMAGE z=%f sz=%f pZ=%f FallTime=%d mZ=%f damage=%d SF=%d", position.z, height, player->GetPositionZ(), movementInfo.GetFallTime(), height, damage, safe_fall);

                        return true;
                    }
                }
            }
        }

        return false;
    }

    void ImmersiveModule::OnResurrect(Player* player)
    {
        if (!GetConfig()->enabled || !player)
            return;

    #ifdef ENABLE_PLAYERBOTS
        // Don't lose stats on bots
        if (!player->isRealPlayer())
            return;
    #endif

        // Don't lose stats on max level
        if (helper::IsMaxLevel(player))
            return;

        // Don't get lose stats on battlegrounds
        if (player->InBattleGround())
            return;

    #if EXPANSION > 0
        // Don't lose stats on arenas
        if (player->InArena())
            return;
    #endif

        const uint8 lossPerDeath = GetConfig()->attributeLossPerDeath;
        const uint32 usedStats = GetUsedStats(player);
        if (lossPerDeath > 0 && usedStats > 0)
        {
            std::map<Stats, int> loss;
            for (uint8 j = STAT_STRENGTH; j < MAX_STATS; ++j)
            {
                loss[(Stats)j] = 0;
            }

            const uint32 owner = player->GetObjectGuid().GetRawValue();
            uint32 pointsToLose = lossPerDeath > usedStats ? usedStats : lossPerDeath;
            while (pointsToLose > 0)
            {
                const Stats statType = (Stats)urand(STAT_STRENGTH, MAX_STATS - 1);
                const uint32 statValue = GetStatsValue(owner, statType);
                if (statValue > 0)
                {
                    SetStatsValue(owner, statType, statValue - 1);
                    loss[statType]++;
                    pointsToLose--;
                }
            }

            std::ostringstream out;
            bool first = true;
            for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
            {
                const uint32 value = loss[(Stats)i];
                if (value > 0)
                {
                    if (!first) out << ", "; else first = false;
                    const uint32 langStat = LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH + i;
                    out << "|cffffa0a0-" << value << "|cffffff00 " << sObjectMgr.GetMangosString(langStat, player->GetSession()->GetSessionDbLocaleIndex());
                }
            }

            SendSysMessage(player, helper::FormatString
            (
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_LOST, player->GetSession()->GetSessionDbLocaleIndex()),
                out.str().c_str()
            ));

            player->InitStatsForLevel(true);
            player->UpdateAllStats();
        }
    }

    uint32 ApplyRandomPercent(uint32 value, const ImmersiveModuleConfig* config)
    {
        if (!config->enabled || !config->sharedRandomPercent) return value;
        float percent = (float)urand(0, config->sharedRandomPercent) - (float)config->sharedRandomPercent / 2;
        return value + (uint32)(value * percent / 100.0f);
    }

    #ifdef ENABLE_PLAYERBOTS
    class OnGiveXPAction : public ImmersiveAction
    {
    public:
        OnGiveXPAction(int32 value, const ImmersiveModuleConfig* config) : ImmersiveAction(config), value(value) {}

        bool Run(Player* player, Player* bot) override
        {
            if ((int)player->GetLevel() - (int)bot->GetLevel() < (int)config->sharedXpPercentLevelDiff)
            {
                return false;
            }

            if (!CheckSharedPercentReqs(player, bot))
            {
                return false;
            }

            bot->GiveXP(ApplyRandomPercent(value, config), NULL);

            Pet* pet = bot->GetPet();
            if (pet && pet->getPetType() == HUNTER_PET)
            {
                pet->GivePetXP(ApplyRandomPercent(value, config));
            }

            return true;
        }

        std::string GetActionMessage(Player* player) override
        {
            return helper::FormatString
            (
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_EXP_GAINED, player->GetSession()->GetSessionDbLocaleIndex()),
                value
            );
        }

    private:
        int32 value;
    };
    #endif

    void ImmersiveModule::OnGiveXP(Player* player, uint32 xp, Creature* victim)
    {
    #ifdef ENABLE_PLAYERBOTS
        if (!GetConfig()->enabled)
            return;

        if (!player->GetPlayerbotMgr())
            return;

        if (GetConfig()->sharedXpPercent < 0.01f)
            return;

        uint32 bonus_xp = xp + (victim ? player->GetXPRestBonus(xp) : 0);
        uint32 botXp = (uint32)(bonus_xp * GetConfig()->sharedXpPercent / 100.0f);
        if (botXp < 1)
        {
            return;
        }

        OnGiveXPAction action(botXp, GetConfig());
        RunAction(player, &action);
    #endif
    }

    void ImmersiveModule::OnGiveLevel(Player* player, uint32 level)
    {
        if (GetConfig()->enabled)
        {
            if (GetConfig()->manualAttributes)
            {
#ifdef ENABLE_PLAYERBOTS
                if (!player->isRealPlayer())
                    return;
#endif

                const uint32 usedStats = GetUsedStats(player);
                const uint32 totalStats = GetTotalStats(player);
                const uint32 availablePoints = (totalStats > usedStats) ? totalStats - usedStats : 0;
                if (availablePoints > 0)
                {
                    SendSysMessage(player, helper::FormatString
                    (
                        sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_POINTS_ADDED, player->GetSession()->GetSessionDbLocaleIndex()),
                        availablePoints
                    ));
                }
            }

            if (GetConfig()->infiniteLeveling)
            {
                if (level > DEFAULT_MAX_LEVEL && level <= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
                {
                    constexpr float increaseRate = 0.04f;
                    const uint32 levelsAboveMax = level - DEFAULT_MAX_LEVEL;
                    uint32 nextLevelXP = sObjectMgr.GetXPForLevel(DEFAULT_MAX_LEVEL);
                    nextLevelXP = (uint32)(nextLevelXP * std::pow(1 + increaseRate, levelsAboveMax));
                    player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, nextLevelXP);
                }
            }
        }
    }

    #ifdef ENABLE_PLAYERBOTS
    class OnGiveMoneyAction : public ImmersiveAction
    {
    public:
        OnGiveMoneyAction(int32 value, const ImmersiveModuleConfig* config) : ImmersiveAction(config), value(value) {}

        bool Run(Player* player, Player* bot) override
        {
            if ((int)player->GetLevel() - (int)bot->GetLevel() < (int)config->sharedXpPercentLevelDiff)
                return false;

            if (!CheckSharedPercentReqs(player, bot))
                return false;

            bot->ModifyMoney(ApplyRandomPercent(value, config));
            return true;
        }

        std::string GetActionMessage(Player* player) override
        {
            return helper::FormatString
            (
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MONEY_GAINED, player->GetSession()->GetSessionDbLocaleIndex()),
                ai::ChatHelper::formatMoney(value).c_str()
            );
        }

    private:
        int32 value;
    };
    #endif

    void ImmersiveModule::OnModifyMoney(Player* player, int32 diff)
    {
    #ifdef ENABLE_PLAYERBOTS
        if (!GetConfig()->enabled)
            return;

        if (!player->GetPlayerbotMgr())
            return;

        if (diff < 1)
            return;

        if (GetConfig()->sharedMoneyPercent < 0.01f)
            return;

        int32 botMoney = (int32)(diff * GetConfig()->sharedMoneyPercent / 100.0f);
        if (botMoney < 1) return;

        OnGiveMoneyAction action(botMoney, GetConfig());
        RunAction(player, &action);
    #endif
    }

    #ifdef ENABLE_PLAYERBOTS
    class OnRewardQuestAction : public ImmersiveAction
    {
    public:
        OnRewardQuestAction(Quest const* quest, const ImmersiveModuleConfig* config) : ImmersiveAction(config), quest(quest) {}

        bool Run(Player* player, Player* bot) override
        {
            if (quest->GetRequiredClasses())
            {
                return false;
            }

            if (!CheckSharedPercentReqs(player, bot))
            {
                return false;
            }

            uint32 questId = quest->GetQuestId();
            if (bot->GetQuestStatus(questId) != QUEST_STATUS_NONE)
            {
                return false;
            }

            bot->SetQuestStatus(questId, QUEST_STATUS_COMPLETE);
            QuestStatusData& sd = bot->getQuestStatusMap()[questId];
            sd.m_explored = true;
            sd.m_rewarded = true;
            sd.uState = (sd.uState != QUEST_NEW) ? QUEST_CHANGED : QUEST_NEW;

            return true;
        }

        std::string GetActionMessage(Player* player) override
        {
            return helper::FormatString
            (
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_QUEST_COMPLETED, player->GetSession()->GetSessionDbLocaleIndex()),
                quest->GetTitle().c_str()
            );
        }

    private:
        Quest const* quest;
    };
    #endif

    void ImmersiveModule::OnRewardQuest(Player* player, const Quest* quest)
    {
    #ifdef ENABLE_PLAYERBOTS
        if (!GetConfig()->enabled)
            return;

        if (!player->GetPlayerbotMgr())
            return;

        if (!GetConfig()->sharedQuests)
            return;

        if (!quest || quest->IsRepeatable())
            return;

        OnRewardQuestAction action(quest, GetConfig());
        RunAction(player, &action);
    #endif
    }

    void ImmersiveModule::OnGossipHello(Player* player, Creature* creature)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (player && creature)
            {
                // Check if speaking with a class trainer
                if (!creature->IsTrainerOf(player, false))
                    return;

                const std::string manageAtributesStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MANAGE);
                player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, manageAtributesStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_MENU, "", false);
            }
        }
    }

    bool ImmersiveModule::OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (player && creature)
            {
                // Check if speaking with a class trainer
                if (!creature->IsTrainerOf(player, false))
                    return false;

                switch (action)
                {
                    case IMMERSIVE_GOSSIP_OPTION_MENU:
                    {
                        player->GetPlayerMenu()->ClearMenus();
                        const std::string checkAtributesStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_CHECK_CURRENT);
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, checkAtributesStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_CHECK_CURRENT, "", false);
                        const std::string resetAttributesStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_UNLEARN);
                        const std::string resetAttributesAreYouSureStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_UNLEARN_SURE);
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, resetAttributesStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_RESET_STATS_MENU, resetAttributesAreYouSureStr, false);
                        const std::string reduceAttributesStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_REDUCE);
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, reduceAttributesStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_MENU, "", false);
                        std::string improveAttributeStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_IMPROVE); improveAttributeStr += " (%d)";
                        const std::string improveStrengthStr = helper::FormatString(improveAttributeStr.c_str(), player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH), GetTotalStatsValue(player, STAT_STRENGTH));
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, improveStrengthStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_IMPROVE_STRENGTH, "", false);
                        const std::string improveAgilityStr = helper::FormatString(improveAttributeStr.c_str(), player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_AGILITY), GetTotalStatsValue(player, STAT_AGILITY));
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, improveAgilityStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_IMPROVE_AGILITY, "", false);
                        const std::string improveStaminaStr = helper::FormatString(improveAttributeStr.c_str(), player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_STAMINA), GetTotalStatsValue(player, STAT_STAMINA));
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, improveStaminaStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_IMPROVE_STAMINA, "", false);
                        const std::string improveIntellectStr = helper::FormatString(improveAttributeStr.c_str(), player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_INTELLECT), GetTotalStatsValue(player, STAT_INTELLECT));
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, improveIntellectStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_IMPROVE_INTELLECT, "", false);
                        const std::string improveSpiritStr = helper::FormatString(improveAttributeStr.c_str(), player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_SPIRIT), GetTotalStatsValue(player, STAT_SPIRIT));
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, improveSpiritStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_IMPROVE_SPIRIT, "", false);
                        player->GetPlayerMenu()->SendGossipMenu(IMMERSIVE_NPC_TEXT, creature->GetObjectGuid());
                        return true;
                    }

                    case IMMERSIVE_GOSSIP_OPTION_CHECK_CURRENT:
                    {
                        PrintHelp(player, true, true);
                        OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_MENU, "", gossipListId);
                        return true;
                    }

                    case IMMERSIVE_GOSSIP_OPTION_IMPROVE_STRENGTH:
                    case IMMERSIVE_GOSSIP_OPTION_IMPROVE_AGILITY:
                    case IMMERSIVE_GOSSIP_OPTION_IMPROVE_STAMINA:
                    case IMMERSIVE_GOSSIP_OPTION_IMPROVE_INTELLECT:
                    case IMMERSIVE_GOSSIP_OPTION_IMPROVE_SPIRIT:
                    {
                        IncreaseStat(player, action - IMMERSIVE_GOSSIP_OPTION_IMPROVE_STRENGTH);
                        OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_MENU, "", gossipListId);
                        return true;
                    }

                    case IMMERSIVE_GOSSIP_OPTION_RESET_STATS_MENU:
                    {
                        player->GetPlayerMenu()->ClearMenus();
                        const std::string resetConfirmStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_UNLEARN_SURE);
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, resetConfirmStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_RESET_STATS_CONFIRM, "", false);
                        player->GetPlayerMenu()->SendGossipMenu(IMMERSIVE_RESET_ATTR_TEXT, creature->GetObjectGuid());
                        return true;
                    }

                    case IMMERSIVE_GOSSIP_OPTION_RESET_STATS_CONFIRM:
                    {
                        ResetStats(player);
                        OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_MENU, "", gossipListId);
                        return true;
                    }

                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_MENU:
                    {
                        player->GetPlayerMenu()->ClearMenus();
                        const std::string removeReductionStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_REDUCE_REMOVE);
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, removeReductionStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_0, "", false);
                        const std::string reduceStr = player->GetSession()->GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_REDUCE_PCT);
                        const std::string reduce10PctStr = helper::FormatString(reduceStr.c_str(), "10%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce10PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_10, "", false);
                        const std::string reduce20PctStr = helper::FormatString(reduceStr.c_str(), "20%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce20PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_20, "", false);
                        const std::string reduce30PctStr = helper::FormatString(reduceStr.c_str(), "30%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce30PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_30, "", false);
                        const std::string reduce40PctStr = helper::FormatString(reduceStr.c_str(), "40%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce40PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_40, "", false);
                        const std::string reduce50PctStr = helper::FormatString(reduceStr.c_str(), "50%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce50PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_50, "", false);
                        const std::string reduce60PctStr = helper::FormatString(reduceStr.c_str(), "60%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce60PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_60, "", false);
                        const std::string reduce70PctStr = helper::FormatString(reduceStr.c_str(), "70%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce70PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_70, "", false);
                        const std::string reduce80PctStr = helper::FormatString(reduceStr.c_str(), "80%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce80PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_80, "", false);
                        const std::string reduce90PctStr = helper::FormatString(reduceStr.c_str(), "90%");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TRAINER, reduce90PctStr, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_90, "", false);
                        player->GetPlayerMenu()->SendGossipMenu(IMMERSIVE_REDUCE_ATTR_TEXT, creature->GetObjectGuid());
                        return true;
                    }

                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_0:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_10:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_20:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_30:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_40:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_50:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_60:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_70:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_80:
                    case IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_90:
                    {
                        ChangeModifier(player, action - IMMERSIVE_GOSSIP_OPTION_REDUCE_STATS_0);
                        OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, IMMERSIVE_GOSSIP_OPTION_MENU, "", gossipListId);
                        return true;
                    }

                    default: break;
                }
            }
        }

        return false;
    }

    void ImmersiveModule::OnGetPlayerLevelInfo(Player* player, PlayerLevelInfo& info)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
    #ifdef ENABLE_PLAYERBOTS
            // Don't use custom stats on random bots
            if (IsRandomBot(player))
                return;
    #endif

            const PlayerInfo* playerInfo = GetPlayerInfo(player->getRace(), player->getClass());
            info = playerInfo->levelInfo[0];

            uint32 owner = player->GetObjectGuid().GetRawValue();
            int modifier = GetModifierValue(owner);
            for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
            {
                info.stats[i] += GetStatsValue(owner, (Stats)i) * modifier / 100;
            }
        }
    }

    uint32 XPGain(const Unit* unit, Unit* target)
    {
        Player* player = nullptr;
        if (unit && unit->IsPlayer())
        {
            player = (Player*)unit;
        }
        else if (unit && unit->IsCreature() && ((Creature*)unit)->GetSubtype() == CREATURE_SUBTYPE_PET)
        {
            player = (Player*)((Pet*)unit)->GetOwner();
        }

        Creature* creatureTarget = target && target->IsCreature() ? (Creature*)target : nullptr;
        Player* playerTarget = target && target->IsPlayer() ? (Player*)target : nullptr;

        if (!creatureTarget && !playerTarget)
            return 0;

        if (creatureTarget && (creatureTarget->IsTotem() || creatureTarget->IsPet() || creatureTarget->IsNoXp() || creatureTarget->IsCritter()))
            return 0;

        if (!player)
            return 0;

        float xpGain = 0.0f;
        const uint32 unitLevel = unit->GetLevel();
        const uint32 targetLevel = target->GetLevel();
        const uint32 nBaseExp = unitLevel * 5 + 45;
        if (targetLevel >= unitLevel)
        {
            uint32 nLevelDiff = targetLevel - unitLevel;
            nLevelDiff = nLevelDiff > 4 ? 4 : nLevelDiff;
            xpGain = nBaseExp * (1.0f + (0.05f * nLevelDiff));
        }
        else
        {
            uint32 nLevelDiff = DEFAULT_MAX_LEVEL > targetLevel ? DEFAULT_MAX_LEVEL - targetLevel : 1;
            xpGain = nBaseExp * (1.0f - (float(nLevelDiff) / 17));
        }

        if (xpGain > 0.0f)
        {
            if (creatureTarget)
            {
                Map* map = creatureTarget->GetMap();
                if (map->IsDungeon() || map->IsRaid())
                {
                    DungeonMap* dungeonMap = (DungeonMap*)map;
                    const uint32 maxPlayers = dungeonMap->GetMaxPlayers();
                    if (dungeonMap->IsRaid())
                    {
                        if (creatureTarget->IsElite())
                        {
                            // Raid boss
                            const CreatureInfo* creatureInfo = creatureTarget->GetCreatureInfo();
                            if (creatureInfo && creatureInfo->Rank == CREATURE_ELITE_WORLDBOSS)
                            {
                                xpGain *= 10.0f;
                            }
                            // Raid elite
                            else
                            {
                                xpGain *= 1.0f;
                            }
                        }
                        // Raid non-elite creature
                        else
                        {
                            xpGain *= 0.25f;
                        }
                    }
                    else if (dungeonMap->IsDungeon())
                    {
                        if (creatureTarget->IsElite())
                        {
                            // Dungeon boss
                            if (sObjectMgr.IsEncounter(creatureTarget->GetEntry(), creatureTarget->GetMapId()))
                            {
                                xpGain *= 5.0f;
                            }
                            // Dungeon elite
                            else
                            {
                                xpGain *= 1.0f;
                            }
                        }
                        // Dungeon non-elite creature
                        else
                        {
                            xpGain *= 0.25f;
                        }
                    }

                    xpGain *= maxPlayers;
                }

                xpGain *= creatureTarget->GetCreatureInfo()->ExperienceMultiplier;
                xpGain = creatureTarget->GetModifierXpBasedOnDamageReceived(xpGain);
            }
            else if (playerTarget)
            {
                // BG player kill
                uint32 maxPlayers = 1;
                if (player->InBattleGround())
                {
                    BattleGround* bg = player->GetBattleGround();
                    if (bg)
                    {
                        maxPlayers = bg->GetMaxPlayers();
                        xpGain *= 2.0f;
                    }
                }
#if EXPANSION > 0
                // Arena player kill
                else if (player->InArena())
                {
                    xpGain *= 1.0f;
                }
#endif
                // Open world player kill
                else
                {
                    xpGain *= 1.0f;
                }

                xpGain *= maxPlayers;
            }

            xpGain *= sWorld.getConfig(CONFIG_FLOAT_RATE_XP_KILL);

            // Divide group experience
            if (player && player->GetGroup())
            {
                Group* group = player->GetGroup();
                xpGain /= group->GetMembersCount();
            }

            return (uint32)(std::nearbyint(xpGain));
        }

        return 0;
    }

    bool ImmersiveModule::OnPreRewardPlayerAtKill(Player* player, Unit* victim)
    {
        if (GetConfig()->enabled && GetConfig()->infiniteLeveling)
        {
            if (player && victim)
            {
                // Check if the player is on the range between the expansion max level and the set max level
                const uint32 playerLevel = player->GetLevel();
                if (playerLevel >= DEFAULT_MAX_LEVEL && playerLevel < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
                {
                    if (victim->IsCreature() && !victim->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
                    {
                        if (victim->GetLevel() > DEFAULT_MAX_LEVEL - 5)
                        {
                            // Calculate the exp given (formula from MaNGOS::XP::Gain)
                            Creature* creatureVictim = (Creature*)victim;
                            player->GiveXP(XPGain(player, creatureVictim), creatureVictim);
                            if (Pet* pet = player->GetPet())
                            {
                                pet->GivePetXP(XPGain(pet, creatureVictim));
                            }

                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    void ImmersiveModule::OnRewardPlayerAtKill(Player* player, Unit* victim)
    {
        if (GetConfig()->enabled && GetConfig()->xpOnPvPKill)
        {
            if (player && victim)
            {
                if (victim->IsPlayer() && player->IsAlive())
                {
                    Player* playerVictim = (Player*)victim;
                    if (playerVictim->GetAura(2479, EFFECT_INDEX_0))
                        return;

                    if (player->GetTeam() == playerVictim->GetTeam())
                        return;

                    // Only consider kills for players with 3 levels differences
                    const uint32 playerLevel = GetConfig()->infiniteLeveling && player->GetLevel() > DEFAULT_MAX_LEVEL + 1 ? DEFAULT_MAX_LEVEL : player->GetLevel();
                    const uint32 victimLevel = GetConfig()->infiniteLeveling && playerVictim->GetLevel() > DEFAULT_MAX_LEVEL + 1 ? DEFAULT_MAX_LEVEL : playerVictim->GetLevel();
                    if (playerLevel <= victimLevel + 3)
                    {
                        // Calculate the exp given (formula from MaNGOS::XP::Gain)
                        player->GiveXP(XPGain(player, playerVictim), nullptr);
                        if (Pet* pet = player->GetPet())
                        {
                            pet->GivePetXP(XPGain(pet, nullptr));
                        }
                    }
                }
            }
        }
    }

    void ImmersiveModule::OnSaveToDB(Player* player)
    {
        SyncAccountReputation(player);
    }

    bool ImmersiveModule::OnRespawn(Creature* creature, time_t& respawnTime)
    {
        if (GetConfig()->enabled && GetConfig()->disableInstanceRespawn)
        {
            // Disable instance creatures respawn 
            if (creature)
            {
                // Ignore manual respawns
                const uint32 creatureId = creature->GetObjectGuid().GetCounter();
                if (creatureRespawnScheduled.find(creatureId) != creatureRespawnScheduled.end())
                {
                    creatureRespawnScheduled.erase(creatureId);
                }
                else
                {
                    Map* map = creature->GetMap();
                    if (map && !map->IsBattleGround() && (map->IsDungeon() || map->IsRaid()))
                    {
                        // Prevent respawning logic (and add 1 hour to the timer)
                        respawnTime = time(nullptr) + 3600;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void ImmersiveModule::OnRespawnRequest(Creature* creature)
    {
        if (GetConfig()->enabled && GetConfig()->disableInstanceRespawn)
        {
            if (creature)
            {
                const uint32 creatureId = creature->GetObjectGuid().GetCounter();
                creatureRespawnScheduled.insert(creatureId);
            }
        }
    }

    bool ImmersiveModule::OnUse(GameObject* gameObject, Unit* user)
    {
        if (GetConfig()->enabled && GetConfig()->fishingBaubles)
        {
            if (gameObject && gameObject->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE && user && user->IsPlayer())
            {
                Player* player = (Player*)user;
                if (gameObject->GetLootState() == GO_READY)
                {
                    // 1) skill must be >= base_zone_skill
                    // 2) if skill == base_zone_skill => 5% chance
                    // 3) chance is linear dependence from (base_zone_skill-skill)

                    uint32 zone, subzone;
                    gameObject->GetZoneAndAreaId(zone, subzone);

                    int32 zone_skill = sObjectMgr.GetFishingBaseSkillLevel(subzone);
                    if (!zone_skill)
                    {
                        zone_skill = sObjectMgr.GetFishingBaseSkillLevel(zone);
                    }

                    // provide error, no fishable zone or area should be 0
                    if (!zone_skill)
                    {
                        sLog.outErrorDb("Fishable areaId %u are not properly defined in `skill_fishing_base_level`.", subzone);
                    }

                    int32 skill = player->GetSkillValue(SKILL_FISHING);
                    int32 chance = skill - zone_skill + 5;
                    int32 roll = irand(1, 100);

                    DEBUG_LOG("Fishing check (skill: %i zone min skill: %i chance %i roll: %i", skill, zone_skill, chance, roll);

                    // normal chance
                    bool success = skill >= zone_skill && chance >= roll;
                    if (success)
                    {
                        // Check for fishing bauble
                        success = HasFishingBauble(player);
                    }

                    GameObject* fishingHole = nullptr;

                    // overwrite fail in case fishhole if allowed (after 3.3.0)
                    if (!success)
                    {
                        if (!sWorld.getConfig(CONFIG_BOOL_SKILL_FAIL_POSSIBLE_FISHINGPOOL))
                        {
                            // TODO: find reasonable value for fishing hole search
                            fishingHole = gameObject->LookupFishingHoleAround(20.0f + CONTACT_DISTANCE);
                            if (fishingHole)
                            {
                                success = true;
                            }
                        }
                    }
                    // just search fishhole for success case
                    else
                    {
                        // TODO: find reasonable value for fishing hole search
                        fishingHole = gameObject->LookupFishingHoleAround(20.0f + CONTACT_DISTANCE);
                    }

                    player->UpdateFishingSkill();

                    // fish catch or fail and junk allowed (after 3.1.0)
                    if (success || sWorld.getConfig(CONFIG_BOOL_SKILL_FAIL_LOOT_FISHING))
                    {
                        // prevent removing GO at spell cancel
                        player->RemoveGameObject(gameObject, false);
                        gameObject->SetOwnerGuid(player->GetObjectGuid());

                        if (fishingHole)                    // will set at success only
                        {
                            fishingHole->Use(player);
                            gameObject->SetLootState(GO_JUST_DEACTIVATED);
                        }
                        else
                        {
                            delete gameObject->m_loot;
                            gameObject->m_loot = new Loot(player, gameObject, success ? LOOT_FISHING : LOOT_FISHING_FAIL);
                            gameObject->m_loot->ShowContentTo(player);
                        }
                    }
                    else
                    {
                        // fish escaped, can be deleted now
                        gameObject->SetLootState(GO_JUST_DEACTIVATED);

                        WorldPacket data(SMSG_FISH_ESCAPED, 0);
                        player->GetSession()->SendPacket(data);
                    }
                

                    player->FinishSpell(CURRENT_CHANNELED_SPELL);
                    return true;
                }
            }
        }

        return false;
    }

    bool ImmersiveModule::OnCalculateEffectiveDodgeChance(const Unit* unit, const Unit* attacker, uint8 attType, const SpellEntry* ability, float& outChance)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (unit && attacker)
            {
                // Check if the stats modifier is set
                if (!CalculateEffectiveChanceDelta(unit) && !CalculateEffectiveChanceDelta(attacker))
                    return false;

                outChance = 0.0f;
                outChance += unit->GetDodgeChance();

                // Own chance appears to be zero / below zero / unmeaningful for some reason (debuffs?): skip calculation, unit is incapable
                if (outChance < 0.005f)
                {
                    outChance = 0.0f;
                    return false;
                }

                const bool weapon = (!ability || IsSpellUseWeaponSkill(ability));
                // Skill difference can be negative (and reduce our chance to mitigate an attack), which means:
                // a) Attacker's level is higher
                // b) Attacker has +skill bonuses
                const bool isPlayerOrPet = unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                const uint32 skill = (weapon ? attacker->GetWeaponSkillValue((WeaponAttackType)attType, unit) : attacker->GetSkillMaxForLevel(unit));
                int32 difference = int32(unit->GetDefenseSkillValue(attacker) - skill);
                difference = CalculateEffectiveChance(difference, attacker, unit, IMMERSIVE_EFFECTIVE_CHANCE_DODGE);

                // Defense/weapon skill factor: for players and NPCs
                float factor = 0.04f;
                // NPCs gain additional bonus dodge chance based on positive skill difference
                if (!isPlayerOrPet && difference > 0)
                    factor = 0.1f;

                outChance += (difference * factor);
                outChance = std::max(0.0f, std::min(outChance, 100.0f));
                return true;
            }
        }

        return false;
    }

    bool ImmersiveModule::OnCalculateEffectiveBlockChance(const Unit* unit, const Unit* attacker, uint8 attType, const SpellEntry* ability, float &outChance)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (unit && attacker)
            {
                // Check if the stats modifier is set
                if (!CalculateEffectiveChanceDelta(unit) && !CalculateEffectiveChanceDelta(attacker))
                    return false;

                outChance = 0.0f;
                outChance += unit->GetBlockChance();
                // Own chance appears to be zero / below zero / unmeaningful for some reason (debuffs?): skip calculation, unit is incapable
                if (outChance < 0.005f)
                {
                    outChance = 0.0f;
                    return false;
                }

                const bool weapon = (!ability || IsSpellUseWeaponSkill(ability));

                // Skill difference can be negative (and reduce our chance to mitigate an attack), which means:
                // a) Attacker's level is higher
                // b) Attacker has +skill bonuses
                const bool isPlayerOrPet = unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                const uint32 skill = (weapon ? attacker->GetWeaponSkillValue((WeaponAttackType)attType, unit) : attacker->GetSkillMaxForLevel(unit));
                int32 difference = int32(unit->GetDefenseSkillValue(attacker) - skill);
                difference = CalculateEffectiveChance(difference, attacker, unit, IMMERSIVE_EFFECTIVE_CHANCE_BLOCK);

                // Defense/weapon skill factor: for players and NPCs
                float factor = 0.04f;
                // NPCs cannot gain bonus block chance based on positive skill difference
                if (!isPlayerOrPet && difference > 0)
                    factor = 0.0f;

                outChance += (difference * factor);
                outChance = std::max(0.0f, std::min(outChance, 100.0f));
                return true;
            }
        }

        return false;
    }

    bool ImmersiveModule::OnCalculateEffectiveParryChance(const Unit* unit, const Unit* attacker, uint8 attType, const SpellEntry* ability, float& outChance)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (unit && attacker)
            {
                // Check if the stats modifier is set
                if (!CalculateEffectiveChanceDelta(unit) && !CalculateEffectiveChanceDelta(attacker))
                    return false;

                if (attType == RANGED_ATTACK)
                {
                    return false;
                }

                outChance = 0.0f;
                outChance += unit->GetParryChance();
                // Own chance appears to be zero / below zero / unmeaningful for some reason (debuffs?): skip calculation, unit is incapable
                if (outChance < 0.005f)
                {
                    outChance = 0.0f;
                    return false;
                }

                const bool weapon = (!ability || IsSpellUseWeaponSkill(ability));

                // Skill difference can be negative (and reduce our chance to mitigate an attack), which means:
                // a) Attacker's level is higher
                // b) Attacker has +skill bonuses
                const bool isPlayerOrPet = unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                const uint32 skill = (weapon ? attacker->GetWeaponSkillValue((WeaponAttackType)attType, unit) : attacker->GetSkillMaxForLevel(unit));
                int32 difference = int32(unit->GetDefenseSkillValue(attacker) - skill);
                difference = CalculateEffectiveChance(difference, attacker, unit, IMMERSIVE_EFFECTIVE_CHANCE_PARRY);

                // Defense/weapon skill factor: for players and NPCs
                float factor = 0.04f;
                // NPCs gain additional bonus parry chance based on positive skill difference (same value as bonus miss rate)
                if (!isPlayerOrPet && difference > 0)
                {
                    if (difference > 10)
                        factor = 0.6f; // Pre-WotLK: 0.2 additional factor for each level above 2
                    else
                        factor = 0.1f;
                }

                outChance += (difference * factor);
                outChance = std::max(0.0f, std::min(outChance, 100.0f));
                return true;
            }
        }

        return false;
    }

    bool ImmersiveModule::OnCalculateEffectiveCritChance(const Unit* unit, const Unit* victim, uint8 attType, const SpellEntry* ability, float& outChance)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (unit && victim)
            {
                // Check if the stats modifier is set
                if (!CalculateEffectiveChanceDelta(unit) && !CalculateEffectiveChanceDelta(victim))
                    return false;

                outChance = 0.0f;
                outChance += (ability ? unit->GetCritChance(ability, SPELL_SCHOOL_MASK_NORMAL) : unit->GetCritChance((WeaponAttackType)attType));
            
                // Own chance appears to be zero / below zero / unmeaningful for some reason (debuffs?): skip calculation, unit is incapable
                if (outChance < 0.005f)
                {
                    outChance = 0.0f;
                    return false;
                }

                // Skip victim calculation if positive ability
                if (ability && IsPositiveSpell(ability, unit, victim))
                {
                    return false;
                }

                const bool weapon = (!ability || IsSpellUseWeaponSkill(ability));
                // Skill difference can be both negative and positive.
                // a) Positive means that attacker's level is higher or additional weapon +skill bonuses
                // b) Negative means that victim's level is higher or additional +defense bonuses
                const bool vsPlayerOrPet = victim->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                const bool ranged = (attType == RANGED_ATTACK);
                // weapon skill does not benefit crit% vs NPCs
                const uint32 skill = (weapon && !vsPlayerOrPet ? unit->GetWeaponSkillValue((WeaponAttackType)attType, victim) : unit->GetSkillMaxForLevel(victim));
                int32 difference = int32(skill - victim->GetDefenseSkillValue(unit));
                difference = CalculateEffectiveChance(difference, unit, victim, IMMERSIVE_EFFECTIVE_CHANCE_CRIT);

                // Weapon skill factor: for players and NPCs
                float factor = 0.04f;
                // Crit suppression against NPCs with higher level
                if (!vsPlayerOrPet) // decrease by 1% of crit per level of target
                    factor = 0.2f;

                outChance += (difference * factor);

                // Victim's crit taken chance
                const SpellDmgClass dmgClass = (ranged ? SPELL_DAMAGE_CLASS_RANGED : SPELL_DAMAGE_CLASS_MELEE);
#if EXPANSION == 2
                outChance += victim->GetCritTakenChance(unit, SPELL_SCHOOL_MASK_NORMAL, dmgClass, ability);
#else
                outChance += victim->GetCritTakenChance(SPELL_SCHOOL_MASK_NORMAL, dmgClass);
#endif
                for (auto i : unit->GetScriptedLocationAuras(SCRIPT_LOCATION_CRIT_CHANCE))
                {
                    if (!i->isAffectedOnSpell(ability))
                        continue;

                    i->OnCritChanceCalculate(victim, outChance, ability);
                }

                outChance = std::max(0.0f, std::min(outChance, 100.0f));
                return true;
            }
        }

        return false;
    }

    bool ImmersiveModule::OnCalculateEffectiveMissChance(const Unit* unit, const Unit* victim, uint8 attType, const SpellEntry* ability, const Spell* const* currentSpells, const SpellPartialResistDistribution& spellPartialResistDistribution, float& outChance)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (unit && victim)
            {
                // Check if the stats modifier is set
                if (!CalculateEffectiveChanceDelta(unit) && !CalculateEffectiveChanceDelta(victim))
                    return false;

                outChance = 0.0f;
                outChance += (ability ? victim->GetMissChance(ability, SPELL_SCHOOL_MASK_NORMAL) : victim->GetMissChance((WeaponAttackType)attType));
            
                // Victim's own chance appears to be zero / below zero / unmeaningful for some reason (debuffs?): skip calculation, unit can't be missed
                if (outChance < 0.005f)
                {
                    outChance = 0.0f;
                    return false;
                }

                const bool ranged = (attType == RANGED_ATTACK);
                const bool weapon = (!ability || IsSpellUseWeaponSkill(ability));
                // Check if dual wielding, add additional miss penalty - when mainhand has on next swing spellInfo, offhand doesnt suffer penalty
                if (!ability && !ranged && unit->hasOffhandWeaponForAttack() && (!currentSpells[CURRENT_MELEE_SPELL] || !IsNextMeleeSwingSpell(currentSpells[CURRENT_MELEE_SPELL]->m_spellInfo)))
                    outChance += 19.0f;

                // Skill difference can be both negative and positive. Positive difference means that:
                // a) Victim's level is higher
                // b) Victim has additional defense skill bonuses
                const bool vsPlayerOrPet = victim->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                const uint32 skill = (weapon ? unit->GetWeaponSkillValue((WeaponAttackType)attType, victim) : unit->GetSkillMaxForLevel(victim));
                int32 difference = int32(victim->GetDefenseSkillValue(unit) - skill);
                difference = CalculateEffectiveChance(difference, unit, victim, IMMERSIVE_EFFECTIVE_CHANCE_MISS);

                // Defense/weapon skill factor: for players and NPCs
                float factor = 0.04f;
                // NPCs gain additional bonus to incoming hit chance reduction based on positive skill difference (same value as bonus parry rate)
                if (!vsPlayerOrPet && difference > 0)
                {
                    if (difference > 10)
                    {
                        // First 10 points of difference (2 levels): usual decrease
                        outChance += (10 * 0.1f);
                        difference -= 10;
                        // Each additional point of difference:
                        factor = 0.4f;
                        outChance += (difference * 0.2f); // Pre-WotLK: Additional 1% miss chance for each level (final @ 3% per level)
                    }
                    else
                        factor = 0.1f;
                }

                outChance += (difference * factor);
                // Victim's auras affecting attacker's hit contribution:
                outChance -= victim->GetTotalAuraModifier(ranged ? SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE : SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE);
                // For elemental melee auto-attacks: full resist outcome converted into miss chance (original research on combat logs)
                if (!ranged && !ability)
                {
                    const float percent = victim->CalculateEffectiveMagicResistancePercent(unit, unit->GetMeleeDamageSchoolMask((attType == BASE_ATTACK), true));
                    if (percent > 0)
                    {
                        if (const uint32 uindex = uint32(percent * 100))
                        {
                            const SpellPartialResistChanceEntry& chances = spellPartialResistDistribution.at(uindex);
                            outChance += float(chances.at(SPELL_PARTIAL_RESIST_PCT_100) / 100);
                        }
                    }
                }

                // Finally, take hit chance
                outChance -= (ability ? unit->GetHitChance(ability, SPELL_SCHOOL_MASK_NORMAL) : unit->GetHitChance((WeaponAttackType)attType));
                float minimum = (ability && ability->DmgClass == SPELL_DAMAGE_CLASS_MAGIC) || difference > 10 ? 1.f : 0;
                outChance = std::max(minimum, std::min(outChance, 100.0f));
                return true;
            }
        }

        return false;
    }

    bool ImmersiveModule::OnCalculateSpellMissChance(const Unit* unit, const Unit* victim, uint32 schoolMask, const SpellEntry* spell, float& outChance)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (unit && victim)
            {
                // Check if the stats modifier is set
                if (!CalculateEffectiveChanceDelta(unit) && !CalculateEffectiveChanceDelta(victim))
                    return false;

                outChance = 0.0f;
                const float minimum = 1.0f; // Pre-WotLK: unavoidable spellInfo miss is at least 1%

                if (spell->HasAttribute(SPELL_ATTR_EX3_NORMAL_RANGED_ATTACK) || spell->DmgClass == SPELL_DAMAGE_CLASS_MELEE || spell->DmgClass == SPELL_DAMAGE_CLASS_RANGED)
                {
                    return false;
                }

                outChance += victim->GetMissChance(spell, (SpellSchoolMask)schoolMask);
                // Victim's own chance appears to be zero / below zero / unmeaningful for some reason (debuffs?): skip calculation, unit can't be missed
                if (outChance < 0.005f)
                {
                    outChance = 0.0f;
                    return false;
                }

                // Level difference: positive adds to miss chance, negative substracts
                const bool vsPlayerOrPet = victim->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                int32 difference = int32(victim->GetLevelForTarget(unit) - unit->GetLevelForTarget(victim));
                difference = CalculateEffectiveChance(difference, unit, victim, IMMERSIVE_EFFECTIVE_CHANCE_SPELL_MISS);

                // Level difference factor: 1% per level
                uint8 factor = 1;
                // NPCs and players gain additional bonus to incoming spellInfo hit chance reduction based on positive level difference
                if (difference > 2)
                {
                    outChance += (2 * factor);
                    // Miss bonus for each additional level of difference above 2
                    factor = (vsPlayerOrPet ? 7 : 11);
                    outChance += ((difference - 2) * factor);
                }
                else
                {
                    outChance += (difference * factor);
                }

                // Reduce by caster's spellInfo hit chance
                outChance -= unit->GetHitChance(spell, (SpellSchoolMask)schoolMask);
                // Reduce (or increase) by victim SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE auras
                outChance -= victim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE, schoolMask);
                outChance = std::max(minimum, std::min(outChance, 100.0f));
                return true;
            }
        }

        return false;
    }

    bool ImmersiveModule::OnGetAttackDistance(const Unit* unit, const Unit* target, float& outDistance)
    {
        if (GetConfig()->enabled && GetConfig()->manualAttributes)
        {
            if (unit && target)
            {
                // Check if the stats modifier is set
                if (!CalculateEffectiveChanceDelta(unit) && !CalculateEffectiveChanceDelta(target))
                    return false;

                float aggroRate = sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO);
                uint32 playerlevel = target->GetLevelForTarget(unit);
                uint32 creaturelevel = unit->GetLevelForTarget(target);

                int32 leveldif = int32(playerlevel) - int32(creaturelevel);
                leveldif = CalculateEffectiveChance(leveldif, unit, target, IMMERSIVE_EFFECTIVE_ATTACK_DISTANCE);

                // "The maximum Aggro Radius has a cap of 25 levels under. Example: A level 30 char has the same Aggro Radius of a level 5 char on a level 60 mob."
                if (leveldif < -25)
                    leveldif = -25;

                // "The aggro radius of a mob having the same level as the player is roughly 18 yards"
                outDistance = unit->GetDetectionRange();
                if (outDistance == 0.f)
                {
                    outDistance = 0.0f;
                    return false;
                }

                // "Aggro Radius varies with level difference at a rate of roughly 1 yard/level"
                // radius grow if playlevel < creaturelevel
                outDistance -= (float)leveldif;

                // detect range auras
                outDistance += unit->GetTotalAuraModifier(SPELL_AURA_MOD_DETECT_RANGE);

                // detected range auras
                outDistance += target->GetTotalAuraModifier(SPELL_AURA_MOD_DETECTED_RANGE);

                // "Minimum Aggro Radius for a mob seems to be combat range (5 yards)"
                if (outDistance < 5)
                    outDistance = 5;

                if (target->IsPlayerControlled() && target->IsCreature() && !target->IsClientControlled()) // player pets do not aggro from so afar
                    outDistance = outDistance * 0.65f;

                outDistance = (outDistance * aggroRate);

                return true;
            }
        }

        return false;
    }

    float ImmersiveModule::GetFallDamage(Player* player, float zdist, float defaultVal)
    {
        return 0.0055f * zdist * zdist * GetConfig()->fallDamageMultiplier;
    }

    bool ImmersiveModule::HasFishingBauble(Player* player)
    {
        Item* const item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        if (!item)
            return false;

        uint32 eId = item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT);
        uint32 eDuration = item->GetEnchantmentDuration(TEMP_ENCHANTMENT_SLOT);
        if (!eDuration)
            return false;

        SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(eId);
        if (!pEnchant)
            return false;

        for (int s = 0; s < 3; ++s)
        {
            uint32 spellId = pEnchant->spellid[s];
            if (pEnchant->type[s] == ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL && spellId)
            {
                SpellEntry const* entry = sSpellTemplate.LookupEntry<SpellEntry>(spellId);
                if (entry)
                {
                    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
                    {
                        if (entry->Effect[i] == SPELL_EFFECT_APPLY_AURA && entry->EffectMiscValue[i] == SKILL_FISHING)
                            return true;
                    }
                }
            }
        }

        return false;
    }

    void ImmersiveModule::DisableOfflineRespawn()
    {
        uint32 lastPing = GetStatsValue(0, "last_ping");
        if (!lastPing) return;

        uint32 offlineTime = sWorld.GetGameTime() - lastPing;
        sLog.outString("Prolonging respawn time: +%u sec", offlineTime);

        CharacterDatabase.BeginTransaction();

        CharacterDatabase.DirectPExecute("update `creature_respawn` set `respawntime` = `respawntime` + '%u'", offlineTime);
        CharacterDatabase.DirectPExecute("update `instance_reset` set `resettime` = `resettime` + '%u'", offlineTime);
        CharacterDatabase.DirectPExecute("update `instance` set `resettime` = `resettime` + '%u'", offlineTime);
        CharacterDatabase.DirectPExecute("update `gameobject_respawn` set `respawntime` = `respawntime` + '%u'", offlineTime);
        SetStatsValue(0, "last_ping", sWorld.GetGameTime());

        CharacterDatabase.CommitTransaction();
    }

    PlayerInfo extraPlayerInfo[MAX_RACES][MAX_CLASSES];

    const PlayerInfo* ImmersiveModule::GetPlayerInfo(uint32 race, uint32 class_)
    {
    #if EXPANSION > 0
        if (class_ == CLASS_SHAMAN && race == RACE_NIGHTELF)
        {
            PlayerInfo const* piSh = sObjectMgr.GetPlayerInfo(RACE_DRAENEI, class_);
            PlayerInfo *result = &extraPlayerInfo[race][class_];
            memcpy(result, piSh, sizeof(PlayerInfo));

            PlayerInfo const* piDr = sObjectMgr.GetPlayerInfo(race, CLASS_DRUID);
            result->displayId_f = piDr->displayId_f;
            result->displayId_m = piDr->displayId_m;

            return result;
        }
    #endif

        if (class_ == CLASS_DRUID && race == RACE_TROLL)
        {
            PlayerInfo const* piSh = sObjectMgr.GetPlayerInfo(RACE_TAUREN, class_);
            PlayerInfo *result = &extraPlayerInfo[race][class_];
            memcpy(result, piSh, sizeof(PlayerInfo));

            PlayerInfo const* piDr = sObjectMgr.GetPlayerInfo(race, CLASS_SHAMAN);
            result->displayId_f = piDr->displayId_f;
            result->displayId_m = piDr->displayId_m;
            return result;
        }

        return sObjectMgr.GetPlayerInfo(race, class_);
    }

    void ImmersiveModule::PrintHelp(Player *player, bool detailed, bool help)
    {
        uint32 usedStats = GetUsedStats(player);
        uint32 totalStats = GetTotalStats(player);
        uint32 purchaseCost = GetStatCost(player) * GetConfig()->manualAttributesIncrease;

        SendSysMessage(player, helper::FormatString
        (
            sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_AVAILABLE, player->GetSession()->GetSessionDbLocaleIndex()),
            (totalStats > usedStats ? totalStats - usedStats : 0),
            formatMoney(purchaseCost).c_str())
        );

        if (detailed)
        {
            PrintUsedStats(player);
        }

        if (help)
        {
            PrintSuggestedStats(player);
        }
    }

    void ImmersiveModule::PrintUsedStats(Player* player)
    {
        uint32 owner = player->GetObjectGuid().GetRawValue();
        uint32 modifier = GetModifierValue(owner);

        std::ostringstream out;
        bool first = true;
        bool used = false;

        PlayerInfo const* info = GetPlayerInfo(player->getRace(), player->getClass());
        PlayerLevelInfo level1Info = info->levelInfo[0];

        for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        {
            uint32 value = level1Info.stats[i];
            value += GetStatsValue(owner, (Stats)i) * modifier / 100;
            if (!value) continue;
            if (!first) out << ", "; else first = false;
            const uint32 langStat = LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH + i;
            out << "|cff00ff00+" << value << "|cffffff00 " << sObjectMgr.GetMangosString(langStat, player->GetSession()->GetSessionDbLocaleIndex());
            used = true;
        }

        if (modifier != 100)
        {
            std::ostringstream modifierStr; modifierStr << modifier << percent(player);
            out << " " << helper::FormatString(sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MODIFIER, player->GetSession()->GetSessionDbLocaleIndex()), modifierStr.str().c_str());
        }

        SendSysMessage(player, helper::FormatString
        (
            sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_ASSIGNED, player->GetSession()->GetSessionDbLocaleIndex()),
            out.str().c_str()
        ));
    }

    void ImmersiveModule::PrintSuggestedStats(Player* player)
    {
        std::ostringstream out;
        PlayerInfo const* info = GetPlayerInfo(player->getRace(), player->getClass());
        uint8 level = player->GetLevel();
        PlayerLevelInfo levelCInfo = info->levelInfo[level - 1];

        bool first = true;
        bool used = false;
        for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        {
            uint32 value = levelCInfo.stats[i];
            value = (uint32)floor(value * GetConfig()->manualAttributesPercent / 100.0f);
            if (!value) continue;
            if (!first) out << ", "; else first = false;
            const uint32 langStat = LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH + i;
            out << "|cff00ff00+" << value << "|cffffff00 " << sObjectMgr.GetMangosString(langStat, player->GetSession()->GetSessionDbLocaleIndex());
            used = true;
        }

        if (used)
        {
            SendSysMessage(player, helper::FormatString
            (
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_SUGGESTED, player->GetSession()->GetSessionDbLocaleIndex()),
                out.str().c_str()
            ));
        }
    }

    void ImmersiveModule::ChangeModifier(Player *player, uint32 type)
    {
        if (!GetConfig()->enabled) 
            return;

        uint32 owner = player->GetObjectGuid().GetRawValue();
        uint32 value = type * 10;
        SetStatsValue(owner, "modifier", value);

        std::ostringstream modifierStr; modifierStr << value << percent(player);
        if (!value || value == 100)
        {
            modifierStr << " " << sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MOD_DISABLED, player->GetSession()->GetSessionDbLocaleIndex());
        }

        SendSysMessage(player, helper::FormatString
        (
            sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MOD_CHANGED, player->GetSession()->GetSessionDbLocaleIndex()),
            modifierStr.str().c_str()
        ));

        player->InitStatsForLevel(true);
        player->UpdateAllStats();
    }

    void ImmersiveModule::IncreaseStat(Player *player, uint32 type)
    {
        if (!GetConfig()->enabled)
        {
            SendSysMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_DISABLED, player->GetSession()->GetSessionDbLocaleIndex()));
            return;
        }

        uint32 owner = player->GetObjectGuid().GetRawValue();
        uint32 usedStats = GetUsedStats(player);
        uint32 totalStats = GetTotalStats(player);
        uint32 cost = GetStatCost(player);
        uint32 attributePointsAvailable = (totalStats > usedStats ? totalStats - usedStats : 0);
        uint32 statIncrease = std::min(GetConfig()->manualAttributesIncrease, attributePointsAvailable);
        uint32 purchaseCost = cost * statIncrease;

        if (usedStats >= totalStats)
        {
            SendSysMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MISSING, player->GetSession()->GetSessionDbLocaleIndex()));
            return;
        }

        if (player->GetMoney() < purchaseCost)
        {
            SendSysMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MISSING_GOLD, player->GetSession()->GetSessionDbLocaleIndex()));
            return;
        }

        uint32 value = GetStatsValue(owner, (Stats)type);
        SetStatsValue(owner, (Stats)type, value + statIncrease);

        usedStats = GetUsedStats(player);
        totalStats = GetTotalStats(player);
        attributePointsAvailable = (totalStats > usedStats ? totalStats - usedStats : 0);

        SendSysMessage(player, helper::FormatString
        (
            sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_GAINED, player->GetSession()->GetSessionDbLocaleIndex()),
            statIncrease,
            sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH + type, player->GetSession()->GetSessionDbLocaleIndex()),
            attributePointsAvailable,
            formatMoney(purchaseCost).c_str()
        ));

        player->InitStatsForLevel(true);
        player->UpdateAllStats();
        player->ModifyMoney(-(int32)purchaseCost);
        player->SaveGoldToDB();
    }

    void ImmersiveModule::ResetStats(Player *player)
    {
        if (!GetConfig()->enabled)
        {
            SendSysMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_DISABLED, player->GetSession()->GetSessionDbLocaleIndex()));
            return;
        }

        uint32 owner = player->GetObjectGuid().GetRawValue();

        for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        {
            SetStatsValue(owner, statValues[(Stats)i], 0);
        }

        uint32 usedStats = GetUsedStats(player);
        uint32 totalStats = GetTotalStats(player);

        SendSysMessage(player, helper::FormatString
        (
            sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_RESET, player->GetSession()->GetSessionDbLocaleIndex()),
            (totalStats > usedStats ? totalStats - usedStats : 0)
        ));

        player->InitStatsForLevel(true);
        player->UpdateAllStats();
    }

    uint32 ImmersiveModule::GetTotalStats(Player *player, uint8 level)
    {
        if (!level) 
        {
            level = player->GetLevel();
        }

        const uint32 maxStats = GetConfig()->manualAttributesMaxPoints;
        if(maxStats > 0)
        {
            // Calculate the amount of base stats
            uint32 base = 0;
            PlayerInfo const* info = GetPlayerInfo(player->getRace(), player->getClass());
            PlayerLevelInfo level1Info = info->levelInfo[0];
            for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
            {
                base += level1Info.stats[i];
            }

            return (uint32)((level * (maxStats - base)) / sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
        }
        else
        {
            PlayerInfo const* info = GetPlayerInfo(player->getRace(), player->getClass());
            PlayerLevelInfo level1Info = info->levelInfo[0];

            PlayerLevelInfo levelCInfo = info->levelInfo[level - 1];
            int total = 0;
            for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
            {
                total += ((int)levelCInfo.stats[i] - (int)level1Info.stats[i]);
            }

            if (level >= DEFAULT_MAX_LEVEL)
            {
                total += 5;
            }

            if (level > DEFAULT_MAX_LEVEL)
            {
                total += (level - DEFAULT_MAX_LEVEL) * 2;
            }

            uint32 byPercent = (uint32)floor(total * GetConfig()->manualAttributesPercent / 100.0f);
            return byPercent / GetConfig()->manualAttributesIncrease * GetConfig()->manualAttributesIncrease;
        }
    }

    uint32 ImmersiveModule::GetUsedStats(Player *player)
    {
        uint32 owner = player->GetObjectGuid().GetRawValue();

        uint32 usedStats = 0;
        for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        {
            usedStats += GetStatsValue(owner, (Stats)i);
        }

        return usedStats;
    }

    uint32 ImmersiveModule::GetStatCost(Player *player, uint8 level, uint32 usedStats)
    {
        if (!level) level = player->GetLevel();
        if (!usedStats) usedStats = GetUsedStats(player);

        uint32 usedLevels = usedStats / 5;
        for (int8 i = level; i >= 1; i--)
        {
            uint32 forLevel = GetTotalStats(player, i);
            if (usedStats > forLevel) 
            {
                usedLevels = i - 1;
                break;
            }
        }

        return GetConfig()->manualAttributesCostMult * (usedLevels * usedLevels + 1);
    }

    uint32 ImmersiveModule::GetStatsValue(uint32 owner, const std::string& type)
    {
        uint32 value = valueCache[owner][type];
        if (!value)
        {
            auto results = CharacterDatabase.PQuery(
                    "select `value` from `custom_immersive_values` where `owner` = '%u' and `type` = '%s'",
                    owner, type.c_str());

            if (results)
            {
                Field* fields = results->Fetch();
                value = fields[0].GetUInt32();
                valueCache[owner][type] = value;
            }
        }

        return value;
    }

    void ImmersiveModule::SetStatsValue(uint32 owner, const std::string& type, uint32 value)
    {
        valueCache[owner][type] = value;
        CharacterDatabase.DirectPExecute("delete from `custom_immersive_values` where `owner` = '%u' and `type` = '%s'",
                owner, type.c_str());

        if (value)
        {
            CharacterDatabase.DirectPExecute(
                    "insert into `custom_immersive_values` (`owner`, `type`, `value`) values ('%u', '%s', '%u')",
                    owner, type.c_str(), value);
        }
    }

    uint32 ImmersiveModule::GetStatsValue(uint32 owner, uint8 type)
    {
        return GetStatsValue(owner, ImmersiveModule::statValues[type]);
    }

    void ImmersiveModule::SetStatsValue(uint32 owner, uint8 type, uint32 value)
    {
        SetStatsValue(owner, ImmersiveModule::statValues[type], value);
    }

    uint32 ImmersiveModule::GetModifierValue(uint32 owner)
    {
        int modifier = GetStatsValue(owner, "modifier");
        if (!modifier) modifier = 100;
        return modifier;
    }

    uint32 ImmersiveModule::GetTotalStatsValue(Player* player, uint8 type)
    {
        if (player)
        {
            const uint32 owner = player->GetObjectGuid().GetRawValue();
            const uint32 modifier = GetModifierValue(owner);
            const PlayerInfo* info = GetPlayerInfo(player->getRace(), player->getClass());
            const uint32 baseStats = info->levelInfo[0].stats[type];
            const uint32 addedStats = GetStatsValue(owner, ImmersiveModule::statValues[type]);
            return (baseStats + addedStats) * modifier / 100;
        }

        return 0;
    }

    void ImmersiveModule::SendSysMessage(Player* player, const std::string& message)
    {
    #ifdef ENABLE_PLAYERBOTS
        if (player->GetPlayerbotAI())
        {
            player->GetPlayerbotAI()->TellPlayerNoFacing(player->GetPlayerbotAI()->GetMaster(), message);
            return;
        }
    #endif

        ChatHandler(player).PSendSysMessage(message.c_str());
    }

    bool ImmersiveAction::CheckSharedPercentReqs(Player* player, Player* bot)
    {
        Group *group = player->GetGroup();
        if (!group) return CheckSharedPercentReqsSingle(player, bot);

        for (GroupReference *gr = group->GetFirstMember(); gr; gr = gr->next())
        {
            Player *member = gr->getSource();
            if (CheckSharedPercentReqsSingle(member, bot)) return true;
        }
        return false;
    }

    bool ImmersiveAction::CheckSharedPercentReqsSingle(Player* player, Player* bot)
    {
        if (!config->enabled) return false;

        if (config->sharedPercentMinLevel && (int)player->GetLevel() < config->sharedPercentMinLevel)
            return false;

        uint8 race1 = player->getRace();
        uint8 cls1 = player->getClass();
        uint8 race2 = bot->getRace();
        uint8 cls2 = bot->getClass();

        if (config->sharedPercentGuildRestiction && player->GetGuildId() != bot->GetGuildId())
            return false;

        if (config->sharedPercentFactionRestiction && (helper::IsAlliance(player) ^ helper::IsAlliance(bot)))
            return false;

        if (config->sharedPercentRaceRestiction == 2)
        {
            if (race1 == RACE_TROLL) race1 = RACE_ORC;
            if (race1 == RACE_DWARF) race1 = RACE_GNOME;

            if (race2 == RACE_TROLL) race2 = RACE_ORC;
            if (race2 == RACE_DWARF) race2 = RACE_GNOME;
        }

        if (config->sharedPercentClassRestiction == 2)
        {
            if (cls1 == CLASS_PALADIN || cls1 == CLASS_SHAMAN) cls1 = CLASS_WARRIOR;
            if (cls1 == CLASS_HUNTER || cls1 == CLASS_ROGUE) cls1 = CLASS_DRUID;
            if (cls1 == CLASS_PRIEST || cls1 == CLASS_WARLOCK) cls1 = CLASS_MAGE;

            if (cls2 == CLASS_PALADIN || cls2 == CLASS_SHAMAN) cls2 = CLASS_WARRIOR;
            if (cls2 == CLASS_HUNTER || cls2 == CLASS_ROGUE) cls2 = CLASS_DRUID;
            if (cls2 == CLASS_PRIEST || cls2 == CLASS_WARLOCK) cls2 = CLASS_MAGE;
        }

        if (config->sharedPercentRaceRestiction && race1 != race2)
            return false;

        if (config->sharedPercentClassRestiction && cls1 != cls2)
            return false;

        return true;
    }

    void ImmersiveModule::RunAction(Player* player, ImmersiveAction* action)
    {
        bool first = true, needMsg = false;
        std::ostringstream out; out << "|cffffff00";
    #ifdef ENABLE_PLAYERBOTS
        for (PlayerBotMap::const_iterator i = player->GetPlayerbotMgr()->GetPlayerBotsBegin(); i != player->GetPlayerbotMgr()->GetPlayerBotsEnd(); ++i)
        {
            Player *bot = i->second;
            if (bot->GetGroup() && bot->GetGroup() == player->GetGroup()) continue;
            if (action->Run(player, bot))
            {
                if (!first)  out << ", "; else first = false;
                out << bot->GetName();
                needMsg = true;
            }
        }
    #endif

        if (!needMsg) return;
        out << "|cffffff00: " << action->GetActionMessage(player);
        SendSysMessage(player, out.str());
    }

    struct AccountCharacter
    {
        uint32 guid;
        uint32 race;
        uint32 clss;
        std::unordered_map<uint32, int32> factions;
    };

    void ImmersiveModule::SyncAccountReputation(Player* player)
    {
        if (GetConfig()->enabled && GetConfig()->accountReputation)
        {
            if (player)
            {
                const auto& characterFactions = player->GetReputationMgr().GetStateList();
                if (!characterFactions.empty())
                {
                    static std::set<uint32> allianceFactions = { 47, 54, 69, 72, 469, 471, 509, 589, 730, 890, 891, 930, 946, 978, 1037, 1068, 1050, 1094, 1126 };
                    static std::set<uint32> hordeFactions = { 67, 68, 76, 81, 510, 530, 729, 889, 892, 911, 922, 941, 947, 1052, 1064, 1067, 1085, 1124 };

                    std::vector<AccountCharacter> accountCharacters;
                    const uint32 currentCharacterGuid = player->GetGUIDLow();
                    const uint32 accountId = player->GetSession()->GetAccountId();
                    auto result = CharacterDatabase.PQuery("SELECT `guid`, `race`, `class` FROM `characters` WHERE `account` = '%u'", accountId);
                    if (result)
                    {
                        do
                        {
                            Field* fields = result->Fetch();
                            const uint32 characterGuid = fields[0].GetUInt32();
                            if (characterGuid != currentCharacterGuid)
                            {
                                auto result2 = CharacterDatabase.PQuery("SELECT `faction`, `standing` FROM `character_reputation` WHERE `guid` = '%u'", characterGuid);
                                if (result2)
                                {
                                    AccountCharacter accountCharacter;
                                    accountCharacter.guid = characterGuid;
                                    accountCharacter.race = fields[1].GetUInt8();
                                    accountCharacter.clss = fields[2].GetUInt8();

                                    do
                                    {
                                        Field* fields2 = result2->Fetch();
                                        const uint32 factionID = fields2[0].GetUInt32();
                                        const uint32 standing = fields2[1].GetInt32();
                                        accountCharacter.factions[factionID] = standing;
                                    } 
                                    while (result2->NextRow());

                                    accountCharacters.push_back(std::move(accountCharacter));
                                }
                            }
                        } 
                        while (result->NextRow());
                    }

                    if (!accountCharacters.empty())
                    {
                        for (const auto& characterFactionIt : characterFactions)
                        {
                            const FactionState& characterFaction = characterFactionIt.second;
                            for (const AccountCharacter& character : accountCharacters)
                            {
                                // Check for alliance/horde only faction
                                const bool isAlliance = IsAlliance(character.race);
                                if ((isAlliance && hordeFactions.find(characterFaction.ID) == hordeFactions.end()) ||
                                    (!isAlliance && allianceFactions.find(characterFaction.ID) == allianceFactions.end()))
                                {
                                    // Only update the reputation if it's lower than the current character reputation amount
                                    auto accountCharacterFactionIt = character.factions.find(characterFaction.ID);
                                    if (accountCharacterFactionIt != character.factions.end())
                                    {
                                        const int32 standing = accountCharacterFactionIt->second;
                                        if (characterFaction.Standing > standing)
                                        {
                                            CharacterDatabase.PExecute("UPDATE `character_reputation` SET `standing` = '%d' WHERE `guid` = '%d' AND `faction` = '%d'", characterFaction.Standing, character.guid, characterFaction.ID);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    int32 ImmersiveModule::CalculateEffectiveChance(int32 difference, const Unit* attacker, const Unit* victim, ImmersiveEffectiveChance type)
    {
        if (!attacker || !victim)
            return difference;

        int32 attackerDelta = CalculateEffectiveChanceDelta(attacker);
        int32 victimDelta = CalculateEffectiveChanceDelta(victim);

        int32 multiplier = 5;
        if (type == IMMERSIVE_EFFECTIVE_CHANCE_SPELL_MISS || type == IMMERSIVE_EFFECTIVE_ATTACK_DISTANCE)
        {
            multiplier = 1;
        }

        switch (type)
        {
            case IMMERSIVE_EFFECTIVE_ATTACK_DISTANCE:
            {
                // victim level - attacker level
                difference += - victimDelta * multiplier 
                              + attackerDelta * multiplier;
                break;
            }

            case IMMERSIVE_EFFECTIVE_CHANCE_MISS:
            case IMMERSIVE_EFFECTIVE_CHANCE_SPELL_MISS:
            {
                // victim defense - attacker offense
                difference += - victimDelta * multiplier
                              + attackerDelta * multiplier;
                break;
            }

            case IMMERSIVE_EFFECTIVE_CHANCE_DODGE:
            case IMMERSIVE_EFFECTIVE_CHANCE_PARRY:
            case IMMERSIVE_EFFECTIVE_CHANCE_BLOCK:
            {
                // attacker defense - victim offense
                difference += - attackerDelta * multiplier
                              + victimDelta * multiplier;
                break;
            }

            case IMMERSIVE_EFFECTIVE_CHANCE_CRIT:
            {
                // attacker offense - victim defense
                difference += - attackerDelta * multiplier
                              + victimDelta * multiplier;
                break;
            }
        }

        return difference;
    }

    uint32 ImmersiveModule::CalculateEffectiveChanceDelta(const Unit* unit)
    {
        if (unit->IsPlayer())
        {
    #ifdef ENABLE_PLAYERBOTS
            // Random bots stats should not be affected
            if (IsRandomBot(unit))
                return 0;
    #endif
            const uint32 modifier = GetModifierValue(unit->GetObjectGuid().GetCounter());
            return unit->GetLevel() * (100 - modifier) / 100;
        }

        return 0;
    }

    std::map<uint8,float> scale;
    void ImmersiveModule::CheckScaleChange(Player* player)
    {
        if (!GetConfig()->enabled) 
            return;

        if (!GetConfig()->scaleModifierWorkaround) 
            return;

        uint8 race = player->getRace();
        if (scale.empty())
        {
            scale[RACE_TROLL] = 0.85f;
            scale[RACE_NIGHTELF] = 0.85f;
            scale[RACE_ORC] = 0.95f;
            scale[RACE_TAUREN] = 0.95f;
            scale[RACE_HUMAN] = 1.0f;
            scale[RACE_DWARF] = 0.85f;
            scale[RACE_GNOME] = 1.15f;
        }

        if (scale.find(race) != scale.end())
        {
            player->SetObjectScale(scale[race] - (player->getGender() == GENDER_MALE ? 0.1f : 0));
        }
    }

}