#include "NiKomStr.h"
#include "DiskUtils.h"

extern struct System *Servermem;

/*
 * Finds the directory in a file area that has the best match
 * with the keys set for the file. If that directory has enough
 * disk space the directory number is returned. If not, the
 * first available directory with enough space is returned. If no
 * directory has enough space -1 is returned.
 */
int ChooseDirectoryInFileArea(int area, char *fileKeys, int size) {
  int i, j, keysMatchingForDir, bestDir = 0, scoreInBestDir = 0;

  for(i = 0; i < 16; i++) {
    if(Servermem->areor[area].dir[i][0] == '\0') {
      continue;
    }
    keysMatchingForDir = 0;
    for(j = 0; j < MAXNYCKLAR; j++) {
      if(BAMTEST(fileKeys, j) && BAMTEST(Servermem->areor[area].nycklar[i], j)) {
        keysMatchingForDir++;
      }
    }
    if(keysMatchingForDir >= scoreInBestDir) {
      bestDir = i;
      scoreInBestDir = keysMatchingForDir;
    }
  }

  if(HasPartitionEnoughFreeSpace(Servermem->areor[area].dir[bestDir], size)) {
    return bestDir;
  }

  // If there is not enough space in the best dir, just pick any dir that has
  // space.
  for(i = 0; i < 16; i++) {
    if(Servermem->areor[area].dir[i][0] == '\0') {
      continue;
    }
    if(HasPartitionEnoughFreeSpace(Servermem->areor[area].dir[i], size)) {
      return i;
    }
  }
  return -1;
}
