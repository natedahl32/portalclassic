ALTER TABLE db_version CHANGE COLUMN required_z2381_s2034_12522_01_mangos_db_script_string required_z2410_s2063_12562_01_mangos_playercreateinfo_action bit;

ALTER TABLE playercreateinfo_action DROP INDEX playercreateinfo_race_class_index;
