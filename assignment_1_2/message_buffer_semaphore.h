#ifndef MESSAGE_BUFFER_SEMAPHORE_H
#define MESSAGE_BUFFER_SEMAPHORE_H

#define SEM_NAME "/ipc_assignment_message_sem"
#define SHM_NAME "/ipc_assignment_message_buffer_sem"

typedef struct {
    int sender_id;
    int data;
} Message;

typedef struct {
    Message message;
    int is_empty;
    int account_id;
} MessageBuffer;

void init_sem(void);
void destroy_sem(void);
void s_wait(void);
void s_quit(void);

int init_buffer(MessageBuffer **buffer);
int attach_buffer(MessageBuffer **buffer);
int detach_buffer(void);
int destroy_buffer(void);
int produce(MessageBuffer **buffer, int sender_id, int data, int account_id);
int consume(MessageBuffer **buffer, Message **message);

#endif
