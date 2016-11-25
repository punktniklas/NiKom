#include <proto/locale.h>
#include "CatalogDefaults.h"

#define CATSTR(foo) GetCatalogStr(g_Catalog, foo, foo ## _STR)

extern struct Catalog *g_Catalog;

void LoadCatalogForUser(void);
