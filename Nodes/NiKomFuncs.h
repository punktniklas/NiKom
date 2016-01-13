#include <stdio.h>
/*
 * TODO: The long time goal is for this file to die. Every .c
 * file should be a module that should have a corresponding .h
 * file that contains the public functions of that module.
 */

/* Prototypes for functions defined in NiKomSer.c */
BYTE OpenConsole(struct Window *window);
BYTE OpenSerial(struct IOExtSer *writereq,struct IOExtSer *readreq,struct IOExtSer *changereq);
BYTE OpenTimer(struct timerequest *treq);
void CloseConsole(void);
void CloseSerial(struct IOExtSer *writereq);
void CloseTimer(struct timerequest *treq);
void writesererr(int);
char gettekn(void);
char congettkn(void);
char sergettkn(void);
void eka(char tecken);
void coneka(char tecken);
void conreqtkn(void);
void serreqtkn(void);
void putstring(char *pekare,int size, long flags);
int puttekn(char *pekare,int size);
void modemcmd(char *pekare,int size);
void paus(ULONG tid);
void getnodeconfig(char *);
char convseventoeight(char foo);
char conveighttoseven(char foo);
void convstring(char *string);
void sendat(char *atstring);
void sendplus(void);
void freealiasmem(void);
void waitconnect(void);
void cleanup(int kod,char *text);
void main(int argc,char **argv);
int getfifoevent(struct MsgPort *fifoport,char *puthere);

/* Lite temporärt såhär.. Egentligen SerialIO.c.. */
int OpenIO(struct Window *);
void CloseIO(void);
void abortserial(void);
int tknwaiting(void);
int incantrader(void);
int sendtocon(char *pekare, int);
int checkcharbuffer(void);
int sendtosercon(char *conpek, char *serpek, int consize, int sersize);

int serputtekn(char *pekare,int size);
void sereka(char tecken);
int sendtoser(char *pekare, int);

/* ServerComm.c.. */
long sendservermess(short, long);
int initnode(int);
void shutdownnode(int);
void handleservermess(struct NiKMess *);

/* Prototypes for functions defined in NiKFuncs.c */
int kommentera(void);
void lasa(void);
int countmote(int mote);
void var(int mot);
int visatext(int text);
void tiden(void);
int skapmot(void);
void listmot(char *);
int parse(char *skri);
int parsemot(char *skri);
void medlem(char *foo);
int uttrad(char *foo);
void sparatext(void);
void linkkom(void);
int initheader(int komm);
int countunread(int meet);
void initLowestPossibleUnreadTexts(void);
void connection(void);
void varmote(int mote);
int skriv(void);
void igen(void);
void atersekom(void);

/* Prototypes for functions defined in NiKFuncs2.c */
int countmail(int user,int brevpek);
void varmail(void);
void visabrev(int brev,int anv);
int recisthere(char *str,int rec);
int updatenextletter(int user);
int parsenamn(char *skri);
int matchar(char *skrivet,char *facit);
char *hittaefter(char *strang);
void listmed(void);
void listratt(void);
void listnyheter(void);
void listflagg(void);
int parseflagga(char *skri);
void slaav(void);
void slapa(void);
int ropa(void);
void writemeet(struct Mote * );
void addratt(void);
void subratt(void);

/* Prototypes for functions defined in NiKFuncs3.c */
void endast(void);
int personlig(void);
void radtext(void);
int radmot(void);
int andmot(void);
void radbrev(void);
void vilka(void);
int skickabrev(void);
void initpersheader(void);
void visainfo(void);
void NiKForbid(void);
void NiKPermit(void);
int readtexthead(int nummer,struct Header *head);
int writetexthead(int nummer,struct Header *head);
int readletterheader(BPTR fh,struct ReadLetter *rl);
int getnextletter(int user);
int getfirstletter(int user);
int readuser(int nummer,struct User *user);
int writeuser(int nummer,struct User *user);
void rensatexter(void);
void gamlatexter(void);
void gamlabrev(void);
void getconfig(void);
void writeinfo(void);
void displaysay(void);
int sag(void);
void writesenaste(void);
void listasenaste(void);
int dumpatext(void);
void listaarende(void);
void tellallnodes(char *str);
int skrivlapp(void);
void radlapp(void);
int userexists(int nummer);

/* Prototypes for functions defined in NiKFuncs4.c */
int movetext(void);
char *getusername(int nummer);
int namematch(char *pat,char *fac);
int skapagrupp(void);
int writegrupp(int nummer,struct UserGroup *pek);
void listagrupper(void);
int adderagruppmedlem(void);
int subtraheragruppmedlem(void);
int parsegrupp(char *skri);
int andragrupp(void);
void raderagrupp(void);
void initgrupp(void);
char *getgruppname(int nummer);
int editgrupp(char *bitmap);
void motesstatus(void);
void hoppaarende(void);
int arearatt(int area, int usrnr, struct User *usr);
int gruppmed(struct UserGroup *grupp,char status,long grupper);
void listgruppmed(void);
struct Alias *parsealias(char *skri);
void listalias(void);
void remalias(void);
void defalias(void);
void alias(void);
int speciallogin(char bokstav);
int readtextlines(char typ,long pos,int rader,int nummer);
void freeeditlist(void);
void listabrev(void);
void rensabrev(void);
int rek_flyttagren(int rot,int ack,int tomeet);
void flyttagren(void);
int execfifo(char *command,int cooked);

/* Prototypes for functions defined in NiKFuncs5.c */
struct Mote *getmotpek(int);
char *getmotnamn(int);
struct Kommando *getkmdpek(int);
int bytnodtyp(void);
void dellostsay(void);
void bytteckenset(void);
void SaveCurrentUser(int, int);   /* Kommandot SPARA */

/* Prototypes for functions defined in NiKEditor.c */
int edittext(char *filnamn);

/* Prototypes for functions defined in NiKFiles.c */
int parsearea(char *skri);
void listarea(void);
int parsenyckel(char *skri);
void listnyckel(void);
void listfiler(void);
void bytarea(void);
void filinfo(void);
void radarea(void);
int andraarea(void);
int skapafil(void);
void radfil(void);
int andrafil(void);
int lagrafil(void);
int flyttafil(void);
int incsearch(char *string,char *patt);
int sokfil(void);
struct Fil *parsefil(char *skri,int area);
struct Fil *parsefilallareas(char *);
void raisefiledl(struct Fil *filpek);
void filstatus(void);
void typefil(void);
void nyafiler(void);
int editkey(char *bitmap);
int choosedir(int area,char *nycklar,int size);
void validerafil(void);
struct Fil *filexists(char *skri,int area);

/* Prototypes for functions defined in NiKTransfer.c */
long __regargs __saveds nik_fopen(char *filename,char *accessmode);
long __regargs __saveds nik_fclose(LONG *fh);
long __regargs __saveds nik_fread(char *databuffer,long size,long count,LONG *fh);
long __regargs __saveds nik_fwrite(char *databuffer,long size,long count,LONG *fh);
long __regargs __saveds nik_fseek(LONG *fh,long offset,long origin);
long __regargs __saveds nik_sread(char *databuffer,long size,long timeout);
long __regargs __saveds nik_swrite(char *databuffer,long size);
long __regargs __saveds nik_update(struct XPR_UPDATE *update);
long __regargs __saveds nik_sflush(void);
long __regargs __saveds nik_chkabort(void);
long __regargs __saveds nik_gets(char *prompt,char *buffer);
long __regargs __saveds nik_finfo(char *filename,long typeinfo);
long __regargs __saveds nik_ffirst(char *buffer,char *pattern);
long __regargs __saveds nik_fnext(long oldstate,char *buffer,char *pattern);
void xpr_setup(struct XPR_IO *sio);
int download(void);
int valnamn(char *namn,int area,char *textbuf);
int upload(void);
int recbinfile(char *dir);
int sendbinfile(void);

/* Prototypes for functions defined in NiKOffline.c */
int grabtext(int text,FILE *fpgrab);
int grabfidotext(int, struct Mote *,FILE *);
int grabbrev(int text,FILE *fpgrab);
int grabkom(FILE *fp);
void grab(void);

/* Prototypes for functions defined in NiKRexx.c */
void sendrexx(int komnr);
void sendautorexx(int komnr);

/* Prototypes for functions defined in NiKUUCico.c */
void douucico(void);

/* Prototypes for functions defined in Brev.c */
int getzone(char *);
int getnode(char *);
int getnet(char *);
int getpoint(char *);
int brev_kommentera(void);
void brev_lasa(int tnr);
int countmail(int user,int brevpek);
void varmail(void);
void visabrev(int brev,int anv);
void visafidobrev(struct ReadLetter *,BPTR,int,int);
int initbrevheader(int tillpers);
int fido_brev(char *,char *,struct Mote *);
void sparabrev(void);
void sprattgok(char *);
void savefidocopy(struct FidoText *,int);

/* Prototypes for functions defined in OrgMeet.c */
int org_skriv(void);
int org_kommentera(void);
void org_lasa(int);
int checkmote(int);
void varmote(int);
int org_visatext(int);
void org_sparatext(void);
void org_linkkom(void);
int org_initheader(int);
void org_endast(int, int);

/* Prototypes for functions defined in FidoMeet.c */
void fido_lasa(int tnr,struct Mote *motpek);
int countfidomote(struct Mote *motpek);
void fido_visatext(int text,struct Mote *motpek);
void fido_endast(struct Mote *,int);
void makefidodate(char *);
void makefidousername(char *,int);
struct FidoDomain *getfidodomain(int,int);
int fido_skriv(int,int);
void fidolistaarende(struct Mote *,int);

/* Prototypes for functions defined in noansi.c */
void noansi(char *);
