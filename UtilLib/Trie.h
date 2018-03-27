struct Trie;

/*
 * Creates a new trie and returns a pointer to it. It must be freed by calling
 * FreeTrie().
 */
struct Trie *CreateTrie(void);

/*
 * Frees the given trie and de-allocates all memory associated with it. If
 * a freeValue function pointer is provided the given function will be called
 * for each value stored in the trie so any memory can be de-allocated.
 */
void FreeTrie(struct Trie *trie, void (* freeValue)(void *));

/*
 * Adds a key/value pair to the trie. Returns true if successful and false
 * otherwise. The key must be a nul-terminated string that only consists
 * of lowercase characters between 'a' and 'z'. The empty string is a valid
 * key.
 */
int TrieAdd(char *key, void *value, struct Trie *trie);

/*
 * Gets the value for the given key if it exists in the trie. Returns NULL
 * otherwise.
 */
void *TrieGet(char *key, struct Trie *trie);
