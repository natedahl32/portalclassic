ALTER TABLE db_version CHANGE COLUMN  required_z0540_119_01_mangos_quest_template required_z0555_120_01_mangos_spell_proc_event bit;

DROP TABLE IF EXISTS `spell_proc_event`;
CREATE TABLE `spell_proc_event` (
  `entry` mediumint(8) unsigned NOT NULL default '0',
  `SchoolMask` tinyint(4) unsigned NOT NULL default '0',
  `SpellFamilyName` smallint(5) unsigned NOT NULL default '0',
  `SpellFamilyMask` bigint(40) unsigned NOT NULL default '0',
  `procFlags` int(10) unsigned NOT NULL default '0',
  `procEx` int(10) unsigned NOT NULL default '0',
  `ppmRate` float NOT NULL default '0',
  `CustomChance` float NOT NULL default '0',
  `Cooldown` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

INSERT INTO `spell_proc_event` VALUES
(  324, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  3),
( 2565, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
( 6866, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000070, 0.000000, 0.000000,  0),
( 7131, 0x00,  0, 0x0000000000000000, 0x00000008, 0x00000020, 0.000000, 0.000000,  0),
( 9452, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000010, 3.000000, 0.000000,  0),
( 9782, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
( 9784, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(11103, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(11119, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(11120, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(11129, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(11180, 0x10,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(11185, 0x00,  3, 0x0000000000000080, 0x00050000, 0x00000000, 0.000000, 0.000000,  0),
(11255, 0x00,  3, 0x0000000000004000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12099, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(12169, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(12284, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.332520, 0.000000,  3),
(12289, 0x00,  0, 0x0000000000000002, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12298, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(12311, 0x00,  4, 0x0000000000000800, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12319, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12322, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 2.000000, 0.000000,  3),
(12357, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12358, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12359, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12360, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12487, 0x00,  3, 0x0000000000000080, 0x00050000, 0x00000000, 0.000000, 0.000000,  0),
(12488, 0x00,  3, 0x0000000000000080, 0x00050000, 0x00000000, 0.000000, 0.000000,  0),
(12598, 0x00,  3, 0x0000000000004000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12668, 0x00,  0, 0x0000000000000002, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12701, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.665040, 0.000000,  3),
(12702, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.997560, 0.000000,  3),
(12703, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 1.330080, 0.000000,  3),
(12704, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 1.662600, 0.000000,  3),
(12724, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(12725, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(12726, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(12727, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(12782, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(12797, 0x00,  0, 0x0000000000000400, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12799, 0x00,  0, 0x0000000000000400, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12800, 0x00,  0, 0x0000000000000400, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12834, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12846, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12847, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12848, 0x04,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12849, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12867, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12958, 0x00,  4, 0x0000000000000800, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(12966, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(12967, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(12968, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(12969, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(12970, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(12971, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12972, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12973, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12974, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(12999, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 4.000000, 0.000000,  3),
(13000, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 6.000000, 0.000000,  3),
(13001, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 8.000000, 0.000000,  3),
(13002, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 10.00000, 0.000000,  3),
(13754, 0x00,  0, 0x0000000000000010, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(13867, 0x00,  0, 0x0000000000000010, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(13877, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(13983, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000018, 0.000000, 0.000000,  0),
(14070, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000018, 0.000000, 0.000000,  0),
(14071, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000018, 0.000000, 0.000000,  0),
(14156, 0x00,  0, 0x00000000003E0000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(14160, 0x00,  0, 0x00000000003E0000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(14161, 0x00,  0, 0x00000000003E0000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(14186, 0x00,  8, 0x0000000040800508, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(14190, 0x00,  8, 0x0000000040800508, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(14193, 0x00,  8, 0x0000000040800508, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(14194, 0x00,  8, 0x0000000040800508, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(14195, 0x00,  8, 0x0000000040800508, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(14892, 0x00,  6, 0x0000000410001E00, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(15088, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(15268, 0x20,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(15277, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 6.000000, 0.000000,  3),
(15286, 0x20,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(15323, 0x20,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(15324, 0x20,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(15325, 0x20,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(15326, 0x20,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(15346, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 6.000000, 0.000000,  3),
(15362, 0x00,  6, 0x0000000410001E00, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(15363, 0x00,  6, 0x0000000410001E00, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(15600, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.600000, 0.000000,  3),
(16164, 0x1C,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16176, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16235, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16240, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16256, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16257, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(16277, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(16278, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(16279, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(16280, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(16281, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16282, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16283, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16284, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16620, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 30),
(16624, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(16850, 0x00,  0, 0x0000000000000004, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(16864, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 2.000000, 0.000000,  3),
(16880, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16923, 0x00,  0, 0x0000000000000004, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(16924, 0x00,  0, 0x0000000000000004, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(16952, 0x00,  0, 0x0000040000039000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16954, 0x00,  0, 0x0000040000039000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16958, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(16961, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(17495, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(17670, 0x00,  0, 0x0000000000000000, 0x00000008, 0x00000000, 0.000000, 0.000000,  0),
(17687, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(17793, 0x00,  5, 0x0000000000000001, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(17796, 0x00,  5, 0x0000000000000001, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(17801, 0x00,  5, 0x0000000000000001, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(17802, 0x00,  5, 0x0000000000000001, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(17803, 0x00,  5, 0x0000000000000001, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(18073, 0x00,  0, 0x0000008000000060, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18094, 0x00,  5, 0x000000000000000A, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18095, 0x00,  5, 0x000000000000000A, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18096, 0x00,  0, 0x0000008000000060, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18119, 0x00,  5, 0x00000000000023e5, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18120, 0x00,  5, 0x00000000000023e5, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18121, 0x00,  5, 0x00000000000023e5, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18122, 0x00,  5, 0x00000000000023e5, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18123, 0x00,  5, 0x00000000000023e5, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(18137, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  3),
(18765, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(18800, 0x00,  0, 0x0000000000000000, 0x00000008, 0x00000000, 0.000000, 0.000000,  0),
(19228, 0x00,  0, 0x0000000000000040, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(19232, 0x00,  0, 0x0000000000000040, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(19233, 0x00,  0, 0x0000000000000040, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(19407, 0x00,  0, 0x0000000000000200, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(19412, 0x00,  0, 0x0000000000000200, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(19413, 0x00,  0, 0x0000000000000200, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(19414, 0x00,  0, 0x0000000000000200, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(19415, 0x00,  0, 0x0000000000000200, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(19572, 0x00,  9, 0x0000000000800000, 0x00004000, 0x00000000, 0.000000, 0.000000,  0),
(19573, 0x00,  9, 0x0000000000800000, 0x00004000, 0x00000000, 0.000000, 0.000000,  0),
(20049, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20056, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20057, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20058, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20059, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20128, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(20131, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(20132, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(20133, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(20134, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(20164, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 5.000000, 0.000000,  3),
(20165, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 20.00000, 0.000000,  3),
(20166, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 20.00000, 0.000000,  3),
(20210, 0x00, 10, 0x00000000C0200000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20212, 0x00, 10, 0x00000000C0200000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20213, 0x00, 10, 0x00000000C0200000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20214, 0x00, 10, 0x00000000C0200000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20215, 0x00, 10, 0x00000000C0200000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20234, 0x00,  0, 0x0000000000008000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(20235, 0x00,  0, 0x0000000000008000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(20375, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 7.000000, 0.000000,  3),
(20500, 0x00,  4, 0x0000000010000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(20501, 0x00,  4, 0x0000000010000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(20725, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20784, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20891, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(20911, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(20925, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(21185, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 10),
(21882, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(21890, 0x00,  4, 0x0000036C2A764EEF, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(22007, 0x00,  0, 0x0000000000200821, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(22618, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(22620, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(22648, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(23547, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000020, 0.000000, 0.000000,  0),
(23548, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(23551, 0x00,  0, 0x00000000000000C0, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(23552, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  3),
(23572, 0x00,  0, 0x00000000000000C0, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(23578, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 2.000000, 0.000000,  3),
(23581, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 2.000000, 0.000000,  3),
(23686, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 2.000000, 0.000000,  3),
(23689, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 4.000000, 0.000000,  3),
(23695, 0x00,  0, 0x0000000000000002, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(23721, 0x00,  0, 0x0000000000000800, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(24658, 0x00,  0, 0x0000000000000000, 0x00014110, 0x00000000, 0.000000, 0.000000,  0),
(25669, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 1.000000, 0.000000,  3),
(25899, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000040, 0.000000, 0.000000,  0),
(26016, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 2.000000, 0.000000,  3),
(26021, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 2.000000, 0.000000,  3),
(26107, 0x00,  7, 0x0000008000800000, 0x00000000, 0x00000074, 0.000000, 0.000000,  0),
(26119, 0x00, 11, 0x0000000090100003, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(26128, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000008, 0.000000, 0.000000,  0),
(26135, 0x00,  0, 0x0000000000800000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(26480, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(27419, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(27498, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(27656, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(27787, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(28592, 0x10,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(28593, 0x10,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(28594, 0x10,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(28595, 0x10,  0, 0x0000000000000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(28716, 0x00,  7, 0x0000000000000010, 0x00048000, 0x00000000, 0.000000, 0.000000,  0),
(28719, 0x00,  7, 0x0000000000000020, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(28744, 0x00,  7, 0x0000000000000040, 0x00044000, 0x00000000, 0.000000, 0.000000,  0),
(28752, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(28789, 0x00, 10, 0x0000000000006000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(28809, 0x00,  0, 0x0000000000001000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(28812, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(28816, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(28823, 0x00,  0, 0x00000000000000C0, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(28847, 0x00,  7, 0x0000000000000020, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(28849, 0x00, 11, 0x0000000000000080, 0x00000000, 0x00000000, 0.000000, 0.000000,  0),
(29074, 0x14,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(29075, 0x14,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(29076, 0x14,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(29150, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29179, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(29180, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(29441, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000008, 0.000000, 0.000000,  1),
(29444, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000008, 0.000000, 0.000000,  1),
(29445, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000008, 0.000000, 0.000000,  1),
(29446, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000008, 0.000000, 0.000000,  1),
(29447, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000008, 0.000000, 0.000000,  1),
(29501, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29624, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29625, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29626, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29632, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29633, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29634, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29635, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29636, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(29637, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000000, 3.000000, 0.000000,  0),
(30160, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(30802, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(30808, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(30809, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(30810, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0),
(30811, 0x00,  0, 0x0000000000000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0);
