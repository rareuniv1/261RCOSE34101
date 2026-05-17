#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MAX_PROCESSES 10
#define NUM_PROCESSES 10
#define MAX_IO_DEVICES 5
#define MAX_TIME 1000    // 시뮬레이션 최대 시간 (타임아웃 방지 및 간트 차트 배열 크기)
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
    // 통계정보
    int Termination_Time;   int Turnaround_Time;    int Waiting_Time;   int IO_Burst_Time_T;
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
        // 통계 변수 초기화
        p->Termination_Time = 0;
        p->Turnaround_Time = 0;
        p->Waiting_Time = 0;
        p->IO_Burst_Time_T = 0;
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
            p->IO_Burst_Time_T += p->IO_Reqs[j].IO_Burst_Time;
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

// ---------------------------------------------- [Ready Phase] ---------------------------------------------------

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
int compare_sjf(const void *a, const void *b) {
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
int compare_priority(const void *a, const void *b) {
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
int compare_srtf(const void *a, const void *b) {
    Process *p1 = *(Process **)a;
    Process *p2 = *(Process **)b;

    if (p1->CPU_Burst_Time_N != p2->CPU_Burst_Time_N)
        return p1->CPU_Burst_Time_N - p2->CPU_Burst_Time_N;
    if (p1->Now_Arrival != p2->Now_Arrival)
        return p1->Now_Arrival - p2->Now_Arrival;
    return p1->PID - p2->PID;
}
int compare_priority_preemp(const void *a, const void *b) {
    Process *p1 = *(Process **)a;
    Process *p2 = *(Process **)b;

    if (p1->Priority != p2->Priority)
        return p1->Priority - p2->Priority;
    if (p1->Now_Arrival != p2->Now_Arrival)
        return p1->Now_Arrival - p2->Now_Arrival;
    return p1->PID - p2->PID;
}
void Sort_ReadyQueue(ReadyQueue* rq, int (*compare_func)(const void*, const void*)) {
    if (rq->count > 1) {
        qsort(rq->list, rq->count, sizeof(Process*), compare_func);
    }
}

// ----------------------------------------- [Ready - Running Phase] ----------------------------------------------

ProcessState Run_Process_Tick(Process* p) {
    if (p == NULL || p->P_State != RUNNING) {
        // 방어적 코드: 실행 상태가 아닌 프로세스가 들어오면 현재 상태 그대로 반환
        return p != NULL ? p->P_State : READY; 
    }
    // 1. CPU를 1초 사용했으므로 잔여 작업시간 1 감소
    p->CPU_Burst_Time_N--;
    // 현재까지 CPU를 실제로 사용한 '누적 시간' 계산
    // (예: 총 10초 중 8초 남았다면, 2초를 사용한 것)
    int cpu_used_time = p->CPU_Burst_Time_T - p->CPU_Burst_Time_N;
    // 2. 프로세스 완전 종료 조건 확인
    if (p->CPU_Burst_Time_N <= 0) {
        p->P_State = TERMINATED;
        printf("[TIME TICK] PID %d: Execution Finished! (Terminated)\n", p->PID);
        return TERMINATED;
    }
    // 3. I/O 발생 조건 확인
    // 현재 진행해야 할 I/O가 남아있다면 (IO_Req_Now 인덱스가 IO_Req_Num보다 작다면)
    if (p->IO_Req_Now < p->IO_Req_Num) {
        // 현재 누적 CPU 사용 시간이 해당 I/O 작업의 시작 시점(IO_Start_Time)과 일치하는가?
        if (cpu_used_time == p->IO_Reqs[p->IO_Req_Now].IO_Start_Time) {
            p->P_State = WAITING;
            printf("[TIME TICK] PID %d: I/O Request Occurred! (Moving to Wait Queue)\n", p->PID);
            return WAITING;
        }
    }
    // 종료되지도 않았고 I/O 발생 시간도 아니라면, 계속 CPU를 쓰기 위해 RUNNING 상태 유지
    return RUNNING;
}
Process* Schedule_FCFS(ReadyQueue* rq) {
    if (rq->count == 0) return NULL; // 큐가 비어있으면 NULL 반환

    // 도착 시간(Now_Arrival) 기준으로 정렬
    Sort_ReadyQueue(rq, compare_fcfs);
    
    // 가장 앞에 있는(도착이 가장 빠른) 프로세스를 꺼내서 반환
    return Remove_ReadyQueue(rq, 0);
}
Process* Schedule_SJF_NonPreemp(ReadyQueue* rq) {
    if (rq->count == 0) return NULL;

    // 잔여 시간(CPU_Burst_Time_N) 기준으로 정렬
    Sort_ReadyQueue(rq, compare_sjf);
    
    // 가장 앞에 있는(작업 시간이 가장 짧은) 프로세스를 꺼내서 반환
    return Remove_ReadyQueue(rq, 0);
}
Process* Schedule_Priority_NonPreemp(ReadyQueue* rq) {
    if (rq->count == 0) return NULL;

    // 우선순위(Priority) 기준으로 정렬
    Sort_ReadyQueue(rq, compare_priority);
    
    // 가장 앞에 있는(우선순위가 가장 높은) 프로세스를 꺼내서 반환
    return Remove_ReadyQueue(rq, 0);
}
Process* Schedule_RR(ReadyQueue* rq) {
    if (rq->count == 0) return NULL;

    // 도착 시간(Now_Arrival) 기준으로 정렬 (FCFS와 동일한 비교 함수 사용)
    Sort_ReadyQueue(rq, compare_fcfs);
    
    // 큐의 가장 앞에 있는 프로세스를 꺼내서 반환
    return Remove_ReadyQueue(rq, 0);
}
Process* Check_Preemptive_SJF(ReadyQueue* rq, Process* current_running, int system_time) {
    if (current_running == NULL || rq->count == 0) {
        return current_running; // 비교할 대상이 없으면 그대로 반환
    }

    // 1. Ready Queue를 SRTF 기준으로 정렬하여 1등을 맨 앞으로 보냄
    Sort_ReadyQueue(rq, compare_srtf);
    Process* top_ready = rq->list[0];

    // 2. [선점 조건] Ready 큐 1등의 남은 시간이 현재 실행 중인 프로세스의 남은 시간보다 짧은가?
    if (top_ready->CPU_Burst_Time_N < current_running->CPU_Burst_Time_N) {
        
        printf("[TIME %d] Preemption! PID %d preempted by PID %d (Remaining: %d < %d)\n", 
               system_time, current_running->PID, top_ready->PID, 
               top_ready->CPU_Burst_Time_N, current_running->CPU_Burst_Time_N);

        // 3. 현재 프로세스를 쫓아낼 준비 (도착 시간 업데이트)
        // -> 이렇게 해야 쫓겨난 애가 나중에 동점자 발생 시 FCFS 조건에서 뒤로 밀립니다!
        current_running->Now_Arrival = system_time;
        
        // 4. 강제 교체 (Context Switch)
        Insert_ReadyQueue(rq, current_running);           // 쫓겨난 애를 큐에 넣음
        Sort_ReadyQueue(rq, compare_srtf);                // 큐 다시 정렬
        return Remove_ReadyQueue(rq, 0);                  // 새로운 1등을 뽑아서 반환
    }

    return current_running; // 선점 조건에 맞지 않으면 기존 프로세스 계속 실행
}
Process* Check_Preemption_Priority(ReadyQueue* rq, Process* current_running, int system_time) {
    if (current_running == NULL || rq->count == 0) return current_running;

    Sort_ReadyQueue(rq, compare_priority_preemp);
    Process* top_ready = rq->list[0];

    // [선점 조건] Ready 큐 1등의 우선순위 숫자가 현재 실행 중인 애보다 작고(우선순위 높음) 강력한가?
    if (top_ready->Priority < current_running->Priority) {
        
        printf("[TIME %d] Preemption! PID %d preempted by PID %d (Priority: %d < %d)\n", 
               system_time, current_running->PID, top_ready->PID, 
               top_ready->Priority, current_running->Priority);

        current_running->Now_Arrival = system_time; // 쫓겨난 시간 기록
        
        Insert_ReadyQueue(rq, current_running);
        Sort_ReadyQueue(rq, compare_priority_preemp);
        return Remove_ReadyQueue(rq, 0);
    }

    return current_running;
}

// --------------------------------------------- [Waiting Phase] -------------------------------------------------

typedef struct {
    Process* queue[MAX_PROCESSES]; // 프로세스 포인터를 담는 배열
    int front;                     // 데이터를 꺼낼(Dequeue) 인덱스
    int rear;                      // 데이터를 넣을(Enqueue) 인덱스
    int count;                     // 현재 큐에 들어있는 데이터의 개수
} WaitQueue;
void Init_WaitQueue(WaitQueue* wq) {
    wq->front = 0;
    wq->rear = 0;
    wq->count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        wq->queue[i] = NULL;
    }
}
void Enqueue_WaitQueue(WaitQueue* wq, Process* p) {
    if (wq->count >= MAX_PROCESSES) {
        printf("Error: Wait Queue is full. Cannot enqueue PID %d\n", p->PID);
        return;
    }
    
    // 상태를 WAITING으로 변경
    p->P_State = WAITING; 
    
    // rear 위치에 삽입 후, 원형으로 회전
    wq->queue[wq->rear] = p;
    wq->rear = (wq->rear + 1) % MAX_PROCESSES; 
    wq->count++;
}
Process* Dequeue_WaitQueue(WaitQueue* wq) {
    if (wq->count == 0) {
        return NULL; // 비어있음
    }
    
    // front 위치의 프로세스를 꺼내고, 원형으로 회전
    Process* p = wq->queue[wq->front];
    wq->queue[wq->front] = NULL;
    wq->front = (wq->front + 1) % MAX_PROCESSES;
    wq->count--;
    
    return p;
}
void WakeUp_Process(WaitQueue* wq, ReadyQueue* rq, int system_time) {
    // 1. Wait Queue에서 프로세스 꺼내기 (순수 자료구조 함수 사용)
    Process* p = Dequeue_WaitQueue(wq);
    
    if (p != NULL) {
        // 2. 다음 I/O 작업을 위해 인덱스 증가
        p->IO_Req_Now++;
        
        // 3. Ready Queue 도착 시간 갱신 (선점형 스케줄링을 위한 필수 작업!)
        p->Now_Arrival = system_time;
        
        // 4. Ready Queue로 자동 삽입
        Insert_ReadyQueue(rq, p);
        
        printf("[TIME %d] PID %d: Wake up! (Wait -> Ready Queue)\n", system_time, p->PID);
    }
}
void Process_IO_Ticks(WaitQueue queues[], ReadyQueue* rq, int system_time) {
    for (int i = 1; i <= MAX_IO_DEVICES; i++) {
        
        // 해당 장치의 큐에 기다리고 있는 프로세스가 있다면
        if (queues[i].count > 0) {
            // 맨 앞에 있는 프로세스의 데이터를 엿봄 (아직 Dequeue는 안 함)
            Process* p = queues[i].queue[queues[i].front];
            int current_io_idx = p->IO_Req_Now;

            // 해당 프로세스의 남은 I/O 시간 1 감소
            p->IO_Reqs[current_io_idx].IO_Burst_Time--;

            // 1초를 깎았는데 만약 0이 되었다면? (I/O 작업 완전 종료)
            if (p->IO_Reqs[current_io_idx].IO_Burst_Time <= 0) {
                // 한 번에 큐 이동, 상태 변경, 시간 갱신 처리!
                WakeUp_Process(&queues[i], rq, system_time);
            }
        }
    }
}

// ------------------------------------------- [Terminating Phase] -------------------------------------------------

void Record_Termination(Process* p, int system_time) {
    if (p == NULL) return;

    p->P_State = TERMINATED;
    p->Termination_Time = system_time;
    p->Turnaround_Time = p->Termination_Time - p->Arrival_Time;

    printf("====================================================\n");
    printf("[TIME %d] 🏁 PID %d Terminated!\n", system_time, p->PID);
    printf(" - Arrival Time   : %d\n", p->Arrival_Time);
    printf(" - Turnaround Time: %d\n", p->Turnaround_Time);
    printf(" - Waiting Time   : %d\n", p->Waiting_Time);
    printf("====================================================\n");
}

// 원본 프로세스 풀 복사 함수 (파이프라인 2 확장을 위한 준비)
void Copy_Process_Pool(Process source[], Process dest[], int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = source[i];
    }
}

// 알고리즘 종류를 구분하기 위한 열거형(Enum)
typedef enum {
    ALGO_FCFS = 2,
    ALGO_NP_SJF = 3,
    ALGO_NP_PRIORITY = 4,
    ALGO_RR = 5,
    ALGO_P_SJF = 6,
    ALGO_P_PRIORITY = 7
} AlgoType;

// 시뮬레이션 실행 결과를 담을 통계 구조체 (8번 비교 화면용)
typedef struct {
    double avg_turnaround;
    double avg_waiting;
    int success; // 실행 여부 플래그
} AlgoResult;

// 단일 시뮬레이션을 수행하는 핵심 코어 함수 (로그 출력 여부를 선택할 수 있음)
// 매개변수에 'int num_processes'가 추가되었습니다!
AlgoResult Execute_Simulation(Process origin_pool[], int num_processes, AlgoType algo, int time_quantum, int show_logs) {
    // 이제 최대 크기(MAX_PROCESSES)만큼 배열을 넉넉히 잡아둡니다.
    Process working_pool[MAX_PROCESSES]; 
    ReadyQueue ready_queue;
    WaitQueue device_wait_queues[MAX_IO_DEVICES + 1];
    
    int gantt_record[MAX_TIME];
    for (int i = 0; i < MAX_TIME; i++) gantt_record[i] = -1;

    // 데이터 복사 및 초기화 (num_processes 만큼만 복사!)
    for (int i = 0; i < num_processes; i++) {
        working_pool[i] = origin_pool[i]; 
    }
    Init_ReadyQueue(&ready_queue);
    for (int i = 1; i <= MAX_IO_DEVICES; i++) Init_WaitQueue(&device_wait_queues[i]);

    int system_time = 0;
    int terminated_count = 0;
    int current_q_time = 0; // RR용 퀀텀 타이머
    Process* current_running = NULL;

    if (show_logs) {
        printf("\n=================================================================\n");
        printf(" 🚀 시뮬레이션 시작! (알고리즘 번호: %d, 프로세스 개수: %d)\n", algo, num_processes);
        printf("=================================================================\n");
    }

    // 종료 조건도 num_processes 로 변경!
    while (terminated_count < num_processes && system_time < MAX_TIME) {
        // 1. 신규 프로세스 도착 확인
        for (int i = 0; i < num_processes; i++) {
            if (working_pool[i].Arrival_Time == system_time) {
                Insert_ReadyQueue(&ready_queue, &working_pool[i]);
                if (show_logs) printf("[TIME %3d] 📥 PID %d 도착! (준비 큐 진입)\n", system_time, working_pool[i].PID);
            }
        }

        // 2. 다중 I/O 장치 처리
        Process_IO_Ticks(device_wait_queues, &ready_queue, system_time);

        // 3. CPU 실행 영역
        if (current_running != NULL) {
            ProcessState next_state = Run_Process_Tick(current_running);
            current_q_time++;

            if (next_state == TERMINATED) {
                // 로그 출력 여부와 상관없이 시간 계산 수행
                current_running->P_State = TERMINATED;
                current_running->Termination_Time = system_time;
                current_running->Turnaround_Time = system_time - current_running->Arrival_Time;

                if (show_logs) {
                    printf("====================================================\n");
                    printf("[TIME %3d] 🏁 PID %d Terminated!\n", system_time, current_running->PID);
                    printf(" - Arrival Time   : %d\n", current_running->Arrival_Time);
                    printf(" - Turnaround Time: %d\n", current_running->Turnaround_Time);
                    printf(" - Waiting Time   : %d\n", current_running->Waiting_Time);
                    printf("====================================================\n");
                }

                terminated_count++;
                current_running = NULL;
                current_q_time = 0;
            } 
            else if (next_state == WAITING) {
                int dev_num = current_running->IO_Reqs[current_running->IO_Req_Now].IO_Device;
                Enqueue_WaitQueue(&device_wait_queues[dev_num], current_running);
                if (show_logs) printf("[TIME %3d] 🔄 PID %d I/O 요청 -> 장치 %d 대기 큐 이동\n", system_time, current_running->PID, dev_num);
                current_running = NULL;
                current_q_time = 0;
            }
            // 3-1. Round Robin 타임 퀀텀 만료 선점 처리
            else if (algo == ALGO_RR && current_q_time >= time_quantum) {
                if (show_logs) printf("[TIME %3d] ⏱️ PID %d 타임 퀀텀 만료! (선점당함)\n", system_time, current_running->PID);
                current_running->Now_Arrival = system_time;
                Insert_ReadyQueue(&ready_queue, current_running);
                current_running = NULL;
                current_q_time = 0;
            }
        }

        // 3-2. 선점형 알고리즘(SRTF, Preemptive Priority) 매 틱 검사
        if (current_running != NULL && ready_queue.count > 0) {
            if (algo == ALGO_P_SJF) {
                current_running = Check_Preemptive_SJF(&ready_queue, current_running, system_time);
                current_running->P_State = RUNNING;
            } 
            else if (algo == ALGO_P_PRIORITY) {
                current_running = Check_Preemption_Priority(&ready_queue, current_running, system_time);
                current_running->P_State = RUNNING;
            }
        }

        // 4. 스케줄러 호출
        if (current_running == NULL && ready_queue.count > 0) {
            switch (algo) {
                case ALGO_FCFS: case ALGO_RR: current_running = Schedule_FCFS(&ready_queue); break;
                case ALGO_NP_SJF:             current_running = Schedule_SJF_NonPreemp(&ready_queue); break;
                case ALGO_NP_PRIORITY:        current_running = Schedule_Priority_NonPreemp(&ready_queue); break;
                case ALGO_P_SJF:              Sort_ReadyQueue(&ready_queue, compare_srtf); current_running = Remove_ReadyQueue(&ready_queue, 0); break;
                case ALGO_P_PRIORITY:         Sort_ReadyQueue(&ready_queue, compare_priority_preemp); current_running = Remove_ReadyQueue(&ready_queue, 0); break;
            }
            current_running->P_State = RUNNING;
            if (show_logs) printf("[TIME %3d] ⚙️  PID %d CPU 할당 완료!\n", system_time, current_running->PID);
        }

        // 5. 간트 차트 기록 및 대기 시간 누적
        if (current_running != NULL) gantt_record[system_time] = current_running->PID;
        for (int i = 0; i < ready_queue.count; i++) ready_queue.list[i]->Waiting_Time++;

        system_time++;
    }

    // 통계 계산 (마찬가지로 num_processes 기준으로 변경)
    double total_tt = 0, total_wt = 0;
    for (int i = 0; i < num_processes; i++) {
        total_tt += working_pool[i].Turnaround_Time;
        total_wt += working_pool[i].Waiting_Time;
    }

    // 간트 차트 출력
    if (show_logs) {
        printf("\n=================================================================\n");
        printf(" 🎨 간트 차트 (CPU 타임라인)\n");
        printf("=================================================================\n");
        int current_pid = gantt_record[0], start_t = 0;
        for (int t = 1; t <= system_time; t++) {
            if (t == system_time || gantt_record[t] != current_pid) {
                if (current_pid == -1) printf("| IDLE (%d-%d) ", start_t, t);
                else printf("| P%d (%d-%d) ", current_pid, start_t, t);
                current_pid = gantt_record[t];
                start_t = t;
            }
        }
        printf("|\n=================================================================\n");
    }

    // 평균 낼 때도 num_processes로 나눔
    AlgoResult res = { total_tt / num_processes, total_wt / num_processes, 1 };
    return res;
}

int main() {
    srand((unsigned int)time(NULL));
    int num_processes = (rand() % 9) + 2;
    Process job_pool_origin[NUM_PROCESSES];
    int is_generated = 0; // 프로세스 집합 생성 여부 체크 플래그
    int menu_choice;
    int time_quantum = 3; // 기본값
    
    AlgoResult benchmark_results[8]; // 2~7번 결과 저장 배열

    while (1) {
        printf("\n=================================================================\n");
        printf(" 🖥️  CPU 스케줄링 대화형 시뮬레이터 (파이프라인 1)\n");
        printf("=================================================================\n");
        printf(" 1. 프로세스 집합 무작위 생성 및 출력\n");
        printf(" 2. FCFS (First-Come, First-Served) 실행\n");
        printf(" 3. Non-preemptive SJF 실행\n");
        printf(" 4. Non-preemptive Priority 실행\n");
        printf(" 5. Round Robin 실행 (Time Quantum 별도 입력)\n");
        printf(" 6. Preemptive SJF (SRTF) 실행\n");
        printf(" 7. Preemptive Priority 실행\n");
        printf(" 8. [종합 평가] 2~7번 알고리즘 평균 시간 비교 테이블 출력\n");
        printf(" 9. 프로그램 종료\n");
        printf("=================================================================\n");
        printf(" 메뉴를 선택하세요 (1~9): ");
        scanf("%d", &menu_choice);

        if (menu_choice == 9) {
            printf("시뮬레이터를 종료합니다. 이용해 주셔서 감사합니다!\n");
            break;
        }

        // 방어적 코드: 프로세스 생성을 안 하고 2~8번을 누른 경우 처리
        if (menu_choice >= 1 && menu_choice <= 8 && !is_generated && menu_choice != 1) {
            printf("\n⚠️ 먼저 1번 메뉴를 선택하여 프로세스 집합을 생성해야 합니다!\n");
            continue;
        }

        switch (menu_choice) {
            case 1:
                // [수정] 변수명(num_processes) 명시
                num_processes = (rand() % 9) + 2;
                
                // [수정] NUM_PROCESSES 대신 num_processes 사용
                Create_Process(job_pool_origin, num_processes);
                printf("\n=================================================================\n");
                printf(" 🏭 시스템 부팅 완료... 신규 프로세스 집합이 생성되었습니다. (총 %d개)\n", num_processes);
                Print_Process_List(job_pool_origin, num_processes);
                is_generated = 1;
                break;

            case 2: case 3: case 4: case 6: case 7:
                // [수정] 두 번째 인자로 num_processes 전달
                Execute_Simulation(job_pool_origin, num_processes, (AlgoType)menu_choice, 0, 1);
                break;

            case 5:
                printf(" 사용할 Time Quantum을 입력하세요 (정수): ");
                scanf("%d", &time_quantum);
                if (time_quantum <= 0) {
                    printf("⚠️ 유효하지 않은 시간입니다. 기본값(3)으로 설정합니다.\n");
                    time_quantum = 3;
                }
                // [수정] 두 번째 인자로 num_processes 전달
                Execute_Simulation(job_pool_origin, num_processes, ALGO_RR, time_quantum, 1);
                break;

            case 8:
                printf("\n=================================================================\n");
                printf(" ⚙️  종합 평가 설정\n");
                printf(" Round Robin에 적용할 Time Quantum 값을 입력하세요 (정수): ");
                scanf("%d", &time_quantum);
                
                if (time_quantum <= 0) {
                    printf("⚠️ 유효하지 않은 시간입니다. 기본값(3)으로 설정합니다.\n");
                    time_quantum = 3;
                }

                // [개선] 몇 개의 프로세스로 측정하는지 안내 출력
                printf("\n동일한 프로세스 셋(%d개)에 대해 모든 알고리즘 성능 측정을 시작합니다...\n", num_processes);
                
                for (int a = 2; a <= 7; a++) {
                    // [수정] 두 번째 인자로 num_processes 전달
                    benchmark_results[a] = Execute_Simulation(job_pool_origin, num_processes, (AlgoType)a, time_quantum, 0);
                }

                printf("\n=================================================================\n");
                // [개선] 표 제목에도 프로세스 개수 표시
                printf(" 📊 스케줄링 알고리즘 종합 성능 평가 표 (프로세스 %d개)\n", num_processes);
                printf("=================================================================\n");
                printf("  알고리즘 종류            |  평균 반환시간 (ATT)  |  평균 대기시간 (AWT) \n");
                printf("-----------------------------------------------------------------\n");
                printf("  [2] FCFS                 |        %6.2f         |        %6.2f\n", benchmark_results[2].avg_turnaround, benchmark_results[2].avg_waiting);
                printf("  [3] Non-preemp SJF       |        %6.2f         |        %6.2f\n", benchmark_results[3].avg_turnaround, benchmark_results[3].avg_waiting);
                printf("  [4] Non-preemp Priority  |        %6.2f         |        %6.2f\n", benchmark_results[4].avg_turnaround, benchmark_results[4].avg_waiting);
                printf("  [5] Round Robin (TQ=%-2d)  |        %6.2f         |        %6.2f\n", time_quantum, benchmark_results[5].avg_turnaround, benchmark_results[5].avg_waiting);
                printf("  [6] Preemptive SJF       |        %6.2f         |        %6.2f\n", benchmark_results[6].avg_turnaround, benchmark_results[6].avg_waiting);
                printf("  [7] Preemptive Priority  |        %6.2f         |        %6.2f\n", benchmark_results[7].avg_turnaround, benchmark_results[7].avg_waiting);
                printf("=================================================================\n");
                break;

            default:
                printf("⚠️ 잘못된 입력입니다. 1번에서 9번 사이의 숫자를 입력해 주세요.\n");
                break;
        }
    }
    return 0;
}