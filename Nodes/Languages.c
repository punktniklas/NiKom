#include <proto/locale.h>
#include <proto/dos.h>

#include "NiKomStr.h"
#include "Nodes.h"
#include "Logging.h"
#include "Terminal.h"
#include "BasicIO.h"

#include "Languages.h"

extern struct System *Servermem;
extern int nodnr, inloggad;
struct Catalog *g_Catalog;
char *g_FlagNames[ANTFLAGG];

void loadNewCatalog(int lang) {
  int err;
  
  CloseCatalog(g_Catalog);
  g_Catalog = NULL;
  if(lang < 0 || lang > 1) {
    LogEvent(SYSTEM_LOG, WARN, "User %d has invalid language setting (%d).", inloggad, lang);
    return;
  }
  if(lang == 0) {
    return;
  }
  g_Catalog = OpenCatalog(NULL, "NiKom.catalog", OC_Language, Servermem->languages[lang], TAG_DONE);
  if(g_Catalog == NULL) {
    err = IoErr();
    if(err != 0) {
      LogEvent(SYSTEM_LOG, WARN, "Could not load catalog for language %s (err = %d).",
               Servermem->languages[lang], err);
    }
  }
}

void initFlagNames(void) {
  g_FlagNames[0] = CATSTR(MSG_FLAG_PROTECTED_STATUS);
  g_FlagNames[1] = CATSTR(MSG_FLAG_LINE_SUBJECT);
  g_FlagNames[2] = CATSTR(MSG_FLAG_PASSWORD_STARS);
  g_FlagNames[3] = CATSTR(MSG_FLAG_NO_AUTO_HELP);
  g_FlagNames[4] = CATSTR(MSG_FLAG_ADVANCED_EDITOR);
  g_FlagNames[5] = CATSTR(MSG_FLAG_FILES_AT_LIST_NEW);
  g_FlagNames[6] = CATSTR(MSG_FLAG_SPACE_AS_PAUSE);
  g_FlagNames[7] = CATSTR(MSG_FLAG_NOTE_AT_MAIL);
  g_FlagNames[8] = CATSTR(MSG_FLAG_ANSI_SEQUENCES);
  g_FlagNames[9] = CATSTR(MSG_FLAG_DISPLAY_FIDO_KLUDGE);
  g_FlagNames[10] = CATSTR(MSG_FLAG_CLEAR_SCREEN_FIDO);
  g_FlagNames[11] = CATSTR(MSG_FLAG_NO_LOGIN_NOTIFICATIONS);
  g_FlagNames[12] = CATSTR(MSG_FLAG_COLORS);
  g_FlagNames[13] = CATSTR(MSG_FLAG_ASCII_7E_IS_DELETE);
}

void LoadCatalogForUser(void) {
  int lang = CURRENT_USER->language;

  loadNewCatalog(lang);
  initFlagNames();
}

static char *languages[] = {
  "English",
  "Svenska"
};


void AskUserForLanguage(struct User *user) {
  int lang, i;

  SendString("\n\n\rThese are the available languages.\n\r");
  SendString("* indicates current choice.\n\n\r");

  SendString("  #  Language\n\r");
  SendString("-----------------------------------------\n\r");
  for(i = 0; i < 2; i++) {
    SendString("%c %2d: %s\n\r", user->language == i ? '*' : ' ', i, languages[i]);
  }
  
  SendString("\n\rChoice: ");
  lang = GetNumber(0, 1, NULL);
  if(ImmediateLogout()) {
    return;
  }
  SendString("\r\n\n%s\n\r", languages[lang]);
  user->language = lang;
  LoadCatalogForUser();
}
