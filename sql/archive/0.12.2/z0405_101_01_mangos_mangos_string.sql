ALTER TABLE db_version CHANGE COLUMN required_z0404_100_02_mangos_mangos_string required_z0405_101_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry=517;
INSERT INTO mangos_string VALUES (517,'%d, Entry %d - |cffffffff|Hgameobject:%d|h[%s X:%f Y:%f Z:%f MapId:%d]|h|r ',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
