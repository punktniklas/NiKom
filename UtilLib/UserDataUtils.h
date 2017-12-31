/*
 * Reads the user data for the given user from disk into the given
 * memory structure. Returns the input user pointer if successful
 * or NULL if not.
 */
struct User *ReadUser(int userId, struct User *user);

/*
 * Writes the user data for the given user to disk from the given
 * memory structure. Returns the input user pointer if successful
 * or NULL if not.
 */
struct User *WriteUser(int userId, struct User *user);

/*
 * Returns a pointer to the users data in Severmem if the user is
 * logged in. Returns NULL otherwise.
 */
struct User *GetLoggedInUser(int userId);
