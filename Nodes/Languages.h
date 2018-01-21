#include <proto/locale.h>
#include "NiKomStr.h"
#include "CatalogDefaults.h"

#define CATSTR(foo) GetCatalogStr(g_Catalog, foo, foo ## _STR)

extern struct Catalog *g_Catalog;
extern char *g_FlagNames[];

void LoadCatalogForUser(struct User *user);
void AskUserForLanguage(struct User *user);
