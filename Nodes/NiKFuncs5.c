#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include "NiKomstr.h"
#include "NiKomlib.h"
#include "NiKomFuncs.h"

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
	if(!motpek) return("<Okänt möte>");
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
	puttekn("\n\n\rVilken nodtyp vill du ha som förinställd?\n\n\r",-1);
	puttekn(" 0: Ingen, jag vill bli tillfrågad vid inloggning.\n\r",-1);
	for(x=0; x<MAXNODETYPES; x++) {
		if(Servermem->nodetypes[x].nummer==0) break;
		sprintf(outbuffer,"%2d: %s\n\r",Servermem->nodetypes[x].nummer,Servermem->nodetypes[x].desc);
		putstring(outbuffer,-1,0);
	}
	while(going) {
		putstring("\n\rVal: ",-1,0);
		if(getstring(EKO,2,NULL)) return(1);
		nr = atoi(inmat);
		if(nr<0) putstring("\n\rDu måste ange ett positivt heltal.\n\r",-1,0);
		else if(nr==0) going=FALSE;
		else if(!(nt=GetNodeType(atoi(inmat)))) putstring("\n\rFinns ingen sådan nodtyp.\n\r",-1,0);
		else going=FALSE;
	}
	if(!nt) {
		Servermem->inne[nodnr].shell=0;
		puttekn("\n\n\rDu har nu ingen förinställd nodtyp.\n\r",-1);
	} else {
		Servermem->inne[nodnr].shell = nt->nummer;
		puttekn("\n\n\rDin förinställda nodtyp är nu:\n\r",-1);
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
	char tkn;
	int chrsetbup,going=TRUE;
	if(argument[0]) {
		puttekn("\n\n\rNytt teckenset: ",-1);
		switch(argument[0]) {
			case '1' :
				Servermem->inne[nodnr].chrset = CHRS_LATIN1;
				puttekn("ISO 8859-1\n\r",-1);
				break;
			case '2' :
				Servermem->inne[nodnr].chrset = CHRS_CP437;
				puttekn("IBM CodePage 437\n\r",-1);
				break;
			case '3' :
				Servermem->inne[nodnr].chrset = CHRS_MAC;
				puttekn("Macintosh\n\r",-1);
				break;
			case '4' :
				Servermem->inne[nodnr].chrset = CHRS_SIS7;
				puttekn("SIS-7\n\r",-1);
				break;
			default :
				puttekn("\n\n\rFelaktig teckenuppsättning, ska vara mellan 1 och 4.\n\r",-1);
				break;
		}
		return;
	}
	chrsetbup = Servermem->inne[nodnr].chrset;
	if(Servermem->nodtyp[nodnr] == NODCON) puttekn("\n\n\r*** OBS! Du kör på en CON-nod, alla teckenset kommer se likadana ut!! ***",-1);
	puttekn("\n\n\rDessa teckenset finns. Ta det alternativ vars svenska tecken ser bra ut.\n\r",-1);
	puttekn("* markerar nuvarande val.\n\n\r",-1);
	puttekn("  Nr Namn                              Exempel\n\r",-1);
	puttekn("----------------------------------------------\n\r",-1);
	Servermem->inne[nodnr].chrset = CHRS_LATIN1;
	sprintf(outbuffer,"%c 1: ISO 8859-1 (ISO Latin 1)          åäö ÅÄÖ\n\r",chrsetbup == CHRS_LATIN1 ? '*' : ' ');
	puttekn(outbuffer,-1);
	Servermem->inne[nodnr].chrset = CHRS_CP437;
	sprintf(outbuffer,"%c 2: IBM CodePage 437 (PC8)            åäö ÅÄÖ\n\r",chrsetbup == CHRS_CP437 ? '*' : ' ');
	puttekn(outbuffer,-1);
	Servermem->inne[nodnr].chrset = CHRS_MAC;
	sprintf(outbuffer,"%c 3: Macintosh                         åäö ÅÄÖ\n\r",chrsetbup == CHRS_MAC ? '*' : ' ');
	puttekn(outbuffer,-1);
	Servermem->inne[nodnr].chrset = CHRS_SIS7;
	sprintf(outbuffer,"%c 4: SIS-7 (SF7, måsvingar)            åäö ÅÄÖ\n\r",chrsetbup == CHRS_SIS7 ? '*' : ' ');
	puttekn(outbuffer,-1);
	Servermem->inne[nodnr].chrset = chrsetbup;
	puttekn("\n\rOm du bara ser ett alternativ ovan beror det av att ditt terminalprogram\n\r",-1);
	puttekn("strippar den 8:e biten i inkommande tecken. Om exemplet ser bra ut, ta SIS-7.\n\r",-1);
	puttekn("\n\rVal (Return för inget): ",-1);
	while(going) {
		tkn = gettekn();
		going = FALSE;
		switch(tkn) {
			case '1' :
				Servermem->inne[nodnr].chrset = CHRS_LATIN1;
				puttekn("ISO 8859-1\n\r",-1);
				break;
			case '2' :
				Servermem->inne[nodnr].chrset = CHRS_CP437;
				puttekn("IBM CodePage\n\r",-1);
				break;
			case '3' :
				Servermem->inne[nodnr].chrset = CHRS_MAC;
				puttekn("Macintosh\n\r",-1);
				break;
			case '4' :
				Servermem->inne[nodnr].chrset = CHRS_SIS7;
				puttekn("SIS-7\n\r",-1);
				break;
			case '\r' : case '\n' :
				puttekn("Avbryter\n\r",-1);
				break;
			default :
				going = TRUE;
		}
	}
}

void SaveCurrentUser(int inloggad, int nodnr)
{
	long tid;

	time(&tid);
	Servermem->inne[nodnr].senast_in=tid;
	writeuser(inloggad,&Servermem->inne[nodnr]);
        WriteUnreadTexts(&Servermem->unreadTexts[nodnr], inloggad);
	SaveProgramCategory(inloggad);
}

void ShowProgramDatacfg(int person)
{
	FILE *fp;
	char buffer[257], buffer2[513], command[81], command2[81],
		 command3[81], command4[81], temp[2];
	int i;

	if(fp=fopen("NiKom:Datocfg/ProgramData.cfg", "r"))
	{
		while(!feof(fp))
		{
			buffer[0] = NULL, buffer2[0] = command[0] = NULL;

			fgets(buffer,256,fp);
			buffer[strlen(buffer)-1] = NULL;
			i = 0;

			while(i<strlen(buffer))
			{
				if(buffer[i] != '%')
				{
					if(buffer2[0] = NULL)
					{
						buffer2[0] = buffer[i];
						buffer2[1] = NULL;
					}
					else
					{
						temp[0] = buffer[i];
						temp[1] = NULL;
						strcat(buffer2,temp);
					}

				}
				else
				{
					if(buffer[i+1] == '%' || buffer[i+1] == NULL)
					{
						strcat(buffer2, "%");
						if(buffer[i+1] != NULL)
							i++;
					}
					else
					{
						command[0] = command2[0] = NULL;
						command3[0] = NULL, command4[0] = NULL;

						while(buffer[++i] != '%' && buffer[i] != NULL)
						{
							if(buffer[i] != '|')
							{
								if(command[0] == NULL)
								{
									command[0] = buffer[i];
									command[1] = NULL;
								}
								else
								{
									temp[0] = buffer[i];
									temp[1] = NULL;
									strcat(command, temp);
								}
							}
							else
							{
								while(buffer[++i] != '%' && buffer[i] != NULL)
								{
									if(command2[0] == NULL)
									{
										command2[0] = buffer[i];
										command2[1] = NULL;
									}
									else
									{
										temp[0] = buffer[i];
										temp[1] = NULL;
										strcat(command2, temp);
									}
								}

								if(buffer[++i] != NULL)
								{
									while(buffer[i] != ',' && buffer[i] != NULL)
									{
										if(command3[0] == NULL)
										{
											command3[0] = buffer[i];
											command3[1] = NULL;
										}
										else
										{
											temp[0] = buffer[i];
											temp[1] = NULL;
											strcat(command3, temp);
										}
										i++;
									}


									while(buffer[i] != '%' && buffer[i] != NULL)
									{
										if(command4[0] == NULL)
										{
											command4[0] = buffer[i];
											command4[1] = NULL;
										}
										else
										{
											temp[0] = buffer[i];
											temp[1] = NULL;
											strcat(command4, temp);
										}
										i++;
									}
								}
							}
						}


						if(buffer2[0] != NULL)
						{
							puttekn(buffer2,-1);
							buffer2[0] = NULL;
						}

						if(command && command2)
						{
							if(command3[0] == NULL)
							{
								puttekn(GetProgramData(person, NULL, command, command2), -1);
								puttekn("\n\r", -1);
							}
							else
							{
								if(atoi(GetProgramData(person, NULL, command, command2)))
									puttekn(command3, -1);
								else
									puttekn(command4, -1);

								puttekn("\n\r", -1);
								i = strlen(buffer);
							}

						}

						printf("Command = !%s!, command2 = !%s!, command3 = !%s!, command4 = !%s!\n", command, command2, command3, command4);
					}
				}
				if(buffer2[0] != NULL)
					puttekn(buffer2,-1);

				i++;
			}
		}
		fclose(fp);
		puttekn("\n\r", -1);
	}
}
