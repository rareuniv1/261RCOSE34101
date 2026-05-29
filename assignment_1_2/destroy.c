#include <sys/mman.h>
#include <semaphore.h>
#include <stdio.h>

#define SHM_NAME "/ipc_assignment_message_buffer_sem"
#define SEM_NAME "/ipc_assignment_message_sem"

int main(){
    if (shm_unlink(SHM_NAME) == -1) {
        printf("shm_unlink error or already removed\n");
    } else {
        printf("destroy shared memory : %s\n", SHM_NAME);
    }

    if (sem_unlink(SEM_NAME) == -1) {
        printf("sem_unlink error or already removed\n");
    } else {
        printf("destroy semaphore : %s\n", SEM_NAME);
    }

    return 0;
}
