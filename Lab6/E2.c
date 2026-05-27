#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#define SHM_NAME "/ipc_semaphore_demo"
#define SEM_NAME "/ipc_semaphore_lock"

void s_wait(sem_t *sem) {
    if (sem_wait(sem) == -1) {
        printf("<s_wait> sem_wait error!\n");
        return;
    }
}

void s_quit(sem_t *sem) {
    if (sem_post(sem) == -1) {
        printf("<s_quit> sem_post error!\n");
        return;
    }
}

int main(){
    int shm_fd;
    int *num;
    void *memory_segment = NULL;

    sem_t *sem;

    // POSIX shared memory
    if ((shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666)) == -1) return -1;

    if (ftruncate(shm_fd, sizeof(int)) == -1) return -1;

    printf("pid : %d\n", getpid());
    printf("shm fd : %d\n", shm_fd);

    memory_segment = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) return -1;

    // POSIX named semaphore
    if ((sem = sem_open(SEM_NAME, O_CREAT, 0666, 1)) == SEM_FAILED) return -1;

    printf("sem name : %s\n", SEM_NAME);

    num = (int*)memory_segment;

    for (int i=0; i<1000000; i++) {
        s_wait(sem);
        (*num)++;
        s_quit(sem);
    }
    printf("num : %d\n", (*num));

    sem_close(sem);
    munmap(memory_segment, sizeof(int));
    close(shm_fd);

    return 0;
}
