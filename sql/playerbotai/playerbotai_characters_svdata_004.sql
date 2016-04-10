ALTER TABLE `playerbot_saved_data` ADD `active_spec` int(11);

UPDATE `playerbot_saved_data` SET `active_spec` = 0;