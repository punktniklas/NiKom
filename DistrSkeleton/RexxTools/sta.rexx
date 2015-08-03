/* Statuserar en person... */

parse arg str
if str = '' then do
	say ""
	say " Använd: Sta (Nr/Namn)"
	say ""
	exit
end
nr=NiKParse(str,'n')
if nr < 0 then do
   say ""
   say ' Existerar ingen sådan användare... '
   say ""
   exit
end
say 'Status              :' Userinfo(nr,'r')
say 'Namn                :' Userinfo(nr,'n')
say 'Gatuadress          :' Userinfo(nr,'g')
say 'Postadress          :' Userinfo(nr,'p')
say 'Land                :' Userinfo(nr,'c')
say 'Telefon             :' Userinfo(nr,'f')
say 'Annan info          :' Userinfo(nr,'a')
say 'Först inloggad      :' Userinfo(nr,'b')
say 'Senast inloggad     :' Userinfo(nr,'e')
foo=Userinfo(nr,'t')
say "-----------------------------------------------------"
say 'Total tid           :' foo%3600||':'||(foo//3600)%60
say 'Lästa texter        :' Userinfo(nr,'l')
say 'Inloggningar        :' Userinfo(nr,'i')
say 'Skrivna texter      :' Userinfo(nr,'s')
say 'Uploads             :' UserInfo(nr,'u')
say 'Downloads           :' UserInfo(nr,'d')
say "-----------------------------------------------------"
say 'Prompt              : 'Userinfo(nr,'m')
say 'Rader               : 'Userinfo(nr,'q')

if exists('NiKom:Lappar/'||nr||'.lapp') then 'type NiKom:Lappar/'||nr||'.lapp'
