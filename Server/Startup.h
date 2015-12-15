void startupNiKom(void);
void RegisterARexxFunctionHost(int add);

extern char pubscreen[40], NiKomReleaseStr[50];
extern int xpos, ypos;
extern struct System *Servermem;
extern struct Window *NiKWindow;
extern struct MsgPort *NiKPort, *permitport, *rexxport, *nodereplyport;
extern struct IntuitionBase *IntuitionBase;
extern struct RsxLib *RexxSysBase;
extern struct Library *UtilityBase;
