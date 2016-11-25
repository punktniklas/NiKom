#include <proto/locale.h>

#include "NiKomStr.h"
#include "Logging.h"

#include "Languages.h"

extern struct System *Servermem;
extern int nodnr, inloggad;
struct Catalog *g_Catalog;

static char *localeLang[] = {
  "english",
  "svenska"
};

void LoadCatalogForUser(void) {
  int lang = Servermem->inne[nodnr].language, err;
  
  CloseCatalog(g_Catalog);
  g_Catalog = NULL;
  if(lang < 0 || lang > 1) {
    LogEvent(SYSTEM_LOG, WARN, "User %d has invalid language setting (%d).", inloggad, lang);
    return;
  }
  if(lang == 0) {
    return;
  }
  g_Catalog = OpenCatalog(NULL, "NiKom.catalog", OC_Language, localeLang[lang], TAG_DONE);
  if(g_Catalog == NULL) {
    err = IoErr();
    if(err != 0) {
      LogEvent(SYSTEM_LOG, WARN, "Could not load catalog for language %s (err = %d).",
               localeLang[lang], err);
    }
  }
}
