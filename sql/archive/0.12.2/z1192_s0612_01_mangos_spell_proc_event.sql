ALTER TABLE db_version CHANGE COLUMN required_z1186_s0600_01_mangos_game_event_creature_data required_z1192_s0612_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (16257, 16277, 16278, 16279, 16280);
DELETE FROM spell_proc_event WHERE entry IN (12966, 12967, 12968, 12969, 12970);

DELETE FROM spell_proc_event WHERE entry IN (12880);
INSERT INTO `spell_proc_event` VALUES
(12880, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);

DELETE FROM spell_proc_event WHERE entry IN (9452, 9799, 12317, 12966, 14531, 16176, 16257, 16487, 17079, 17364, 20127, 20177, 27811, 30003);
INSERT INTO `spell_proc_event` VALUES
(9452,  0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(9799,  0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12317, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12966, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00010000, 0.000000, 0.000000,  0),
(14531, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16176, 0x00, 11, 0x00000000000001C0, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16257, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00010000, 0.000000, 0.000000,  0),
(16487, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(17079, 0x00,  7, 0x0000000000080000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(17364, 0x08,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(20127, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20177, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(27811, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(30003, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000800, 0.000000, 0.000000,  0);
