DROP TABLE IF EXISTS `reserved_name`;
CREATE TABLE `reserved_name` (
  `name` VARCHAR(12) NOT NULL DEFAULT '',
  PRIMARY KEY  (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player Reserved Names';