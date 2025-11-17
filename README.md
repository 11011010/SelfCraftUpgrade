# SelfCraftUpgrade

An AzerothCore module that provides stat upgrades for self-crafted players who kill specific boss NPCs.

## Description

This module is designed to work with the [mod-challenge-modes](https://github.com/ZhengPeiRu21/mod-challenge-modes) module, specifically for self-crafted characters. When a self-crafted player kills certain boss NPCs (like Ragnaros), all epic items they wear receive additional stats based on the item level difference between the boss's dropped loot and the equipped item.

### Stat Calculation Formula

The additional stat for each item stat is calculated using:

```
additional_stat = Old_Stat × ((1.0109)^(Boss_Max_iLevel - Item_iLevel)) - Old_Stat
```

Where:
- `Old_Stat` = The base stat value on the item
- `Boss_Max_iLevel` = The maximum item level that the killed boss drops
- `Item_iLevel` = The item level of the equipped item
- `1.0109` = The stat multiplier (configurable)

## Features

- Tracks boss kills per player in a database
- Automatically applies stat upgrades to epic items when equipped
- Configurable boss NPC IDs and their corresponding max item levels
- Configurable stat multiplier
- Optional announcements when killing qualifying bosses
- Works with any epic quality items below the boss's max item level

## Installation

### 1. Clone the module into your AzerothCore modules directory:

```bash
cd /path/to/azerothcore/modules
git clone https://github.com/11011010/SelfCraftUpgrade.git
```

### 2. Re-run CMake and rebuild:

```bash
cd /path/to/azerothcore/build
cmake ..
make -j$(nproc)
```

### 3. Import the SQL files:

```bash
mysql -u root -p acore_characters < /path/to/azerothcore/modules/SelfCraftUpgrade/sql/characters/selfcraft_upgrade.sql
```

### 4. Configure the module:

Copy `SelfCraftUpgrade.conf.dist` to your server's `etc/` directory as `SelfCraftUpgrade.conf` and adjust settings as needed.

## Configuration

### Basic Settings

```ini
SelfCraftUpgrade.Enable = 1              # Enable/disable the module
SelfCraftUpgrade.AnnounceOnKill = 1      # Announce when boss is killed
SelfCraftUpgrade.StatMultiplier = 1.0109 # Base multiplier for calculations
```

### Boss Configuration

Define which bosses grant upgrades and their max item levels:

```ini
# Format: SelfCraftUpgrade.Boss.<NPC_ID> = <Max_Item_Level>
SelfCraftUpgrade.Boss.11502 = 268  # Ragnaros (Molten Core)
SelfCraftUpgrade.Boss.10184 = 268  # Onyxia
SelfCraftUpgrade.Boss.11583 = 277  # Nefarian (Blackwing Lair)
SelfCraftUpgrade.Boss.15727 = 277  # C'Thun (AQ40)
SelfCraftUpgrade.Boss.15990 = 283  # Kel'Thuzad (Naxxramas)
```

## Example

If a player kills Ragnaros (max ilevel 268) and equips an epic item with ilevel 200 and 20 Strength:

```
Level difference = 268 - 200 = 68
Multiplier = 1.0109^68 ≈ 2.0
New Strength = 20 × 2.0 = 40
Additional Strength = 40 - 20 = 20
```

The item would gain +20 Strength.

## Requirements

- AzerothCore (latest version)
- mod-challenge-modes module (for self-crafted character detection)
- MySQL/MariaDB database

## Credits

- Module created for the 11011010 self-crafted challenge server
- Based on the self-crafted concept from [mod-challenge-modes](https://github.com/ZhengPeiRu21/mod-challenge-modes)

## License

This module is released under the GNU AGPL v3 license, the same license as AzerothCore.