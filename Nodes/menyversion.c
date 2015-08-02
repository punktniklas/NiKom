#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <stdlib.h>
#include <stdio.h>
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "NiKomStr.h"

extern char outbuffer[],reggadnamn[];
extern struct System *Servermem;

char menydosversion[]="\0$VER: MenyCon/Ser " NIKVERSION "." NIKREVISION " " __AMIGADATE__;

void aboutmeny(void) {
        int libver, librev;
        char rel[20];
        GetNiKomVersion(&libver,&librev,rel);
        sprintf(outbuffer,"\n\n\rNiKom %s, © Tomas Kärki 1996-1998\r\n",rel);
        puttekn(outbuffer,-1);

        sprintf(outbuffer,"Meny-nod v%d.%d © Tomas Kärki 1996-1998\r\n",atoi(NIKVERSION),atoi(NIKREVISION));
        puttekn(outbuffer,-1);

		sprintf(outbuffer,"Server v%d.%d\r\n", Servermem->serverversion, Servermem->serverrevision);
		puttekn(outbuffer,-1);

        sprintf(outbuffer,"NiKom.library v%d.%d\r\n", libver, librev);
        puttekn(outbuffer,-1);
	if(reggadnamn[0])
	{
		puttekn("Registrerad på ",-1);
		puttekn(reggadnamn,-1);
		puttekn("\n\r",-1);
	}
/*	else puttekn("Oregistrerad demoversion\n\r",-1); */
}
