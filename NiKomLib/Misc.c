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

/****** nikom.library/NiKParse ******************************************

    NAME
        NiKParse  -- Parsrar olika typer av information och ger ett nummer som svar
    SYNOPSIS
        nummer = NiKParse( string, subjekt )
        d0                 a0      d0
        int NiKParse( char * , char * )

    FUNCTION

		Parsrar ett mötesnamn, areanamn, kommandonamn, användarnamn
		eller nyckelnamn till resp. nummer.

    INPUTS

		Subject kan vara följande:
		k - kommando
		m - mötesnamn
		n - användarnamn
		a - areanamn
		y - nyckelnamn

		Sträng är en sträng innehållande ett namn på något som
		visas av subjektet.

    RESULT

        Numret på den typ som önskades.
        
        Returnerar:
        -1 om namnet på "string":en inte kunde hittas för det aktuella subjektet.
        -2 Om subjektet inte finns.
        

    EXAMPLE

		say NiKParse('ni li','n') ==> 0
		variabel="li mö"
		say NiKParse(variabel,'k') ==> 101

    NOTES

    BUGS

    SEE ALSO
        

******************************************************************************
*
*/

int __saveds __asm LIBNiKParse(register __a0 char *string, register __d0 char subject, register __a6 struct NiKomBase *NiKomBase)
{
	int nummer;

	switch(subject)
	{
		case 'k' : case 'K' :
			nummer = parsekom(string, NiKomBase);
			break;
		case 'm' : case 'M' :
			nummer = parsemot(string, NiKomBase);
			break;
		case 'n' : case 'N' :
			nummer = parsenamn(string, NiKomBase);
			break;
		case 'a' : case 'A' :
			nummer = parsearea(string, NiKomBase);
			break;
		case 'y' : case 'Y' :
			nummer = parsenyckel(string, NiKomBase);
			break;
		default :
			nummer = -2;
			break;
	}	
	
	return nummer;
}

/****** nikom.library/SysInfo ******************************************

    NAME
        SysInfo  -- 
    SYNOPSIS
        nummer = SysInfo( subject )
        d0                a0
		int SysInfo( char * )

    FUNCTION

		Parsrar ett mötesnamn, areanamn, kommandonamn, användarnamn
		eller nyckelnamn till resp. nummer.

    INPUTS

		Subject kan vara följande:
		
		a - högsta användarnummer
		n - antal nycklar
		h - högsta textnummer
		k - antal kommandon
		l - lägsta textnummer
		m - högsta mötesnummer
		o - högsta areanummer
		bx - Returnerar en sträng med bps och antalet som ringt med denna
			 bps för nummer bps connect nummer x separerat med ett mellanslag.
		c - 
		t - Totalt antal olika bps connects som BBSen haft. (används tillsammans
			med bx för att få fram olika connect hastigheter till BBSen, och hur
			många av varje.)

    RESULT

        Värdet på den associerat till subjektet eller -1 om subjektet inte finns.

    EXAMPLE

		printf("%d\n",SysInfo("h")); ==> 6648
		puts(SysInfo("b5")) ==> 14400 144

    NOTES

    BUGS

    SEE ALSO
        

******************************************************************************
*
*/

int __saveds __asm LIBSysInfo(register __a0 char *subject, register __a6 struct NiKomBase *NiKomBase)
{
	int varde;

	switch(subject[0])
	{
		case 'a' : case 'A' :
			return (int)((struct ShortUser *)NiKomBase->Servermem->user_list.mlh_TailPred)->nummer;
			break;

		case 'n' : case 'N' :
			return (int)NiKomBase->Servermem->info.nycklar;
			break;

		case 'h' : case 'H' :
			return (int)NiKomBase->Servermem->info.hightext;
			break;
		
		case 'k' : case 'K' :
			return (int)NiKomBase->Servermem->info.kommandon;
			break;
		
		case 'l' : case 'L' :
			return (int)NiKomBase->Servermem->info.lowtext;
			break;
		
		case 'm' : case 'M' :
			return (int)NiKomBase->Servermem->info.moten;
			break;
		
		case 'o' : case 'O' :
			return (int)NiKomBase->Servermem->info.areor;
			break;
		
		case 'b' : case 'B' :
			varde = atoi(&subject[1]);
			if(varde > -1 && varde <= countbps(NiKomBase))
				return (int)NiKomBase->Servermem->info.bps[varde];
			else
				return -2;
			break;

		case 't' : case 'T' :
			return countbps(NiKomBase);
			break;

		otherwise :
			return -1;
	}
}

int countbps(struct NiKomBase *NiKomBase)
{
	int antbps = 0;
	
	while(NiKomBase->Servermem->info.bps[antbps] > 0)
		antbps++;

	return antbps;
}

/****** nikom.library/CommandInfo ******************************************

    NAME
        CommandInfo  -- Läser in information om en användare.
    SYNOPSIS
        nummer = CommandInfo(kommandonummer, subject )
        d0                	  d0			,	a0

        int CommandInfo( ULONG , char * )

    FUNCTION

		Parsrar ett mötesnamn, areanamn, kommandonamn, användarnamn
		eller nyckelnamn till resp. nummer.

    INPUTS

		Subject kan vara följande:
		a - argument (0=inget, 1=numeriskt, 2=annat)
		d - Antal dagar man ska ha varit registrerad för att använda komamndot.
		l - Antal inloggningar man måste ha gjort för att använda kommandot.
		n - Namnet
		o - Antal ord, 1 eller 2.
		s - Status man måste ha för att använda kommandot.
		x - Lösenord som behövs för att använda komamndot.

    RESULT

        Värdet på den associerat till subjektet och det angivna
        kommandonumret.
        -1 om kommandot inte finns.

    EXAMPLE

		printf("%d\n",CommandInfo("h")); ==> 6648
		puts(CommandInfo("b5")) ==> 14400 144

    NOTES

    BUGS

    SEE ALSO

CommandInfo(kommandonr,subject)

Följande subject finns:
a - argument (0=inget, 1=numeriskt, 2=annat)
d - Antal dagar man ska ha varit registrerad för att använda komamndot.
l - Antal inloggningar man måste ha gjort för att använda kommandot.
n - Namnet
o - Antal ord, 1 eller 2.
s - Status man måste ha för att använda kommandot.
x - Lösenord som behövs för att använda komamndot.

Felvärde:
1 - Inte tillräckligt med argument.

Returvärden:
-1 - Finns inget kommando med det numret.
Annars den begärda informationen.

Ex:
say CommandInfo(102,'n') --> LISTA ANVÄNDARE
        

******************************************************************************
*
*/


/****** nikom.library/SendNodeMess ******************************************

    NAME
        SendNodeMess  -- Läser in information om en användare.
    SYNOPSIS
        nummer = SendNodeMess( subject )
        d0                a0
        int NiKParse( char * , char * )

    FUNCTION

		Parsrar ett mötesnamn, areanamn, kommandonamn, användarnamn
		eller nyckelnamn till resp. nummer.

    INPUTS

		Subject kan vara följande:
		
		a - högsta användarnummer
		n - antal nycklar
		h - högsta textnummer
		k - antal kommandon
		l - lägsta textnummer
		m - högsta mötesnummer
		o - högsta areanummer
		bx - Returnerar en sträng med bps och antalet som ringt med denna
			 bps för nummer bps connect nummer x separerat med ett mellanslag.
		t - Totalt antal olika bps connects som BBSen haft. (används tillsammans
			med bx för att få fram olika connect hastigheter till BBSen, och hur
			många av varje.)

    RESULT

        Värdet på den associerat till subjektet.

    EXAMPLE

		printf("%d\n",SendNodeMess("h")); ==> 6648
		puts(SendNodeMess("b5")) ==> 14400 144

    NOTES

    BUGS

    SEE ALSO
        

******************************************************************************
*
*/

/* Funktioner som används i olika delar av libraryts rutiner... */

int parsekom(char *skri, struct NiKomBase *NiKomBase)
{
	int nummer=0,argtyp;
	char *arg2,*ord2;
	struct Kommando *kompek,*forst=NULL;
	if(skri[0]==0) return(-3);
	if(isdigit(skri[0])) {
		/* argument=skri; */
		return(212);
	}
	arg2=hittaefter(skri);
	if(isdigit(arg2[0])) argtyp=KOMARGNUM;
	else if(!arg2[0]) argtyp=KOMARGINGET;
	else argtyp=KOMARGCHAR;
	for(kompek=(struct Kommando *)NiKomBase->Servermem->kom_list.mlh_Head;kompek->kom_node.mln_Succ;kompek=(struct Kommando *)kompek->kom_node.mln_Succ) {
		if(matchar(skri,kompek->namn)) {
			ord2=hittaefter(kompek->namn);
			if((kompek->antal_ord==2 && matchar(arg2,ord2) && arg2[0]) || kompek->antal_ord==1) {
				if(kompek->antal_ord==1) {
					if(kompek->argument==KOMARGNUM && argtyp==KOMARGCHAR) continue;
					if(kompek->argument==KOMARGINGET && argtyp!=KOMARGINGET) continue;
				}
				if(forst==NULL) {
					forst=kompek;
					nummer=kompek->nummer;
				}
				else if(forst==(struct Kommando *)1L) {
					/* puttekn(kompek->namn,-1);
					puttekn("\r",-1); */
				} else {
					/* puttekn("\r\n\nFLERTYDIGT KOMMANDO\r\n\n",-1);
					puttekn(forst->namn,-1);
					puttekn("\r",-1);
					puttekn(kompek->namn,-1);
					puttekn("\r",-1); */
					forst=(struct Kommando *)1L;
				}
			}
		}
	}
	/* if(forst!=NULL && forst!=(struct Kommando *)1L) {
		argument=hittaefter(skri);
		if(forst->antal_ord==2) argument=hittaefter(argument);
	} */
	if(forst==NULL) return(-1);
	else if(forst==(struct Kommando *)1L) return(-2);
	/* else if(forst->status > NiKomBase->Servermem->inne[nodnr].status ||
		forst->minlogg > NiKomBase->Servermem->inne[nodnr].loggin ||
		forst->mindays*86400 > time(&tid)-NiKomBase->Servermem->inne[nodnr].forst_in) return(-4);
	if(forst->losen[0]) {
		puttekn("\r\n\nLösen: ",-1);
		getstring(EJEKO,20,NULL);
		if(strcmp(forst->losen,inmat)) return(-5);
	} */
	return(nummer);
}

int parsemot(char *skri, struct NiKomBase *NiKomBase)
{
	struct Mote *motpek=(struct Mote *)NiKomBase->Servermem->mot_list.mlh_Head;
	int going2=TRUE,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') return(-3);
	for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
		faci=motpek->namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=hittaefter(skri2);
				faci=hittaefter(faci);
				if(skri2[0]==0) return((int)motpek->nummer);
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=hittaefter(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
	}
	return(found);
}

int parsearea(char *skri, struct NiKomBase *NiKomBase)
{
	int going=TRUE,going2=TRUE,count=0,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') {
		found=-3;
		going=FALSE;
	}
	while(going) {
		if(count==NiKomBase->Servermem->info.areor) going=FALSE;
		faci=NiKomBase->Servermem->areor[count].namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=hittaefter(skri2);
				faci=hittaefter(faci);
				if(skri2[0]==0) {
					found=count;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=hittaefter(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		count++;
	}
	return(found);
}

int parsenyckel(char *skri, struct NiKomBase *NiKomBase)
{
	int going=TRUE,going2=TRUE,count=0,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') {
		found=-3;
		going=FALSE;
	}
	while(going) {
		if(count==NiKomBase->Servermem->info.nycklar) going=FALSE;
		faci=NiKomBase->Servermem->Nyckelnamn[count];
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=hittaefter(skri2);
				faci=hittaefter(faci);
				if(skri2[0]==0) {
					found=count;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=hittaefter(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		count++;
	}
	return(found);
}

void MakeUserFilePath(char* string, int userId, char *fileName) {
  sprintf(string, "NiKom:Users/%d/%d/%s", userId/100, userId, fileName);
}
