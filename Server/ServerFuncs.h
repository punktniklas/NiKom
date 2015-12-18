/* Prototypes for functions defined in NiKHost.c */
int readuser(int nummer,struct User *);
int writeuser(int nummer,struct User *);
void userinfo(struct RexxMsg *mess);
void motesinfo(struct RexxMsg *mess);
void chgmote(struct RexxMsg *mess);
void writemeet(struct Mote *motpek);
void nikparse(struct RexxMsg *mess);
void senaste(struct RexxMsg *mess);
void sysinfo(struct RexxMsg *mess);
void rexxreadcfg(struct RexxMsg *mess);
void handlerexx(struct RexxMsg *mess);
int parsenamn(char *skri);
int matchar(char *skrivet,char *facit);
char *hittaefter(char *strang);
int parse(char *skri);
int parsemot(char *skri);
int parsearea(char *skri);
int parsenyckel(char *skri);
struct Fil *parsefil(char *skri,int area);
char *getusername(int nummer);
int userexists(int nummer);
int getuserstatus(int nummer);
void kominfo(struct RexxMsg *mess);
void filinfo(struct RexxMsg *mess);
void areainfo(struct RexxMsg *mess);
void chguser(struct RexxMsg *mess);
void skapafil(struct RexxMsg *mess);
int valnamn(char *,int);

/* Prototypes for functions defined in NiKHost2.c */
int readtexthead(int nummer,struct Header *head);
int writetexthead(int nummer,struct Header *head);
void rexxnodeinfo(struct RexxMsg *mess);
void rexxnextfile(struct RexxMsg *mess);
void rexxraderafil(struct RexxMsg *mess);
void createtext(struct RexxMsg *mess);
int updatenextletter(int user);
void createletter(struct RexxMsg *mess);
void textinfo(struct RexxMsg *mess);
void nextunread(struct RexxMsg *mess);
void freeeditlist(void);
struct Mote *getmotpek(int);
char *getmotnamn(int);
void meetright(struct RexxMsg *);
void meetmember(struct RexxMsg *);
void chgmeetright(struct RexxMsg *);
void chgfile(struct RexxMsg *);
int parsegrupp(char *);
void keyinfo(struct RexxMsg *);
void getdir(struct RexxMsg *);
int choosedir(int, char *, int);
void rexxPurgeOldTexts(struct RexxMsg *);
void movefile(struct RexxMsg *);
void rexxnextpatternfile(struct RexxMsg *);

/* Prototypes for functions defined in NiKHost2.c */
void rxsendnodemess(struct RexxMsg *);

/* Prototypes for functions defined in NiKHost3.c */
void rxsendnodemess(struct RexxMsg *);
void rexxstatusinfo(struct RexxMsg *);
void rexxarearight(struct RexxMsg *);
void sortbps(long *bps[], long *);
void swapbps(long *bps, long *);
void rexxstatusinfo(struct RexxMsg *);
void rexxsysteminfo(struct RexxMsg *);
int arearatt(int, int, struct User *);
void rexxmarktextread(struct RexxMsg *);
void rexxmarktextunread(struct RexxMsg *);
void rexxmarktext(struct RexxMsg *mess, int desiredUnreadStatus);
void rexxconsoletext(struct RexxMsg *);
void rexxcheckuserpassword(struct RexxMsg *);

/* NodeComm.c */
void setnodestate(struct NiKMess *);
long sendnodemess(short, long, long, long, long);
