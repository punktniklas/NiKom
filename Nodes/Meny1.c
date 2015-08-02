#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <error.h>
#include <limits.h>

#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "MenyFuncs.h"

#include "MenyStr.h"
#include "sprakstrings.h"


#define READTEXT        1
#define READBREV        2
#define EXPERTMODE      1
#define NOEXPERTMODE    0
#define EKO             1
#define EJEKO           0

extern char outbuffer[], *argument, inmat[],reggadnamn[], dosversion[];
extern struct System *Servermem;
extern int inloggad, radcnt, nodnr, area2, mote2, felcnt;

BOOL readonlymode=FALSE;

char menyarg[100];
char menyinmat[100];
char filnamn[100];
char dirnamn[100];

struct Meny *activemenu, *backupmenu;
struct Meny *menylista=NULL, *numeny=NULL;
struct UserCfg *ucfg;
struct keywordlist *menarg;
struct menykommando *menykommandon=NULL;

char tempbuffer[400], temp[200];


/* freemenus (struct Meny *menu)

        rensar en menydefinition rekursivt.
*/
BOOL freemenus(struct Meny *menu)
{

        struct keywordlist *klist, *klist1;
        struct Meny *menu1;
        int i;

        if(menu==NULL)
                return(TRUE);
        free(menu->name);
        if(menu->menytext!=NULL)
        {
                i=0;
                while((menu->menytext)[i]!=NULL)
                {
                        free((menu->menytext)[i]);
                        i++;
                }

        }
        if(menu->key!=NULL)
        {
                klist=menu->key;
                while(klist!=NULL)
                {
                        if(klist->visastring!=NULL)
                                free(klist->visastring);
                        free(klist->name);
                        free(klist->argument);
                        klist1=klist;
                        klist=klist->next;
                        free(klist1);
                }

        }
        menu1=menu->next;
        free(menu);
        freemenus(menu1);
}

/* readmenus

        Läser in en ny menydefinition. Om det är något fel i den som skall läsas
        in så rensar den bort denna ur minnet och återgår till den som var vald
        innan. Annars så rensar den bort den gamla.
*/
BOOL readmenus(char *filnamn)
{
        int lines, i, j;
        struct Meny *oldmenylista=NULL;
        struct keywordlist *klist;
        FILE *fp;
        char *defarg;

        if(filnamn[strlen(filnamn)-1]==10)
                filnamn[strlen(filnamn)-1]=0;

        if(menylista!=NULL)
        {
                oldmenylista=menylista;
                menylista=NULL;
        }

        if (fp=fopen(filnamn,"r"))
        {
                fgets(tempbuffer,15,fp);
                if(strncmp(tempbuffer,"#MENUDEF_1",10) != NULL)
                {
                        printf("%s\n%s\n",cfg_header_error,tempbuffer);
                        if(oldmenylista!=NULL)
                                menylista=oldmenylista;
                        return(FALSE);
                }

                fgets(tempbuffer,15,fp);
                if(strncmp(tempbuffer,"SHOWMENU",8) != NULL)
                {
                        printf("%s\n%s\n",cfg_header_error,tempbuffer);
                        if(oldmenylista!=NULL)
                                menylista=oldmenylista;
                        return(FALSE);
                }
                else
                {
                        if(menylista==NULL)
                                menylista=numeny=malloc(sizeof(struct Meny));
                        menylista->showmenukey=tempbuffer[9];
                }

                while(!feof(fp) && fgets(tempbuffer,199,fp))
                {
                        if( strncmp( tempbuffer, "NAME=", 5) == 0)  //en ny meny
                        {
                                numeny->next=malloc(sizeof(struct Meny));
                                numeny=numeny->next;
                                numeny->next=NULL;
                                numeny->key=NULL;
                                numeny->name=malloc (strlen(tempbuffer)-3);
                                strcpy(numeny->name,&tempbuffer[5]);
                                fgets (tempbuffer,199,fp);

                                if( strncmp( tempbuffer, "STATUS=", 7) != 0)
                                {
                                        printf("%s\n%s\n",cfg_file_error,tempbuffer);
                                        fclose(fp);
                                        if(oldmenylista!=NULL)
                                        {
                                                freemenus(menylista->next);
                                                menylista=oldmenylista;
                                        }
                                        return(FALSE);
                                }
                                numeny->status=atoi(&tempbuffer[7]);

                                fgets (tempbuffer,199,fp);

                                numeny->startkom=NULL;

                                if( strncmp( tempbuffer, "STARTKOM:", 9) == 0)
                                {
                                        /* kommando skall köra vid start av denna meny */

                                        numeny->startkom=malloc(sizeof(struct keywordlist));

                                        if(feof(fp)) break;
                                        fgets(tempbuffer,199,fp);
                                        if(tempbuffer[0]==' ')
                                        {
                                                numeny->startkom->visastring=malloc(strlen(tempbuffer)+1);
                                                strcpy(numeny->startkom->visastring,&tempbuffer[1]);
                                                continue;
                                        }

                                        numeny->startkom->key=tempbuffer[0];
                                        numeny->startkom->name=malloc(strlen(tempbuffer)+1);
                                        if(tempbuffer[2]=='#')
                                        {
                                                numeny->startkom->arg=tempbuffer[3];
                                                j=5;
                                        }
                                        else
                                        {
                                                numeny->startkom->arg='Y';
                                                j=2;
                                        }
                                        defarg=strchr(&tempbuffer[j],' ');
                                        numeny->startkom->argument=NULL;
                                        if(defarg!=NULL)
                                        {
                                                numeny->startkom->argument=malloc(strlen(defarg)+1);
                                                if(defarg[strlen(defarg)-1]=='\n')
                                                        defarg[strlen(defarg)-1]='\0';
                                                strcpy(numeny->startkom->argument,&defarg[1]);
                                                defarg[0]='\0';
                                        }
                                        strcpy(numeny->startkom->name,&tempbuffer[j]);

                                        fgets(tempbuffer,199,fp);
                                }


                                numeny->visamote=FALSE;
                                if(strncmp(tempbuffer,"VISAMOTE",8)==0)
                                {
                                        numeny->visamote=TRUE;
                                        fgets(tempbuffer,199,fp);
                                }

                                numeny->visaarea=FALSE;
                                if(strncmp(tempbuffer,"VISAAREA",8)==0)
                                {
                                        numeny->visaarea=TRUE;
                                        fgets(tempbuffer,199,fp);
                                }

                                if( strncmp( tempbuffer, "RETURN=", 7) == 0)
                                {
                                        if(strncmp(&tempbuffer[7],"READTEXT",8) == 0)
                                                numeny->ret=READTEXT;
                                        else if(strncmp(&tempbuffer[7],"READBREV",8) == 0)
                                                numeny->ret=READBREV;
                                        else

                                                numeny->ret=0;
                                        fgets(tempbuffer,199,fp);
                                }
                                else numeny->ret=0;

                                if( strncmp( tempbuffer, "LINES=", 6) != 0)
                                {
                                        printf("%s\n%s\n",cfg_file_error, tempbuffer);
                                        fclose(fp);
                                        if(oldmenylista!=NULL)
                                        {
                                                freemenus(menylista->next);
                                                menylista=oldmenylista;
                                        }
                                        return(FALSE);
                                }
                                lines=atoi(&tempbuffer[6]);
                                numeny->menytext=malloc(sizeof(void *)*(lines+1));

                                for( i=0;i<lines;i++)
                                {
                                        fgets(tempbuffer,199,fp);
                                        (numeny->menytext)[i]=malloc(strlen(tempbuffer)+1);
                                        strcpy((numeny->menytext)[i],tempbuffer);
                                }
                                (numeny->menytext)[i]=malloc(1);
                                (numeny->menytext)[i]=NULL;

                                numeny->key=NULL;
                                klist=numeny->key;

                                for(;;)
                                {
                                        if(feof(fp)) break;
                                        fgets(tempbuffer,199,fp);
                                        if(tempbuffer[0]=='@' && tempbuffer[1]=='@')
                                                break;
                                        if(tempbuffer[0]==' ')
                                        {
                                                klist->visastring=malloc(strlen(tempbuffer)+1);
                                                strcpy(klist->visastring,&tempbuffer[1]);
                                                continue;
                                        }

                                        if(klist==NULL)
                                        {
                                                klist=numeny->key=malloc(sizeof(struct keywordlist));
                                                klist->next=NULL;
                                                klist->visastring=NULL;
                                        }
                                        else
                                        {
                                                klist->next=malloc(sizeof(struct keywordlist));
                                                klist=klist->next;
                                                klist->visastring=NULL;
                                                klist->next=NULL;
                                        }
                                        klist->key=tempbuffer[0];
                                        klist->name=malloc(strlen(tempbuffer)+1);
                                        if(tempbuffer[2]=='#')
                                        {
                                                klist->arg=tempbuffer[3];
                                                j=5;
                                        }
                                        else
                                        {
                                                klist->arg='Y';
                                                j=2;
                                        }
                                        defarg=strchr(&tempbuffer[j],' ');
                                        klist->argument=NULL;
                                        if(defarg!=NULL)
                                        {
                                                klist->argument=malloc(strlen(defarg)+1);
                                                if(defarg[strlen(defarg)-1]=='\n')
                                                        defarg[strlen(defarg)-1]='\0';
                                                strcpy(klist->argument,&defarg[1]);
                                                defarg[0]='\0';
                                        }
                                        strcpy(klist->name,&tempbuffer[j]);
                                }
                        }
                }
        }
        else
        {
                printf("%s\n", readmenus_open_error);
                printf(" %s\n", filnamn);
                if(oldmenylista!=NULL)
                {
                        freemenus(menylista->next);
                        menylista=oldmenylista;
                }
                return(FALSE);
        }
        fclose(fp);
        if(oldmenylista!=NULL)
        {
                freemenus(oldmenylista->next);
        }
        return(TRUE);
}

/* printmenutext()

        Skriver ut en meny på skärmen. tar hänsyn till om expertmode är valt,
        och skriver då endast ut en rad med alla val.
*/
void printmenutext(struct Meny *menu, int expmode)
{
        int i=0;
        struct keywordlist *klist;

        klist=menu->key;
        puttekn("\r",-1);
        if(expmode==NOEXPERTMODE)
        {
                while( (menu->menytext)[i] !=NULL)
                {
                        sprintf(tempbuffer,"%s\r",(menu->menytext)[i]);
                        puttekn(tempbuffer,-1);
                        i++;
                }
        }
        else
        {
                while(klist!=NULL)
                {
                        outbuffer[i]=klist->key;
                        outbuffer[i+1]=',';
                        klist=klist->next;
                        i=i+2;
                }
                outbuffer[i]=' ';
                outbuffer[i+1]=menylista->showmenukey;
                outbuffer[i+2]='\r';
                outbuffer[i+3]='\n';
                outbuffer[i+4]='\0';
                puttekn(outbuffer,-1);
        }
}

/* checkchoise()

        matchar en tangent med en lista med möjliga val.
        returnerar namnet/nummret på det som valts.
*/
char * checkchoise(struct keywordlist *klist,char choise)
{

        choise=ToUpper(choise);
        while(klist!=NULL)
        {
                if(klist->key==choise)
                        return klist->name;
                klist=klist->next;
        }
        return NULL;
}

/* checkarg()

        Letar reda på argumentlistan till ett val
*/
void checkarg(struct keywordlist *klist,char choise)
{
        choise=ToUpper(choise);
        while(klist!=NULL)
        {
                if(klist->key==choise)
                {
                        menarg=klist;
                        return;
                }
                klist=klist->next;

        }
        return;
}

/*      changemenu()

        byter aktiv meny
*/
struct Meny *changemenu( struct Meny *menu,
            char *menynamn,
            BOOL status)
{
        if(ucfg->scrclr==TRUE)
                puttekn("\x1b[2J \x1b[0;0H",-1);
/*        puttekn(change_menu_text,-1);
        sprintf(tempbuffer," %s",menynamn);
        puttekn(tempbuffer,-1);
*/
        while(menu!=NULL)
        {
                if(menu!=menylista)
                {
                        if(strcmp(menynamn,menu->name)==0)
                        {
                                if(menu->status<=Servermem->inne[nodnr].status)
                                {
                                        if(menu->ret==READBREV)
                                                mote2=-1;
                                        return menu;
                                }
                        }
                }
                menu=menu->next;
        }
        puttekn("\r\n",-1);
        sprintf(tempbuffer,no_menu_def);
        puttekn(tempbuffer,-1);
        puttekn("\r\n",-1);
        return 0;
}

/*      setmenyarg()

        Sätter argument till en funktion. Kollar först om kommandot
        skall ha ett argument, och letar sen efter M-fältet. Om detta
        inte finns så skrivs ARGUMENT: ut.
*/
int setmenyarg(int funknum, char *visastring)
{
        if(visastring!=NULL)
                sprintf(outbuffer,"\r\n%s\r\n",visastring);
        else
        {
                puttekn("\r\n",-1);
                sprintf(outbuffer,"%s",argument_text);
        }
        puttekn(outbuffer,-1);
        if(getstring(EKO,99,NULL)) return(1);
        strcpy(menyarg,inmat);
}


/*      menykmd()

        Menynodens motsvarighet till dokmd(). Tar hand om alla funktioner
        som är unika för menynoden, skickar annars vidare till dokmd() för
        vidare behandling.
*/
int menykmd(int funknum, int kmd,int menystatus)
{

    if(menyparse(funknum, kmd, menystatus) == 0)
        {
                felcnt = 0;
                return(-5);
        }

        if(funknum==10) { if(bytmenydef()) return(-8);}
        else if(funknum==11) { if(menyflaggor()) return(-8);}
        else if(funknum==12) bytsprak();
        else if(funknum==13) { if(visainfomeny()) return(-8); }
        else if(funknum==14) return(cdmeny());
        else if(funknum==15) varmotemeny(mote2);
        else if(funknum==16) { if(medlemmeny()) return(-8); }
        else if(funknum==17) return(gameny());
        else if(funknum==99) aboutmeny();
//      else if(funknum==18) adt(2419200);
        else if(funknum==19) { readonlymode=TRUE; }
        else return(dokmd(funknum, kmd));

        return(dokmd(-1*funknum, kmd));

}

int menyparse(int funknum, int kmd,int menystatus)
{
    struct Kommando *kompek;

        struct menykommando *komtemp;
        long tid;

        if( funknum < 0 )
                funknum = funknum * -1;

// Först kollar vi om kommandot får utföras, denna
// koll utförs endast för kommandon som är menykommandon

        komtemp=menykommandon;
        if(komtemp!=NULL)
        {
                while(komtemp!=NULL)
                {
                        if(komtemp->nummer==funknum)
                        {
                                if( (komtemp->status > Servermem->inne[nodnr].status)
                                        || (komtemp->minlogg > Servermem->inne[nodnr].loggin)
                                        || (komtemp->mindays*86400
                                                > time(&tid)-Servermem->inne[nodnr].forst_in)
                                        || (komtemp->grupper
                                                && !(komtemp->grupper
                                                        & Servermem->inne[nodnr].grupper)))
                                {
                                        sprintf(outbuffer,"\r\n%s\r\n",no_rights);
                                        puttekn(outbuffer,-1);
                                        felcnt = 0;
                                        return(0);
                                }
                                else if( komtemp->losen[0] )
                                {
                        puttekn("\r\n\nLösen: ",-1);
						if(Servermem->inne[nodnr].flaggor & STAREKOFLAG)
	                        getstring(STAREKO,20,NULL);
						else
							getstring(EJEKO,20,NULL);
                        if(strcmp(komtemp->losen,inmat)) return(0);
                        return(1);
                                }
                                else return(1);
                        }
                        komtemp=komtemp->next;
                }
        }


    for(kompek=(struct Kommando *)Servermem->kom_list.mlh_Head
        ;kompek->kom_node.mln_Succ;kompek
        =(struct Kommando *)kompek->kom_node.mln_Succ)
    {
        if(kompek->nummer == funknum)
        {
            if( (kompek->status > Servermem->inne[nodnr].status)
                || (kompek->minlogg > Servermem->inne[nodnr].loggin)
                || (kompek->mindays*86400 > time(&tid)-Servermem->inne[nodnr].forst_in)
                || (kompek->grupper && !(kompek->grupper & Servermem->inne[nodnr].grupper)) )
                        {
                                sprintf(outbuffer,"\r\n%s\r\n",no_rights);
                                puttekn(outbuffer,-1);
                                felcnt = 0;
                                return(0);
                        }
                        else if( kompek->losen[0] )
                        {
                puttekn("\r\n\nLösen: ",-1);
                if(Servermem->inne[nodnr].flaggor & STAREKOFLAG)
	                getstring(STAREKO,20,NULL);
				else
					getstring(EJEKO,20,NULL);
                if(strcmp(kompek->losen,inmat)) return(0);
                return(1);
                        }
            return(1);
        }
    }
        return(0);
}


/*      bytmenydef()

        Låter användaren välja vilken menydefinition som skall köras.
*/
int bytmenydef()
{
        int i, j;
        FILE *fp;
        BOOL status;

        if(ucfg->scrclr==TRUE)
                puttekn("\x1b[2J \x1b[0;0H",-1);

        puttekn("\r\n",-1);
        fp=fopen("nikom:datocfg/menudef.cfg","r");
        if(fp==NULL)
        {
                printf(no_menu_defs);
                printf("\n");
                return(0);
        }
        while(feof(fp)==NULL && fgets(tempbuffer,199,fp)!=NULL && strncmp(tempbuffer,"#MENUTEXT",9)!=NULL);
        if(feof(fp)!=NULL)
                return (1);
        while(feof(fp)==NULL && fgets(tempbuffer,199,fp)!=NULL && tempbuffer[0]!='#')
        {
                puttekn("\r",-1);
                puttekn(tempbuffer,-1);
        }
        puttekn("\r",-1);
        if(getstring(EKO,2,NULL)) return(1);
        if(strncmp(tempbuffer,"#MENUS",6)!=NULL)
                 while(feof(fp)==NULL && fgets(tempbuffer,199,fp)!=NULL && strncmp(tempbuffer,"#MENUS",6)!=NULL);
        j=atoi(inmat);
        if(j==0) return(0);
        for(i=0;i<j;i++)
        {
                if(feof(fp)!=NULL)
                       return (0);
                fgets( tempbuffer,199,fp);
                if(strncmp(tempbuffer,"#DEFUSERCFG",11)==NULL)
                        return(0);
        }
        free(ucfg->name);
        ucfg->name=malloc(strlen(tempbuffer)+3);
        tempbuffer[strlen(tempbuffer)-1]='\0';
        strcpy(ucfg->name,tempbuffer);
        status=readmenus(ucfg->name);
        if(status==TRUE)
        {
                activemenu=changemenu(menylista,"MAIN\n", FALSE);
                menyinmat[0]=0;
                writeuserfile();
        }
        fclose(fp);
        return(0);
}


/*      readuserconfig()

        Läser in användarens config-gil. Om denna inte finns så läses standard-
        inställningarna in istället.
*/
BOOL readuserconfig(int usernum)
{
        FILE *fp;
        BOOL status;

        ucfg->name=malloc(255);
        ucfg->sprak=malloc(255);
        ucfg->name[0]='\0';
        ucfg->sprak[0]='\0';
        ucfg->hotkeys=FALSE;
        ucfg->expert=NOEXPERTMODE;
        ucfg->scrclr=TRUE;

        sprintf(filnamn,"NiKom:Users/%d/%d/menycfg",inloggad/100,inloggad);
        if((fp=fopen(filnamn,"r"))==NULL)
        {
                status=readdefuserconf(usernum);
                return(status);
        }
        status=readuserfile(fp);
        fclose(fp);
        if(status==FALSE)
        {
                status=readdefuserconf(usernum);
                if(status==FALSE)
                        return(FALSE);
                fp=fopen(filnamn,"r");
                readuserfile(fp);
                fclose(fp);
        }
        return(TRUE);
}

/*      readdefuserconf()

        Läser in standardinställningarna för användare.
*/
BOOL readdefuserconf(int usernum)
{
        FILE *fp;
        BOOL status;

        fp=fopen("nikom:datocfg/menudef.cfg","r");
        if(fp==NULL)
        {
                printf(no_menu_def);
                printf("\n");
                return(FALSE);
        }
        while(feof(fp)==NULL && fgets(tempbuffer,199,fp)!=NULL && strncmp(tempbuffer,"#DEFUSERCFG",11)!=NULL);
        if(strncmp(tempbuffer,"#DEFUSERCFG",11)==NULL)
        {
                status=readuserfile(fp);
                if(status==FALSE)
                        printf(parameter_check);
                        printf("\n");

        }
        else
        {
                fclose(fp);
                return(FALSE);
        }
        fclose(fp);
        return(TRUE);
}

/*      readuserfile()

        Tar en öppnad fil som argument, och läser in användardefinition ur denna.
        Filpekaren skall vara framläst till det avsnitt där användardatat ligger i filen.
*/
BOOL readuserfile(FILE *fp)
{
        char tmp[200];
        if((fgets(tmp,3,fp))==NULL)
                return(FALSE);
        if(tmp[0]=='#' || strlen(tmp)<2)
                return(FALSE);
        if(tmp[0]=='y' || tmp[0]=='Y')
                ucfg->hotkeys=TRUE;
        if((fgets(tmp,200,fp))==NULL)
                return(FALSE);
        if(tmp[0]=='#' || strlen(tmp)<2)
                return(FALSE);
        strcpy(ucfg->name,tmp);
        if((fgets(tmp,3,fp))==NULL)
                return(FALSE);
        if(tmp[0]=='#' || strlen(tmp)<2)
                return(FALSE);
        if(tmp[0]=='y' || tmp[0]=='Y')
                ucfg->expert=EXPERTMODE;

        if((fgets(tmp,3,fp))==NULL)
                return(FALSE);
        if(tmp[0]=='#' || strlen(tmp)<2)
                return(FALSE);
        if(tmp[0]=='y' || tmp[0]=='Y')
                ucfg->scrclr=TRUE;

        if((fgets(tmp,200,fp))==NULL)
                return(FALSE);
        if(tmp[0]=='#' || strlen(tmp)<2)
                return(FALSE);
        strcpy(ucfg->sprak,tmp);
        return(TRUE);
}



/*      writeuserfile()

        Skriver användarens definitionsfil.
*/
void writeuserfile()
{
        FILE *fp;

        sprintf(filnamn,"NiKom:Users/%d/%d/menycfg",inloggad/100,inloggad);
        fp=fopen(filnamn,"w");
        if(fp==NULL)
        {
                sprintf(filnamn,"NiKom:Users/%d/%d/menycfg",inloggad/100,inloggad);
                fp=fopen(filnamn,"w");
                if(fp==NULL)
                {
                        sprintf(outbuffer,"%s %s\n",could_not_write_to, filnamn);
                        puttekn(outbuffer,-1);
                        return;
                }
        }
        if(ucfg->hotkeys==TRUE)
                fprintf(fp,"Y\n");
        else
                fprintf(fp,"N\n");

        fprintf(fp,"%s\n",ucfg->name);

        if(ucfg->expert==EXPERTMODE)
                fprintf(fp,"Y\n");
        else
                fprintf(fp,"N\n");

        if(ucfg->scrclr==TRUE)
                fprintf(fp,"Y\n");
        else
                fprintf(fp,"N\n");


        fprintf(fp,"%s\n",ucfg->sprak);
        fclose(fp);
}


int visainfomeny(void)
{
        __aligned struct FileInfoBlock mininfo;
        int error;
        int nr=0, val;
        BOOL infostatus=FALSE;
        struct enkellista *fillista, *firstfile;
        char filnamn[100],pattern[100];
        sprintf(pattern,"NiKom:texter/#?.txt");
        if(error=dfind(&mininfo,pattern,0))
        {
                puttekn("\r\n\n",-1);
                puttekn(no_info_files,-1);
                puttekn("\r\n",-1);
                return(0);
        }
        fillista=firstfile=malloc(sizeof(struct enkellista));
        fillista->next=NULL;
        if(strlen(&(mininfo.fib_Comment)[0])>0)
        {
                fillista->name=malloc(strlen(mininfo.fib_FileName)+1);
                strcpy(fillista->name,&(mininfo.fib_FileName)[0]);
                sprintf(filnamn,"%-3d%-25s%s\n",nr,fillista->name, &(mininfo.fib_Comment)[0]);
                puttekn(filnamn,-1);
                puttekn("\r",-1);
                infostatus=TRUE;
        }
        while((error=dnext(&mininfo))==0)
        {
                if(strlen(&(mininfo.fib_Comment)[0])>0)
                {
                        if(infostatus!=FALSE)
                        {
                                nr++;
                                fillista->next=malloc(sizeof(struct enkellista));
                                fillista=fillista->next;
                                fillista->next=NULL;
                        }
                        infostatus=TRUE;
                        fillista->name=malloc(strlen(mininfo.fib_FileName)+1);
                        strcpy(fillista->name,mininfo.fib_FileName);
                        sprintf(filnamn,"%-3d%-25s%s\n\r",nr,fillista->name, &(mininfo.fib_Comment)[0]);
                        puttekn(filnamn,-1);
                }
        }
        puttekn("\r\n",-1);
        if(infostatus==FALSE)
        {
                puttekn(no_info_files,-1);
                puttekn("\r\n",-1);
                deallocinfolista(firstfile);
                return(0);
        }
        puttekn(what_info_file,-1);
        puttekn(" ",-1);
        if(getstring(EKO,2,NULL))
        {
                deallocinfolista(firstfile);
                return(1);
        }
        val=atoi(inmat);
        if(val>nr || val<0 || inmat[0]<'0' || inmat[0]>'9')
        {
                puttekn("\r\n",-1);
                puttekn(illegal_choise,-1);
                puttekn("\r\n",-1);
                deallocinfolista(firstfile);
                return(0);
        }
        fillista=firstfile;
        for(nr=0;nr<val;nr++)
                fillista=fillista->next;
        sprintf(filnamn,"NiKom:Texter/%s",fillista->name);
        sendfile(filnamn);
        deallocinfolista(firstfile);
        return(0);
}

void deallocinfolista(struct enkellista *firstfile)
{
        struct enkellista *foo;

        while(firstfile->next!=NULL)
        {
                foo=firstfile->next;
                free(firstfile->name);
                free(firstfile);
                firstfile=foo;
        }
        free(firstfile->name);
        free(firstfile);
}


int cdmeny(void) {
        int x, nr=0, val;

        puttekn("\r\n\n",-1);
        for(x=0;x<Servermem->info.areor;x++) {
                if(!Servermem->areor[x].namn[0] || !arearatt(x, inloggad, &Servermem->inne[nodnr])) continue;
                sprintf(outbuffer,"%-3d%s",nr,Servermem->areor[x].namn);
                puttekn(outbuffer,-1);
                nr++;
                if(Servermem->areor[x].flaggor & AREA_NOUPLOAD) puttekn(" (Ingen upload)",-1);
                if(Servermem->areor[x].flaggor & AREA_NODOWNLOAD) puttekn(" (Ingen download)",-1);
                puttekn("\n\r",-1);
        }
        eka('\n');

        if(nr==0)
        {
                puttekn(no_areas,-1);
                puttekn("\r\n",-1);
                return(0);
        }
        puttekn(what_area,-1);
        puttekn(" ",-1);
        if(getstring(EKO,2,NULL)) return(-8);
        val=atoi(inmat);
        if(val>nr-1 || val<0 || inmat[0]<'0' || inmat[0]>'9')
        {
                puttekn("\r\n",-1);
                puttekn(illegal_choise,-1);
                puttekn("\r\n",-1);
                return(0);
        }
        nr=0;
        for(x=0;x<Servermem->info.areor;x++)
        {
                if(!Servermem->areor[x].namn[0] || !arearatt(x, inloggad, &Servermem->inne[nodnr])) continue;
                if(nr==val)
                {
                        area2=x;
                        sprintf(outbuffer,"\r\n\nDu befinner dig nu i arean %s.\n\r",Servermem->areor[x].namn);
                        puttekn(outbuffer,-1);
                        if(Servermem->areor[x].flaggor & AREA_NOUPLOAD) puttekn(" (Ingen upload)",-1);
                        if(Servermem->areor[x].flaggor & AREA_NODOWNLOAD) puttekn(" (Ingen download)",-1);
                        puttekn("\n\r",-1);
                }
                nr++;
        }
        return(0);
}


void varmotemeny(int mote) {
        int cnt;
        struct Mote *motpek;
        motpek = getmotpek(mote);

        if(mote==-1) varmail();

        sprintf(outbuffer,"\r\n\nDu befinner dig i %s.\r\n",motpek->namn);
        puttekn(outbuffer,-1);

        if(motpek->type == MOTE_FIDO) {
                if(motpek->lowtext > motpek->texter) strcpy(outbuffer,"Det finns inga texter.\n\r");
                else sprintf(outbuffer,"Det finns texter numrerade från %d till %d.\n\r",motpek->lowtext,motpek->texter);
                puttekn(outbuffer,-1);
        }
        cnt=countunread(mote);
        if(!cnt) puttekn("Du har inga olästa texter\r\n",-1);
        else if(cnt==1) {
                puttekn("Du har 1 oläst text\r\n",-1);
        } else {
                sprintf(outbuffer,"Du har %d olästa texter\r\n",cnt);
                puttekn(outbuffer,-1);
        }
}





int medlemmeny(void)
{
        struct Mote *listpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        int nr=0, val, radantal=0;
        BOOL medlemstatus=FALSE;
        char muval;


        for(;listpek->mot_node.mln_Succ;listpek=(struct Mote *)listpek->mot_node.mln_Succ)
        {
                if((listpek->status & SUPERHEMLIGT) && Servermem->inne[nodnr].status < Servermem->cfg.st.medmoten)
                        continue;
                if(listpek->status & HEMLIGT)
                        if(!MayBeMemberConf(listpek->nummer, inloggad, &Servermem->inne[nodnr]))
                                continue;
                if((Servermem->inne[nodnr].rader)==radantal+2)
                {
                        puttekn(option_what_meet,-1);
                        puttekn(" ",-1);
                        if(getstring(EKO,2,NULL)) return(1);
                        val=atoi(inmat);
                        if(strlen(inmat)==0)
                                radantal=0;
                        else if(val>nr-1 || val<0 || inmat[0]<'0' || inmat[0]>'9')
                        {
                                puttekn("\r\n",-1);
                                puttekn(illegal_choise,-1);
                                puttekn("\r\n",-1);
                                return(0);
                        }
                        else
                        {
                                medlemstatus=TRUE;
                                break;
                        }


                }
                if(IsMemberConf(listpek->nummer, inloggad, &Servermem->inne[nodnr]))
                        sprintf(outbuffer,"%-3d%3d %s ",nr, countunread(listpek->nummer),listpek->namn);
                else
                        sprintf(outbuffer,"%-3d   *%s ",nr, listpek->namn);
                if(puttekn(outbuffer,-1)) return(0);
                if(listpek->status & SLUTET) if(puttekn(" (Slutet)",-1)) return(0);
                if(listpek->status & SKRIVSKYDD) if(puttekn(" (Skrivskyddat)",-1)) return(0);
                if(listpek->status & KOMSKYDD) if(puttekn(" (Kom.skyddat)",-1)) return(0);
                if(listpek->status & HEMLIGT) if(puttekn(" (Hemligt)",-1)) return(0);
                if(listpek->status & AUTOMEDLEM) if(puttekn(" (Auto)",-1)) return(0);
                if(listpek->status & SKRIVSTYRT) if(puttekn(" (Skrivstyrt)",-1)) return(0);
                if((listpek->status & SUPERHEMLIGT) && Servermem->inne[nodnr].status >= Servermem->cfg.st.medmoten) if(puttekn(" (ARexx-styrt)",-1)) return(0);
                if(puttekn("\r\n",-1)) return(0);
                nr++;
                radantal++;
        }
        if(nr==0)
        {
                puttekn(no_meets,-1);
                puttekn("\r\n",-1);
                return(0);
        }
        if(medlemstatus==FALSE)
        {
                puttekn(whatmeet,-1);
                puttekn(" ",-1);
                if(getstring(EKO,2,NULL)) return(1);
                val=atoi(inmat);
        }
        if(val>nr-1 || val<0 || inmat[0]<'0' || inmat[0]>'9')
        {
                puttekn("\r\n",-1);
                puttekn(illegal_choise,-1);
                puttekn("\r\n",-1);
                return(0);
        }
        puttekn("\r\n",-1);
        puttekn(member_notmember,-1);
        if(getstring(EKO,2,NULL)) return(1);
        muval=ToUpper(inmat[0]);

        if(muval==medlem_key[0] || muval==uttrad_key[0])
        {
                nr=0;
                listpek=(struct Mote *)Servermem->mot_list.mlh_Head;

                for(;listpek->mot_node.mln_Succ;listpek=(struct Mote *)listpek->mot_node.mln_Succ)
                {
                        if((listpek->status & SUPERHEMLIGT) && Servermem->inne[nodnr].status < Servermem->cfg.st.medmoten)
                                continue;
                        if(listpek->status & HEMLIGT)
                                if(!MayBeMemberConf(listpek->nummer, inloggad, &Servermem->inne[nodnr]))
                                        continue;
                        if(nr==val)
                        {
                                if(muval==uttrad_key[0])
                                        uttrad(listpek->namn);
                                else
                                        medlem(listpek->namn);
                                return(0);
                        }
                        nr++;
                }
                return(0);
        }
        puttekn("\r\n",-1);
        puttekn(illegal_choise,-1);
        puttekn("\r\n",-1);
        return(0);
}





int gameny(void)
{
        int motnr;

        struct Mote *listpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        int cnt;
        int nr=0, val, radantal=0;
        BOOL medlemstatus=FALSE;

        for(;listpek->mot_node.mln_Succ;listpek=(struct Mote *)listpek->mot_node.mln_Succ)
        {
                if((listpek->status & SUPERHEMLIGT) && Servermem->inne[nodnr].status < Servermem->cfg.st.medmoten)
                        continue;
                if(listpek->status & HEMLIGT)
                        if(!MayBeMemberConf(listpek->nummer, inloggad, &Servermem->inne[nodnr]))
                                continue;
                if((Servermem->inne[nodnr].rader)==radantal+2)
                {
                        puttekn(option_what_meet,-1);
                        puttekn(" ",-1);
                        if(getstring(EKO,2,NULL)) return(-8);
                        val=atoi(inmat);
                        if(strlen(inmat)==0)
                                radantal=0;
                        else if(val>nr-1 || val<0 || inmat[0]<'0' || inmat[0]>'9')
                        {
                                puttekn("\r\n",-1);
                                puttekn(illegal_choise,-1);
                                puttekn("\r\n",-1);
                                return(-5);
                        }
                        else
                        {
                                medlemstatus=TRUE;
                                break;
                        }


                }
                if(IsMemberConf(listpek->nummer, inloggad, &Servermem->inne[nodnr]))
                {
                        sprintf(outbuffer,GREEN "%-3d" DEFAULT "%3d %s ",nr, countunread(listpek->nummer),listpek->namn);
                        if(puttekn(outbuffer,-1)) return(-5);
                        if(listpek->status & SLUTET)
                        {
                                sprintf(outbuffer,RED " (Slutet)" DEFAULT);
                                if(puttekn(outbuffer,-1)) return(-5);
                        }
                        if(listpek->status & SKRIVSKYDD)
                        {
                                sprintf(outbuffer,GREEN " (Skrivskyddat)" DEFAULT);
                                if(puttekn(outbuffer,-1)) return(-5);
                        }

                        if(listpek->status & KOMSKYDD)
                        {
                                sprintf(outbuffer,ORANGE " (Kom.skyddat)" DEFAULT);
                                if(puttekn(outbuffer,-1)) return(-5);
                        }
                        if(listpek->status & HEMLIGT)
                        {
                                sprintf(outbuffer,RED " (Hemligt)" DEFAULT);
                                if(puttekn(outbuffer,-1)) return(-5);
                        }
                        if(listpek->status & AUTOMEDLEM)
                        {
                                sprintf(outbuffer,RED " (Auto)" DEFAULT);
                                if(puttekn(outbuffer,-1)) return(-5);
                        }
                        if(listpek->status & SKRIVSTYRT)
                        {
                                sprintf(outbuffer,GREEN " (Skrivstyrt)" DEFAULT);
                                if(puttekn(outbuffer,-1)) return(-5);
                        }

                        if((listpek->status & SUPERHEMLIGT) && Servermem->inne[nodnr].status >= Servermem->cfg.st.medmoten)
                        {
                                sprintf(outbuffer, ORANGE " (ARexx-styrt)" DEFAULT);
                                if(puttekn(outbuffer,-1)) return(-5);
                        }
                                if(puttekn("\r\n",-1)) return(-5);

                        nr++;
                        radantal++;
                }
        }
        if(nr==0)
        {
                sprintf(outbuffer,RED "%s" DEFAULT, no_meets);
                puttekn(outbuffer,-1);
                return(-5);
        }
        if(medlemstatus==FALSE)
        {
                sprintf(outbuffer,RED "%s" DEFAULT " ", whatmeet);
                puttekn(outbuffer,-1);
                if(getstring(EKO,2,NULL)) return(-8);
                val=atoi(inmat);
        }
        if(val>nr-1 || val<0 || inmat[0]<'0' || inmat[0]>'9')
        {
                puttekn("\r\n",-1);
                sprintf(outbuffer,GREEN "%s" DEFAULT "\r\n", illegal_choise);
                puttekn(outbuffer,-1);
                return(-5);
        }
        puttekn("\r\n",-1);

        nr=0;
        listpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        for(;listpek->mot_node.mln_Succ;listpek=(struct Mote *)listpek->mot_node.mln_Succ)
        {
                if((listpek->status & SUPERHEMLIGT) && Servermem->inne[nodnr].status < Servermem->cfg.st.medmoten)
                        continue;
                if(listpek->status & HEMLIGT)
                        if(!MayBeMemberConf(listpek->nummer, inloggad, &Servermem->inne[nodnr]))
                                continue;
                if(IsMemberConf(listpek->nummer, inloggad, &Servermem->inne[nodnr]))
                {
                        if(nr==val)
                        {
                                motnr=parsemot(listpek->namn);
                                if(activemenu->ret!=READTEXT)
                                {

                                        sprintf(outbuffer,"\r\n\nDu befinner dig i" GREEN " %s" DEFAULT ".\r\n",listpek->namn);
                                        puttekn(outbuffer,-1);
                                        cnt=countunread(motnr);
                                        if(!cnt)
                                        {
                                                sprintf(outbuffer,"Du har" GREEN " inga" DEFAULT " olästa texter\r\n");
                                                puttekn(outbuffer,-1);
                                        }
                                        else if(cnt==1)
                                        {
                                                sprintf(outbuffer,"Du har" GREEN " 1" DEFAULT " oläst text\r\n");
                                                puttekn(outbuffer,-1);
                                        }
                                        else
                                        {
                                                sprintf(outbuffer,"Du har" GREEN " %d" DEFAULT " olästa texter\r\n",cnt);
                                                puttekn(outbuffer,-1);
                                        }

                                }
                                return(motnr);
                        }
                        nr++;
                }
        }
        return(-5);
}



BOOL menygetkmd(void)
{
        BPTR fh;
        struct menykommando *komtemp;
        char buffer[100],*pekare;
        int going=TRUE, grupp;
        if(!(fh=Open("NiKom:DatoCfg/menudef.cfg",MODE_OLDFILE)))  return(FALSE);
        buffer[0]=0;

        while(buffer[0]!='#' || buffer[1]!='C')
        {
                if(!(pekare=FGets(fh,buffer,99)))
                {
                        if(!IoErr())
                        {
                                Close(fh);
                                printf("test1\n");
                                menykommandon=NULL;
                                menykommandon->next=NULL;
                                return(TRUE);
                        }
                        else
                        {
                                printf("Error while reading menudef.cfg\n");
                                Close(fh);
                                return(FALSE);
                        }
                }
        }
        menykommandon=malloc(sizeof(struct menykommando));
        menykommandon->next=NULL;
        menykommandon->nummer=-4711;

        komtemp=menykommandon;
        while(going) {
                if(!(pekare=FGets(fh,buffer,99)))
                {
                        if(!IoErr()) break;
                        else
                        {
                                printf("Error while reading menudef.cfg\n");
                                Close(fh);
                                return(FALSE);
                        }
                }
                switch(buffer[0]) {
                        case 'S' :
                                komtemp->status=atoi(&buffer[2]);
                                break;
                        case 'L' :
                                komtemp->minlogg=atoi(&buffer[2]);
                                break;
                        case 'D' :
                                komtemp->mindays=atoi(&buffer[2]);
                                break;
                        case 'X' :
                                strncpy(komtemp->losen,&buffer[2],19);
                                komtemp->losen[strlen(komtemp->losen)-1]=0;
                                break;
                        case 'W' :
                                strncpy(komtemp->vilkainfo,&buffer[2],49);
                                komtemp->vilkainfo[strlen(komtemp->vilkainfo)-1]=0;
                                break;
                        case 'F' :
                                strncpy(komtemp->logstr,&buffer[2],49);
                                komtemp->logstr[strlen(komtemp->logstr)-1]=0;
                                break;
                        case '(' :
                                komtemp->before=atoi(&buffer[2]);
                                break;
                        case ')' :
                                komtemp->after=atoi(&buffer[2]);
                                break;
                        case 'G' :
                                grupp = parsegrupp(&buffer[2]);
                                if(grupp == -1) printf("I menudef.cfg: Okänd grupp %s\n",&buffer[2]);
                                else BAMSET((char *)&komtemp->grupper,grupp);
                                break;
                        case '#' :
                                komtemp->next=malloc(sizeof(struct menykommando));
                                komtemp=komtemp->next;
                                komtemp->nummer=atoi(&buffer[2]);
                                komtemp->status=0;
                                komtemp->minlogg=0;
                                komtemp->mindays=0;
                                komtemp->before=0;
                                komtemp->after=0;
                                komtemp->losen[0]='\0';
                                komtemp->vilkainfo[0]='\0';
                                komtemp->logstr[0]='\0';
                                komtemp->next=NULL;
                                komtemp->grupper=0;
                                break;

                }
        }
        Close(fh);
        return(TRUE);
}

struct menykommando *menygetkmdpek(int kmdnr)
{
        struct menykommando *komtemp;

        komtemp=menykommandon;

        while(komtemp!=NULL)
        {
                if(komtemp->nummer==kmdnr) return(komtemp);
                komtemp=komtemp->next;
        }
        return(NULL);
}



int menyflaggor(void)
{
        int x, flaggnr=0, val;
        char onoffval;

        puttekn("\r\n\n",-1);
        for(x=31;x>(31-ANTFLAGG);x--)
        {
                sprintf(outbuffer,GREEN "%-3d" DEFAULT,flaggnr);
                puttekn(outbuffer,-1);
                flaggnr++;
                if(BAMTEST((char *)&Servermem->inne[nodnr].flaggor,x)) puttekn("<På>  ",-1);
                else puttekn("<Av>  ",-1);
                sprintf(outbuffer,"%s\r\n",Servermem->flaggnamn[31-x]);
                puttekn(outbuffer,-1);
        }

        sprintf(outbuffer,GREEN "%-3d" DEFAULT,flaggnr);
        puttekn(outbuffer,-1);
        flaggnr++;
        if(ucfg->hotkeys==TRUE)
        {
                sprintf(outbuffer,"<%s>  %s\r\n",flag_on,hotkeys_str);
                puttekn(outbuffer,-1);
        }
        else
        {
                sprintf(outbuffer,"<%s>  %s\r\n",flag_off,hotkeys_str);
                puttekn(outbuffer,-1);
        }

        sprintf(outbuffer,GREEN "%-3d" DEFAULT,flaggnr);
        puttekn(outbuffer,-1);
        flaggnr++;
        if(ucfg->expert==EXPERTMODE)
        {
                sprintf(outbuffer,"<%s>  %s\r\n",flag_on,expert_str);
                puttekn(outbuffer,-1);
        }
        else
        {
                sprintf(outbuffer,"<%s>  %s\r\n",flag_off,expert_str);
                puttekn(outbuffer,-1);
        }

        sprintf(outbuffer,GREEN "%-3d" DEFAULT,flaggnr);
        puttekn(outbuffer,-1);
        flaggnr++;
        if(ucfg->scrclr==TRUE)
        {
                sprintf(outbuffer,"<%s>  %s\r\n",flag_on,scrclr_str);
                puttekn(outbuffer,-1);
        }
        else
        {
                sprintf(outbuffer,"<%s>  %s\r\n",flag_off,scrclr_str);
                puttekn(outbuffer,-1);
        }


        puttekn(what_flag,-1);
        puttekn(" ",-1);
        if(getstring(EKO,2,NULL)) return(1);
        val=atoi(inmat);
        if(val>flaggnr-1 || val<0 || inmat[0]<'0' || inmat[0]>'9')
        {
                puttekn("\r\n",-1);
                puttekn(illegal_choise,-1);
                puttekn("\r\n",-1);
                return(0);
        }
        puttekn("\r\n",-1);
        puttekn(on_off,-1);
        if(getstring(EKO,2,NULL)) return(1);
        onoffval=ToUpper(inmat[0]);

        flaggnr=0;
        if(onoffval==on_key[0] || onoffval==off_key[0])
        {
                if(val>=ANTFLAGG)
                {
                        if(val==ANTFLAGG)
                        {
                                if(onoffval==on_key[0])
                                        ucfg->hotkeys=TRUE;
                                else
                                        ucfg->hotkeys=FALSE;
                        }
                        else if(val==ANTFLAGG+1)
                        {
                                if(onoffval==on_key[0])
                                        ucfg->expert=EXPERTMODE;
                                else
                                        ucfg->expert=NOEXPERTMODE;
                        }
                        else if(val==ANTFLAGG+2)
                        {
                                if(onoffval==on_key[0])
                                        ucfg->scrclr=TRUE;
                                else
                                        ucfg->scrclr=FALSE;
                        }

                        writeuserfile();
                        return(0);
                }
                for(x=31;x>(31-val);x--)
                        flaggnr++;
                argument=Servermem->flaggnamn[31-x];
                if(onoffval==on_key[0])
                        slapa();
                else
                        slaav();
                return(0);
        }
        puttekn("\r\n",-1);
        puttekn(illegal_choise,-1);
        puttekn("\r\n",-1);
        return(0);
}



