#include <dos/dos.h>
#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "NiKomStr.h"

extern char outbuffer[],reggadnamn[];
extern struct System *Servermem;

char dosversion[]="\0$VER: NiKomCon/Ser " NIKVERSION "." NIKREVISION " " __AMIGADATE__;

void nikversion(void)
{
	int libver, librev;
	char rel[20];
	GetNiKomVersion(&libver,&librev,rel);
	sprintf(outbuffer,"\n\n\rNiKom %s © Niklas Lindholm 1990-1996, 2015 & Tomas Kärki 1996-1998\r\n",rel);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"KOM-nod v%d.%d\r\n",atoi(NIKVERSION),atoi(NIKREVISION));
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Server v%d.%d\r\n",Servermem->serverversion, Servermem->serverrevision);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"NiKom.library v%d.%d\r\n",libver,librev);
	puttekn(outbuffer,-1);
	if(reggadnamn[0]) {
		puttekn("Registrerad på ",-1);
		puttekn(reggadnamn,-1);
		puttekn("\r\n",-1);
	}
/*	else puttekn("Oregistrerad demoversion\r\n",-1); */
}
