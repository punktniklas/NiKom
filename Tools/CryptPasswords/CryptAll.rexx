/* Runs CryptPasswords for all users */

do i=0 to SysInfo('a')
  if UserInfo(i,'n')='-1' then iterate
  say 'Encrypting user' i
  'CryptPasswords -u' i
end
