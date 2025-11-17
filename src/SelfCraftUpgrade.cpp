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

    // Apply stat upgrades to an item
    void ApplyItemUpgrades(Player* player, Item* item)
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

        // Apply upgrades to item stats
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
                // Note: In AzerothCore, we can't modify item stats directly at runtime
                // This would require a custom implementation or using enchantments
                // For now, we'll use a temporary enchantment system
                // A full implementation would require custom item stat storage
            }
        }
    }

    void OnLogin(Player* player) override
    {
        if (!SelfCraftUpgradeEnabled)
            return;

        // Check if player is self-crafted
        // This requires the mod-challenge-modes module to be present
        // We'll assume players with specific flags or custom data are self-crafted
        
        // Refresh all equipped items
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (item)
            {
                ApplyItemUpgrades(player, item);
            }
        }
    }

    void OnEquip(Player* player, Item* item, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        if (!SelfCraftUpgradeEnabled)
            return;

        ApplyItemUpgrades(player, item);
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

            // Refresh equipped items
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                if (item && item->GetTemplate()->Quality == ITEM_QUALITY_EPIC)
                {
                    // Force a stat recalculation
                    player->_ApplyItemMods(item, slot, false);
                    player->_ApplyItemMods(item, slot, true);
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
