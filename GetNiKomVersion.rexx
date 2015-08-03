/* Program som plockar ut angivet fält ur filen NiKomVersion */

parse arg type filename .

if ~open('fil',filename,'r') then do
	say 'Can not find file ' || filename
	exit 1
end

do while ~eof('fil')
	rad = readln('fil')
	if abbrev(rad,type) then do
		say '"' || word(rad,2) || '"'
		exit
	end
end

say 'Can not find line ' || type
exit 1
