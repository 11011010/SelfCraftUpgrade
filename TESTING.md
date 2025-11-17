# Testing Checklist

Use this checklist to verify that the Self Craft Upgrade module is working correctly after installation.

## Pre-Testing Setup

- [ ] Module compiled successfully with AzerothCore
- [ ] SQL table `selfcraft_upgrade_kills` created in characters database
- [ ] Configuration file `SelfCraftUpgrade.conf` copied to server config directory
- [ ] Configuration file edited with appropriate boss IDs and item levels
- [ ] Server restarted after module installation

## Basic Functionality Tests

### Test 1: Configuration Loading
- [ ] Check server startup logs for module loading confirmation
- [ ] Verify no errors related to SelfCraftUpgrade in logs
- [ ] Confirm `SelfCraftUpgrade.Enable = 1` in configuration

### Test 2: Database Connectivity
```sql
-- Should return empty result if no bosses killed yet
SELECT * FROM selfcraft_upgrade_kills;
```
- [ ] Query executes without errors
- [ ] Table structure matches expected schema

### Test 3: Boss Kill Detection
- [ ] Kill a configured boss (e.g., Ragnaros - Entry 11502)
- [ ] Verify announcement message appears in chat
- [ ] Check database for recorded kill:
```sql
SELECT * FROM selfcraft_upgrade_kills WHERE guid = YOUR_PLAYER_GUID;
```
- [ ] Confirm boss_entry and max_ilevel are correct

### Test 4: Item Stat Upgrades
- [ ] Equip an epic item with item level below boss max item level
- [ ] Open character sheet and note stats
- [ ] Stats should be higher than base item stats
- [ ] Unequip the item
- [ ] Stats should return to previous values

### Test 5: Item Level Threshold
- [ ] Equip an epic item with item level equal to or above boss max item level
- [ ] Stats should NOT receive bonuses
- [ ] Only lower item level items should be upgraded

### Test 6: Multiple Boss Kills
- [ ] Kill a second boss with higher max item level
- [ ] Verify database has both entries
- [ ] Re-equip items - bonuses should increase (based on highest boss killed)

### Test 7: Non-Epic Items
- [ ] Equip rare (blue) or lower quality items
- [ ] Should NOT receive stat bonuses (only epics are upgraded)

## Advanced Tests

### Test 8: Login/Logout Persistence
- [ ] Equip epic items with bonuses active
- [ ] Log out completely
- [ ] Log back in
- [ ] Bonuses should be reapplied automatically

### Test 9: Bonus Calculations
Verify the formula is correct:
- [ ] Take note of an item's base stat (e.g., +40 Strength)
- [ ] Note item level (e.g., 100)
- [ ] Note highest boss max ilevel (e.g., 268)
- [ ] Calculate expected: `40 × (1.0109^(268-100)) = 40 × (1.0109^168) ≈ 234`
- [ ] Check character sheet - should show approximately 234 Strength from that item
- [ ] Use the formula in EXAMPLES.md to verify other items

### Test 10: Pet/Minion Kills
- [ ] Kill a boss using warlock pet or hunter pet
- [ ] Kill should still count for the player
- [ ] Database should record the kill

### Test 11: Multiple Equipment Slots
- [ ] Equip multiple epic items in different slots
- [ ] Each slot should get appropriate bonuses
- [ ] Total stats should be sum of all bonuses plus base

### Test 12: Configuration Changes
- [ ] Modify `StatMultiplier` in config (e.g., 1.012 instead of 1.0109)
- [ ] Reload configuration (`.reload config` or restart)
- [ ] Bonuses should recalculate with new multiplier on next item equip

## Edge Cases

### Test 13: Rapid Equip/Unequip
- [ ] Quickly equip and unequip the same item multiple times
- [ ] No crashes or memory leaks
- [ ] Stats remain consistent

### Test 14: Full Equipment Change
- [ ] Unequip all epic items
- [ ] Stats return to base
- [ ] Equip a complete set of epic items
- [ ] All slots get bonuses correctly

### Test 15: Boss Without Configuration
- [ ] Kill a boss that's NOT in the configuration
- [ ] Should not record in database
- [ ] Should not show announcement
- [ ] No errors in logs

## Performance Tests

### Test 16: Multiple Players
- [ ] Have multiple players equip/unequip items simultaneously
- [ ] No server lag or crashes
- [ ] Each player gets correct bonuses independently

### Test 17: Database Load
- [ ] Check query performance:
```sql
EXPLAIN SELECT MAX(max_ilevel) FROM selfcraft_upgrade_kills WHERE guid = 1;
```
- [ ] Should use the `idx_guid` index
- [ ] Query should be fast (< 1ms typically)

## Troubleshooting Tests

If tests fail, check:

### Issue: No bonuses applied
- [ ] Is `SelfCraftUpgrade.Enable = 1`?
- [ ] Is the item epic quality?
- [ ] Is item level below boss max level?
- [ ] Has player killed a configured boss?
- [ ] Check database for recorded kills

### Issue: Incorrect bonus amounts
- [ ] Verify `StatMultiplier` in config
- [ ] Recalculate formula manually
- [ ] Check for any custom modifications

### Issue: Bonuses don't persist
- [ ] Check for errors in logs during login
- [ ] Verify database connectivity
- [ ] Test with a clean login (no other mods interfering)

### Issue: Crashes or errors
- [ ] Check worldserver logs for stack traces
- [ ] Verify module compiled correctly
- [ ] Check for conflicts with other modules
- [ ] Ensure AzerothCore version compatibility

## Success Criteria

All tests should pass for the module to be considered fully functional:
- ✅ Bosses kills are tracked correctly
- ✅ Stat bonuses are calculated using the correct formula
- ✅ Bonuses apply/remove properly on equip/unequip
- ✅ Only epic items below boss max ilevel are upgraded
- ✅ No crashes, memory leaks, or errors
- ✅ Performance is acceptable

## Reporting Issues

If tests fail, collect the following information:
1. Which test failed
2. Expected vs actual behavior
3. Worldserver logs (with timestamps)
4. Database queries showing current state
5. AzerothCore version
6. Other installed modules
7. Configuration file contents

Submit issues to: https://github.com/11011010/SelfCraftUpgrade/issues
