ALTER TABLE db_version CHANGE COLUMN required_z1603_s1106_01_mangos_mangos_string required_z1614_s1120_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (503);

INSERT INTO mangos_string VALUES
(503,'The distance is: (3D) %f (2D) %f - (3D, point-to-point) %f yards.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
