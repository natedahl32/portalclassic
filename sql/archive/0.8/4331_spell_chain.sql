DELETE FROM `spell_chain` WHERE `spell_id` IN (974,32593,32594);
INSERT INTO `spell_chain` VALUES
(974,0,974,1),
(32593,974,974,2),
(32594,32593,974,3);

