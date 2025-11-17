/*
 * Self Craft Upgrade Module for AzerothCore
 * 
 * This module allows self-crafted players to gain additional stats on equipped items
 * by killing specific boss NPCs. The stat upgrade is calculated using the formula:
 * additional_stat = Old_Stat × ((1.0109)^(Boss_Max_iLevel - Item_iLevel)) - Old_Stat
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ItemTemplate.h"
#include <cmath>
#include <unordered_map>

// Configuration values
bool SelfCraftUpgradeEnabled = true;
bool AnnounceOnKill = true;
float StatMultiplier = 1.0109f;
std::unordered_map<uint32, uint32> BossMaxItemLevels;

// Store stat bonuses per player per item slot
struct ItemStatBonus
{
    std::unordered_map<uint32, int32> statBonuses; // statType -> bonus amount
};

std::unordered_map<ObjectGuid, std::unordered_map<uint8, ItemStatBonus>> playerItemBonuses;

// Helper functions
namespace SelfCraftUpgradeHelper
{
    // Calculate the additional stat based on the formula
    int32 CalculateAdditionalStat(int32 oldStat, uint32 bossMaxILevel, uint32 itemILevel)
    {
        if (itemILevel >= bossMaxILevel || oldStat <= 0)
            return 0;

        int32 levelDiff = static_cast<int32>(bossMaxILevel) - static_cast<int32>(itemILevel);
        float multiplier = std::pow(StatMultiplier, levelDiff);
        int32 newStat = static_cast<int32>(oldStat * multiplier);
        return newStat - oldStat;
    }

    // Remove bonuses from a specific slot
    void RemoveItemBonuses(Player* player, uint8 slot)
    {
        if (!player)
            return;

        ObjectGuid playerGuid = player->GetGUID();
        auto playerIt = playerItemBonuses.find(playerGuid);
        if (playerIt == playerItemBonuses.end())
            return;

        auto slotIt = playerIt->second.find(slot);
        if (slotIt == playerIt->second.end())
            return;

        // Remove all stat bonuses for this slot
        for (const auto& [statType, bonus] : slotIt->second.statBonuses)
        {
            switch (statType)
            {
                case ITEM_MOD_AGILITY:
                    player->HandleStatModifier(UNIT_MOD_STAT_AGILITY, TOTAL_VALUE, float(bonus), false);
                    break;
                case ITEM_MOD_STRENGTH:
                    player->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, TOTAL_VALUE, float(bonus), false);
                    break;
                case ITEM_MOD_INTELLECT:
                    player->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, TOTAL_VALUE, float(bonus), false);
                    break;
                case ITEM_MOD_SPIRIT:
                    player->HandleStatModifier(UNIT_MOD_STAT_SPIRIT, TOTAL_VALUE, float(bonus), false);
                    break;
                case ITEM_MOD_STAMINA:
                    player->HandleStatModifier(UNIT_MOD_STAT_STAMINA, TOTAL_VALUE, float(bonus), false);
                    break;
            }
        }

        playerIt->second.erase(slotIt);
        player->UpdateAllStats();
    }

    // Calculate and apply stat upgrades to player based on equipped item
    void ApplyItemUpgrades(Player* player, Item* item, uint8 slot)
    {
        if (!item || !player)
            return;

        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (!itemTemplate)
            return;

        // Only upgrade epic items (quality 4)
        if (itemTemplate->Quality != ITEM_QUALITY_EPIC)
            return;

        uint32 itemILevel = itemTemplate->ItemLevel;
        
        // Get player's highest boss kill max ilevel
        QueryResult result = CharacterDatabase.Query("SELECT MAX(max_ilevel) FROM selfcraft_upgrade_kills WHERE guid = {}", player->GetGUID().GetCounter());
        
        if (!result)
            return;

        Field* fields = result->Fetch();
        uint32 maxBossILevel = fields[0].Get<uint32>();

        if (maxBossILevel <= itemILevel)
            return;

        // Store bonuses for this slot
        ObjectGuid playerGuid = player->GetGUID();
        ItemStatBonus& slotBonuses = playerItemBonuses[playerGuid][slot];
        slotBonuses.statBonuses.clear();

        // Calculate and apply stat bonuses to player for each stat on the item
        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        {
            if (itemTemplate->ItemStat[i].ItemStatType == 0)
                continue;

            int32 baseStat = itemTemplate->ItemStat[i].ItemStatValue;
            if (baseStat <= 0)
                continue;

            int32 additionalStat = CalculateAdditionalStat(baseStat, maxBossILevel, itemILevel);
            
            if (additionalStat > 0)
            {
                uint32 statType = itemTemplate->ItemStat[i].ItemStatType;
                slotBonuses.statBonuses[statType] = additionalStat;
                
                // Apply the bonus directly to player stats
                switch (statType)
                {
                    case ITEM_MOD_AGILITY:
                        player->HandleStatModifier(UNIT_MOD_STAT_AGILITY, TOTAL_VALUE, float(additionalStat), true);
                        break;
                    case ITEM_MOD_STRENGTH:
                        player->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, TOTAL_VALUE, float(additionalStat), true);
                        break;
                    case ITEM_MOD_INTELLECT:
                        player->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, TOTAL_VALUE, float(additionalStat), true);
                        break;
                    case ITEM_MOD_SPIRIT:
                        player->HandleStatModifier(UNIT_MOD_STAT_SPIRIT, TOTAL_VALUE, float(additionalStat), true);
                        break;
                    case ITEM_MOD_STAMINA:
                        player->HandleStatModifier(UNIT_MOD_STAT_STAMINA, TOTAL_VALUE, float(additionalStat), true);
                        break;
                }
            }
        }
        
        // Update player stats
        player->UpdateAllStats();
    }
}

class SelfCraftUpgradeWorldScript : public WorldScript
{
public:
    SelfCraftUpgradeWorldScript() : WorldScript("SelfCraftUpgradeWorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        SelfCraftUpgradeEnabled = sConfigMgr->GetOption<bool>("SelfCraftUpgrade.Enable", true);
        AnnounceOnKill = sConfigMgr->GetOption<bool>("SelfCraftUpgrade.AnnounceOnKill", true);
        StatMultiplier = sConfigMgr->GetOption<float>("SelfCraftUpgrade.StatMultiplier", 1.0109f);

        // Load boss configurations
        BossMaxItemLevels.clear();
        
        // Load all boss entries from config
        std::vector<uint32> bossIds = {11502, 10184, 11583, 15727, 15990}; // Default bosses
        
        for (uint32 bossId : bossIds)
        {
            std::string configKey = "SelfCraftUpgrade.Boss." + std::to_string(bossId);
            uint32 maxILevel = sConfigMgr->GetOption<uint32>(configKey, 0);
            if (maxILevel > 0)
            {
                BossMaxItemLevels[bossId] = maxILevel;
            }
        }
    }
};

class SelfCraftUpgradePlayerScript : public PlayerScript
{
public:
    SelfCraftUpgradePlayerScript() : PlayerScript("SelfCraftUpgradePlayerScript") { }

    void OnLogin(Player* player) override
    {
        if (!SelfCraftUpgradeEnabled)
            return;

        // Clear any old bonuses
        playerItemBonuses.erase(player->GetGUID());

        // Refresh all equipped items
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (item)
            {
                SelfCraftUpgradeHelper::ApplyItemUpgrades(player, item, slot);
            }
        }
    }

    void OnLogout(Player* player) override
    {
        if (!SelfCraftUpgradeEnabled)
            return;

        // Clean up player data
        playerItemBonuses.erase(player->GetGUID());
    }

    void OnEquip(Player* player, Item* item, uint8 /*bag*/, uint8 slot, bool /*update*/) override
    {
        if (!SelfCraftUpgradeEnabled)
            return;

        // Remove old bonuses from this slot first
        SelfCraftUpgradeHelper::RemoveItemBonuses(player, slot);

        // Apply new bonuses
        SelfCraftUpgradeHelper::ApplyItemUpgrades(player, item, slot);
    }

    void OnUnequip(Player* player, Item* /*item*/, uint8 /*bag*/, uint8 slot, bool /*update*/) override
    {
        if (!SelfCraftUpgradeEnabled)
            return;

        // Remove bonuses when item is unequipped
        SelfCraftUpgradeHelper::RemoveItemBonuses(player, slot);
    }
};

class SelfCraftUpgradeCreatureScript : public CreatureScript
{
public:
    SelfCraftUpgradeCreatureScript() : CreatureScript("SelfCraftUpgradeCreatureScript") { }

    struct SelfCraftUpgradeCreatureAI : public ScriptedAI
    {
        SelfCraftUpgradeCreatureAI(Creature* creature) : ScriptedAI(creature) { }

        void JustDied(Unit* killer) override
        {
            if (!SelfCraftUpgradeEnabled)
                return;

            Player* player = killer->ToPlayer();
            if (!player)
            {
                // Check if killer is a pet or controlled unit
                if (killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                    player = killer->GetCharmerOrOwnerPlayerOrPlayerItself();
            }

            if (!player)
                return;

            uint32 creatureEntry = me->GetEntry();
            
            // Check if this creature is a configured boss
            auto it = BossMaxItemLevels.find(creatureEntry);
            if (it == BossMaxItemLevels.end())
                return;

            uint32 maxILevel = it->second;
            
            // Record the kill in the database
            CharacterDatabase.Execute("INSERT INTO selfcraft_upgrade_kills (guid, boss_entry, max_ilevel) VALUES ({}, {}, {}) ON DUPLICATE KEY UPDATE max_ilevel = {}, kill_time = CURRENT_TIMESTAMP",
                player->GetGUID().GetCounter(), creatureEntry, maxILevel, maxILevel);

            if (AnnounceOnKill)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You have defeated {}! Your epic items up to item level {} will receive stat upgrades.", me->GetName(), maxILevel);
            }

            // Recalculate and refresh equipped items
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                if (item && item->GetTemplate()->Quality == ITEM_QUALITY_EPIC)
                {
                    // Remove old bonuses and reapply with new max ilevel
                    SelfCraftUpgradeHelper::RemoveItemBonuses(player, slot);
                    SelfCraftUpgradeHelper::ApplyItemUpgrades(player, item, slot);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new SelfCraftUpgradeCreatureAI(creature);
    }
};

void AddSelfCraftUpgradeScripts()
{
    new SelfCraftUpgradeWorldScript();
    new SelfCraftUpgradePlayerScript();
    new SelfCraftUpgradeCreatureScript();
}
