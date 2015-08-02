/* Prototyper för Meny1.c... */

BOOL readdefuserconf(int);
BOOL readuserfile(FILE *);
void writeuserfile();
int visainfomeny(void);
int menyparse(int, int, int);
int menyflaggor(void);
int cdmeny(void);
void deallocinfolista(struct enkellista *);
void varmotemeny(int);
void printmenutext(struct Meny *, int);
BOOL readmenus(char *);
int bytmenydef(void);
char * checkchoise(struct keywordlist *klist,char choise);
struct Meny *changemenu( struct Meny *, char *, BOOL);
int setmenyarg(int, char *);
struct menykommando *menygetkmdpek(int);
int menykmd(int, int, int);
BOOL menygetkmd(void);
BOOL readuserconfig(int);
int readlang(char *);
void checkarg(struct keywordlist *,char);
int medlemmeny(void);
int gameny(void);
void writeuserfile(void);

/* Prototyper från sprakstrings.h */

BOOL bytsprak(void);

/* Prototyper från menyversion.h */

void aboutmeny(void);
