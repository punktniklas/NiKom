enum NotificationType {
  NOTIF_TYPE_UNKNOWN = 0,
  NOTIF_TYPE_REACTION = 1
};

enum ReactionType {
  NOTIF_REACTION_LIKE = 1,
  NOTIF_REACTION_DISLIKE = 2
};

// File format: <reaction type> <user id> <text id>
struct ReactionNotif {
  enum ReactionType reactionType;
  int userId, textId;
};


// File format: <type> <notification specific data>
struct Notification {
  enum NotificationType type;
  union {
    struct ReactionNotif reaction;
  };
  struct Notification *next;
};

struct Notification *ReadNotifications(int userId, int clear);
void FreeNotifications(struct Notification *list);
int CountNotifications(int userId);
void AddNotification(int userId, struct Notification *notification);
