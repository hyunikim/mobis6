#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>  
#include <ctype.h>
#include <limits.h>

static void scenario_title(const char* title) {
    printf("\n=== %s ===\n", title);
}

static void line(void) {
    printf("------------------------------------------------------------\n");
}

static void scenario_arr30(void) {
    scenario_title("Scenario 1: 배열 인덱스 범위 초과로 인한 비정상 메모리 접근 여부 확인 (ARR30-C)");

    int scores[5] = {90, 85, 78, 92, 88};
    int student_index = -1;

    line();
    printf("  [성적 조회] 학생 인덱스 = %d (배열 크기: 5)\n", student_index);

    if (student_index < 5) {
        printf("  scores[%d] = %d\n", student_index, scores[student_index]);
    }

    #define ROWS 3
    #define COLS 4
    int seats[ROWS][COLS];

    line();
    printf("  [좌석 초기화] seats[%d][%d]\n", ROWS, COLS);

    for (int i = 0; i < COLS; i++) {
        for (int j = 0; j < ROWS; j++) {
            seats[i][j] = (i + 1) * 10 + (j + 1);
        }
    }
    printf("  seats[0][0]=%d, seats[2][3]=%d\n", seats[0][0], seats[2][3]);

    #undef ROWS
    #undef COLS
}

static void scenario_arr32(void) {
    scenario_title("Scenario 2: 가변 길이 배열(VLA) 크기 미검증으로 인한 스택 고갈 위험 확인 (ARR32-C)");

    size_t count = 0;

    line();
    printf("  [설문 수집] 응답 수 = %zu\n", count);

    int *responses = (int *)alloca(count * sizeof(int));
    printf("  alloca(%zu) = %p\n", count * sizeof(int), (void*)responses);

    size_t huge = 999999999;
    line();
    printf("  [설문 수집] 응답 수 = %zu (매우 큰 값)\n", huge);
    printf("  이 크기로 스택 배열을 만들면 스택 고갈 위험\n");
}

static void scenario_str31(void) {
    scenario_title("Scenario 3: 문자열 저장 공간 부족으로 인한 버퍼 오버플로 여부 확인 (STR31-C)");

    /* [Step 1] strcpy 오버플로: 인접 canary 변수 오염 감지 */
    volatile int canary = 0xDEADBEEF;   /* username 바로 뒤 스택 변수 */
    char username[9];
    const char *input_name = "Alexander_Hamilton"; /* 18자 + '\0' = 19바이트 */

    line();
    printf("  [이름 등록] 버퍼: %zu, 입력: \"%s\" (길이: %zu)\n",
           sizeof(username), input_name, strlen(input_name));
    printf("  [오버플로 전] canary = 0x%08X\n", (unsigned int)canary);

    strcpy(username, input_name);   /* 버퍼 초과 기록 → canary 오염 가능 */
    printf("  저장된 이름: \"%s\"\n", username);
    printf("  [오버플로 후] canary = 0x%08X\n", (unsigned int)canary);

    /* [Step 2] sprintf 오버플로: guard 배열 오염 감지
     * 구조체로 메모리 순서를 강제: filepath[20] 직후에 guard[8] 보장 */
    struct {
        char filepath[20];
        char guard[8];
    } s;
    memcpy(s.guard, "STABLEOK", 8);

    const char *dir  = "/var/log/application";     /* 20자 */
    const char *file = "debug_output_2024.log";    /* 21자 → 합계 43바이트 */

    line();
    printf("  [경로 생성] 버퍼: %zu, 실제 결과 길이: %zu\n",
           sizeof(s.filepath), strlen(dir) + 1 + strlen(file));
    printf("  [오버플로 전] guard = \"%.8s\"\n", s.guard);

    sprintf(s.filepath, "%s/%s", dir, file);  /* 43바이트 → 20바이트 버퍼 초과 */
    printf("  경로: \"%s\"\n", s.filepath);
    printf("  [오버플로 후] guard = \"%.8s\"\n", s.guard);
}

static void scenario_str37(void) {
    scenario_title("Scenario 4: 문자 분류 함수에 부적절한 값 전달로 인한 정의되지 않은 동작 확인 (STR37-C)");

    const char *name = "Caf\xe9";

    line();
    printf("  [이름 검증] 입력: \"%s\"\n", name);
    printf("  [비교] unsafe=isalpha(ch)\n");
    printf("  [규칙] ctype 인자 허용범위: EOF(-1) 또는 0..%u\n", (unsigned)UCHAR_MAX);

    for (int i = 0; name[i] != '\0'; i++) {
        char ch = name[i];
        int unsafe_arg = (int)ch;
        int unsafe_domain_ok = (unsafe_arg == EOF) || (unsafe_arg >= 0 && unsafe_arg <= UCHAR_MAX);
        int unsafe = isalpha(ch);
        printf("    [%d] byte=0x%02X, unsafe_arg=%d (%s) -> isalpha(ch)=%d\n",
               i,
               (unsigned char)ch,
               unsafe_arg,
               unsafe_domain_ok ? "valid" : "invalid",
               unsafe);
    }
}

int main(void) {
    printf("Application started\n");

    scenario_arr30();
    scenario_arr32();
    scenario_str31();
    scenario_str37();

    printf("\nApplication finished\n");
    return 0;
}
