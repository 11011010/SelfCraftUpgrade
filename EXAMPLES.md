# Usage Examples and Testing

## Example Scenario

Let's say you're playing a self-crafted character on a vanilla WoW server. You're wearing various epic items from different raids.

### Your Current Gear

- **Helmet**: Lionheart Helm (iLevel 100)
  - +40 Strength
  - +40 Stamina
  
- **Chest**: Breastplate of Might (iLevel 226)
  - +35 Strength
  - +35 Stamina

### Boss Kills

You defeat Ragnaros (Boss Entry: 11502, Max iLevel: 268)

### Calculations

#### For Lionheart Helm (iLevel 100)

Level difference = 268 - 100 = 168

For Strength (+40):
```
multiplier = 1.0109^168 ≈ 5.86
new_strength = 40 × 5.86 = 234.4 → 234
additional_strength = 234 - 40 = 194
```

For Stamina (+40):
```
multiplier = 1.0109^168 ≈ 5.86
new_stamina = 40 × 5.86 = 234.4 → 234
additional_stamina = 234 - 40 = 194
```

**Result**: Your Lionheart Helm now effectively gives you +234 Strength and +234 Stamina!

#### For Breastplate of Might (iLevel 226)

Level difference = 268 - 226 = 42

For Strength (+35):
```
multiplier = 1.0109^42 ≈ 1.57
new_strength = 35 × 1.57 = 54.95 → 54
additional_strength = 54 - 35 = 19
```

For Stamina (+35):
```
multiplier = 1.0109^42 ≈ 1.57
new_stamina = 35 × 1.57 = 54.95 → 54
additional_stamina = 54 - 35 = 19
```

**Result**: Your Breastplate of Might now gives you +54 Strength and +54 Stamina.

### After Defeating Kel'Thuzad (Max iLevel: 283)

Your max iLevel increases to 283, so the calculations update:

#### For Lionheart Helm (iLevel 100)

Level difference = 283 - 100 = 183

For Strength (+40):
```
multiplier = 1.0109^183 ≈ 7.04
new_strength = 40 × 7.04 = 281.6 → 281
additional_strength = 281 - 40 = 241
```

**Result**: Now your Lionheart Helm gives +281 Strength (gained 47 more Strength!)

## Testing the Module

### 1. Manual Testing

```sql
-- Check your current boss kills
SELECT * FROM selfcraft_upgrade_kills WHERE guid = YOUR_PLAYER_GUID;

-- Manually add a boss kill for testing
INSERT INTO selfcraft_upgrade_kills (guid, boss_entry, max_ilevel) 
VALUES (YOUR_PLAYER_GUID, 11502, 268);

-- Check your character's stats before and after equipping items
-- Log out and log back in to ensure stats are recalculated
```

### 2. In-Game Testing

1. **Before killing any bosses**:
   - Note your character stats
   - Equip an epic item
   - Stats should not change (no upgrades yet)

2. **Kill a configured boss** (e.g., Ragnaros):
   - You should see a message: "You have defeated Ragnaros! Your epic items up to item level 268 will receive stat upgrades."
   - Check your stats - they should have increased immediately

3. **Equip a different epic item**:
   - If it's below the boss max iLevel, stats should increase
   - If it's at or above the boss max iLevel, no bonus

4. **Unequip the item**:
   - Stats should decrease by the bonus amount

### 3. Verifying Formula

Use this SQL query to see what bonuses are being calculated:

```sql
-- This shows the multiplier for different level gaps
SELECT 
    boss_ilevel,
    item_ilevel,
    (boss_ilevel - item_ilevel) AS level_diff,
    POWER(1.0109, (boss_ilevel - item_ilevel)) AS multiplier,
    base_stat,
    FLOOR(base_stat * POWER(1.0109, (boss_ilevel - item_ilevel))) AS new_stat,
    FLOOR(base_stat * POWER(1.0109, (boss_ilevel - item_ilevel))) - base_stat AS bonus
FROM (
    SELECT 268 AS boss_ilevel, 100 AS item_ilevel, 40 AS base_stat
    UNION ALL SELECT 268, 200, 40
    UNION ALL SELECT 268, 226, 35
    UNION ALL SELECT 283, 100, 40
) AS test_cases;
```

## Common Scenarios

### Scenario 1: Upgrading from Pre-Raid Bis to Raid Gear

- You have crafted epics (iLevel ~60-100)
- You kill MC bosses (max iLevel 268)
- Your crafted gear gets MASSIVE upgrades (5-7x stats)
- As you get better drops, the upgrade bonus decreases

### Scenario 2: Progressive Raiding

- Start with MC (268) → Your gear gets boosted
- Move to BWL (277) → Existing items get slightly better
- Clear AQ40 (277) → Same tier, no change
- Defeat Naxx (283) → All items get another boost

### Scenario 3: Mixed Gear

You'll have items at different levels:
- Low iLevel items: HUGE bonuses (but still might not be BiS)
- Mid iLevel items: Moderate bonuses
- High iLevel items: Small or no bonuses

This creates interesting gearing decisions!

## Debugging

Enable verbose logging in AzerothCore to see what's happening:

```cpp
// Add LOG_INFO statements in the code
LOG_INFO("module.selfcraftupgrade", "Player {} killed boss {}. Max iLevel: {}", 
    player->GetName(), me->GetEntry(), maxILevel);

LOG_INFO("module.selfcraftupgrade", "Item {} (iLevel {}) gets +{} to stat {}", 
    item->GetEntry(), itemILevel, additionalStat, statType);
```

## Performance Considerations

The module performs a database query every time an item is equipped. For optimal performance:

1. Ensure the `idx_guid` index exists on `selfcraft_upgrade_kills`
2. The query uses `MAX()` which is efficient with proper indexing
3. Results are cached in memory per player session

## Calculator

Want to calculate bonuses before killing a boss? Use this formula:

```
bonus = old_stat × ((1.0109)^(boss_max_ilevel - item_ilevel)) - old_stat
```

Or use a spreadsheet with this formula:
```excel
=FLOOR(B2*POWER(1.0109,A2-C2),1)-B2

Where:
A2 = Boss Max iLevel
B2 = Item Base Stat
C2 = Item iLevel
```
