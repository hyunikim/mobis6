#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

static void scenario_title(const char* title) {
    printf("\n=== %s ===\n", title);
}

static void line(void) {
    printf("------------------------------------------------------------\n");
}

static void scenario_int31(void) {
    scenario_title("Scenario 1:  정수 타입 변환 시 데이터 손실 및 값 왜곡 여부 확인");
    unsigned int user_input_age = 300; 

    line();
    printf("  [나이 입력] 사용자가 입력한 값: %u\n", user_input_age);

    signed char age = (signed char)user_input_age;

    printf("  [나이 저장] signed char에 저장된 값: %d\n", (int)age);

    int api_result = -1;  

    line();
    printf("  [재고 조회] API 반환값: %d (에러 코드)\n", api_result);

    unsigned int stock_count = (unsigned int)api_result;

    printf("  [재고 수량] unsigned로 변환된 값: %u\n", stock_count);

    int scores[5];
    memset(scores, 100, sizeof(scores));

    line();
    printf("  [점수 초기화] memset(scores, 100, ...) 실행\n");
    printf("  [기대값: 100, 100, 100, 100, 100]\n");
    printf("  [실제값: %d, %d, %d, %d, %d]\n",
           scores[0], scores[1], scores[2], scores[3], scores[4]);
}

static void scenario_int32(void) {
    scenario_title("Scenario 2: 정수 연산 시 오버플로(Overflow)로 인한 계산 결과 왜곡 여부 확인");

    int32_t unit_price = 50000;    
    int32_t quantity   = 50000;    
    int32_t shipping   = 3000;     

    line();
    printf("  [주문 정보] 단가=%'" PRId32 "원, 수량=%'" PRId32 "개, 배송비=%'" PRId32 "원\n",
           unit_price, quantity, shipping);

    int32_t subtotal = unit_price * quantity;
    int32_t total    = subtotal + shipping;

    int64_t subtotal64 = (int64_t)unit_price * (int64_t)quantity;
    int64_t total64    = subtotal64 + (int64_t)shipping;

    printf("  [32-bit 계산] 소계=%" PRId32 ", 총액=%" PRId32 "\n", subtotal, total);
    printf("  [64-bit 참고] 소계=%" PRId64 ", 총액=%" PRId64 "\n", subtotal64, total64);
    printf("  [기대 총액: %" PRId64 "원 | 실제 총액: %" PRId32 "원]\n", total64, total);
}

static int run_child_mode(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "--child-divzero") == 0) {
        volatile int32_t total = 450;
        volatile int32_t count = 0;
        volatile int32_t avg = total / count;
        (void)avg;
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "--child-min-div-neg1") == 0) {
        volatile int32_t val = INT32_MIN;
        volatile int32_t div = -1;
        volatile int32_t result = val / div;
        (void)result;
        return 0;
    }
    return -1;
}

#ifdef _WIN32
static void run_child_and_show(const char* args, const char* desc) {
    char exe_path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) { printf("  ERROR\n"); return; }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "\"%s\" %s", exe_path, args);

    STARTUPINFOA si; PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        printf("  ERROR: CreateProcess failed\n"); return;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD ec = 0;
    GetExitCodeProcess(pi.hProcess, &ec);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);

    printf("  [%s] 프로세스 종료 코드 = 0x%08lX\n", desc, (unsigned long)ec);
    if (ec != 0)
        printf("  <-- 프로그램이 비정상 종료됨 (정상이라면 0이어야 한다)\n");
}
#endif

static void scenario_int33(void) {
    scenario_title("Scenario 3: 나눗셈 연산 시 0 나누기 및 경계조건으로 인한 비정상 종료 여부 확인");
    line();
    printf("  [평균 계산] 총점=450, 학생수=0\n");
    printf("  450 / 0 을 실행합니다...\n");
#ifdef _WIN32
    run_child_and_show("--child-divzero", "0으로 나누기");
#else
    printf("  NOTE: Windows 환경에서 관찰 가능\n");
#endif
    line();
    printf("  [경계 조건] INT32_MIN(%" PRId32 ") / -1 을 실행합니다...\n", INT32_MIN);
    printf("  수학적 결과: +2147483648 (int32_t 최대: %" PRId32 ")\n", INT32_MAX);
#ifdef _WIN32
    run_child_and_show("--child-min-div-neg1", "INT_MIN/-1");
#else
    printf("  NOTE: Windows 환경에서 관찰 가능\n");
#endif
}

static void scenario_flp34(void) {
    scenario_title("Scenario 4: 부동소수점 변환 시 범위 초과 및 특수값(NaN/Inf) 유입으로 인한 데이터 무효화 여부 확인");

    double sensor_reading = 1e40; 

    line();
    printf("  [센서 원본] double = %.2g\n", sensor_reading);

    float temp_f = (float)sensor_reading;
    printf("  [float 변환] %.9g\n", temp_f);

    int32_t temp_display = (int32_t)temp_f;
    printf("  [정수 변환] 표시 온도 = %" PRId32 "도\n", temp_display);

    line();
    double broken_sensor = NAN;
    printf("  [센서 에러] double = %.9g\n", broken_sensor);

    float broken_f = (float)broken_sensor;
    int32_t broken_i = (int32_t)broken_f;
    printf("  [float 변환] %.9g\n", broken_f);
    printf("  [정수 변환] 표시 온도 = %" PRId32 "도\n", broken_i);
}

struct Product {
    int32_t id;
    float   price;
};

static int product_equal_memcmp(const struct Product* a, const struct Product* b) {
    return 0 == memcmp(a, b, sizeof(struct Product));
}

static void scenario_flp37(void) {
    scenario_title("Scenario 5: 부동소수점 포함 구조체의 비트 단위 비교(memcmp)로 인한 동등성 판단 오류 여부 확인");
    struct Product free_a = { 1001, 0.0f };
    struct Product free_b = { 1001, -0.0f };

    line();
    printf("  [상품A] id=%d, price=%.1f원\n", free_a.id, free_a.price);
    printf("  [상품B] id=%d, price=%.1f원\n", free_b.id, free_b.price);
    printf("  memcmp 비교 => %s\n",
           product_equal_memcmp(&free_a, &free_b) ? "같음" : "다름");
    printf("  가격 == 비교 => %s\n",
           (free_a.price == free_b.price) ? "같음" : "다름");

    uint32_t nan_bits = 0x7FC00000u;
    float nan_price;
    memcpy(&nan_price, &nan_bits, sizeof(nan_price));

    struct Product broken_a = { 2002, nan_price };
    struct Product broken_b = { 2002, nan_price };

    line();
    printf("  [상품C] id=%d, price=%.1f원 (NaN)\n", broken_a.id, broken_a.price);
    printf("  [상품D] id=%d, price=%.1f원 (NaN)\n", broken_b.id, broken_b.price);
    printf("  memcmp 비교 => %s\n",
           product_equal_memcmp(&broken_a, &broken_b) ? "같음" : "다름");
    printf("  가격 == 비교 => %s\n",
           (broken_a.price == broken_b.price) ? "같음" : "다름");
}

int main(int argc, char** argv) {
    int child = run_child_mode(argc, argv);
    if (child >= 0) return child;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    setlocale(LC_ALL, ".UTF-8");

    scenario_int31();
    scenario_int32();
    scenario_int33();
    scenario_flp34();
    scenario_flp37();

    return 0;
}
