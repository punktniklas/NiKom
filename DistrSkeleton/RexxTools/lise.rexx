/* Program som listar senaste inloggningarna */

arg n
if n='' then n=9

do x=0 to n-1
	say left(UserInfo(LastLogin(x,'a'),'n'),40,' ') LastLogin(x,'s') '  ' LastLogin(x,'u')
end
