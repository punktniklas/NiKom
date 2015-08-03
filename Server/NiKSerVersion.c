#include "NiKomStr.h"
#include <stdlib.h>

extern struct System *Servermem;

char dosversion[]="\0$VER: NiKServer " NIKSERVERVERSION "." NIKSERVERREVISION " " __AMIGADATE__;

void GetServerversion(void)
{
	Servermem->serverversion = atoi(NIKSERVERVERSION);
	Servermem->serverrevision = atoi(NIKSERVERREVISION);
}
