#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H

#define SHM_NAME "/ipc_assignment_message_buffer"

typedef struct {
    int sender_id;
    int data;
} Message;

typedef struct {
    Message message;
    int is_empty;
    int account_id;
} MessageBuffer;

int init_buffer(MessageBuffer **buffer);
int attach_buffer(MessageBuffer **buffer);
int detach_buffer(void);
int destroy_buffer(void);
int produce(MessageBuffer **buffer, int sender_id, int data, int account_id);
int consume(MessageBuffer **buffer, Message **message);

#endif
