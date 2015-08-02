
struct keywordlist {
        char    key;                    // tangent
        char   *name;                   // pekare till sträng, innehållandes antingen menynamn eller
                                        // funktionsnummer
        char    arg;                    // Y= argument, N=inget argument tillåts.
        char   *argument;               // Ett defaultargument.
        char   *visastring;             // Det som visas för användaren då ett argument skall sättas
        struct keywordlist *next;       // Pekare till nästa keyword, NULL om sista
};



struct Meny {
        char *name;             // Menynamn
        int     status;         // Vilken status man måste ha för att se denna menydefinition
        int     ret;            // status för vad som gäller vid endast return-tryckning
        void **menytext;        // pekare till array med menytextrader
        struct keywordlist *key;// pekare till lista med keywords
        struct Meny *next;      // pekare till nästa meny
        BOOL visaarea;
        BOOL visamote;
        char showmenukey;       //Tangent för att visa menyn.
        struct keywordlist *startkom;  // Kommando som skall köras vid start av denna meny.
};

struct UserCfg {
        char *name;             //pekare till namnet på den menydefinitionsfil som användaren valt
        BOOL hotkeys;           //TRUE=Hotkeys, FALSE=möjlighetatt stacka flera komamndon på en rad
        BOOL expert;            //TRUE=expertmode ON, FALSE=expertmode off
        char *sprak;            //pekare till namnet på den språkfil som användaren valt
        BOOL scrclr;            //TRUE=rensa skrämen då och då.
};

struct enkellista {
        char *name;
        struct enkellista *next;
};


/*
0  -  Black              Dark Grey            Black           ESC [ 30 m
1  -  Red                Bright Red           Bright Red      ESC [ 31 m
2  -  Green              Bright Green         Bright Green    ESC [ 32 m
3  -  Orange             Yellow               Yellow          ESC [ 33 m
4  -  Dark Blue          Bright Blue          Dark Blue       ESC [ 34 m
5  -  Violet             Bright Violet        Violet          ESC [ 35 m
6  -  Cyan               Bright Cyan          Medium Blue     ESC [ 36 m
7  -  Light Grey         White                White           ESC [ 37 m        DEFAULT
*/

#define BLACK           "\x1b[30m"
#define RED             "\x1b[31m"
#define GREEN           "\x1b[32m"
#define ORANGE          "\x1b[33m"
#define DARK_BLUE       "\x1b[34m"
#define VIOLET          "\x1b[35m"
#define CYAN            "\x1b[36m"
#define LIGHT_GREY      "\x1b[37m"
#define DEFAULT         "\x1b[0m"


/*
* S= Statusnivå
* L= Minsta antal inloggningar för att utföra kommandot
* X= Ett eventuellt lösenord för att få utföra komamndot
*/

struct menykommando
{
        int nummer;             // nummret på kommandot
        int status;             // MInimal status
        int minlogg;             // minst antal inloggningar
        int mindays;            // minst antal dagar
        char losen[20];         // lösen
        char vilkainfo[50];     // Vad som syns vid VILKA -V
        char logstr[50];        // vad som skrivs till loggfilen
        long before;
        long after;
        long grupper;
        struct menykommando *next;
};

