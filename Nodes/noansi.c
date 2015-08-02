#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "NiKomFuncs.h"

/*	namn:		noansi()

	argument:	pekare till textsträng

	gör:		Strippar bor ANSI-sekvenser ur en textsträng.

*/
void noansi(char *ansistr)
{
	char *strptr, *tempstr=NULL;
	int index=0, status=0;


	while( (strptr=strchr(ansistr,'\x1b'))!=NULL)
	{
		index++;
		if(strptr[index]=='[')
		{
			index++;
                        while( ((strptr[index]>='0' && strptr[index]<='9')
					|| strptr[index]==';')
				&& strptr[index]!=NULL)
			{
				index++;
				if(strptr[index]==';')
				{
					tempstr=strptr;
					strptr=&strptr[index];
					index=1;
				}
			}
		}
		if(strptr[index]==NULL)
			return;
		if(strptr[index]=='m')
		{
			if(tempstr!=NULL)
				memmove(tempstr,&strptr[index+1],strlen(&strptr[index])+1);
			else
				memmove(strptr,&strptr[index+1],strlen(&strptr[index])+1);
			ansistr=strptr;
		}
		else
			ansistr=&strptr[index];
		index=status=0;
		tempstr=NULL;
	}
}