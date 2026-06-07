#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#define MAX_PROCESSES 10    // 프로세스의 전체 개수
#define NUM_PROCESSES 10    // 동일하지만 레거시를 위함
#define MAX_IO_DEVICES 5    // 전체 IO 디바이스 개수
#define MAX_TIME 1000       // 전체 시간


//---------------------------------------------- [Log system] --------------------------------------------------
FILE *log_file = NULL; // 전역 로그 파일 포인터

// 터미널과 로그 파일에 동시에 출력하는 커스텀 출력 함수
// 원래 printf는 format string - 인자들로 구성되므로 여기서도 똑같이 구성!
int my_printf(const char *format, ...) {
    int result;     va_list args;
    // 1. 터미널(stdout)에 출력
    va_start(args, format);
    result = vprintf(format, args);
    va_end(args);
    // 2. 파일(log_file)이 열려있다면 파일에도 출력
    if (log_file != NULL) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file); // 파일에 바로바로 쓰기
    }
    return result;
}
// 기존 코드의 모든 printf를 my_printf로 자동 치환 - 일일이 다 바꾸기에는 너무 시간과 비용이..
#define printf my_printf
// ----------------------------------------------- [New Phase] -------------------------------------------------

// 프로세스의 현재 상태를 나타내는 열거형
typedef enum { NEW, READY, RUNNING, WAITING, TERMINATED } ProcessState;
// I/O 작업 정보 구조체
typedef struct { int IO_Device;  int IO_Start_Time;  int IO_Burst_Time; } IO_Req;
// 프로세스 구조체
typedef struct {
    // 기본정보
    int PID;    int Arrival_Time;   int CPU_Burst_Time_T; int Priority;
    // IO 정보
    int IO_Req_Num; IO_Req IO_Reqs[3];
    // 상태정보
    ProcessState P_State;   int Now_Arrival;    int CPU_Burst_Time_N;   int IO_Req_Now;
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
        p->Now_Arrival = p->Arrival_Time;   p->CPU_Burst_Time_N = p->CPU_Burst_Time_T;  p->IO_Req_Now = 0;
        // 통계 변수 초기화 - 다 0으로.
        p->Termination_Time = 0;    p->Turnaround_Time = 0; p->Waiting_Time = 0;    p->IO_Burst_Time_T = 0; 
        // IO 요청 만들기 시작
        int used_io_start_time[MAX_TIME] = {0};
        // 가능한 I/O 발생 시점은 [1, CPU_Burst_Time_T - 1]
        int max_possible_io_points = p->CPU_Burst_Time_T - 1;
        if (p->IO_Req_Num > max_possible_io_points) {p->IO_Req_Num = max_possible_io_points;}
        for (int j = 0; j < p->IO_Req_Num; j++) {
            // 각 IO 요청의 디바이스나 버스트 타임 랜덤하게 설정
            p->IO_Reqs[j].IO_Device = (rand() % 5) + 1;      // [1, 5]
            p->IO_Reqs[j].IO_Burst_Time = (rand() % 20) + 1; // [1, 20]
            int start_time;
            // I/O 발생 시간은 CPU 작업 중간에 일어나야 하므로 [1, CPU_Burst_Time_T - 1] 범위 내에서 생성
            // 단, 이미 사용된 start_time은 다시 사용하지 않음
            do {
                start_time = (rand() % (p->CPU_Burst_Time_T - 1)) + 1;
            } while (used_io_start_time[start_time]);
            used_io_start_time[start_time] = 1;
            p->IO_Reqs[j].IO_Start_Time = start_time;
            p->IO_Burst_Time_T += p->IO_Reqs[j].IO_Burst_Time;
        }
        // I/O 작업 정렬 : 개수가 최대 3개이므로 가벼운 버블 정렬(Bubble Sort) 사용
        for (int j = 0; j < p->IO_Req_Num - 1; j++) {
            for (int k = 0; k < p->IO_Req_Num - j - 1; k++) {
                if (p->IO_Reqs[k].IO_Start_Time > p->IO_Reqs[k + 1].IO_Start_Time) {
                    IO_Req temp = p->IO_Reqs[k];
                    p->IO_Reqs[k] = p->IO_Reqs[k + 1];
                    p->IO_Reqs[k + 1] = temp;
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

// Ready queue 구조체 : 프로세스 주소 배열 + 프로세스 개수
typedef struct {
    Process* list[MAX_PROCESSES]; // 프로세스 포인터 배열
    int count;                    // 현재 큐에 있는 프로세스 개수
} ReadyQueue;
// Ready queue 초기화 : 안에 있는 거 다 NULL로 비우면서, count = 0
void Init_ReadyQueue(ReadyQueue* rq) {
    rq->count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) { rq->list[i] = NULL; }
}
// 리스트 끝에 프로세스 추가 : 상태는 먼저 READY로 만들어줌
void Insert_ReadyQueue(ReadyQueue* rq, Process* p) {
    if (rq->count >= MAX_PROCESSES) { printf("Error: Ready Queue is full.\n"); return; }
    p->P_State = READY;
    rq->list[rq->count++] = p;
}
// 특정 인덱스의 프로세스를 꺼내고(Remove), 빈자리를 당겨서 채움
Process* Remove_ReadyQueue(ReadyQueue* rq, int index) {
    if (index < 0 || index >= rq->count) { return NULL; } // 범위 밖이면 NULL 리턴
    Process* p = rq->list[index];
    // 삭제된 원소 뒤의 항목들을 앞으로 한 칸씩 이동
    for (int i = index; i < rq->count - 1; i++) { rq->list[i] = rq->list[i + 1]; }
    //빈자리 처리 : 맨 끝은 NULL로!
    rq->list[rq->count - 1] = NULL; rq->count--;
    return p;
}

// ---------------------------------------------- [Ready Phase] ---------------------------------------------------

int compare_fcfs(const void *a, const void *b) {
    Process *p1 = *(Process **)a;    Process *p2 = *(Process **)b;
    // 1순위: 도착 시간이 빠른 순서
    if (p1->Now_Arrival != p2->Now_Arrival) {   return p1->Now_Arrival - p2->Now_Arrival;   }
    // 2순위 (Tie-breaker): 도착 시간이 같다면 PID가 작은(먼저 생성된) 프로세스 우선
    return p1->PID - p2->PID;
}
int compare_sjf(const void *a, const void *b) {
    Process *p1 = *(Process **)a;   Process *p2 = *(Process **)b;
    // 1순위: 남은 CPU 작업 시간이 짧은 순서
    if (p1->CPU_Burst_Time_N != p2->CPU_Burst_Time_N) { return p1->CPU_Burst_Time_N - p2->CPU_Burst_Time_N; }
    // 2순위 (Tie-breaker): 작업 시간이 같다면 먼저 도착한 순서 (FCFS)
    if (p1->Now_Arrival != p2->Now_Arrival) {   return p1->Now_Arrival - p2->Now_Arrival;   }
    // 3순위: 도착 시간까지 같다면 PID 순서
    return p1->PID - p2->PID;
}
int compare_priority(const void *a, const void *b) {
    Process *p1 = *(Process **)a;   Process *p2 = *(Process **)b;
    // 1순위: 우선순위 값이 작은 순서 (1이 가장 높고 10이 가장 낮음)
    if (p1->Priority != p2->Priority) { return p1->Priority - p2->Priority; }
    // 2순위 (Tie-breaker): 우선순위가 같다면 먼저 도착한 순서 (FCFS)
    if (p1->Now_Arrival != p2->Now_Arrival) {   return p1->Now_Arrival - p2->Now_Arrival;   }
    // 3순위: PID 순서
    return p1->PID - p2->PID;
}
//SRTF = Preemptive SJF
int compare_srtf(const void *a, const void *b) {
    Process *p1 = *(Process **)a;   Process *p2 = *(Process **)b;
    // 1순위 :  CPU burst time
    if (p1->CPU_Burst_Time_N != p2->CPU_Burst_Time_N)
        return p1->CPU_Burst_Time_N - p2->CPU_Burst_Time_N;
    // 2순위 : Ready queue Arrival time
    if (p1->Now_Arrival != p2->Now_Arrival)
        return p1->Now_Arrival - p2->Now_Arrival;
    return p1->PID - p2->PID;
}
int compare_priority_preemp(const void *a, const void *b) {
    Process *p1 = *(Process **)a;   Process *p2 = *(Process **)b;

    if (p1->Priority != p2->Priority)
        return p1->Priority - p2->Priority;
    if (p1->Now_Arrival != p2->Now_Arrival)
        return p1->Now_Arrival - p2->Now_Arrival;
    return p1->PID - p2->PID;
}
// 위에서의 정렬 알고리즘 적용
void Sort_ReadyQueue(ReadyQueue *q, int (*cmp)(const void*, const void*)) {
    for (int i = 1; i < q->count; i++) {
        Process *key = q->list[i]; // 현재 삽입할 프로세스 포인터
        int j = i - 1;
        // 만약 범위 안에 있고, 비교 알고리즘상 j가 현재 삽입할 프로세스 포인터보다 우선순위가 높으면
        while (j >= 0 && cmp(&q->list[j], &key) > 0) {
            q->list[j + 1] = q->list[j];
            j = j - 1;
        }
        q->list[j + 1] = key; // 최종 위치에 삽입
    }
}

// ----------------------------------------- [Ready - Running Phase] ----------------------------------------------
// 1틱씩 진행하며 검사하기
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
// 여기부터 스케줄링 알고리즘
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
    // 1. Ready Queue를 SJF 기준으로 정렬하여 1등을 맨 앞으로 보냄
    Sort_ReadyQueue(rq, compare_srtf);
    Process* top_ready = rq->list[0];
    // 2. [선점 조건] Ready 큐 1등의 남은 시간이 현재 실행 중인 프로세스의 남은 시간보다 짧은가?
    if (top_ready->CPU_Burst_Time_N < current_running->CPU_Burst_Time_N) {
        printf("[TIME %d] Preemption! PID %d preempted by PID %d (Remaining: %d < %d)\n", 
               system_time, current_running->PID, top_ready->PID, 
               top_ready->CPU_Burst_Time_N, current_running->CPU_Burst_Time_N);
        // 3. 현재 프로세스를 쫓아낼 준비 (Ready queue 도착 시간 업데이트)
        current_running->Now_Arrival = system_time;
        // 4. 강제 교체 (Context Switch 발생)
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
    int front;                     // 데이터를 꺼낼 인덱스
    int rear;                      // 데이터를 넣을 인덱스
    int count;                     // 현재 큐에 들어있는 데이터의 개수
} WaitQueue;
void Init_WaitQueue(WaitQueue* wq) {
    wq->front = 0;  wq->rear = 0;   wq->count = 0;  // 다 초기화시키기
    for (int i = 0; i < MAX_PROCESSES; i++) { wq->queue[i] = NULL; }
}
void Enqueue_WaitQueue(WaitQueue* wq, Process* p) {
    // 꽉 차면 Wait queue Error 반환
    if (wq->count >= MAX_PROCESSES) {
        printf("Error: Wait Queue is full. Cannot enqueue PID %d\n", p->PID);   return;
    }
    // 상태를 WAITING으로 변경
    p->P_State = WAITING; 
    // rear 위치에 삽입 후, 원형으로 회전
    wq->queue[wq->rear] = p;    wq->rear = (wq->rear + 1) % MAX_PROCESSES;  wq->count++;
}
Process* Dequeue_WaitQueue(WaitQueue* wq) {
    // 비어있으면 뺄 게 없다!
    if (wq->count == 0) { return NULL; }
    // front 위치의 프로세스를 꺼내고, 원형으로 회전
    Process* p = wq->queue[wq->front];
    wq->queue[wq->front] = NULL;    wq->front = (wq->front + 1) % MAX_PROCESSES;    wq->count--;
    return p;
}
void WakeUp_Process(WaitQueue* wq, ReadyQueue* rq, int system_time) {
    // 1. Wait Queue에서 프로세스 꺼내기
    Process* p = Dequeue_WaitQueue(wq);
    if (p != NULL) {
        // 2. 다음 I/O 작업으로 포인터 넘기고 + 3. Ready queue 도착시간 갱신
        p->IO_Req_Now++;    p->Now_Arrival = system_time;
        // 4. 이후 삽입하기
        Insert_ReadyQueue(rq, p);
        printf("[TIME %d] PID %d: Wake up! (Wait -> Ready Queue)\n", system_time, p->PID);
    }
}
void Process_IO_Ticks(WaitQueue queues[], ReadyQueue* rq, int system_time) {
    for (int i = 1; i <= MAX_IO_DEVICES; i++) {
        // 장치 큐에 기다리는 녀석이 있다면?
        if (queues[i].count > 0) {
            // 맨 앞에 있는 녀석 보기
            Process* p = queues[i].queue[queues[i].front];
            int current_io_idx = p->IO_Req_Now;
            // 해당 프로세스의 남은 I/O 시간 1 감소
            p->IO_Reqs[current_io_idx].IO_Burst_Time--;
            // 1초를 깎았는데 만약 0이 되었다면? (I/O 작업 완전 종료) -> 한 번에 WakeUp처리
            if (p->IO_Reqs[current_io_idx].IO_Burst_Time <= 0) {
                WakeUp_Process(&queues[i], rq, system_time);
            }
        }
    }
}

// ------------------------------------------- [Terminating Phase] -------------------------------------------------
// 종료 시 호출되는 함수
void Record_Termination(Process* p, int system_time) {
    // 프로세스가 주어지지 않으면 암것도 안함
    if (p == NULL) return;
    // 프로세스 종료 처리 : system_time으로 종료 시간 기록 + turnaround time으로 내부 처리
    p->P_State = TERMINATED;    p->Termination_Time = system_time;  p->Turnaround_Time = p->Termination_Time - p->Arrival_Time;

    printf("====================================================\n");
    printf("[TIME %d] PID %d Terminated!\n", system_time, p->PID);
    printf(" - Arrival Time   : %d\n", p->Arrival_Time);
    printf(" - Turnaround Time: %d\n", p->Turnaround_Time);
    printf(" - Waiting Time   : %d\n", p->Waiting_Time);
    printf("====================================================\n");
}
// 원본 프로세스 풀 복사 함수 (파이프라인 2 : 다중 프로세스 셋 비교 확장을 위한 준비)
void Copy_Process_Pool(Process source[], Process dest[], int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = source[i];
    }
}
// 알고리즘 종류를 구분하기 위한 열거형(Enum) : Menu 용
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
// 단일 시뮬레이션을 수행하는 핵심 코어 함수
// 매개변수에 'int num_processes'가 추가되었습니다!
AlgoResult Execute_Simulation(Process origin_pool[], int num_processes, AlgoType algo, int time_quantum, int show_logs) {
    // 프로세스 Set + Ready queue + Wait queue 최대 크기(MAX_PROCESSES)만큼 넉넉히 초기화
    Process working_pool[MAX_PROCESSES];    ReadyQueue ready_queue; WaitQueue device_wait_queues[MAX_IO_DEVICES + 1];
    // 체크용 기록 만들기 -> 처음엔 다 -1로!
    int gantt_record[MAX_TIME];
    for (int i = 0; i < MAX_TIME; i++) gantt_record[i] = -1;
    // 데이터 복사 및 초기화 (num_processes 만큼만 복사!)
    for (int i = 0; i < num_processes; i++) { working_pool[i] = origin_pool[i]; }
    // Ready queue 초기화
    Init_ReadyQueue(&ready_queue);
    for (int i = 1; i <= MAX_IO_DEVICES; i++) { Init_WaitQueue(&device_wait_queues[i]);}
    // 시스템 타이머 초기화
    int system_time = 0;    int terminated_count = 0;   int current_q_time = 0; // RR용 퀀텀 타이머
    Process* current_running = NULL;
    // 시뮬레이션 시작!
    if (show_logs) {
        printf("\n=================================================================\n");
        printf(" Start Simulation! (Algorithm: %d, Number of processes: %d)\n", algo, num_processes);
        printf("=================================================================\n");
    }
    // 종료 조건 = 프로세스들이 모두 종료되거나 Time Expire 발생 시
    while (terminated_count < num_processes && system_time < MAX_TIME) {
        // 1. 신규 프로세스 도착 확인
        for (int i = 0; i < num_processes; i++) {
            // Ready queue에 새로 뭔가 들어왔나요?
            if (working_pool[i].Arrival_Time == system_time) {
                Insert_ReadyQueue(&ready_queue, &working_pool[i]); 
                if (show_logs) {
                    printf("[TIME %3d] PID %d Arrived! (In Ready Queue)\n",
                           system_time, working_pool[i].PID);
                }
            }
        }
        // 2. 다중 I/O 장치 처리 -> I/O가 끝난 프로세스는 이 시점에 Ready Queue로 복귀합니다
        Process_IO_Ticks(device_wait_queues, &ready_queue, system_time);
        // 3. 선점형 알고리즘(SRTF, Preemptive Priority) 매 tick 검사
        // 중요: CPU를 1 tick 실행하기 전에 선점 여부를 먼저 확인
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
        // 4. CPU가 비어 있으면 스케줄러 호출
        // 이제 이 시점에서 이번 tick에 실행할 프로세스를 확정합니다.
        if (current_running == NULL && ready_queue.count > 0) {
            switch (algo) {
                case ALGO_FCFS:
                    current_running = Schedule_FCFS(&ready_queue);
                    break;
                case ALGO_RR:
                    current_running = Schedule_FCFS(&ready_queue);
                    break;
                case ALGO_NP_SJF:
                    current_running = Schedule_SJF_NonPreemp(&ready_queue);
                    break;
                case ALGO_NP_PRIORITY:
                    current_running = Schedule_Priority_NonPreemp(&ready_queue);
                    break;
                case ALGO_P_SJF:
                    Sort_ReadyQueue(&ready_queue, compare_srtf);
                    current_running = Remove_ReadyQueue(&ready_queue, 0);
                    break;
                case ALGO_P_PRIORITY:
                    Sort_ReadyQueue(&ready_queue, compare_priority_preemp);
                    current_running = Remove_ReadyQueue(&ready_queue, 0);
                    break;
            }
            current_running->P_State = RUNNING; current_q_time = 0; // CPU 이제 점유하므로 Time quantum도 초기화
            if (show_logs) {
                printf("[TIME %3d] PID %d CPU Allocated!\n",
                       system_time, current_running->PID);
            }
        }
        // 5. 이번 tick에 실제로 실행될 프로세스를 Gantt Chart에 기록
        // 이제 기록 대상과 실제 CPU_Burst_Time_N이 감소하는 대상이 일치합니다.
        if (current_running != NULL) {
            gantt_record[system_time] = current_running->PID;
        }
        // 6. CPU 실행 영역
        if (current_running != NULL) {
            ProcessState next_state = Run_Process_Tick(current_running);
            current_q_time++;
            /*
             * Ready Queue에 남아 있는 프로세스들은 이번 tick 동안 CPU를 기다린 것입니다.
             * 단, RR에서 방금 실행을 마치고 다시 Ready Queue에 들어갈 프로세스는 이번 tick 동안 기다린 것이 아니죠.
             * 재삽입 전에 Ready queue에 있던 애들만 Waiting Time을 먼저 증가시킵니다.
             */
            for (int i = 0; i < ready_queue.count; i++) {
                ready_queue.list[i]->Waiting_Time++;
            }
            // 만약 종료된다면
            if (next_state == TERMINATED) {
                // system_time에서 1 tick 실행했으므로 실제 종료 시점은 system_time + 1, Turnaround Time도 이때 확정합니다.
                current_running->P_State = TERMINATED;
                current_running->Termination_Time = system_time + 1;
                current_running->Turnaround_Time = current_running->Termination_Time - current_running->Arrival_Time;
                // 로그에는 이렇게 남죠.
                if (show_logs) {
                    printf("====================================================\n");
                    printf("[TIME %3d] PID %d Terminated!\n",
                           system_time + 1, current_running->PID);
                    printf(" - Arrival Time   : %d\n", current_running->Arrival_Time);
                    printf(" - Turnaround Time: %d\n", current_running->Turnaround_Time);
                    printf(" - Waiting Time   : %d\n", current_running->Waiting_Time);
                    printf("====================================================\n");
                }
                // 종료된 게 하나 더 늘었으니까 카운트 해주고, 현재 CPU에 있는 것도 없으니 TQ도 초기화.
                terminated_count++; current_running = NULL; current_q_time = 0;
            } 
            // 만약 Waiting이라면, IO 발생한 것이므로,
            else if (next_state == WAITING) {
                // 해당 IO 작업의 디바이스 번호를 가져온 후
                int dev_num = current_running->IO_Reqs[current_running->IO_Req_Now].IO_Device;
                // 해당 디바이스의 Wait queue에 집어넣기
                Enqueue_WaitQueue(&device_wait_queues[dev_num], current_running);
                // 로그 출력
                if (show_logs) {
                    printf("[TIME %3d] PID %d I/O Request -> Move to Device %d Wait Queue \n",
                           system_time + 1, current_running->PID, dev_num);
                }
                // running -> wait이므로 NULL.
                current_running = NULL; current_q_time = 0;
            }
            // 6-1. Round Robin 타임 퀀텀 만료 선점 처리
            else if (algo == ALGO_RR && current_q_time >= time_quantum) {
                if (show_logs) {
                    printf("[TIME %3d] PID %d Time Quantum Expired! (Preemption Occured...)\n",
                           system_time + 1, current_running->PID);
                }

                // 이번 tick이 끝난 뒤 Ready Queue로 돌아가므로 system_time + 1 사용
                current_running->Now_Arrival = system_time + 1;
                Insert_ReadyQueue(&ready_queue, current_running);

                current_running = NULL; current_q_time = 0;
            }
        }
        else {
            // CPU가 IDLE인 경우에도 Ready Queue에 남아 있는 프로세스가 있다면 대기 시간 증가
            // 일반적으로 current_running == NULL이고 ready_queue.count > 0인 상황은
            // 위의 스케줄러 호출에서 처리되므로 거의 발생하지 않습니다.
            if (ready_queue.count > 0) {
                printf("Warning: CPU is idle while Ready Queue is not empty.\n");
            }
        }
        system_time++;
    }
    // 통계 계산 (마찬가지로 num_processes 기준으로 변경)
    double total_tt = 0, total_wt = 0;
    for (int i = 0; i < num_processes; i++) {
        total_tt += working_pool[i].Turnaround_Time;    total_wt += working_pool[i].Waiting_Time;
    }
    // 간트 차트 출력
    if (show_logs) {
        printf("\n=================================================================\n");
        printf(" Gantt Chart (CPU Timeline)\n");
        printf("=================================================================\n");
        int current_pid = gantt_record[0];
        int start_t = 0;
        for (int t = 1; t <= system_time; t++) {
            if (t == system_time || gantt_record[t] != current_pid) {
                // 마지막 블록이 IDLE이고, 시뮬레이션의 끝이라면 그리지 않고 탈출
                if (current_pid == -1 && t == system_time) { break; }
                // IDLE이라면
                if (current_pid == -1) { printf("| IDLE (%d-%d) ", start_t, t); }
                // 프로세스라면
                else { printf("| P%d (%d-%d) ", current_pid, start_t, t); }
                current_pid = gantt_record[t];  start_t = t;
            }
        }
        printf("|\n=================================================================\n");
    }
    // 평균 낼 때도 num_processes로 나눔
    AlgoResult res = { total_tt / num_processes, total_wt / num_processes, 1 };
    return res;
}

int main() {
    // 난수 준비
    srand((unsigned int)time(NULL));
    // 로그 기록하기
    log_file = fopen("scheduler_log.txt", "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        printf("\n\n=================================================================\n");
        printf(" [SYSTEM LOG] Simulator Boot Sequence Started at: %s", ctime(&now));
        printf("=================================================================\n");
    } else {
        fprintf(stdout, " Warning: Failed to open scheduler_log.txt. Running without file logging.\n");
    }
    // 각종 변수들 초기화하기
    Process job_pool_origin[MAX_PROCESSES];
    AlgoResult benchmark_results[8];
    int is_generated = 0; 
    int menu_choice;
    int time_quantum = 5; 
    int num_processes = 0;
    // 메뉴 시작
    while (1) {
        // 메뉴 출력
        printf("\n=================================================================\n");
        printf(" CPU Scheduling Interactive Simulator\n");
        printf("=================================================================\n");
        printf(" 1. Generate New Process Set (Random 2~10)\n");
        printf(" 2. Run FCFS (First-Come, First-Served)\n");
        printf(" 3. Run Non-preemptive SJF\n");
        printf(" 4. Run Non-preemptive Priority\n");
        printf(" 5. Run Round Robin (RR)\n");
        printf(" 6. Run Preemptive SJF (SRTF)\n");
        printf(" 7. Run Preemptive Priority\n");
        printf(" 8. [Evaluation] Print Average Time Comparison Table (Algos 2-7)\n");
        printf(" 9. [Stress Test] Run 20 Independent Process Sets (Monte Carlo)\n");
        printf(" 10. Exit Program\n");
        printf("=================================================================\n");
        printf(" Select Menu (1~10): ");
        // 숫자가 아닐 경우 x
        if (scanf("%d", &menu_choice) != 1) {
            while(getchar() != '\n'); 
            continue;
        }
        // 로그 파일이 있으면 다 저장
        if (log_file != NULL) {
            fprintf(log_file, "%d\n", menu_choice);
            fflush(log_file); // 즉시 저장
        }
        // 10이라면 종료
        if (menu_choice == 10) {
            printf(" Exiting simulator. Thank you!\n");
            break;
        }
        // 2~8인데 1이 없는 경우 -> 알고리즘 돌려야 하는데 프로세스 셋이 없음 -> 1 하고 오세요!
        if (menu_choice >= 2 && menu_choice <= 8 && !is_generated) {
            printf("Please generate a process set first! (Select Menu 1)\n");
            continue;
        }
        // 1~9까지의 동작 정리
        switch (menu_choice) {
            case 1:
                num_processes = (rand() % 9) + 2;   // 생성할 프로세스 개수 [2,10]
                Create_Process(job_pool_origin, num_processes); // 프로세스 생성
                printf("\n=================================================================\n");
                printf("System boot complete... New process set generated. (Total: %d)\n", num_processes);
                Print_Process_List(job_pool_origin, num_processes); // 프로세스 출력
                is_generated = 1;
                break;

            case 2: case 3: case 4: case 6: case 7: // 프로세스들 생성하기
                Execute_Simulation(job_pool_origin, num_processes, (AlgoType)menu_choice, 0, 1);
                break;

            case 5:
                printf(" Enter Time Quantum for Round Robin (Integer): "); //먼저 TQ 입력받고
                if (scanf("%d", &time_quantum) != 1) {
                    while (getchar() != '\n');
                    printf("Invalid input. Setting to default (3).\n");
                    time_quantum = 3;
                }
                else if (time_quantum <= 0) {
                    printf("Invalid time. Setting to default (3).\n");
                    time_quantum = 3;
                }
                Execute_Simulation(job_pool_origin, num_processes, ALGO_RR, time_quantum, 1);   // 그 다음에 실행 ㄱㄱ
                break;

            case 8:
                printf("\n=================================================================\n");
                printf("Comprehensive Evaluation Setup\n");
                printf(" Enter Time Quantum for Round Robin (Integer): ");
                if (scanf("%d", &time_quantum) != 1) {
                    while (getchar() != '\n');
                    printf("Invalid input. Setting to default (3).\n");
                    time_quantum = 3;
                }
                else if (time_quantum <= 0) {
                    printf("Invalid time. Setting to default (3).\n");
                    time_quantum = 3;
                }

                printf("\n Starting performance measurement for all algorithms on the same process set (%d processes)...\n", num_processes);
                
                for (int a = 2; a <= 7; a++) {
                    benchmark_results[a] = Execute_Simulation(job_pool_origin, num_processes, (AlgoType)a, time_quantum, 0);
                }

                printf("\n=================================================================\n");
                printf("Scheduling Algorithm Comprehensive Performance Table\n");
                printf("=================================================================\n");
                printf("  Algorithm Type           | Avg Turnaround (ATT) | Avg Waiting (AWT) \n");
                printf("-----------------------------------------------------------------\n");
                printf("  [2] FCFS                 |        %6.2f        |        %6.2f\n", benchmark_results[2].avg_turnaround, benchmark_results[2].avg_waiting);
                printf("  [3] Non-preemp SJF       |        %6.2f        |        %6.2f\n", benchmark_results[3].avg_turnaround, benchmark_results[3].avg_waiting);
                printf("  [4] Non-preemp Priority  |        %6.2f        |        %6.2f\n", benchmark_results[4].avg_turnaround, benchmark_results[4].avg_waiting);
                printf("  [5] Round Robin (TQ=%-2d)  |        %6.2f        |        %6.2f\n", time_quantum, benchmark_results[5].avg_turnaround, benchmark_results[5].avg_waiting);
                printf("  [6] Preemptive SJF       |        %6.2f        |        %6.2f\n", benchmark_results[6].avg_turnaround, benchmark_results[6].avg_waiting);
                printf("  [7] Preemptive Priority  |        %6.2f        |        %6.2f\n", benchmark_results[7].avg_turnaround, benchmark_results[7].avg_waiting);
                printf("=================================================================\n");
                break;

            case 9:
                printf("\n=================================================================\n");
                printf("[Stress Test] Testing 20 independent process sets.\n");
                printf(" Enter Time Quantum for Round Robin (Integer): ");
                
                if (scanf("%d", &time_quantum) != 1) {
                    while (getchar() != '\n');
                    printf("Invalid input. Setting to default (3).\n");
                    time_quantum = 3;
                }
                else if (time_quantum <= 0) {
                    printf("Invalid time. Setting to default (3).\n");
                    time_quantum = 3;
                }

                printf("\nRunning 20 simulations in the background. Please wait...\n");

                int num_test_sets = 20;
                double total_tt_sum[8] = {0}; 
                double total_wt_sum[8] = {0}; 

                for (int test = 1; test <= num_test_sets; test++) {
                    int current_num = (rand() % 9) + 2;
                    Process temp_pool[MAX_PROCESSES];
                    Create_Process(temp_pool, current_num);

                    for (int a = 2; a <= 7; a++) {
                        AlgoResult res = Execute_Simulation(temp_pool, current_num, (AlgoType)a, time_quantum, 0);
                        total_tt_sum[a] += res.avg_turnaround;
                        total_wt_sum[a] += res.avg_waiting;
                    }
                }

                printf("\n=================================================================\n");
                printf("Large-scale Statistical Evaluation (Avg of %d sets)\n", num_test_sets);
                printf("=================================================================\n");
                printf("  Algorithm Type           | Final Avg Turnaround | Final Avg Waiting \n");
                printf("-----------------------------------------------------------------\n");
                printf("  [2] FCFS                 |        %6.2f        |        %6.2f\n", total_tt_sum[2] / num_test_sets, total_wt_sum[2] / num_test_sets);
                printf("  [3] Non-preemp SJF       |        %6.2f        |        %6.2f\n", total_tt_sum[3] / num_test_sets, total_wt_sum[3] / num_test_sets);
                printf("  [4] Non-preemp Priority  |        %6.2f        |        %6.2f\n", total_tt_sum[4] / num_test_sets, total_wt_sum[4] / num_test_sets);
                printf("  [5] Round Robin (TQ=%-2d)  |        %6.2f        |        %6.2f\n", time_quantum, total_tt_sum[5] / num_test_sets, total_wt_sum[5] / num_test_sets);
                printf("  [6] Preemptive SJF       |        %6.2f        |        %6.2f\n", total_tt_sum[6] / num_test_sets, total_wt_sum[6] / num_test_sets);
                printf("  [7] Preemptive Priority  |        %6.2f        |        %6.2f\n", total_tt_sum[7] / num_test_sets, total_wt_sum[7] / num_test_sets);
                printf("=================================================================\n");
                break;

            default:
                printf("Invalid input. Please enter a number between 1 and 10.\n");
                break;
        }
    }
    if (log_file != NULL) {
        fclose(log_file);
    }
    return 0;
}