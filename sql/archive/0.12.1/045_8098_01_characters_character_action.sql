ALTER TABLE character_db_version CHANGE COLUMN required_041_7903_01_characters_character_pet required_045_8098_01_characters_character_action bit;

ALTER TABLE character_action
  CHANGE COLUMN action action int(11) unsigned NOT NULL default '0';

UPDATE character_action
  SET action = action | ( misc << 16 );

ALTER TABLE character_action
    DROP COLUMN misc;
