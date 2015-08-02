#include "/Include/NiKomStr.h"
#include "/Include/NiKomLib.h"
#include "NiKomBase.h"

#include "funcs.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

/****** nikom.library/MarkTextRead ******************************************

    NAME
        MarkTextRead - Markerar en text som läst
    SYNOPSIS
        error = MarkTextRead( anvnummer, textnr )
        d0                		d0		   d1
        int MarkTextRead(int, int)

    FUNCTION

		Markerar en text i mötessystemet som läst.

    INPUTS

		Textnr - Textnumret på texten som ska markeras som läst.

    RESULT

		error
		
		 0 -	Allt gick OK!
		-1 -	Användaren finns inte.
		-2 -	Texten finns inte.
		-3 -	Texten är redan läst.
		-4 -	Kunde inte läsa in användaren. (slut på minne?)
		-5 -	Texten är raderad.

    EXAMPLE

		printf("%d\n",SendNodeMess("h")); ==> 6648
		puts(SendNodeMess("b5")) ==> 14400 144

    NOTES

		Notera att denna funktion bara fungerar på vanliga texter
		i mötessystemet, inte på Fidonet-texter.

    BUGS

    SEE ALSO
        

******************************************************************************
*
*/

int __saveds __asm LIBMarkTextRead(register __d0 int anvnummer, register __d1 textnr, register __a6 struct NiKomBase *NiKomBase)
{
	int nodnr;
	char bitmap[MAXTEXTS/8], *pek;
	if(textnr < NiKomBase->Servermem->info.lowtext || textnr > NiKomBase->Servermem->info.hightext)
		return -2;
	
	if(!userexists(anvnummer,NiKomBase))
		return -1;

	if(NiKomBase->Servermem->texts[textnr%MAXTEXTS]==-1)
		return -5;
		
	for(nodnr=0;nodnr<MAXNOD;nodnr++)
		if(NiKomBase->Servermem->inloggad[nodnr] == anvnummer) break;

	if(nodnr<MAXNOD)
	{
		if(!BAMTEST(NiKomBase->Servermem->bitmaps[nodnr],textnr%MAXTEXTS))
			return -3;
		BAMCLEAR(NiKomBase->Servermem->bitmaps[nodnr],textnr%MAXTEXTS);
		if(NiKomBase->Servermem->inne[nodnr].textpek > textnr)
			NiKomBase->Servermem->inne[nodnr].textpek = textnr;
	}
	else
	{
		pek = &bitmap[0];
		if(!ReadUserBitmap(&pek,anvnummer))
			return -4;
		if(!BAMTEST(bitmap,textnr%MAXTEXTS))
			return -3;
		BAMCLEAR(bitmap,textnr%MAXTEXTS);
		if(!WriteUserBitmap(&pek,anvnummer))
			return -4;
	}

	return 0;
}

int __saveds __asm LIBMarkTextUnRead(register __d0 int anvnummer, register __d1 textnr, register __a6 struct NiKomBase *NiKomBase)
{
	int nodnr;
	char bitmap[MAXTEXTS/8], *pek;

	if(textnr < NiKomBase->Servermem->info.lowtext || textnr > NiKomBase->Servermem->info.hightext)
		return -2;

	if(!userexists(anvnummer,NiKomBase))
		return -1;

	if(NiKomBase->Servermem->texts[textnr%MAXTEXTS]==-1)
		return -5;
		
	for(nodnr=0;nodnr<MAXNOD;nodnr++)
		if(NiKomBase->Servermem->inloggad[nodnr] == anvnummer) break;

	if(nodnr<MAXNOD)
	{
		if(BAMTEST(NiKomBase->Servermem->bitmaps[nodnr],textnr%MAXTEXTS))
			return -3;
		BAMSET(NiKomBase->Servermem->bitmaps[nodnr],textnr%MAXTEXTS);
		if(NiKomBase->Servermem->inne[nodnr].textpek > textnr)
			NiKomBase->Servermem->inne[nodnr].textpek = textnr;
	}
	else
	{
		pek = &bitmap[0];
		if(!ReadUserBitmap(&pek,anvnummer))
			return -4;
		if(BAMTEST(bitmap,textnr%MAXTEXTS))
			return -3;
		BAMSET(bitmap,textnr%MAXTEXTS);
		if(!WriteUserBitmap(&pek,anvnummer))
			return -4;
	}

	return 0;
}



/* Funktioner som används i olika delar av dessa library rutiner. */

int ReadUserBitmap(char **bitmap, int usernumber)
{
	BPTR file;
	char filnamn[41];

	sprintf(filnamn, "NiKom:Users/%d/%d/Bitmap0",usernumber/100,usernumber);
	if(file = Open(filnamn, MODE_OLDFILE))
	{
		if(Read(file,(void *)*bitmap,MAXTEXTS/8) == -1)
		{
			Close(file);
			return 0;
		}
		Close(file);
		return 1;
	}
	
	return 0;
}

int WriteUserBitmap(char **bitmap, int usernumber)
{
	BPTR file;
	char filnamn[41];

	sprintf(filnamn, "NiKom:Users/%d/%d/Bitmap0",usernumber/100,usernumber);
	if(file = Open(filnamn, MODE_NEWFILE))
	{
		if(Write(file,(void *)*bitmap,MAXTEXTS/8) == -1)
		{
			Close(file);
			return 0;
		}
		Close(file);
		return 1;
	}
	
	return 0;
}

int userexists(int nummer, struct NiKomBase *NiKomBase)
{
	struct ShortUser *letpek;
	int found=FALSE;
	for(letpek=(struct ShortUser *)NiKomBase->Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			found=TRUE;
			break;
		}
	}
	return(found);
}
