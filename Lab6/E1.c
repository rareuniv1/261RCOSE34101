#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define SHM_NAME "/ipc_no_semaphore_demo"

int main(){
    int shm_fd;
    int *num;
    void *memory_segment = NULL;

    // POSIX shared memory
    if ((shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666)) == -1) return -1;

    if (ftruncate(shm_fd, sizeof(int)) == -1) return -1;

    printf("pid : %d\n", getpid());
    printf("shm fd : %d\n", shm_fd);

    memory_segment = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) return -1;

    num = (int*)memory_segment;

    for (int i=0; i<1000000; i++) {
        (*num)++;
    }
    printf("num : %d\n", (*num));

    munmap(memory_segment, sizeof(int));
    close(shm_fd);

    return 0;
}
