#include <stdbool.h>

#define MAX_IO_EVENTS 3

// 1. I/O 작업 상세 구조체
typedef struct {
    int device_id;  // I/O 장치 번호
    int io_start_offset;  // I/O 작업 시작시간
    int io_burst;  // I/O 작업 자체에 소요되는 시간
    int io_end_time;  // I/O 작업 잔여시간
} IO_Request;

// 2. 프로세스 통합 구조체
typedef struct {
    // 고정적
    int pid;  //프로세스 번호
    int arrival_time;  // 도착시간
    int cpu_burst;          // 총 CPU 실행시간
    int priority;           // 우선순위
    int io_count;           // 총 I/O 발생 횟수
    IO_Request io_requests[MAX_IO_EVENTS]; // I/O 상세 정보 배열
    // 변동
    int accumulated_cpu;  // 현재까지 누적된 시간
    int rem_cpu;  // 잔여 CPU 실행시간 
    int next_io_idx;  // 다음 처리해야 할 I/O 작업
    
    bool is_terminated;  // 프로세스 종료 여부
    int wait_time;  // Ready Queue에서 대기한 총 시간 (통계용)
} Process;