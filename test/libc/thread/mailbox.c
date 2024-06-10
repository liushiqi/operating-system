#include <mailbox.h>
#include <string.h>

#define MAX_NUM_BOX 32

mailbox_t mailboxes[MAX_NUM_BOX];

mailbox_t *mailbox_open(char *name) {
  for (int i = 0; i < MAX_NUM_BOX; ++i) {
    if (mailboxes[i].activated && strcmp(name, mailboxes[i].name) == 0)
      return mailboxes + i;
  }
  for (int i = 0; i < MAX_NUM_BOX; ++i) {
    if (!mailboxes[i].activated) {
      strncpy(mailboxes[i].name, name, sizeof(mailboxes[i].name));
      memset(mailboxes[i].content, 0, sizeof(mailboxes[i].content));
      mailboxes[i].length = 0;
      mailboxes[i].activated = true;
      mutex_init(&mailboxes[i].lock);
      condition_variable_init(&mailboxes[i].cond);
      return mailboxes + i;
    }
  }
  return NULL;
}

void mailbox_close(mailbox_t *mailbox) { mailbox->activated = false; }

void mailbox_send(mailbox_t *mailbox, const void *msg, int msg_length) {
  mutex_lock(&mailbox->lock);
  while (msg_length + mailbox->length >= MAX_MAILBOX_LENGTH) {
    condition_variable_wait(&mailbox->cond, &mailbox->lock);
  }
  memcpy(mailbox->content + mailbox->length, msg, msg_length);
  mailbox->length += msg_length;
  condition_variable_broadcast(&mailbox->cond);
  mutex_unlock(&mailbox->lock);
}

void mailbox_receive(mailbox_t *mailbox, void *msg, int msg_length) {
  mutex_lock(&mailbox->lock);
  while (mailbox->length - msg_length < 0) {
    condition_variable_wait(&mailbox->cond, &mailbox->lock);
  }
  memcpy(msg, mailbox->content, msg_length);
  memmove(mailbox->content, mailbox->content + msg_length, mailbox->length - msg_length);
  mailbox->length -= msg_length;
  condition_variable_broadcast(&mailbox->cond);
  mutex_unlock(&mailbox->lock);
}
