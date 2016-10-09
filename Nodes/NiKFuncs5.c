#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "CharacterSets.h"

#define EKO		1
#define EJEKO	0

extern struct System *Servermem;
extern int nodnr, inloggad;
extern char outbuffer[],inmat[], *argument;

struct Mote *getmotpek(int motnr) {
	struct Mote *letpek=(struct Mote *)Servermem->mot_list.mlh_Head;
	for(;letpek->mot_node.mln_Succ;letpek=(struct Mote *)letpek->mot_node.mln_Succ)
		if(letpek->nummer==motnr) return(letpek);
	return(NULL);
}

char *getmotnamn(int motnr) {
	struct Mote *motpek=getmotpek(motnr);
	if(!motpek) return("<Ok�nt m�te>");
	return(motpek->namn);
}

struct Kommando *getkmdpek(int kmdnr) {
	struct Kommando *letpek=(struct Kommando *)Servermem->kom_list.mlh_Head;
	for(;letpek->kom_node.mln_Succ;letpek=(struct Kommando *)letpek->kom_node.mln_Succ)
		if(letpek->nummer==kmdnr) return(letpek);
	return(NULL);
}

int bytnodtyp(void) {
	int going=TRUE,nr,x;
	struct NodeType *nt=NULL;
	puttekn("\n\n\rVilken nodtyp vill du ha som f�rinst�lld?\n\n\r",-1);
	puttekn(" 0: Ingen, jag vill bli tillfr�gad vid inloggning.\n\r",-1);
	for(x=0; x<MAXNODETYPES; x++) {
		if(Servermem->nodetypes[x].nummer==0) break;
		sprintf(outbuffer,"%2d: %s\n\r",Servermem->nodetypes[x].nummer,Servermem->nodetypes[x].desc);
		putstring(outbuffer,-1,0);
	}
	while(going) {
		putstring("\n\rVal: ",-1,0);
		if(getstring(EKO,2,NULL)) return(1);
		nr = atoi(inmat);
		if(nr<0) putstring("\n\rDu m�ste ange ett positivt heltal.\n\r",-1,0);
		else if(nr==0) going=FALSE;
		else if(!(nt=GetNodeType(atoi(inmat)))) putstring("\n\rFinns ingen s�dan nodtyp.\n\r",-1,0);
		else going=FALSE;
	}
	if(!nt) {
		Servermem->inne[nodnr].shell=0;
		puttekn("\n\n\rDu har nu ingen f�rinst�lld nodtyp.\n\r",-1);
	} else {
		Servermem->inne[nodnr].shell = nt->nummer;
		puttekn("\n\n\rDin f�rinst�llda nodtyp �r nu:\n\r",-1);
		puttekn(nt->desc,-1);
		puttekn("\n\n\r",-1);
	}
	return(0);
}

void dellostsay(void) {
	struct SayString *pek, *tmppek;
	pek = Servermem->say[nodnr];
	Servermem->say[nodnr]=NULL;
	while(pek) {
		tmppek = pek->NextSay;
		FreeMem(pek,sizeof(struct SayString));
		pek = tmppek;
	}
}

void bytteckenset(void) {
  int showExample = FALSE;

  if(argument[0]) {
    switch(argument[0]) {
    case '1' :
      Servermem->inne[nodnr].chrset = CHRS_LATIN1;
      SendString("\n\n\rNytt teckenset: ISO 8859-1\n\r");
      return;
    case '2' :
      Servermem->inne[nodnr].chrset = CHRS_CP437;
      SendString("\n\n\rNytt teckenset: IBM CodePage 437\n\r");
      return;
    case '3' :
      Servermem->inne[nodnr].chrset = CHRS_MAC;
      SendString("\n\n\rNytt teckenset: Mac OS Roman\n\r");
      return;
    case '4' :
      Servermem->inne[nodnr].chrset = CHRS_SIS7;
      SendString("\n\n\rNytt teckenset: SIS-7\n\r");
      return;
    case '-':
      if(argument[1] == 'e' || argument[1] == 'E') {
        showExample = TRUE;
      } else {
        SendString("\n\n\rFelaktigt alternativ.\n\r");
        return;
      }
      break;
    default :
      SendString("\n\n\rFelaktig teckenupps�ttning, ska vara mellan 1 och 4.\n\r");
      return;
    }
  }
  AskUserForCharacterSet(FALSE, showExample);
}

void SaveCurrentUser(int inloggad, int nodnr)
{
	long tid;

	time(&tid);
	Servermem->inne[nodnr].senast_in=tid;
	writeuser(inloggad,&Servermem->inne[nodnr]);
        WriteUnreadTexts(&Servermem->unreadTexts[nodnr], inloggad);
}
