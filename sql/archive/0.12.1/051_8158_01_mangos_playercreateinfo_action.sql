DROP TABLE IF EXISTS `playercreateinfo_action`;
CREATE TABLE `playercreateinfo_action` (
  `race` tinyint(3) unsigned NOT NULL default '0',
  `class` tinyint(3) unsigned NOT NULL default '0',
  `button` smallint(5) unsigned NOT NULL default '0',
  `action` int(11) unsigned NOT NULL default '0',
  `type` smallint(5) unsigned NOT NULL default '0',
  KEY `playercreateinfo_race_class_index` (`race`,`class`),
  PRIMARY KEY (`race`,`class`,`button`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

INSERT INTO `playercreateinfo_action` VALUES
(1,1,72,6603,0),
(1,1,73,78,0),
(1,1,83,117,128),
(1,2,0,6603,0),
(1,2,1,21084,0),
(1,2,2,635,0),
(1,2,10,159,128),
(1,2,11,2070,128),
(1,4,0,6603,0),
(1,4,1,1752,0),
(1,4,2,2098,0),
(1,4,3,2764,0),
(1,4,11,2070,128),
(1,5,0,6603,0),
(1,5,1,585,0),
(1,5,2,2050,0),
(1,5,10,159,128),
(1,5,11,2070,128),
(1,8,0,6603,0),
(1,8,1,133,0),
(1,8,2,168,0),
(1,8,10,159,128),
(1,8,11,2070,128),
(1,9,0,6603,0),
(1,9,1,686,0),
(1,9,2,687,0),
(1,9,10,159,128),
(1,9,11,4604,128),
(2,1,72,6603,0),
(2,1,73,78,0),
(2,1,83,117,128),
(2,3,0,6603,0),
(2,3,1,2973,0),
(2,3,2,75,0),
(2,3,10,159,128),
(2,3,11,117,128),
(2,4,0,6603,0),
(2,4,1,1752,0),
(2,4,2,2098,0),
(2,4,11,117,128),
(2,7,0,6603,0),
(2,7,1,403,0),
(2,7,2,331,0),
(2,7,10,159,128),
(2,7,11,117,128),
(2,9,0,6603,0),
(2,9,1,686,0),
(2,9,2,687,0),
(2,9,10,159,128),
(2,9,11,117,128),
(3,1,72,6603,0),
(3,1,73,78,0),
(3,1,83,117,128),
(3,2,0,6603,0),
(3,2,1,21084,0),
(3,2,2,635,0),
(3,2,10,159,128),
(3,2,11,4540,128),
(3,3,0,6603,0),
(3,3,1,2973,0),
(3,3,2,75,0),
(3,3,10,159,128),
(3,3,11,117,128),
(3,4,0,6603,0),
(3,4,1,1752,0),
(3,4,2,2098,0),
(3,4,3,2764,0),
(3,4,11,4540,128),
(3,5,0,6603,0),
(3,5,1,585,0),
(3,5,2,2050,0),
(3,5,10,159,128),
(3,5,11,4540,128),
(4,1,72,6603,0),
(4,1,73,78,0),
(4,1,74,20580,0),
(4,1,83,117,128),
(4,3,0,6603,0),
(4,3,1,2973,0),
(4,3,2,75,0),
(4,3,3,20580,0),
(4,3,10,159,128),
(4,3,11,117,128),
(4,4,0,6603,0),
(4,4,1,1752,0),
(4,4,2,2098,0),
(4,4,3,2764,0),
(4,4,4,20580,0),
(4,4,11,4540,128),
(4,5,0,6603,0),
(4,5,1,585,0),
(4,5,2,2050,0),
(4,5,3,20580,0),
(4,5,10,159,128),
(4,5,11,2070,128),
(4,11,0,6603,0),
(4,11,1,5176,0),
(4,11,2,5185,0),
(4,11,3,20580,0),
(4,11,10,159,128),
(4,11,11,4536,128),
(5,1,72,6603,0),
(5,1,73,78,0),
(5,1,83,4604,128),
(5,4,0,6603,0),
(5,4,1,1752,0),
(5,4,2,2098,0),
(5,4,3,2764,0),
(5,4,11,4604,128),
(5,5,0,6603,0),
(5,5,1,585,0),
(5,5,2,2050,0),
(5,5,10,159,128),
(5,5,11,4604,128),
(5,8,0,6603,0),
(5,8,1,133,0),
(5,8,2,168,0),
(5,8,10,159,128),
(5,8,11,4604,128),
(5,9,0,6603,0),
(5,9,1,686,0),
(5,9,2,687,0),
(5,9,10,159,128),
(5,9,11,4604,128),
(6,1,72,6603,0),
(6,1,73,78,0),
(6,1,74,20549,0),
(6,1,83,4540,128),
(6,3,0,6603,0),
(6,3,1,2973,0),
(6,3,2,75,0),
(6,3,3,20549,0),
(6,3,10,159,128),
(6,3,11,117,128),
(6,7,0,6603,0),
(6,7,1,403,0),
(6,7,2,331,0),
(6,7,3,20549,0),
(6,7,10,159,128),
(6,7,11,4604,128),
(6,11,0,6603,0),
(6,11,1,5176,0),
(6,11,2,5185,0),
(6,11,3,20549,0),
(6,11,10,159,128),
(6,11,11,4536,128),
(7,1,72,6603,0),
(7,1,73,78,0),
(7,1,83,117,128),
(7,4,0,6603,0),
(7,4,1,1752,0),
(7,4,2,2098,0),
(7,4,3,2764,0),
(7,4,11,117,128),
(7,8,0,6603,0),
(7,8,1,133,0),
(7,8,2,168,0),
(7,8,10,159,128),
(7,8,11,4536,128),
(7,9,0,6603,0),
(7,9,1,686,0),
(7,9,2,687,0),
(7,9,10,159,128),
(7,9,11,4604,128),
(8,1,72,6603,0),
(8,1,73,78,0),
(8,1,74,2764,0),
(8,1,83,117,128),
(8,3,0,6603,0),
(8,3,1,2973,0),
(8,3,2,75,0),
(8,3,10,159,128),
(8,3,11,4604,128),
(8,4,0,6603,0),
(8,4,1,1752,0),
(8,4,2,2098,0),
(8,4,3,2764,0),
(8,4,11,117,128),
(8,5,0,6603,0),
(8,5,1,585,0),
(8,5,2,2050,0),
(8,5,10,159,128),
(8,5,11,4540,128),
(8,7,0,6603,0),
(8,7,1,403,0),
(8,7,2,331,0),
(8,7,10,159,128),
(8,7,11,117,128),
(8,8,0,6603,0),
(8,8,1,133,0),
(8,8,2,168,0),
(8,8,10,159,128),
(8,8,11,117,128),
(10,2,0,6603,0),
(10,2,1,21084,0),
(10,2,2,635,0),
(10,2,3,28734,0),
(10,2,4,28730,0),
(10,2,10,159,128),
(10,2,11,20857,128),
(10,3,0,6603,0),
(10,3,1,2973,0),
(10,3,2,75,0),
(10,3,3,28734,0),
(10,3,4,28730,0),
(10,3,10,159,128),
(10,3,11,20857,128),
(10,4,0,6603,0),
(10,4,1,1752,0),
(10,4,2,2098,0),
(10,4,3,2764,0),
(10,4,4,28734,0),
(10,4,5,25046,0),
(10,4,11,20857,128),
(10,5,0,6603,0),
(10,5,1,585,0),
(10,5,2,2050,0),
(10,5,3,28734,0),
(10,5,4,28730,0),
(10,5,10,159,128),
(10,5,11,20857,128),
(10,8,0,6603,0),
(10,8,1,133,0),
(10,8,2,168,0),
(10,8,3,28734,0),
(10,8,4,28730,0),
(10,8,10,159,128),
(10,8,11,20857,128),
(10,9,0,6603,0),
(10,9,1,686,0),
(10,9,2,687,0),
(10,9,3,28734,0),
(10,9,4,28730,0),
(10,9,10,159,128),
(10,9,11,20857,128),
(11,1,0,6603,0),
(11,1,72,6603,0),
(11,1,73,78,0),
(11,1,74,28880,0),
(11,1,83,4540,128),
(11,1,84,6603,0),
(11,1,96,6603,0),
(11,1,108,6603,0),
(11,2,0,6603,0),
(11,2,1,21084,0),
(11,2,2,635,0),
(11,2,3,28880,0),
(11,2,10,159,128),
(11,2,11,4540,128),
(11,2,83,4540,128),
(11,3,0,6603,0),
(11,3,1,2973,0),
(11,3,2,75,0),
(11,3,3,28880,0),
(11,3,10,159,128),
(11,3,11,4540,128),
(11,3,72,6603,0),
(11,3,73,2973,0),
(11,3,74,75,0),
(11,3,82,159,128),
(11,3,83,4540,128),
(11,5,0,6603,0),
(11,5,1,585,0),
(11,5,2,2050,0),
(11,5,3,28880,0),
(11,5,10,159,128),
(11,5,11,4540,128),
(11,5,83,4540,128),
(11,7,0,6603,0),
(11,7,1,403,0),
(11,7,2,331,0),
(11,7,3,28880,0),
(11,7,10,159,128),
(11,7,11,4540,128),
(11,8,0,6603,0),
(11,8,1,133,0),
(11,8,2,168,0),
(11,8,3,28880,0),
(11,8,10,159,128),
(11,8,11,4540,128),
(11,8,83,4540,128);
