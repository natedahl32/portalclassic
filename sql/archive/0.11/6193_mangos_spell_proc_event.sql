
DELETE FROM `spell_proc_event` WHERE `entry` IN (39442,39438,39440,39444);
INSERT INTO `spell_proc_event` VALUES
(39442,0,0,0,0,0x0000000000000000,0x00020001,0),
(39438,0,0,0,0,0x0000000000000000,0x00080001,0),
(39440,0,0,0,0,0x0000000000000000,0x00020000,0),
(39444,0,0,0,0,0x0000000000000000,0x00100002,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (36111,39443);
INSERT INTO `spell_proc_event` VALUES
(36111,0,0,0,0,0x0000000000000000,0x00000001,0),
(39443,0,0,0,0,0x0000000000000000,0x00401000,0);
