#include "immersivepch.h"

#include "SpellEffectDefines.h"
#include "GossipDef.h"
#include "Language.h"
#include "World.h"

#ifdef ENABLE_MANGOSBOTS
#include "PlayerbotAIConfig.h"
#include "PlayerbotAI.h"
#include "ChatHelper.h"
#endif

using namespace immersive;

std::map<Stats, std::string> Immersive::statValues;

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
        out << gold <<  "g";
        space = true;
    }

    if (silver > 0 && gold < 50)
    {
        if (space) out << " ";
        out << silver <<  "s";
        space = true;
    }

    if (copper > 0 && gold < 10)
    {
        if (space) out << " ";
        out << copper <<  "c";
    }

    return out.str();
}

Immersive::Immersive()
{
    statValues[STAT_STRENGTH] = "Strength";
    statValues[STAT_AGILITY] = "Agility";
    statValues[STAT_STAMINA] = "Stamina";
    statValues[STAT_INTELLECT] = "Intellect";
    statValues[STAT_SPIRIT] = "Spirit";
}

PlayerInfo extraPlayerInfo[MAX_RACES][MAX_CLASSES];

PlayerInfo const* Immersive::GetPlayerInfo(uint32 race, uint32 class_)
{

#if defined (MANGOSBOT_ONE) || defined (MANGOSBOT_TWO) || MAX_EXPANSION >= 1
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

void Immersive::GetPlayerLevelInfo(Player *player, PlayerLevelInfo* info)
{
    if (!sImmersiveConfig.enabled) 
        return;

    if (!sImmersiveConfig.manualAttributes) 
        return;

#ifdef ENABLE_MANGOSBOTS
    // Don't use custom stats on random bots
    uint32 account = sObjectMgr.GetPlayerAccountIdByGUID(player->GetObjectGuid());
    if (sPlayerbotAIConfig.IsInRandomAccountList(account))
        return;
#endif

    PlayerInfo const* playerInfo = GetPlayerInfo(player->getRace(), player->getClass());
    *info = playerInfo->levelInfo[0];

    uint32 owner = player->GetObjectGuid().GetRawValue();
    int modifier = GetModifierValue(owner);
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        info->stats[i] += GetStatsValue(owner, (Stats)i) * modifier / 100;
    }
}

void Immersive::OnGossipSelect(Player *player, WorldObject* source, uint32 gossipListId, GossipMenuItemData *menuData)
{
    bool closeGossipWindow = false;
    if (!sImmersiveConfig.enabled)
    {
        SendMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_DISABLED, player->GetSession()->GetSessionDbLocaleIndex()));
        closeGossipWindow = true;
    }
    else
    {
        switch (menuData->m_gAction_poi)
        {
            // Help
            case 0: 
            {
                PrintHelp(player, true, true);
                break;
            }

            // Increase stats
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            {
                IncreaseStat(player, menuData->m_gAction_poi - 1);
                break;
            }

            // Reset stats
            case 6:
            {
                ResetStats(player);
                break;
            }

            // Reduce stat modifier
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            {
                ChangeModifier(player, menuData->m_gAction_poi - 11);
                break;
            }

            default: break;
        }
    }

    if (closeGossipWindow)
    {
        player->GetPlayerMenu()->CloseGossip();
    }
    else
    {
        player->PrepareGossipMenu(source, 60001);
        player->SendPreparedGossip(source);
    }
}

float Immersive::GetFallDamage(Player* player, float zdist, float defaultVal)
{
    if (!sImmersiveConfig.enabled || !player)
        return defaultVal;

    // Don't get extra fall damage on battlegrounds
    if (player->InBattleGround())
        return defaultVal;

#if defined (MANGOSBOT_ONE) || defined (MANGOSBOT_TWO) || MAX_EXPANSION >= 1
    // Don't get extra fall damage on arenas
    if (player->InArena())
        return defaultVal;
#endif

#ifdef ENABLE_MANGOSBOTS
    // Don't apply extra fall damage on bots
    if (!player->isRealPlayer())
    {
        return defaultVal;
    }
#endif

    return 0.0055f * zdist * zdist * sImmersiveConfig.fallDamageMultiplier;
}

void Immersive::OnDeath(Player *player)
{
    if (!sImmersiveConfig.enabled || !player)
        return;

#ifdef ENABLE_MANGOSBOTS
    // Don't lose stats on bots
    if (!player->isRealPlayer())
        return;
#endif

    // Don't get extra fall damage on battlegrounds
    if (player->InBattleGround())
        return;

#if defined (MANGOSBOT_ONE) || defined (MANGOSBOT_TWO) || MAX_EXPANSION >= 1
    // Don't get extra fall damage on arenas
    if (player->InArena())
        return;
#endif

    const uint8 lossPerDeath = sImmersiveConfig.attributeLossPerDeath;
    const uint32 usedStats = GetUsedStats(player);
    if(lossPerDeath > 0 && usedStats > 0)
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
            if(statValue > 0)
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
            if(value > 0)
            {
                if (!first) out << ", "; else first = false;
                const uint32 langStat = LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH + i;
                out << "|cffffa0a0-" << value << "|cffffff00 " << sObjectMgr.GetMangosString(langStat, player->GetSession()->GetSessionDbLocaleIndex());
            }
        }

        SendMessage(player, FormatString(
                    sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_LOST, player->GetSession()->GetSessionDbLocaleIndex()),
                    out.str().c_str()));

        player->InitStatsForLevel(true);
        player->UpdateAllStats();
    }
}

std::string percent(Player *player)
{
#ifdef ENABLE_MANGOSBOTS
    return player->GetPlayerbotAI() ? "%" : "%%";
#else
    return "%%";
#endif
}

void Immersive::PrintHelp(Player *player, bool detailed, bool help)
{
    uint32 usedStats = GetUsedStats(player);
    uint32 totalStats = GetTotalStats(player);
    uint32 purchaseCost = GetStatCost(player) * sImmersiveConfig.manualAttributesIncrease;

    SendMessage(player, FormatString(
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_AVAILABLE, player->GetSession()->GetSessionDbLocaleIndex()),
                (totalStats > usedStats ? totalStats - usedStats : 0),
                formatMoney(purchaseCost).c_str()));

    if (detailed)
    {
        PrintUsedStats(player);
    }

    if (help)
    {
        PrintSuggestedStats(player);
    }
}

void Immersive::PrintUsedStats(Player* player)
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
        out << " " << FormatString(sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MODIFIER, player->GetSession()->GetSessionDbLocaleIndex()), modifierStr.str().c_str());
    }

    SendMessage(player, FormatString(
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_ASSIGNED, player->GetSession()->GetSessionDbLocaleIndex()),
                out.str().c_str()));
}

void Immersive::PrintSuggestedStats(Player* player)
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
        value = (uint32)floor(value * sImmersiveConfig.manualAttributesPercent / 100.0f);
        if (!value) continue;
        if (!first) out << ", "; else first = false;
        const uint32 langStat = LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH + i;
        out << "|cff00ff00+" << value << "|cffffff00 " << sObjectMgr.GetMangosString(langStat, player->GetSession()->GetSessionDbLocaleIndex());
        used = true;
    }

    if (used)
    {
        SendMessage(player, FormatString(
                    sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_SUGGESTED, player->GetSession()->GetSessionDbLocaleIndex()),
                    out.str().c_str()));
    }
}

void Immersive::ChangeModifier(Player *player, uint32 type)
{
    if (!sImmersiveConfig.enabled) 
        return;

    uint32 owner = player->GetObjectGuid().GetRawValue();
    uint32 value = type * 10;
    SetValue(owner, "modifier", value);

    std::ostringstream modifierStr; modifierStr << value << percent(player);
    if (!value || value == 100)
    {
        modifierStr << " " << sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MOD_DISABLED, player->GetSession()->GetSessionDbLocaleIndex());
    }

    SendMessage(player, FormatString(
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MOD_CHANGED, player->GetSession()->GetSessionDbLocaleIndex()),
                modifierStr.str().c_str()));

    player->InitStatsForLevel(true);
    player->UpdateAllStats();
}

void Immersive::IncreaseStat(Player *player, uint32 type)
{
    if (!sImmersiveConfig.enabled)
    {
        SendMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_DISABLED, player->GetSession()->GetSessionDbLocaleIndex()));
        return;
    }

    uint32 owner = player->GetObjectGuid().GetRawValue();
    uint32 usedStats = GetUsedStats(player);
    uint32 totalStats = GetTotalStats(player);
    uint32 cost = GetStatCost(player);
    uint32 attributePointsAvailable = (totalStats > usedStats ? totalStats - usedStats : 0);
    uint32 statIncrease = std::min(sImmersiveConfig.manualAttributesIncrease, attributePointsAvailable);
    uint32 purchaseCost = cost * statIncrease;

    if (usedStats >= totalStats)
    {
        SendMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MISSING, player->GetSession()->GetSessionDbLocaleIndex()));
        return;
    }

    if (player->GetMoney() < purchaseCost)
    {
        SendMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_MISSING_GOLD, player->GetSession()->GetSessionDbLocaleIndex()));
        return;
    }

    uint32 value = GetStatsValue(owner, (Stats)type);
    SetStatsValue(owner, (Stats)type, value + statIncrease);

    usedStats = GetUsedStats(player);
    totalStats = GetTotalStats(player);
    attributePointsAvailable = (totalStats > usedStats ? totalStats - usedStats : 0);

    SendMessage(player, FormatString(
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_GAINED, player->GetSession()->GetSessionDbLocaleIndex()),
                statIncrease,
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_STRENGTH + type, player->GetSession()->GetSessionDbLocaleIndex()),
                attributePointsAvailable,
                formatMoney(purchaseCost).c_str()));

    PrintUsedStats(player);

    player->InitStatsForLevel(true);
    player->UpdateAllStats();
    player->ModifyMoney(-(int32)purchaseCost);
    player->SaveGoldToDB();
}

void Immersive::ResetStats(Player *player)
{
    if (!sImmersiveConfig.enabled)
    {
        SendMessage(player, sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_DISABLED, player->GetSession()->GetSessionDbLocaleIndex()));
        return;
    }

    uint32 owner = player->GetObjectGuid().GetRawValue();

    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        SetValue(owner, statValues[(Stats)i], 0);
    }

    uint32 usedStats = GetUsedStats(player);
    uint32 totalStats = GetTotalStats(player);

    SendMessage(player, FormatString(
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_RESET, player->GetSession()->GetSessionDbLocaleIndex()),
                (totalStats > usedStats ? totalStats - usedStats : 0)));

    player->InitStatsForLevel(true);
    player->UpdateAllStats();
}

uint32 Immersive::GetTotalStats(Player *player, uint8 level)
{
    constexpr uint8 maxLvl = 60;
    if (!level) 
    {
        level = player->GetLevel();
    }

    const uint32 maxStats = sImmersiveConfig.manualAttributesMaxPoints;
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

        return (uint32)((level * (maxStats - base)) / maxLvl);
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

        if(level >= maxLvl)
        {
            total += 5;
        }

        uint32 byPercent = (uint32)floor(total * sImmersiveConfig.manualAttributesPercent / 100.0f);
        return byPercent / sImmersiveConfig.manualAttributesIncrease * sImmersiveConfig.manualAttributesIncrease;
    }
}

uint32 Immersive::GetUsedStats(Player *player)
{
    uint32 owner = player->GetObjectGuid().GetRawValue();

    uint32 usedStats = 0;
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        usedStats += GetStatsValue(owner, (Stats)i);
    }

    return usedStats;
}

uint32 Immersive::GetStatCost(Player *player, uint8 level, uint32 usedStats)
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

    return sImmersiveConfig.manualAttributesCostMult * (usedLevels * usedLevels + 1);
}

uint32 Immersive::GetValue(uint32 owner, const std::string& type)
{
    uint32 value = valueCache[owner][type];

    if (!value)
    {
        auto results = CharacterDatabase.PQuery(
                "select `value` from immersive_values where owner = '%u' and `type` = '%s'",
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

void Immersive::SetValue(uint32 owner, const std::string& type, uint32 value)
{
    valueCache[owner][type] = value;
    CharacterDatabase.DirectPExecute("delete from immersive_values where owner = '%u' and `type` = '%s'",
            owner, type.c_str());

    if (value)
    {
        CharacterDatabase.DirectPExecute(
                "insert into immersive_values (owner, `type`, `value`) values ('%u', '%s', '%u')",
                owner, type.c_str(), value);
    }
}

uint32 Immersive::GetStatsValue(uint32 owner, Stats type)
{
    return GetValue(owner, Immersive::statValues[type]);
}

void Immersive::SetStatsValue(uint32 owner, Stats type, uint32 value)
{
    SetValue(owner, Immersive::statValues[type], value);
}

uint32 Immersive::GetModifierValue(uint32 owner)
{
    int modifier = GetValue(owner, "modifier");
    if (!modifier) modifier = 100;
    return modifier;
}

void Immersive::SendMessage(Player *player, const std::string& message)
{
#ifdef ENABLE_MANGOSBOTS
    if (player->GetPlayerbotAI())
    {
        player->GetPlayerbotAI()->TellPlayerNoFacing(player->GetPlayerbotAI()->GetMaster(), message);
        return;
    }
#endif

    ChatHandler(player).PSendSysMessage(message.c_str());
}

std::string Immersive::FormatString(const char* format, ...)
{
    va_list ap;
    char out[2048];
    va_start(ap, format);
    vsnprintf(out, 2048, format, ap);
    va_end(ap);
    return std::string(out);
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

bool PlayerIsAlliance(Player* player)
{
    const uint8 race = player->getRace();
    return race == RACE_HUMAN || 
           race == RACE_DWARF || 
           race == RACE_NIGHTELF ||
#if defined (MANGOSBOT_ONE) || defined (MANGOSBOT_TWO) || MAX_EXPANSION >= 1
           race == RACE_DRAENEI ||
#endif
           race == RACE_GNOME;
}

bool ImmersiveAction::CheckSharedPercentReqsSingle(Player* player, Player* bot)
{
    if (!sImmersiveConfig.enabled) return false;

    if (sImmersiveConfig.sharedPercentMinLevel && (int)player->GetLevel() < sImmersiveConfig.sharedPercentMinLevel)
        return false;

    uint8 race1 = player->getRace();
    uint8 cls1 = player->getClass();
    uint8 race2 = bot->getRace();
    uint8 cls2 = bot->getClass();

    if (sImmersiveConfig.sharedPercentGuildRestiction && player->GetGuildId() != bot->GetGuildId())
        return false;

    if (sImmersiveConfig.sharedPercentFactionRestiction && (PlayerIsAlliance(player) ^ PlayerIsAlliance(bot)))
        return false;

    if (sImmersiveConfig.sharedPercentRaceRestiction == 2)
    {
        if (race1 == RACE_TROLL) race1 = RACE_ORC;
        if (race1 == RACE_DWARF) race1 = RACE_GNOME;

        if (race2 == RACE_TROLL) race2 = RACE_ORC;
        if (race2 == RACE_DWARF) race2 = RACE_GNOME;
    }

    if (sImmersiveConfig.sharedPercentClassRestiction == 2)
    {
        if (cls1 == CLASS_PALADIN || cls1 == CLASS_SHAMAN) cls1 = CLASS_WARRIOR;
        if (cls1 == CLASS_HUNTER || cls1 == CLASS_ROGUE) cls1 = CLASS_DRUID;
        if (cls1 == CLASS_PRIEST || cls1 == CLASS_WARLOCK) cls1 = CLASS_MAGE;

        if (cls2 == CLASS_PALADIN || cls2 == CLASS_SHAMAN) cls2 = CLASS_WARRIOR;
        if (cls2 == CLASS_HUNTER || cls2 == CLASS_ROGUE) cls2 = CLASS_DRUID;
        if (cls2 == CLASS_PRIEST || cls2 == CLASS_WARLOCK) cls2 = CLASS_MAGE;
    }

    if (sImmersiveConfig.sharedPercentRaceRestiction && race1 != race2)
        return false;

    if (sImmersiveConfig.sharedPercentClassRestiction && cls1 != cls2)
        return false;

    return true;
}

void Immersive::RunAction(Player* player, ImmersiveAction* action)
{
    bool first = true, needMsg = false;
    std::ostringstream out; out << "|cffffff00";
#ifdef ENABLE_MANGOSBOTS
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
    out << "|cffffff00: " << action->GetMessage(player);
    SendMessage(player, out.str());
}

uint32 ApplyRandomPercent(uint32 value)
{
    if (!sImmersiveConfig.enabled || !sImmersiveConfig.sharedRandomPercent) return value;
    float percent = (float) urand(0, sImmersiveConfig.sharedRandomPercent) - (float)sImmersiveConfig.sharedRandomPercent / 2;
    return value + (uint32) (value * percent / 100.0f);
}

#ifdef ENABLE_MANGOSBOTS
class OnGiveXPAction : public ImmersiveAction
{
public:
    OnGiveXPAction(int32 value) : ImmersiveAction(), value(value) {}

    bool Run(Player* player, Player* bot) override
    {
        if ((int)player->GetLevel() - (int)bot->GetLevel() < (int)sImmersiveConfig.sharedXpPercentLevelDiff)
        {
            return false;
        }

        if (!CheckSharedPercentReqs(player, bot))
        {
            return false;
        }

        bot->GiveXP(ApplyRandomPercent(value), NULL);

        Pet *pet = bot->GetPet();
        if (pet && pet->getPetType() == HUNTER_PET)
        {
            pet->GivePetXP(ApplyRandomPercent(value));
        }

        return true;
    }

    std::string GetMessage(Player* player) override
    {
        return Immersive::FormatString(
               sObjectMgr.GetMangosString(LANG_IMMERSIVE_EXP_GAINED, player->GetSession()->GetSessionDbLocaleIndex()),
               value);
    }

private:
    int32 value;
};
#endif

void Immersive::OnGiveXP(Player *player, uint32 xp, Unit* victim)
{
#ifdef ENABLE_MANGOSBOTS
    if (!sImmersiveConfig.enabled) 
        return;

    if (!player->GetPlayerbotMgr())
        return;

    if (sImmersiveConfig.sharedXpPercent < 0.01f) 
        return;

    uint32 bonus_xp = xp + (victim ? player->GetXPRestBonus(xp) : 0);
    uint32 botXp = (uint32) (bonus_xp * sImmersiveConfig.sharedXpPercent / 100.0f);
    if (botXp < 1) 
    {
        return;
    }

    OnGiveXPAction action(botXp);
    RunAction(player, &action);
#endif
}

void Immersive::OnGiveLevel(Player* player)
{
    if (!sImmersiveConfig.enabled)
        return;

    if (!sImmersiveConfig.manualAttributes)
        return;

    const uint32 usedStats = GetUsedStats(player);
    const uint32 totalStats = GetTotalStats(player);
    const uint32 availablePoints = (totalStats > usedStats) ? totalStats - usedStats : 0;
    if (availablePoints > 0)
    {
        SendMessage(player, FormatString(
                    sObjectMgr.GetMangosString(LANG_IMMERSIVE_MANUAL_ATTR_POINTS_ADDED, player->GetSession()->GetSessionDbLocaleIndex()),
                    availablePoints));
    }
}

#ifdef ENABLE_MANGOSBOTS
class OnGiveMoneyAction : public ImmersiveAction
{
public:
    OnGiveMoneyAction(int32 value) : ImmersiveAction(), value(value) {}

    bool Run(Player* player, Player* bot) override
    {
        if ((int)player->GetLevel() - (int)bot->GetLevel() < (int)sImmersiveConfig.sharedXpPercentLevelDiff)
            return false;

        if (!CheckSharedPercentReqs(player, bot))
            return false;

        bot->ModifyMoney(ApplyRandomPercent(value));
        return true;
    }

    virtual std::string GetMessage(Player* player)
    {
            return Immersive::FormatString(
                sObjectMgr.GetMangosString(LANG_IMMERSIVE_MONEY_GAINED, player->GetSession()->GetSessionDbLocaleIndex()),
                ai::ChatHelper::formatMoney(value).c_str()
            );
    }

private:
    int32 value;
};
#endif

void Immersive::OnModifyMoney(Player *player, int32 delta)
{
#ifdef ENABLE_MANGOSBOTS
    if (!sImmersiveConfig.enabled) 
        return;

    if (!player->GetPlayerbotMgr())
        return;

    if (delta < 1) 
        return;

    if (sImmersiveConfig.sharedMoneyPercent < 0.01f)
        return;

    int32 botMoney = (int32) (delta * sImmersiveConfig.sharedMoneyPercent / 100.0f);
    if (botMoney < 1) return;

    OnGiveMoneyAction action(botMoney);
    RunAction(player, &action);
#endif
}

#ifdef ENABLE_MANGOSBOTS
class OnReputationChangeAction : public ImmersiveAction
{
public:
    OnReputationChangeAction(FactionEntry const* factionEntry, int32 value) : ImmersiveAction(), factionEntry(factionEntry), value(value) {}

    bool Run(Player* player, Player* bot) override
    {
        if (!CheckSharedPercentReqs(player, bot))
        {
            return false;
        }
        else
        {
            bot->GetReputationMgr().ModifyReputation(factionEntry, ApplyRandomPercent(value));
            return true;
        }
    }

    string GetMessage(Player* player) override
    {
        return Immersive::FormatString(
               sObjectMgr.GetMangosString(LANG_IMMERSIVE_REPUTATION_GAINED, player->GetSession()->GetSessionDbLocaleIndex()),
               value);
    }

private:
    FactionEntry const* factionEntry;
    int32 value;
};
#endif

void Immersive::OnReputationChange(Player* player, FactionEntry const* factionEntry, int32& standing, bool incremental)
{
#ifdef ENABLE_MANGOSBOTS
    if (!sImmersiveConfig.enabled)
        return;

    if (!player->GetPlayerbotMgr())
        return;

    if (!incremental)
        return;

    if (sImmersiveConfig.sharedRepPercent < 0.01f)
        return;

    int32 value = (uint32) (standing * sImmersiveConfig.sharedRepPercent / 100.0f);
    if (value < 1) return;

    OnReputationChangeAction action(factionEntry, value);
    RunAction(player, &action);
#endif
}

#ifdef ENABLE_MANGOSBOTS
class OnRewardQuestAction : public ImmersiveAction
{
public:
    OnRewardQuestAction(Quest const* quest) : ImmersiveAction(), quest(quest) {}

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

    std::string GetMessage(Player* player) override
    {
        return Immersive::FormatString(
               sObjectMgr.GetMangosString(LANG_IMMERSIVE_QUEST_COMPLETED, player->GetSession()->GetSessionDbLocaleIndex()),
               quest->GetTitle().c_str());
    }

private:
    Quest const* quest;
};
#endif

void Immersive::OnRewardQuest(Player* player, Quest const* quest)
{
#ifdef ENABLE_MANGOSBOTS
    if (!sImmersiveConfig.enabled)
        return;

    if (!player->GetPlayerbotMgr())
        return;

    if (!sImmersiveConfig.sharedQuests)
        return;

    if (!quest || quest->IsRepeatable()) 
        return;

    OnRewardQuestAction action(quest);
    RunAction(player, &action);
#endif
}

bool Immersive::OnFishing(Player* player, bool success)
{
    if (!sImmersiveConfig.enabled || !success)
        return success;

    if (!sImmersiveConfig.fishingBaubles)
        return success;

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
            SpellEntry const *entry = sSpellTemplate.LookupEntry<SpellEntry>(spellId);
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

int32 Immersive::CalculateEffectiveChance(int32 difference, const Unit* attacker, const Unit* victim, ImmersiveEffectiveChance type)
{
    if (!sImmersiveConfig.enabled) 
        return difference;

    if (!sImmersiveConfig.manualAttributes)
        return difference;

    int32 attackerDelta = CalculateEffectiveChanceDelta(attacker);
    int32 victimDelta = CalculateEffectiveChanceDelta(victim);

    int32 multiplier = 5;
    if (type == IMMERSIVE_EFFECTIVE_CHANCE_SPELL_MISS || type == IMMERSIVE_EFFECTIVE_ATTACK_DISTANCE)
        multiplier = 1;

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

uint32 Immersive::CalculateEffectiveChanceDelta(const Unit* unit)
{
    if (unit->GetObjectGuid().IsPlayer())
    {
        int modifier = GetModifierValue(unit->GetObjectGuid().GetCounter());
#ifdef ENABLE_MANGOSBOTS
        if (sPlayerbotAIConfig.IsInRandomAccountList(sObjectMgr.GetPlayerAccountIdByGUID(unit->GetObjectGuid())))
            return 0;
#endif
        return unit->GetLevel() * (100 - modifier) / 100;
    }

    return 0;
}

void Immersive::OnGossipHello(Player* player, Creature* creature)
{
#if defined (MANGOSBOT_ONE) || MAX_EXPANSION == 1
    if (player && creature)
    {
        GossipMenu& menu = player->GetPlayerMenu()->GetGossipMenu();
        uint32 textId = player->GetGossipTextId(menu.GetMenuId(), creature);
        GossipText const* text = sObjectMgr.GetGossipText(textId);
        if (text)
        {
            for (int i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; i++)
            {
                std::string text0 = text->Options[i].Text_0;
                if (!text0.empty()) creature->MonsterSay(text0.c_str(), 0, player);
                std::string text1 = text->Options[i].Text_1;
                if (!text1.empty() && text0 != text1) creature->MonsterSay(text1.c_str(), 0, player);
            }
        }
    }

#endif
}

std::map<uint8,float> scale;
void Immersive::CheckScaleChange(Player* player)
{
    if (!sImmersiveConfig.enabled) 
        return;

    if (!sImmersiveConfig.scaleModifierWorkaround) 
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

void Immersive::Update(uint32 elapsed)
{
    if (!sImmersiveConfig.enabled)
        return;

    if (updateDelay > elapsed)
    {
        updateDelay -= elapsed;
        return;
    }
    
    updateDelay = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);
    
    if (sImmersiveConfig.disableOfflineRespawn)
    {
        SetValue(0, "last_ping", sWorld.GetGameTime());
    }
}

void Immersive::Init()
{
    if (!sImmersiveConfig.enabled)
        return;

    updateDelay = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);
    
    if (sImmersiveConfig.disableOfflineRespawn)
    {
        DisableOfflineRespawn();
    }
}

void Immersive::DisableOfflineRespawn()
{
    uint32 lastPing = GetValue(0, "last_ping");
    if (!lastPing) return;
    
    uint32 offlineTime = sWorld.GetGameTime() - lastPing; 
    sLog.outString("Prolonging respawn time: +%u sec", offlineTime);
    
    CharacterDatabase.BeginTransaction();
    
    CharacterDatabase.DirectPExecute("update `creature_respawn` set `respawntime` = `respawntime` + '%u'", offlineTime);
    CharacterDatabase.DirectPExecute("update `instance_reset` set `resettime` = `resettime` + '%u'", offlineTime);
    CharacterDatabase.DirectPExecute("update `instance` set `resettime` = `resettime` + '%u'", offlineTime);
    CharacterDatabase.DirectPExecute("update `gameobject_respawn` set `respawntime` = `respawntime` + '%u'", offlineTime);
    SetValue(0, "last_ping", sWorld.GetGameTime());
    
    CharacterDatabase.CommitTransaction();
}

float Immersive::GetFallThreshold()
{
    return sImmersiveConfig.enabled ? 4.57f : 14.57f;
}

INSTANTIATE_SINGLETON_1( immersive::Immersive );
