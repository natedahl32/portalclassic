INSERT INTO command VALUES('goxy','3','Syntax: .goxy #x #y [#mapid]\r\n\r\nTeleport player to point with (#x,#y) coordinates at ground(water) level at map #mapid or same map if #mapid not provided.');
UPDATE command SET `help` = 'Syntax: .taxicheat #flag\r\n\r\nTemporary grant access or remove to all taxi routes for the selected character. If no character is selected, hide or reveal all routes to you.\r\n\r\nUse a #flag of value 1 to add access, use a #flag value of 0 to remove access. Visited taxi nodes sill accessible after removing access.' WHERE `name` = 'taxicheat';
