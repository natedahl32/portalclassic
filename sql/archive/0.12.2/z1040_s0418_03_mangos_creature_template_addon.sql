ALTER TABLE db_version CHANGE COLUMN required_z1040_s0418_02_mangos_creature_addon required_z1040_s0418_03_mangos_creature_template_addon bit;

ALTER TABLE creature_template_addon
  DROP COLUMN bytes0;
