struct User *NodeReadUser(int userId, struct User *user);
struct User *NodeWriteUser(int userId, struct User *user);

/*
 * Returns a pointer to a User structure for the given user, or NULL
 * if the user is not found (or some other error). The data is copied
 * from Servermem if the user is currently logged in. Otherwise it's read
 * from disk.
 * The returned pointer is to static memory in this function and is only
 * valid until the next time this function is called.
 */
struct User *GetUserData(int userId);
