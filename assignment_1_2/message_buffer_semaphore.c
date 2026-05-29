#include "message_buffer_semaphore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

int shm_fd = -1;
void *memory_segment = NULL;

sem_t *sem = SEM_FAILED;

void init_sem(void) {
    /*---------------------------------------*/
    /* TODO 1 : init semaphore               */
    /* create/open POSIX named semaphore     */
    /* using sem_open() with initial value 1 */

    /* TODO 1 : END                          */
    /*---------------------------------------*/
    printf("init semaphore : %s\n", SEM_NAME);
}

void destroy_sem(void) {
    /*---------------------------------------*/
    /* TODO 2 : destroy semaphore            */
    /* close using sem_close() and remove    */
    /* named semaphore using sem_unlink()    */

    /* TODO 2 : END                          */
    /*---------------------------------------*/
}

void s_wait(void) {
    if (sem == SEM_FAILED) {
        sem = sem_open(SEM_NAME, 0);
        if (sem == SEM_FAILED) {
            printf("<s_wait> sem_open error!\n");
            return;
        }
    }

    if (sem_wait(sem) == -1) {
        printf("<s_wait> sem_wait error!\n");
	return;
    }
}

void s_quit(void) {
    if (sem == SEM_FAILED) {
        sem = sem_open(SEM_NAME, 0);
        if (sem == SEM_FAILED) {
            printf("<s_quit> sem_open error!\n");
            return;
        }
    }

    if (sem_post(sem) == -1) {
        printf("<s_quit> sem_post error!\n");
	return;
    }
}

/*---------------------------------------------*/
/* TODO 3 : use s_quit() and s_wait()          */
/* use POSIX shared memory and semaphore       */
/* in the functions below                      */

int init_buffer(MessageBuffer **buffer) {
    /*---------------------------------------*/
    /* TODO 3-1 : init buffer                */
    /* (1) create POSIX shared memory        */
    /*     using shm_open()                  */
    /* (2) set size using ftruncate()        */
    /* (3) map memory using mmap()           */
    /* (4) initialize MessageBuffer fields   */

    /* TODO 3-1 : END                        */
    /*---------------------------------------*/

    printf("init buffer\n");
    return 0;
}

int attach_buffer(MessageBuffer **buffer) {
    /*---------------------------------------*/
    /* TODO 3-2 : attach buffer              */
    /* do not consider "no buffer situation" */
    /* (1) open POSIX shared memory          */
    /*     using shm_open()                  */
    /* (2) map memory using mmap()           */
    /* (3) assign mapped address to *buffer  */

    /* TODO 3-2 : END                        */
    /*---------------------------------------*/

    printf("attach buffer\n");
    printf("\n");
    return 0;
}

int detach_buffer(void) {
    if (munmap(memory_segment, sizeof(MessageBuffer)) == -1) {
        printf("munmap error!\n\n");
        return -1;
    }

    if (shm_fd != -1) close(shm_fd);
    if (sem != SEM_FAILED) sem_close(sem);

    printf("detach buffer\n\n");
    return 0;
}

int destroy_buffer(void) {
    if(shm_unlink(SHM_NAME) == -1) {
        printf("shm_unlink error!\n\n");
        return -1;
    }

    printf("destroy shared_memory\n\n");
    return 0;
}

int produce(MessageBuffer **buffer, int sender_id, int data, int account_id) {

    /*---------------------------------------*/
    /* TODO 3-3 : produce message            */
    /* protect the shared buffer using       */
    /* s_wait() and s_quit()                 */
    /* write sender_id, data, and account_id */
    /* to the shared single-slot buffer      */

    /* TODO 3-3 : END                        */
    /*---------------------------------------*/

    printf("produce message\n");

    return 0;
}

int consume(MessageBuffer **buffer, Message **message) {

    /*---------------------------------------*/
    /* TODO 3-4 : consume message            */
    /* protect the shared buffer using       */
    /* s_wait() and s_quit()                 */
    /* read the message from shared memory   */
    /* and update the buffer state           */

    /* TODO 3-4 : END                        */
    /*---------------------------------------*/
    return 0;
}

/* TODO 3 : END                                */
/*-------------------------------------------------------*/
