ALTER TABLE db_version CHANGE COLUMN required_z2187_s1820_12269_02_mangos_gameobject_loot_template required_z2187_s1820_12269_03_mangos_fishing_loot_template bit;

ALTER TABLE fishing_loot_template DROP COLUMN lootcondition, DROP COLUMN condition_value1, DROP COLUMN condition_value2;
