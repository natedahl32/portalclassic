ALTER TABLE db_version CHANGE COLUMN required_z2187_s1820_12269_07_mangos_disenchant_loot_template required_z2187_s1820_12269_08_mangos_mail_loot_template bit;

ALTER TABLE pickpocketing_loot_template DROP COLUMN lootcondition, DROP COLUMN condition_value1, DROP COLUMN condition_value2;
