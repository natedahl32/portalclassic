ALTER TABLE db_version CHANGE COLUMN required_z0792_s0103_01_mangos_mangos_string required_z0792_s0103_02_mangos_command bit;

DELETE FROM command WHERE name IN (
  'debug update','debug Mod32Value','debug modvalue','modify spell','debug spellmods','modify bit',
  'debug getvalue','debug getitemvalue','debug setvalue','debug setitemvalue','debug moditemvalue'
);

INSERT INTO command (name, security, help) VALUES
('debug getitemvalue',3,'Syntax: .debug getitemvalue #itemguid #field [int|hex|bit|float]\r\n\r\nGet the field #field of the item #itemguid in your inventroy.\r\n\r\nUse type arg for set output format: int (decimal number), hex (hex value), bit (bitstring), float. By default use integer output.'),
('debug getvalue',3,'Syntax: .debug getvalue #field [int|hex|bit|float]\r\n\r\nGet the field #field of the selected target. If no target is selected, get the content of your field.\r\n\r\nUse type arg for set output format: int (decimal number), hex (hex value), bit (bitstring), float. By default use integer output.'),
('debug moditemvalue',3,'Syntax: .debug modvalue #guid #field [int|float| &= | |= | &=~ ] #value\r\n\r\nModify the field #field of the item #itemguid in your inventroy by value #value. \r\n\r\nUse type arg for set mode of modification: int (normal add/subtract #value as decimal number), float (add/subtract #value as float number), &= (bit and, set to 0 all bits in value if it not set to 1 in #value as hex number), |= (bit or, set to 1 all bits in value if it set to 1 in #value as hex number), &=~ (bit and not, set to 0 all bits in value if it set to 1 in #value as hex number). By default expect integer add/subtract.'),
('debug modvalue',3,'Syntax: .debug modvalue #field [int|float| &= | |= | &=~ ] #value\r\n\r\nModify the field #field of the selected target by value #value. If no target is selected, set the content of your field.\r\n\r\nUse type arg for set mode of modification: int (normal add/subtract #value as decimal number), float (add/subtract #value as float number), &= (bit and, set to 0 all bits in value if it not set to 1 in #value as hex number), |= (bit or, set to 1 all bits in value if it set to 1 in #value as hex number), &=~ (bit and not, set to 0 all bits in value if it set to 1 in #value as hex number). By default expect integer add/subtract.'),
('debug setitemvalue',3,'Syntax: .debug setitemvalue #guid #field [int|hex|bit|float] #value\r\n\r\nSet the field #field of the item #itemguid in your inventroy to value #value.\r\n\r\nUse type arg for set input format: int (decimal number), hex (hex value), bit (bitstring), float. By default expect integer input format.'),
('debug setvalue',3,'Syntax: .debug setvalue #field [int|hex|bit|float] #value\r\n\r\nSet the field #field of the selected target to value #value. If no target is selected, set the content of your field.\r\n\r\nUse type arg for set input format: int (decimal number), hex (hex value), bit (bitstring), float. By default expect integer input format.'),
('debug spellmods',3,'Syntax: .debug spellmods (flat|pct) #spellMaskBitIndex #spellModOp #value\r\n\r\nSet at client side spellmod affect for spell that have bit set with index #spellMaskBitIndex in spell family mask for values dependent from spellmod #spellModOp to #value.');

