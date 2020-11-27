struct EditContext {
  char *fileName;
  char *subject;
  int subjectMaxLen;
  short *confId;
  char *mailRecipients;
};

int edittext(struct EditContext *context);
