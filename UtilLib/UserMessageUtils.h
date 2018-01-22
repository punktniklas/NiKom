#define NIK_MESSAGETYPE_LOGNOTIFY   1    // Someone has logged in/out

/*
 * Sends a user message to one or all logged in users. If toUserId is -1 the message
 * will be sent to all users. If fromUserId is -1 the message will be seen a system message.
 * If messageType has the flag NIK_MESSAGETYPE_LOGNOTIFY set and a user has the flag NOLOGNOTIFY
 * set the message will not be sent to that user.
 *
 * Return values for single recipient:
 * 0 - Could not allocate a new message
 * 1 - Message sent, there were no messages already in the queue.
 * 2 - Message sent, it's been placed last in the queue
 * 3 - The given recipient is not logged in.
 *
 * Return values for all users:
 * 0 - Could not allocate a new message
 * 1 - Message sent
 */
int SendUserMessage(int fromUserId, int toUserId, char *str, int messageType);

struct SayString *UnLinkUserMessages(int userDataSlot);

int HasUnreadUserMessages(int userDataSlot);

void ClearUserMessages(int userDataSlot);
