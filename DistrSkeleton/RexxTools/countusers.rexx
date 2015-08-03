/* Program som räknar användarna i NiKom */

cnt=0
do x=0 to 500
	if UserInfo(x,'n')~=-1 then cnt=cnt+1
end

say cnt 'användare!'