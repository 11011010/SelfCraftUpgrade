# Integration with mod-challenge-modes

This module is designed to work with the [mod-challenge-modes](https://github.com/ZhengPeiRu21/mod-challenge-modes) module, specifically for self-crafted characters.

## How it Works

The Self Craft Upgrade module does NOT check if a player is self-crafted. It applies upgrades to ALL players who kill the configured bosses. If you want to restrict this to only self-crafted players, you have two options:

### Option 1: Modify the Module (Recommended if mod-challenge-modes is installed)

Add a check in the `ApplyItemUpgrades` function to verify if the player is self-crafted. You would need to check with the mod-challenge-modes module's API or database tables.

Example modification in `src/SelfCraftUpgrade.cpp`:

```cpp
void ApplyItemUpgrades(Player* player, Item* item, uint8 slot)
{
    if (!item || !player)
        return;

    // Check if player is self-crafted (add this check)
    // This assumes mod-challenge-modes stores this in player custom data or a database table
    // QueryResult result = CharacterDatabase.Query("SELECT 1 FROM challenge_modes WHERE guid = {} AND is_selfcrafted = 1", player->GetGUID().GetCounter());
    // if (!result)
    //     return;

    // ... rest of the function
}
```

### Option 2: Only Enable for Specific Players

Use the module as-is, but only record boss kills for self-crafted players by manually managing the database or restricting which players can participate in boss fights.

## Database Integration

The module uses a separate table `selfcraft_upgrade_kills` to track which bosses each player has killed. This table stores:
- Player GUID
- Boss NPC Entry
- Max Item Level granted by that boss
- Kill timestamp

This design allows the module to work independently of mod-challenge-modes while still being compatible with it.

## Configuration for Self-Crafted Servers

For a self-crafted challenge server, you might want to:

1. Set appropriate boss IDs and item levels in the config
2. Consider increasing `StatMultiplier` for more significant upgrades
3. Enable `AnnounceOnKill` to motivate players

Example config for a progressive server:

```ini
SelfCraftUpgrade.Enable = 1
SelfCraftUpgrade.AnnounceOnKill = 1
SelfCraftUpgrade.StatMultiplier = 1.012  # Slightly higher for more noticeable upgrades

# Vanilla bosses
SelfCraftUpgrade.Boss.11502 = 268  # Ragnaros
SelfCraftUpgrade.Boss.10184 = 268  # Onyxia
SelfCraftUpgrade.Boss.11583 = 277  # Nefarian
SelfCraftUpgrade.Boss.15727 = 277  # C'Thun
SelfCraftUpgrade.Boss.15990 = 283  # Kel'Thuzad
```

## Testing

To test the module:

1. Kill a configured boss (e.g., Ragnaros)
2. Check the database: `SELECT * FROM selfcraft_upgrade_kills WHERE guid = <your_player_guid>;`
3. Equip an epic item with item level lower than the boss's max item level
4. Verify that your stats increased
5. Unequip the item and verify stats decreased

## Troubleshooting

**Stats not applying?**
- Check that the module is enabled in config
- Verify the boss is in the configuration with a valid max item level
- Ensure the item is epic quality (purple)
- Confirm the item level is below the boss's max item level
- Check database for recorded kills

**Stats seem wrong?**
- Verify the `StatMultiplier` in config
- Calculate manually: `new_stat = old_stat × (1.0109^(boss_ilevel - item_ilevel))`
- Check logs for any errors
