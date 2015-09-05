#ifndef NIKOMSTR_H
#define NIKOMSTR_H

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif /* EXEC_PORTS_H */

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif /* EXEC_SEMAPHORES_H */

#include <time.h>

/* Från nod till server */
#define NYNOD        1
#define NODSLUTAR    2
#define SPARATEXTEN  3
#define SPARABREVET  4
#define FORBID       5
#define RADERATEXTER 6
#define RADERABREV   7
#define READCFG      8
#define WRITEINFO    9
#define GETADRESS    10
#define FREEADRESS   11
#define NIKMESS_SETNODESTATE 12
#define NIKMESS_SENDNODEMESS 13

/* Maximala antal */
#define MAXMOT			5
#define MAXKOM			15
#define MAXBREV			2048
#define MAXNOD			32
#define MAXMOTE			2048
#define MAXSAYTKN		1000
#define MAXSENASTE		128
#define MAXNYCKLAR		128
#define MAXAREA			32
#define MAXVOTE			32
#define MAXNODETYPES	16
#define MAXFULLEDITTKN	79		/* Maximalt antal tecken per rad i
									fullskärmseditorn. OBS! Lägg till 1! */
#define MAXPRGCATCACHE		MAXNOD*2	/* Används inte ännu */
#define MAXUSERSCACHED		15
#define MAXTIMETOCACHEUSER	10	/* Tid i sekunder! */

#define UNREADTEXTS_BITMAPSIZE   32768

/* Defines för getstring() */

#define NUMONLY		3
#define STAREKO		2
#define EKO			1
#define EJEKO		0

/* Status på möten */
#define SLUTET       1
#define SKRIVSKYDD   2
#define KOMSKYDD     4
#define HEMLIGT      8
#define ENKOMMENTAR  16
#define AUTOMEDLEM   32
#define SUPERHEMLIGT 64
#define SKRIVSTYRT   128

/* Mötestyper */
#define MOTE_ORGINAL 0
#define MOTE_NTSTYLE 1
#define MOTE_FIDO    2
#define MOTE_UUCP    3

/* Vilken sorts nod */
#define NODCON       1
#define NODSER       2
#define NODSPAWNED   3 /* En som körs från PreNoden */

/* Flaggor */
#define ANTFLAGG     13 /* Antalet flaggor */
#define SKYDDAD      1
#define STRECKRAD    2
#define STAREKOFLAG	 4
#define INGENHELP    8
#define FULLSCREEN   16
#define FILLISTA     32
#define MELLANSLAG   64
#define LAPPBREV     128
#define SEKVENSANSI  256
#define SHOWKLUDGE   512
#define CLEARSCREEN  1024
#define NOLOGNOTIFY  2048
#define ANSICOLOURS  4096

/* Status på texter */
#define RADERAD      1
#define KOMSKYDDAD   2

/* Senast lästa texttyp och vad som skrivs */
#define BREV         1
#define TEXT         2
#define ANNAT        3
#define BREV_FIDO    4
#define TEXT_FIDO    5

/* Vad gör användaren */
#define INGET        1 /* Se tiden-prompten */
#define SKRIVER      2 /* Vilket möte finns i varmote */
#define UPLOAD       3 /* Vilken area finns i varmote, filnamn finns i vilkastr */
#define DOWNLOAD     4 /* Vilken area finns i varmote, filnamn finns i vilkastr */
#define LASER        5 /* Vilket möte finns i varmote */
#define GORNGTANNAT  6 /* Lämplig sträng finns vilkastr */

/* Protokoll */
#define ZMODEM       1
#define XMODEM       2

/* Argumenttyper till kommandon */
#define KOMARGINGET  0
#define KOMARGNUM    1
#define KOMARGCHAR   2

/* Vad som ska loggas */
#define LOG_CONNECTION 1    /* Connect at xxxx bps */
#define LOG_INLOGG     2    /* xxx #yy loggar in på nod z */
#define LOG_FAILINLOGG 4    /* xxx #yy angivet som namn, fel lösen */
#define LOG_UTLOGG     8    /* xxx #yy loggar ut från nod z */
#define LOG_CARDROPPED 16   /* xxx #yy släpper carriern */
#define LOG_RECFILE    32   /* Basen tar emot filen zzzz från xxx #yy */
#define LOG_SENDFILE   64   /* Basen skickar filen zzzz till xxx #yy */
#define LOG_UPLOAD     128  /* xxx #yy laddar upp filen zzzz */
#define LOG_DOWNLOAD   256  /* xxx #yy laddar ner filen zzzz */
#define LOG_TEXT       512  /* xxx #yy skriver en text i mötet zzzz */
#define LOG_BREV       1024 /* xxx #yy skickar ett brev till zzz #qq */
#define LOG_NOCONNECT  2048 /* RING på nod x, men ingen CONNECT */

/* Flaggor för en fil */
#define FILE_LONGDESC  1    /* Det finns en lång beskrivning */
#define FILE_NOTVALID  2    /* Filen är inte validerad */
#define FILE_FREEDL    4    /* Ingen ratio räknas när filen laddas ned */

/* Flaggor för en area */
#define AREA_NODOWNLOAD   1    /* Ingen download tillåts från arean */
#define AREA_NOUPLOAD     2    /* Ingen upload tillåts till arean */
#define AREA_LONGDESCDIR  4    /* Directory 15 innehåller sökvägen till långa beskrivningar */

/* De olika semaforer som finns */
#define NIKSEM_CONF       0
#define NIKSEM_TEXTS      1
#define NIKSEM_USERS      2
#define NIKSEM_FILEAREAS  3
#define NIKSEM_SAYMSGS    4
#define NIKSEM_GROUPS     5
#define NIKSEM_COMMANDS   6
#define NIKSEM_LATEST     7
#define NIKSEM_MAILBOXES  8
#define NIKSEM_PRGCAT	  9
#define NIKSEM_UNREAD     10
#define NIKSEM_CONFTEXTS  11

#define NIKSEM_NOOF       12 /* Hur många det finns */

#define BAMTEST(a,b) (((char *)(a))[(b)/8] & 1 << 8-1-(b)%8)
#define BAMSET(a,b) (((char *)(a))[(b)/8] |= 1 << 8-1-(b)%8)
#define BAMCLEAR(a,b) (((char *)(a))[(b)/8] &= ~(1 << 8-1-(b)%8))

#define ITER_EL(var, list, nodeField, type) for(var = (type)list.mlh_Head; var->nodeField.mln_Succ; var = (type) var->nodeField.mln_Succ)

/* Flaggor till putstring() */
#define PS_NOCONV 1

struct Statuscfg {
   char skriv,texter,brev,medmoten,radmoten,sestatus,anv,chgstatus,psw,
      bytarea,radarea,filer,laddaner,editor,crashmail, grupper;
};

struct AutoRexxCfg {
   long preinlogg,postinlogg,utlogg,nyanv,preup1,preup2,postup1,postup2,noright,
      nextmeet,nexttext,nextkom,setid,nextletter,cardropped;
};

#define NICFG_CLOSEDBBS			1
#define NICFG_VALIDATEFILES		2
#define NICFG_STARATNOECHO		4
#define NICFG_LOCALCOLOURS		8
#define NICFG_CRYPTEDPASSWORDS	16

struct Config {
   struct Statuscfg st;
   struct AutoRexxCfg ar;
   long defaultflags, diskfree, logmask, cfgflags;
   short maxtid[101],logintries;
   char uldlratio[101],inaktiv[101],defaultrader,
      defaultprotokoll,defaultstatus,brevnamn[41],ny[21],ultmp[100],
      logfile[100];
};

struct ReadLetter {
   long number;
   char systemid[20],from[100],to[100],messageid[100],reply[100],date[40],
      subject[100];
};

struct Header {
   short rader,mote,status;
   long person,kom_av[MAXKOM],kom_till_per,nummer,textoffset,
      tid,kom_i[MAXKOM],kom_till_nr,reserv1,reserv2;
   char arende[41];
};

struct Mote {
   struct MinNode mot_node;
   long skapat_tid,grupper,skapat_av,mad,sortpri,lowtext,domain,texter, renumber_offset,
  	   reserv1, reserv2, reserv3, reserv4, reserv5;
   short gren,status,charset,nummer;
   char type,namn[41],origin[70],tagnamn[50],dir[80];
};

struct ShortUser {
   struct MinNode user_node;
   long nummer,grupper;
   char namn[41],status;
};

struct User {
   long tot_tid,forst_in,senast_in,read,skrivit,flaggor,former_textpek,brevpek,
      grupper,defarea,downloadbytes,chrset,uploadbytes,reserv5,upload,download,
      loggin,shell;
   char namn[41],gata[41],postadress[41],land[41],telefon[21],
      annan_info[61],losen[16],status,rader,protokoll,
      prompt[6],motratt[MAXMOTE/8],motmed[MAXMOTE/8],vote[MAXVOTE];
};

struct UnreadTexts {
  long bitmapStartText;
  char bitmap[UNREADTEXTS_BITMAPSIZE/8];
  long lowestPossibleUnreadText[MAXMOTE];
};

struct Kommando {
   struct MinNode kom_node;
   long nummer,minlogg,mindays,before,after,grupper;
   char namn[31],status,antal_ord,argument,losen[20],beskr[70],secret,
   		vilkainfo[50],logstr[50],menu[50];
};

struct SysInfo {
   long hightext,lowtext,inloggningar,skrivna,lasta,uloads,dloads,
      radbytes,lowbrev,highbrev,radbytesbrev,users;
   short nycklar,kommandon,moten,areor;
   long bps[50], antbps[50];
};

struct UserGroup {
   struct MinNode grupp_node;
   char namn[41],flaggor,autostatus,nummer;
   int groupadmin;
};

struct SayString {
   int fromuser;
   long timestamp;
   char text[MAXSAYTKN];
   struct SayString *NextSay;
};

struct ChatRequest {
   long busy,fromuser;
   struct MsgPort *chatport;
};

struct Inloggning {
   long utloggtid,anv;
   short tid_inloggad,read,write,ul,dl;
};

struct Fil {
   struct MinNode f_node;
   long tid,size,senast_dl,uppladdare,dir,flaggor,validtime;
   short downloads;
   char namn[41],nycklar[MAXNYCKLAR/8],status,beskr[71];
   long index;
};

struct DiskFil {
   struct MinNode f_node;
   long tid,size,senast_dl,uppladdare,dir,flaggor,validtime;
   short downloads;
   char namn[41],nycklar[MAXNYCKLAR/8],status,beskr[71];
};

struct Area {
   struct MinList ar_list;
   long skapad_tid,skapad_av,senast_ul,senast_dl,grupper,flaggor,reserv2,
      reserv3,reserv4;
   short filer,mote;
   char namn[41],dir[16][41],nycklar[16][MAXNYCKLAR/8],status;
};

struct Alias {
   struct MinNode alias_node;
   char namn[21],blirtill[41];
};

struct SpecialLogin {
   struct MinNode login_node;
   long rexxprg;
   char bokstav;
};

struct EditLine {
   struct MinNode line_node;
   long number;
   char text[81];
};

struct FidoDomain {
	short zone,net,node,point,nummer,chrs;
	char domain[20],zones[50];
};

struct FidoAlias {
	long nummer;
	char namn[36];
};

struct FidoData {
	struct FidoDomain fd[10];
	struct FidoAlias fa[20];
	long lastmatrix,mailgroups;
	char mailstatus, bounce, crashstatus, fidologfile[100],matrixdir[100],defaultorigin[70];
};

struct NodeType {
	int nummer;
	char path[50],desc[80];
};

struct ProgramDataCache
{
	struct ProgramCategory *PrgCat;
	struct ProgramDataCache *prev, *next;

	int usernumber;
};

struct LegacyConversionData {
  long lowTextWhenBitmap0ConversionStarted;
};

/* Keeps track of what conference a text is in. A value of -1 means the text
 * is deleted. Index 0 of the array is Sysinfo.lowtext. */
struct ConferenceTexts {
  int arraySize;
  short *texts;
};

struct System {
  struct ConferenceTexts confTexts;
  struct MinList kom_list;
  struct User inne[MAXNOD];
  struct UnreadTexts unreadTexts[MAXNOD];
  long nodtyp[MAXNOD],inloggad[MAXNOD],action[MAXNOD],varmote[MAXNOD],
    xtratime[MAXNOD],connectbps[MAXNOD], idletime[MAXNOD],
    maxinactivetime[MAXNOD], serverversion, serverrevision;
  char *vilkastr[MAXNOD], nodid[MAXNOD][20], watchserial[MAXNOD],
    CallerID[MAXNOD][81];
  struct MinList mot_list;
  struct MinList user_list;
  struct MinList shell_list;
  struct MinList grupp_list;
  char flaggnamn[32][51];
  struct Config cfg;
  struct SysInfo info;
  struct SayString *say[MAXNOD];
  struct Inloggning senaste[MAXSENASTE];
  struct Area areor[MAXAREA];
  char Nyckelnamn[MAXNYCKLAR][41];
  struct MinList special_login;
  struct FidoData fidodata;
  struct NodeType nodetypes[MAXNODETYPES];
  struct SignalSemaphore semaphores[NIKSEM_NOOF];
  struct ProgramDataCache *PrgDataCache;
  struct LegacyConversionData legacyConversionData;
};

struct NiKMess {
   struct Message stdmess;
   char nod;
   short kommando;
   long data;
   long extdata1;
   long extdata2;
};

struct TransferFiles {
        struct MinNode node;
        struct Fil *filpek;
        long cps;
        char path[123],sucess, Filnamn[257];
};
#endif /* NIKOMSTR_H */
