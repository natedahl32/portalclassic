ALTER TABLE db_version CHANGE COLUMN required_z0588_126_02_mangos_spell_proc_event required_z0595_xxx_01_mangos_spell_proc_event bit;

DELETE FROM  `spell_proc_event` WHERE `entry` IN (29062);

INSERT INTO `spell_proc_event` VALUES
(29062, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0);
