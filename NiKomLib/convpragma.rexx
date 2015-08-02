/* Lägger till tagcalls på de funktioner som ska ha det */

parse arg infil utfil .

func.1 = 'ReadFidoText'
func.2 = 'WriteFidoText'
func.3 = 'EditUser'
func.4 = 'CreateUser'

numfuncs = 4

if ~open('infil',infil,'r') then do
	say 'Kan inte öppna' infil
	exit
end

if ~open('utfil',utfil,'w') then do
	say 'Kan inte öppna' utfil
	exit
end

do while ~eof('infil')
	rad = readln('infil')
	parse var rad 'NiKomBase' name rest
	name = strip(name)
	if chgfunc(name) then do
		nyrad = '#pragma tagcall NiKomBase' name || 'Tags' || rest
		call writeln('utfil',nyrad)
	end
	call writeln('utfil',rad)
end
exit

chgfunc:
	parse arg fname

	do i = 1 to numfuncs
		if fname = func.i then return 1
	end
	return 0
