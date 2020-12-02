struct EditContext {
  char *fileName;
  char *subject;
  int subjectMaxLen;
  short *confId;
  char *mailRecipients;
  int replyingToText; // Currently only Fido texts
  int replyingInConfId;
};

int edittext(struct EditContext *context);
