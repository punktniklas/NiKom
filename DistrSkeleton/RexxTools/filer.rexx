/* Program som listar filer */

parse arg area

arnr=NiKParse(area,'a')
if arnr<0 then do
	say 'Finns ingen sådan area'
	exit
end

fil=FileInfo('*','n',arnr)

do while left(fil,1)~='-'
	say left(fil,25) right(FileInfo(fil,'s',arnr),7) left(FileInfo(fil,'t',arnr),6) left(UserInfo(FileInfo(fil,'u',arnr),'n'),30) right(FileInfo(fil,'d',arnr),3)
	say '  ' FileInfo(fil,'b',arnr)
	say
	fil=NextFile(fil,arnr)
end
