#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#include "NiKomFuncs.h"
#include "NiKomstr.h"
#include "MenyFuncs.h"

#define ERROR   10
#define OK              0
#define EKO             1
#define EJEKO   0
#define KOM             1
#define EJKOM   0
#define BREVKOM -1

extern char outbuffer[],inmat[],backspace[],prenamn,commandhistory[][1025];

#define cfg_header_error     lang[0]
#define cfg_file_error       lang[1]
#define readmenus_open_error lang[2]
#define change_menu_text     lang[3]
#define no_menu_def          lang[4]
#define argument_text        lang[5]
#define no_menu_defs         lang[6]
#define parameter_check      lang[7]
#define flag_on              lang[8]
#define flag_off             lang[9]
#define expert_str           lang[10]
#define scrclr_str           lang[11]
#define hotkeys_str          lang[12]
#define not_used3            lang[13]
#define not_used4            lang[14]
#define not_used2            lang[15]
#define could_not_write_to   lang[16]
#define illegal_choise       lang[17]
#define severe_error         lang[18]
#define faulty_menu_def      lang[19]
#define missing_lang_file    lang[20]
#define could_not_create     lang[21]
#define no_info_files	 	lang[22]
#define what_info_file		lang[23]
#define what_area		lang[24]
#define go_mailbox		lang[25]
#define unread_texts 		lang[26]
#define no_areas 		lang[27]
#define no_meets		lang[28]
#define option_what_meet	lang[29]
#define whatmeet		lang[30]
#define member_notmember        lang[31]
#define medlem_key		lang[32]
#define uttrad_key		lang[33]
#define no_rights		lang[34]
#define what_flag		lang[35]
#define on_off			lang[36]
#define on_key			lang[37]
#define off_key			lang[38]
#define are_in			lang[39]
#define ANTAL_STRINGS	     39

char *lang[]={
"Fel i konfigurationsfilsheader",
"Fel i konfigurationsfil",
"ReadMenus(): Kunde inte öppna filen",
"Byter till meny:",
"Kan inte hitta den menydefinitionen",
"Argument:",
"Hittar inte några menydefinitioner",
"Du bör se över standardparametrarna i menudef.cfg",
"På",
"Av",
"Expertmode",
"Skärmrensningar",
"Hotkeys",
"E",
"H",
"A",
"FEL: Kunde inte skriva",
"Felaktigt val",
"Något allvarligt fel. prova med KOM-noden istället",
"Den valda menykonfigurationen är felaktig. Välj en annan",
"Den valda språkfilen finns inte. Välj en annan",
"Kunde inte skapa",
"Finns inga info-filer!",
"Vilken Info-fil?",
"Vilken area?",
"Gå till brevlådan. Du har olästa brev!",
"Du har olästa inlägg!",
"Hittar inga filareor!",
"Hittar inga möten!",
"Vilket möte? (RETURN för att se fler):",
"Vilket möte?",
"Bli Medlem eller Utträd (M/U)?",
"M",
"U",
"Du har ingen rätt att utföra det kommandot",
"Vilken flagga vill du ändra?",
"Slå av eller på (A/P)?",
"P",
"A",
"Du befinner dig i: "
};

extern char tempbuffer[], temp[];

int readlang(char *filnamn)
{
	FILE *fp;
	int counter;

        if(filnamn[strlen(filnamn)-1]==10)
                filnamn[strlen(filnamn)-1]=0;
	fp=fopen(filnamn,"r");
	if(fp==NULL)
	{
		puttekn("Can't find the chosen language file\n",-1);
		return(2);
	}
	if(feof(fp))
        {
                fclose(fp);
                return(FALSE);
        }
        fgets(tempbuffer,199,fp);
	if(strncmp(tempbuffer,"#LANGFILE_1",11) != NULL)
	{
		fclose(fp);
		return(FALSE);
	}

	for(counter=0;counter<=ANTAL_STRINGS;counter++)
	{
		if(feof(fp))
		{
			fclose(fp);
			return(FALSE);
		}
		fgets(tempbuffer,199,fp);
		if(strlen(tempbuffer)<2)
			return(FALSE);
		lang[counter]=malloc(sizeof(char)*strlen(tempbuffer));
		strcpy(lang[counter],tempbuffer);
		lang[counter][strlen(tempbuffer)-1]='\0';
	}
	fclose(fp);
	return TRUE;
}


BOOL bytsprak()
{
	FILE *fp;
        int i,j;

	fp=fopen("nikom:datocfg/menudef.cfg","r");
	if(fp==NULL)
	{
		printf(no_menu_def);
		printf("\n");
		return(FALSE);
	}
	while(feof(fp)==NULL && fgets(tempbuffer,199,fp)!=NULL && strncmp(tempbuffer,"#LANGUAGESLIST",14)!=NULL);
	if(strncmp(tempbuffer,"#LANGUAGESLIST",14)==NULL)
	{
		fgets(tempbuffer,199,fp);
		while(tempbuffer[0]!='#' && feof(fp)==NULL)
		{
			puttekn(tempbuffer,-1);
			fgets(tempbuffer,199,fp);
		}
	        getstring(EKO,2,NULL);
       		if(strncmp(tempbuffer,"#LANGUAGES",10)!=NULL)
                while(feof(fp)==NULL && fgets(tempbuffer,199,fp)!=NULL && strncmp(tempbuffer,"#MENUS",6)!=NULL);
        	j=atoi(inmat);
	        for(i=0;i<j;i++)
        	{
                	if(feof(fp)!=NULL)
                       		return (FALSE);
	                fgets( tempbuffer,199,fp);
        	}
        	tempbuffer[strlen(tempbuffer)-1]='\0';
        	readlang(tempbuffer);
        }
        else
        {
        	fclose(fp);
        	return(FALSE);
        }
        fclose(fp);
        writeuserfile();
        return(TRUE);
}