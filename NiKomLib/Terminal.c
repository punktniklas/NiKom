/*
 * Terminal.c - har hand om gränssnittet mellan basen och användaren.
 */

#include <exec/types.h>
#include <string.h>
#include <limits.h>
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"

/* Namn: ConvChrsToAmiga
*  Parametrar: a0 - En pekare till strängen som ska konverteras
*              d0 - Längden på strängen som ska konverteras. Vid 0
*                   konverteras det till noll-byte.
*              d1 - Från vilken teckenuppsättning ska det konverteras,
*                   definierat i NiKomLib.h
*
*  Returvärden: Inga
*
*  Beskrivning: Konverterar den inmatade strängen till ISO 8859-1 från den
*               teckenuppsättning som angivits. De tecken som inte finns i
*               ISO 8859-1 ersätts med ett '?'.
*               För CHRS_CP437 och CHRS_MAC8 konverteras bara tecken högre
*               än 128. För CHRS_SIS7 konverteras bara tecken högre än 32.
*               Tecken över 128 i SIS-strängar tolkas som ISO 8859-1.
*               CHRS_LATIN1 konverteras inte alls.
*/

void __saveds __asm LIBConvChrsToAmiga(register __a0 char *str, register __d0 int len,
	register __d1 int chrs, register __a6 struct NiKomBase *NiKomBase) {

	int x;
	if(chrs==CHRS_LATIN1) return;
	if(!len) len = INT_MAX;
	for(x=0;str[x] && x<len;x++) {
		switch(chrs) {
			case CHRS_CP437 :
				if(str[x]>=128) str[x] = NiKomBase->IbmToAmiga[str[x]];
				break;
			case CHRS_SIS7 :
				if(str[x]>=32) str[x] = NiKomBase->SF7ToAmiga[str[x]];
				break;
			case CHRS_MAC :
				if(str[x]>=128) str[x] = NiKomBase->MacToAmiga[str[x]];
				break;
			default :
				str[x] = convnokludge(str[x]);
		}
	}
}

/* Namn: ConvChrsFromAmiga
*  Parametrar: a0 - En pekare till strängen som ska konverteras
*              d0 - Längden på strängen som ska konverteras. Vid 0
*                   konverteras det till noll-byte.
*              d1 - Till vilken teckenuppsättning ska det konverteras,
*                   definierat i NiKomLib.h
*
*  Returvärden: Inga
*
*  Beskrivning: Konverterar den inmatade strängen från ISO 8859-1 till den
*               teckenuppsättning som angivits. De tecken som inte finns i
*               destinationsteckenuppsättningen ersätts med ett '?'.
*               För CHRS_CP437 och CHRS_MAC8 konverteras bara tecken högre
*               än 128. För CHRS_SIS7 konverteras bara tecken högre än 32.
*               CHRS_LATIN1 konverteras inte alls.
*/

void __saveds __asm LIBConvChrsFromAmiga(register __a0 char *str, register __d0 int len,
	register __d1 int chrs, register __a6 struct NiKomBase *NiKomBase) {

	int x;
	if(chrs==CHRS_LATIN1) return;
	if(!len) len = INT_MAX;
	for(x=0;str[x] && x<len;x++) {
		switch(chrs) {
			case CHRS_CP437 :
				if(str[x]>=128) str[x] = NiKomBase->AmigaToIbm[str[x]];
				break;
			case CHRS_SIS7 :
				if(str[x]>=32) str[x] = NiKomBase->AmigaToSF7[str[x]];
				break;
			case CHRS_MAC :
				if(str[x]>=128) str[x] = NiKomBase->AmigaToMac[str[x]];
				break;
		}
	}
}

UBYTE convnokludge(UBYTE tkn) {
	UBYTE rettkn;
	switch(tkn) {
		case 0x86: rettkn='å'; break;
		case 0x84: rettkn='ä'; break;
		case 0x94: rettkn='ö'; break;
		case 0x8F: rettkn='Å'; break;
		case 0x8E: rettkn='Ä'; break;
		case 0x99: rettkn='Ö'; break;
		case 0x91: rettkn='æ'; break;
		case 0x9b: rettkn='ø'; break;
		case 0x92: rettkn='Æ'; break;
		case 0x9d: rettkn='Ø'; break;
		default : rettkn=tkn;
	}
	return(rettkn);
}

/*	namn:		noansi()

	argument:	pekare till textsträng

	gör:		Strippar bor ANSI-sekvenser ur en textsträng.

*/

void __saveds __asm LIBStripAnsiSequences(register __a0 char *ansistr, register __a6 struct NiKomBase *NiKomBase) {

        char *strptr, *tempstr=NULL, *orgstr;
        int index=0, status=0;


        while( (strptr=strchr(ansistr,'\x1b'))!=NULL)
        {
                orgstr = ansistr;
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
                        ansistr=orgstr;
                }
                else
                        ansistr=&strptr[index];
                index=status=0;
                tempstr=NULL;
        }
}
