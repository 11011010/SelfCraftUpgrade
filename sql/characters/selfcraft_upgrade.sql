-- Table to track boss kills per player for self-crafted upgrade system
CREATE TABLE IF NOT EXISTS `selfcraft_upgrade_kills` (
    `guid` INT UNSIGNED NOT NULL COMMENT 'Player GUID',
    `boss_entry` INT UNSIGNED NOT NULL COMMENT 'Boss NPC Entry',
    `max_ilevel` SMALLINT UNSIGNED NOT NULL COMMENT 'Max item level from this boss',
    `kill_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When the boss was killed',
    PRIMARY KEY (`guid`, `boss_entry`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Tracks boss kills for self-crafted upgrade system';
