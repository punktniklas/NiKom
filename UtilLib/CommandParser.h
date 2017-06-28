struct LangCommand *ChooseLangCommand(struct Kommando *cmd, int lang);
int ParseCommand(char *str, int lang, struct User *user, struct Kommando *result[], char *argbuf);
int HasUserCmdPermission(struct Kommando *cmd, struct User *user);
