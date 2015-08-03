/* Ger info om en fil i filarean */

parse arg namn .

heltnamn=FileInfo(namn,'n',0)
if heltnamn=-1 then do
	say 'Finns ingen sådan fil'
	exit
end

say heltnamn
say FileInfo(heltnamn,'b',0)
say AreaInfo(0,'d'||FileInfo(heltnamn,'i',0))||heltnamn
