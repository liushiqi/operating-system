#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#include "stdbool.h"
#include <thread.h>

#define MAX_MAILBOX_LENGTH 64
#define MAX_NAME_LENGTH 32

typedef struct mailbox {
  char name[MAX_NAME_LENGTH];
  uint8_t content[MAX_MAILBOX_LENGTH];
  bool activated;
  int length;
  mutex_t lock;
  condition_variable_t cond;
} mailbox_t;

mailbox_t *mailbox_open(char *);
void mailbox_close(mailbox_t *mailbox);
void mailbox_send(mailbox_t *mailbox, const void *msg, int msg_length);
void mailbox_receive(mailbox_t *mailbox, void *msg, int msg_length);

#endif
