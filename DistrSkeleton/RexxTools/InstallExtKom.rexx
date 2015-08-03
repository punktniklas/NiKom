/* Program för att installera nya ExtKomxxxx.nik

För att man ska kunna installera ett kommando med det här programmet krävs
det att det följer standarden och har följande fält i kommentaren i början
av filen:
 N=Namn på kommandot
 O=Antal ord
 #=Nummer på kommandot

Eventuellt kan det även stå vilken typ av argument kommandot tar. Står
inget sådant fält så antas det att kommandot inte tar något argument. Se
Installation.doc för mer information om hur kommandon definieras i
Kommandon.cfg
*/

parse arg filnamn .

if ~open('fil',filnamn,'R') then do
	say 'Kan inte hitta någon fil med namnet' filnamn
	exit 10
end

namnfound=0
ordfound=0
nummerfound=0
argumentfound=0

do until eof('fil')
	inrad=readln('fil')
	select
		when left(inrad,2)='N=' then do
			namnfound=1
			namn=right(inrad,length(inrad)-2)
		end
		when left(inrad,2)='O=' then do
			ordfound=1
			ord=right(inrad,length(inrad)-2)
		end
		when left(inrad,2)='#=' then do
			nummerfound=1
			nummer=right(inrad,length(inrad)-2)
		end
		when left(inrad,2)='A=' then do
			argumentfound=1
			argument=right(inrad,length(inrad)-2)
		end
		when index(inrad,'*/')>0 then leave
		otherwise nop
	end
end
call close('fil')

if ~namnfound & ~ordfound & ~nummerfound then do
	say 'Inte tillräckligt med information i början av programmet.'
	say 'Kan inte installera det.'
	exit
end

say 'Vilken status ska behövas för att utföra kommandot?'
parse pull status
say
say 'Minsta antal dagar en användare en användare måste ha varit registrerad'
say 'för att få utföra kommandot (0 för ingen begränsning)'
parse pull dagar
say
say 'Minsta antal inloggningar för att få utföra kommandot'
say '(0 för ingen begränsning)'
parse pull inlogg
say
say 'Ev lösenord för att få utföra kommandot'
parse pull losen

say
say
say 'Installera filen' filnamn
say 'som NiKom:Rexx/ExtKom'||nummer||'.nik'
say
say 'N='||namn
say 'O='||ord
say '#='||nummer
if argumentfound then say 'A='||argument
if dagar>0 then say 'D='||dagar
if inlogg>0 then say 'L='||inlogg
if losen~='' then say 'X='||losen
say
say 'Är detta ok? (j/N)'
parse pull ok
if upper(ok)~='J' then exit

address command 'copy' filnamn 'NiKom:Rexx/ExtKom'||nummer||'.nik'
if exists(nummer||'.man') then address command 'copy' nummer||'.man NiKom:Lappar/'
if ~open('cfg','NiKom:DatoCfg/Kommandon.cfg','A') then do
	say 'Kunde inte öppna Kommandon.cfg'
	exit 10
end

call writeln('cfg','')
call writeln('cfg','N='||namn)
call writeln('cfg','O='||ord)
call writeln('cfg','#='||nummer)
if argumentfound then call writeln('cfg','A='||argument)
if dagar>0 then call writeln('cfg','D='||dagar)
if inlogg>0 then call writeln('cfg','L='||inlogg)
if losen~='' then call writeln('cfg','X='||losen)
call close('cfg')

say
say 'Läsa in konfigurationsfilerna så att kommandot kan användas? (j/N)'
parse pull ok
if upper(ok)~='J' then call ReadConfig()
