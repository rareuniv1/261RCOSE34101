#include <stdio.h>
#include <stdlib.h>
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
void Create_Process(Process* plist, int count) {
    for (int i = 0; i < count; i++) {
        plist[i].pid = i + 1;
        // 1. 기본 정보 설정 (확률공간 0~20, CPU burst는 4~20)
        plist[i].arrival_time = rand() % 21;           // 0 ~ 20초
        plist[i].cpu_burst = (rand() % 17) + 4;        // 4 ~ 20초 (하한 4초)
        plist[i].io_count = rand() % (MAX_IO_EVENTS + 1); // 0 ~ 3회
        // 2. I/O 상세 정보 설정
        // CPU burst를 io_count + 1 등분하여 겹치지 않는 offset 생성
        int section = plist[i].cpu_burst / (plist[i].io_count + 1);
        for (int j = 0; j < plist[i].io_count; j++) {
            plist[i].io_requests[j].device_id = rand() % 5 + 1; // 장치 1~5
            plist[i].io_requests[j].io_burst = rand() % 21;  // I/O 시간 0~20
            // I/O 시작 시점(offset) 분산 배치
            // 각 섹션 안에서 랜덤하게 발생하게 하여 중복 방지
            int min_offset = (j * section) + 1;
            int max_offset = (j + 1) * section;
            if (max_offset >= plist[i].cpu_burst) max_offset = plist[i].cpu_burst - 1;
            plist[i].io_requests[j].io_start_offset = min_offset + (rand() % (max_offset - min_offset + 1));
        }

        // 3. 가변 상태 초기화
        plist[i].accumulated_cpu = 0;
        plist[i].rem_cpu = plist[i].cpu_burst;
        plist[i].next_io_idx = 0;
        plist[i].is_terminated = false;
    }
}