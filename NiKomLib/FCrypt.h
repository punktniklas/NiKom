/* Function descriptions are from looking at the code in FCrypt.c.
 * They are hopefully correct
 */

/* Encrypts the string in buf using the given salt. At most 8 characters
 * from buf will be used and salt must contain 2 characters. The result
 * will be written to ret as a 13 character string with a nul byte
 * terminator meaning that the array must be at least 14 characters.
 * Returns the pointer to the given ret array.
 */
char *des_fcrypt(const char *buf, const char *salt, char *ret);

/* As des_fcrypt() but writes the result to an internal static array
 * and returns that pointer. Consecutive calls will overwrite previous
 * result. Must not be called from concurrent code, such as nikom.library..
 */
char *crypt(const char *buf, const char *salt);

char *generateSalt(char *saltbuf, int saltLength);
