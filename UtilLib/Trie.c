#include <exec/memory.h>
#include <proto/exec.h>

#include "Trie.h"

struct Trie {
  struct Trie *children[26];
  void *value;
};

struct Trie *CreateTrie(void) {
  return (struct Trie *)AllocMem(sizeof(struct Trie), MEMF_CLEAR | MEMF_PUBLIC);
}

void FreeTrie(struct Trie *trie, void(* freeValue)(void *)) {
  int i;
  if(trie->value != NULL && freeValue != NULL) {
    freeValue(trie->value);
  }
  for(i = 0; i < 26; i++) {
    if(trie->children[i] != NULL) {
      FreeTrie(trie->children[i], freeValue);
    }
  }
  FreeMem(trie, sizeof(struct Trie));
}

int TrieAdd(char *key, void *value, struct Trie *trie) {
  int index;
  if(*key == '\0') {
    trie->value = value;
    return TRUE;
  }
  if(*key < 'a' || *key > 'z') {
    return FALSE;
  }
  index = *key - 'a';
  if(trie->children[index] == NULL) {
    if((trie->children[index] = CreateTrie()) == NULL) {
      return FALSE;
    }
  }
  return TrieAdd(++key, value, trie->children[index]);
}

void *TrieGet(char *key, struct Trie *trie) {
  int index;
  if(*key == '\0') {
    return trie->value;
  }
  if(*key < 'a' || *key > 'z') {
    return NULL;
  }
  index = *key - 'a';
  return trie->children[index] == NULL ? NULL : TrieGet(++key, trie->children[index]);
}
