DELETE FROM `spell_proc_event` WHERE `entry` IN (39446);
INSERT INTO `spell_proc_event` VALUES
(39446,0,0,0,0,0x0000000000000000,0x00000004,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (32863,36123,38252,39367);
INSERT INTO `spell_proc_event` VALUES
(32863,0,0,0,0,0x0000000000000000,0x00008000,0),
(36123,0,0,0,0,0x0000000000000000,0x00008000,0),
(38252,0,0,0,0,0x0000000000000000,0x00008000,0),
(39367,0,0,0,0,0x0000000000000000,0x00008000,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (37381);
INSERT INTO `spell_proc_event` VALUES
(37381,0,0,0,0,0x0000000000000000,0x000a0001,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (39372);
INSERT INTO `spell_proc_event` VALUES
(39372,0,0,0,0,0x0000000000000000,0x00020000,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (28789);
INSERT INTO `spell_proc_event` VALUES
(28789,0,0,0,10,0x0000000000006000,0x20000000,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (28849);
INSERT INTO `spell_proc_event` VALUES
(28849,0,0,0,11,0x0000000000000080,0x00004000,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (37655,38334);
INSERT INTO `spell_proc_event` VALUES
(37655,0,0,0,0,0x0000000000000000,0x00004000,0),
(38334,0,0,0,0,0x0000000000000000,0x00004000,0);
