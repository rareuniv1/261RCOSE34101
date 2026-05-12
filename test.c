#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MAX_PROCESSES 10
#define MAX_IO_DEVICES 5
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_PROCESSES 5  // 테스트를 위해 프로세스 5개로 설정
#define MAX_IO_DEVICES 5

// (이전에 작성한 모든 구조체와 함수들이 위에 선언되어 있다고 가정합니다)

// 원본 프로세스 풀 복사 함수 (파이프라인 2 확장을 위한 준비)
void Copy_Process_Pool(Process source[], Process dest[], int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = source[i];
    }
}

int main() {
    // 1. 초기 셋팅 및 난수 시드 고정
    srand((unsigned int)time(NULL)); 
    
    Process job_pool_origin[NUM_PROCESSES];
    Process working_pool[NUM_PROCESSES];
    
    ReadyQueue ready_queue;
    WaitQueue device_wait_queues[MAX_IO_DEVICES + 1];

    // 2. 프로세스 팩토리 가동 (원본 데이터 생성)
    Create_Process(job_pool_origin, NUM_PROCESSES);
    
    printf("\n=================================================================\n");
    printf(" 🏭 System Booting... Initialized Process Pool (Origin Data)\n");
    Print_Process_List(job_pool_origin, NUM_PROCESSES);
    
    // 시뮬레이션을 위해 원본 훼손을 막고 복사본(working_pool) 사용
    Copy_Process_Pool(job_pool_origin, working_pool, NUM_PROCESSES);

    // 3. 큐 초기화
    Init_ReadyQueue(&ready_queue);
    for (int i = 1; i <= MAX_IO_DEVICES; i++) {
        Init_WaitQueue(&device_wait_queues[i]);
    }

    // ====================================================================
    // ⏰ 시뮬레이션 메인 루프 시작
    // ====================================================================
    int system_time = 0;
    int terminated_count = 0;
    Process* current_running = NULL;

    printf("\n=================================================================\n");
    printf(" 🚀 Simulation Start! (Algorithm: Non-preemptive SJF)\n");
    printf("=================================================================\n");

    while (terminated_count < NUM_PROCESSES) {
        
        // 1. 신규 프로세스 도착 확인 (Ready Queue 진입)
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (working_pool[i].Arrival_Time == system_time) {
                Insert_ReadyQueue(&ready_queue, &working_pool[i]);
                printf("[TIME %3d] 📥 PID %d Arrived! (Entered Ready Queue)\n", system_time, working_pool[i].PID);
            }
        }

        // ====================================================================
        // 🛠️ [수정됨] 2. 다중 I/O 장치 병렬 실행 영역 (순서 올림)
        // 먼저 각 I/O 장치들의 남은 시간을 1초씩 깎아줍니다.
        // 이렇게 하면 아래 CPU 영역에서 이번 Tick에 막 I/O를 요청한 애가
        // 중복해서 1초를 더 깎이는 '더블 카운팅' 버그가 완벽히 차단됩니다!
        // ====================================================================
        Process_IO_Ticks(device_wait_queues, &ready_queue, system_time);

        // ====================================================================
        // 3. CPU 실행 영역 (1 Tick)
        // ====================================================================
        if (current_running != NULL) {
            ProcessState next_state = Run_Process_Tick(current_running);

            if (next_state == TERMINATED) {
                Record_Termination(current_running, system_time);
                terminated_count++;
                current_running = NULL; 
            } 
            else if (next_state == WAITING) {
                int dev_num = current_running->IO_Reqs[current_running->IO_Req_Now].IO_Device;
                Enqueue_WaitQueue(&device_wait_queues[dev_num], current_running);
                printf("[TIME %3d] 🔄 PID %d Requested I/O -> Device %d Wait Queue\n", 
                       system_time, current_running->PID, dev_num);
                current_running = NULL; 
            }
        }

        // 4. 스케줄링 영역 (CPU가 비어있을 때 다음 프로세스 선정)
        if (current_running == NULL && ready_queue.count > 0) {
            current_running = Schedule_SJF_NonPreemp(&ready_queue);
            current_running->P_State = RUNNING;
            printf("[TIME %3d] ⚙️  PID %d Scheduled to RUN on CPU!\n", system_time, current_running->PID);
        }
        for (int i = 0; i < ready_queue.count; i++) {
            ready_queue.list[i]->Waiting_Time++;
        }
        // 5. 시간 흐름
        system_time++;
        
        // (무한 루프 방지용 안전장치)
        if (system_time > 1000) {
            printf("\n[ERROR] Simulation Timeout! Check Arrival Times or Loop Logic.\n");
            break;
        }
    }

    // ====================================================================
    // 📊 최종 통계 출력 영역
    // ====================================================================
    printf("\n=================================================================\n");
    printf(" 📈 Simulation Completed! Final Statistics\n");
    printf("=================================================================\n");
    
    double total_turnaround = 0, total_waiting = 0;
    for (int i = 0; i < NUM_PROCESSES; i++) {
        total_turnaround += working_pool[i].Turnaround_Time;
        total_waiting += working_pool[i].Waiting_Time;
        printf("PID %2d | Turnaround: %3d | Waiting: %3d\n", 
               working_pool[i].PID, working_pool[i].Turnaround_Time, working_pool[i].Waiting_Time);
    }
    
    printf("-----------------------------------------------------------------\n");
    printf(" Average Turnaround Time : %.2f\n", total_turnaround / NUM_PROCESSES);
    printf(" Average Waiting Time    : %.2f\n", total_waiting / NUM_PROCESSES);
    printf("=================================================================\n");

    return 0;
}