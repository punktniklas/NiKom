#include <stdio.h>

void MakeUserFilePath(char* string, int userId, char *fileName) {
  sprintf(string, "NiKom:Users/%d/%d/%s", userId/100, userId, fileName);
}
