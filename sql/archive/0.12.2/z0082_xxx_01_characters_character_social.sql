ALTER TABLE character_db_version CHANGE COLUMN required_z0001_xxx_01_characters_character_db_version required_z0082_xxx_01_characters_character_social bit;

ALTER TABLE character_social DROP COLUMN note;
