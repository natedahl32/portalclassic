ALTER TABLE db_version CHANGE COLUMN required_z0770_s0080_02_mangos_scripted_areatrigger required_z0770_s0080_03_mangos_scripted_event_id bit;

RENAME TABLE event_id_scripts TO scripted_event_id;
