ALTER TABLE character_db_version CHANGE COLUMN required_z0778_s0087_02_characters_pet_aura required_z0788_s0099_01_characters_character_aura bit;

ALTER TABLE `character_aura`
  ADD COLUMN `item_guid` int(11) unsigned NOT NULL default '0' AFTER `caster_guid`,
  DROP PRIMARY KEY,
  ADD PRIMARY KEY  (`guid`,`caster_guid`,`item_guid`,`spell`,`effect_index`);

