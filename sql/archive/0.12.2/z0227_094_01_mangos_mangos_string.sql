ALTER TABLE db_version CHANGE COLUMN required_z0226_081_01_mangos_mangos_string required_z0227_094_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry=60;
