/* Program för att reda på kommandon som inte har manualfiler */

if ~open('fil','NiKom:DatoCfg/Kommandon.cfg','R') then do
	say 'Scheisse, kunde inte öppna filen!'
	exit 10
end

do until eof('fil')
	foo=readln('fil')
	if left(foo,1)='N' then namn=right(foo,length(foo)-2)
	else if left(foo,1)='#' then do
		if ~exists('NiKom:Manualer/'||right(foo,length(foo)-2)||'.man') then say namn '#'||right(foo,length(foo)-2)
	end
end
