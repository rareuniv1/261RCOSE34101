#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MAX_PROCESSES 10

// ----------------------------------------------- [New Phase] -------------------------------------------------

// 프로세스의 현재 상태를 나타내는 열거형
typedef enum { NEW, READY, RUNNING, WAITING, TERMINATED } ProcessState;
// I/O 작업 정보 구조체
typedef struct { int IO_Device;  int IO_Start_Time;  int IO_Burst_Time; } IO_Req;
// 프로세스 구조체
typedef struct {
    // 기본 정보
    int PID;    int Arrival_Time;   int CPU_Burst_Time_T; int Priority;
    // IO 정보
    int IO_Req_Num; IO_Req IO_Reqs[3];
    // 상태정보
    ProcessState P_State;
    int Now_Arrival;    int CPU_Burst_Time_N;   int IO_Req_Now;
} Process;
// 프로세스 초기화 함수
void Create_Process(Process job_pool[], int num_processes) {
    for (int i = 0; i < num_processes; i++) {
        Process *p = &job_pool[i];
        p->PID = i + 1; 
        // 고정변수 랜덤 할당
        p->Arrival_Time = rand() % 41;                  // [0, 40]
        p->CPU_Burst_Time_T = (rand() % 17) + 4;        // [4, 20]
        p->Priority = (rand() % 10) + 1;                // [1, 10]
        p->IO_Req_Num = rand() % 4;                     // [0, 3]
        // 초기 상태는 NEW(생성됨)로 설정
        p->P_State = NEW; 
        // 동적 변수 초기화
        p->Now_Arrival = p->Arrival_Time;
        p->CPU_Burst_Time_N = p->CPU_Burst_Time_T;
        p->IO_Req_Now = 0;
        
        // I/O 작업 생성
        for (int j = 0; j < p->IO_Req_Num; j++) {
            p->IO_Reqs[j].IO_Device = (rand() % 5) + 1;      // [1, 5]
            p->IO_Reqs[j].IO_Burst_Time = (rand() % 20) + 1; // [1, 20]
            // I/O 발생 시간은 CPU 작업 중간에 일어나야 하므로 [1, CPU_Burst_Time_T - 1] 범위 내에서 생성
            if (p->CPU_Burst_Time_T > 1) {
                p->IO_Reqs[j].IO_Start_Time = (rand() % (p->CPU_Burst_Time_T - 1)) + 1;
            } else {
                p->IO_Reqs[j].IO_Start_Time = 1; 
            }
        }
        
        // I/O 작업 정렬 : 개수가 최대 3개이므로 가벼운 버블 정렬(Bubble Sort) 사용
        for (int j = 0; j < p->IO_Req_Num - 1; j++) {
            for (int k = 0; k < p->IO_Req_Num - j - 1; k++) {
                if (p->IO_Reqs[k].IO_Start_Time > p->IO_Reqs[k+1].IO_Start_Time) {
                    IO_Req temp = p->IO_Reqs[k];
                    p->IO_Reqs[k] = p->IO_Reqs[k+1];
                    p->IO_Reqs[k+1] = temp;
                }
            }
        }
    }
}
// ProcessState 열거형을 출력용 문자열로 변환하는 헬퍼 함수
const char* Get_State_String(ProcessState state) {
    switch(state) {
        case NEW:        return "NEW";
        case READY:      return "READY";
        case RUNNING:    return "RUN";
        case WAITING:    return "WAIT";
        case TERMINATED: return "TERM";
        default:               return "UNKNOWN";
    }
}
// 프로세스 리스트 출력 함수
void Print_Process_List(Process pool[], int count) {
    printf("=========================================================================================================\n");
    printf(" PID | Arr | CPU_T | Pri | State | Now_Arr | CPU_N | IO_Idx | I/O Requests (Device, Start, Burst)\n");
    printf("=========================================================================================================\n");
    
    for (int i = 0; i < count; i++) {
        Process p = pool[i];
        
        // 기본 정보 및 P3-4 내부 변수 출력
        printf(" %3d | %3d | %5d | %3d | %5s | %7d | %5d | %6d | ",
               p.PID, 
               p.Arrival_Time, 
               p.CPU_Burst_Time_T, 
               p.Priority,
               Get_State_String(p.P_State),
               p.Now_Arrival, 
               p.CPU_Burst_Time_N, 
               p.IO_Req_Now);

        // P3-3 I/O 작업 정보 출력
        if (p.IO_Req_Num == 0) {
            printf("None\n");
        } else {
            for (int j = 0; j < p.IO_Req_Num; j++) {
                printf("[D%d: S%d, B%d] ", 
                       p.IO_Reqs[j].IO_Device, 
                       p.IO_Reqs[j].IO_Start_Time, 
                       p.IO_Reqs[j].IO_Burst_Time);
            }
            printf("\n"); // I/O 출력이 끝나면 줄바꿈
        }
    }
    printf("=========================================================================================================\n");
}

// ------------------------------------------- [Ready Ready Phase] -------------------------------------------------

// Ready queue 리스트
typedef struct {
    Process* list[MAX_PROCESSES]; // 프로세스 포인터 배열
    int count;                    // 현재 큐에 있는 프로세스 개수
} ReadyQueue;
// Ready queue 초기화 
void Init_ReadyQueue(ReadyQueue* rq) {
    rq->count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) { rq->list[i] = NULL; }
}
// 리스트 끝에 프로세스 추가
void Insert_ReadyQueue(ReadyQueue* rq, Process* p) {
    if (rq->count >= MAX_PROCESSES) {
        printf("Error: Ready Queue is full.\n");
        return;
    }
    p->P_State = READY;
    rq->list[rq->count++] = p;
}
// 특정 인덱스의 프로세스를 꺼내고(Remove), 빈자리를 당겨서 채움
Process* Remove_ReadyQueue(ReadyQueue* rq, int index) {
    if (index < 0 || index >= rq->count) {
        return NULL;
    }
    Process* p = rq->list[index];
    // 삭제된 원소 뒤의 항목들을 앞으로 한 칸씩 이동
    for (int i = index; i < rq->count - 1; i++) {
        rq->list[i] = rq->list[i + 1];
    }
    rq->list[rq->count - 1] = NULL;
    rq->count--;
    return p;
}

// ---------------------------------------------- [Ready Phase] -------------------------------------------------

int compare_fcfs(const void *a, const void *b) {
    Process *p1 = *(Process **)a;
    Process *p2 = *(Process **)b;
    // 1순위: 도착 시간이 빠른 순서
    if (p1->Now_Arrival != p2->Now_Arrival) {
        return p1->Now_Arrival - p2->Now_Arrival;
    }
    // 2순위 (Tie-breaker): 도착 시간이 같다면 PID가 작은(먼저 생성된) 프로세스 우선
    return p1->PID - p2->PID;
}
int compare_sjf_nonpreemp(const void *a, const void *b) {
    Process *p1 = *(Process **)a;
    Process *p2 = *(Process **)b;
    // 1순위: 남은 CPU 작업 시간이 짧은 순서
    if (p1->CPU_Burst_Time_N != p2->CPU_Burst_Time_N) {
        return p1->CPU_Burst_Time_N - p2->CPU_Burst_Time_N;
    }
    // 2순위 (Tie-breaker): 작업 시간이 같다면 먼저 도착한 순서 (FCFS)
    if (p1->Now_Arrival != p2->Now_Arrival) {
        return p1->Now_Arrival - p2->Now_Arrival;
    }
    // 3순위: 도착 시간까지 같다면 PID 순서
    return p1->PID - p2->PID;
}
int compare_priority_nonpreemp(const void *a, const void *b) {
    Process *p1 = *(Process **)a;
    Process *p2 = *(Process **)b;
    // 1순위: 우선순위 값이 작은 순서 (1이 가장 높고 10이 가장 낮음)
    if (p1->Priority != p2->Priority) {
        return p1->Priority - p2->Priority;
    }
    // 2순위 (Tie-breaker): 우선순위가 같다면 먼저 도착한 순서 (FCFS)
    if (p1->Now_Arrival != p2->Now_Arrival) {
        return p1->Now_Arrival - p2->Now_Arrival;
    }
    // 3순위: PID 순서
    return p1->PID - p2->PID;
}


int main() {
    // 시드 설정 (테스트 시에는 특정 시드값을 고정해서 디버깅하는 것도 좋습니다)
    srand((unsigned int)time(NULL)); 

    int num_processes = rand()%9+2; // 5개만 테스트로 생성
    Process job_pool[num_processes];

    // 1. 프로세스 생성 및 초기화
    Create_Process(job_pool, num_processes);

    // 2. 생성된 프로세스 목록 출력
    printf(">>> System Booting... Initializing PCB Factory...\n");
    Print_Process_List(job_pool, num_processes);

    // 3. 이후 Ready Queue 이동 및 Simulation Loop 시작...
    
    return 0;
}