select * from talents
select * from spells

select * from talents where Id = 796


declare @classname varchar(20)
set @classname = 'pet_tenacity'

select
	UPPER(@classname + '_' + REPLACE(REPLACE(spells.field120, ' ', '_'), '''', '')) + ' = ' + CONVERT(varchar, talents.ID) + ', // TabId = ' + CONVERT(varchar, talents.field1) + ' Tab = 0 Row = ' + CONVERT(varchar, talents.field2) + ' Column = ' + CONVERT(varchar, talents.field3)
from talents
	inner join spells on talents.field4 = spells.ID
where talents.field1 = 409
order by talents.field2, talents.field3
