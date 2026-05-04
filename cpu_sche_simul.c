#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// 질문 결과 : I/O는 예를 들어 0~3 범위 내에서 일어나도록 할 수 있다.
#define MAX_IO_EVENTS 3 
// 한 줄에 담을 수 있는 범위에 따라 달라질 듯?
#define MAX_PROCESSES 10

// 1. 개별 IO 요청을 담을 구조체 IO_Request
typedef struct {
    int io_start_offset; // I/O가 시작될 부분
    int io_burst;        // I/O 종료시각 : io_burst + io_start_offset + Waiting time
} IO_Request;
// 2. 프로세스 구조체
typedef struct {
    int pid;                                // 프로세스를 나타낼 숫자
    int arrival_time;                       // 프로세스가 Ready Queue에 도착하는 시간
    int cpu_burst;                          // 프로세스의 총 CPU running time
    
    int io_count;                           // 프로세스의 IO 인터럽트 발생 횟수
    IO_Request io_requests[MAX_IO_EVENTS];  // 프로세스의 IO 작업 정보
    int next_io_idx;                        // 다음에 처리할 IO 작업을 가리키는 포인터 역할
    
    int priority;                           // 프로세스의 우선순위
} Process;

// 3. 프로세스 생성기 : 외부에서 프로세스 개수 count를 받아 plist를 뽑아냄
void Create_Process(Process* plist, int count) {
    for (int i = 0; i < count; i++) {
        plist[i].pid = i + 1;                       // pid는 순서대로 붙임
        plist[i].arrival_time = rand() % 20;        // arrival_time은 넉넉히 0~19까지의 범위로 설정 (이유 없음! - 일단 해봄)
        plist[i].cpu_burst = (rand() % 20) + 5;     // cpu burst time은 최소 5~24까지의 범위로 설정 (하한은 이유 있음 : 3회 IO!) 
        plist[i].priority = (rand() % 15) + 1;      // priority는 1~15의 공간임 (최대 8개 - 15까지 설정해야 적당함)
        plist[i].next_io_idx = 0;                   // 큐의 Front 초기화

        int num_io = rand() % (MAX_IO_EVENTS + 1);  // I/O 발생 횟수 랜덤 지정 (0 ~ MAX_IO_EVENTS)
        
        if (num_io >= plist[i].cpu_burst) {         // I/O 횟수가 CPU 시간보다 많아지지 않도록 보장 (최소 1초 간격 보장)
            num_io = plist[i].cpu_burst - 1;        // 물론 앞에서, 4까지는 안전할 수 있도록 만듬. 하지만 MAX_IO_EVENTS가 바뀌었을 때 고려
        }                                           // 아무리 랜덤해야 한다고 해도, IO 횟수가 CPU 시간보다 많아진다면 본말전도
        plist[i].io_count = num_io;

        // I/O 데이터 생성
        for (int j = 0; j < num_io; ) {
            int offset = (rand() % (plist[i].cpu_burst - 1)) + 1;           // offset은 CPU burst 시간까지임
            
            int duplicate = 0;
            for (int k = 0; k < j; k++) {
                if (plist[i].io_requests[k].io_start_offset == offset) {    // 데이터를 생성했는데 만약 중복이라면
                    duplicate = 1;              
                    break;                                                  // 큐에 추가 x
                }
            }
            
            
            if (!duplicate) {                                               // 중복 아닐 때만
                plist[i].io_requests[j].io_start_offset = offset;           // 큐에 추가될 수 있도록 함
                plist[i].io_requests[j].io_burst = (rand() % 10) + 1;        // IO 버스트는 1~10의 짧은 시간 동안 이루어지도록 함
                j++;                                                        
            }
        }

        // 발생 시점(offset) 기준으로 오름차순 정렬 (버블 정렬)
        for (int x = 0; x < num_io - 1; x++) {
            for (int y = 0; y < num_io - 1 - x; y++) {
                if (plist[i].io_requests[y].io_start_offset > plist[i].io_requests[y+1].io_start_offset) {
                    IO_Request temp = plist[i].io_requests[y];
                    plist[i].io_requests[y] = plist[i].io_requests[y+1];
                    plist[i].io_requests[y+1] = temp;
                }
            }
        }
    }
}
// 4. 출력 함수
void Print_Processes(Process* plist, int count) {
    printf("========================================================================================\n");
    printf(" PID | Arrival | CPU | I/O Cnt | I/O Details [Offset:Burst]                 | Priority \n");
    printf("========================================================================================\n");
    for (int i = 0; i < count; i++) {
        printf(" %3d | %7d | %3d | %7d | ",
               plist[i].pid, plist[i].arrival_time, plist[i].cpu_burst, plist[i].io_count);
               
        // 다중 I/O 출력 (문자열 폭을 맞추기 위해 버퍼 사용)
        char io_str[60] = "";
        if (plist[i].io_count == 0) {
            sprintf(io_str, "None");
        } else {
            int pos = 0;
            for (int j = 0; j < plist[i].io_count; j++) {
                pos += sprintf(&io_str[pos], "[O:%2d, B:%d] ", 
                               plist[i].io_requests[j].io_start_offset,
                               plist[i].io_requests[j].io_burst);
            }
        }
        
        // %-42s : 왼쪽 정렬하여 42칸 차지 (줄 맞춤 용도)
        printf("%-42s | %8d \n", io_str, plist[i].priority);
    }
    printf("========================================================================================\n");
}

// 1. 큐(Queue) 구조체 정의
typedef struct {
    Process* items[MAX_PROCESSES]; // 구조체 포인터 배열로 큐 구현
    int front;
    int rear;
    int count;
} Queue;
// 글로벌 시스템 큐 선언
Queue ready_queue;
Queue waiting_queue;
// 2. 큐 초기화 함수
void Init_Queue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}
// 3. 환경 설정 함수 (Config)
void Config() {
    // 시스템 큐 초기화
    Init_Queue(&ready_queue);
    Init_Queue(&waiting_queue);
    
    printf("[Config] Ready Queue 및 Waiting Queue 초기화 완료.\n");
}

// 4. 업데이트된 메인 함수
int main() {
    int process_count = 7;
    Process job_pool[7];  
    // 1) 난수 시드 초기화 (필수)
    srand((unsigned int)time(NULL));
    // 2) 시스템 환경 설정 (큐 초기화)
    Config();
    // 3) 프로세스 생성
    Create_Process(job_pool, process_count);
    
    printf("[System] %d개의 프로세스가 생성되었습니다. 스케줄러 시뮬레이션을 시작할 준비가 되었습니다.\n", process_count);
    Print_Processes(job_pool, process_count);

    // TODO: 여기에 current_time 기반의 시뮬레이션 루프가 들어갑니다.
    // 예: while(종료조건) { ... current_time++; }

    return 0;
}