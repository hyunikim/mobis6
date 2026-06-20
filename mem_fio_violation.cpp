#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <new>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

static void scenario_title(const char *t) {
    printf("\n=== %s ===\n", t);
}
static void line(void) {
    printf("------------------------------------------------------------\n");
}

struct UserRecord {
    int  user_id;
    char username[64];
    char email[128];
    int  privilege_level;
};

static void scenario_mem35() {
    scenario_title("Scenario 1: 메모리 할당 크기 오계산");

    /* malloc(sizeof(rec)) 실제 코드 패턴: sizeof(포인터)=4 bytes 만 할당 */
    /* 힙에서 인접 공간 포함해 할당: [4 bytes 유효 영역 | 8 bytes sentinel 영역] */
    char *pool     = (char *)malloc(sizeof(UserRecord *) + 8);
    unsigned int *sentinel = (unsigned int *)(pool + sizeof(UserRecord *));
    *sentinel = 0xDEADBEEFu;

    UserRecord *rec = (UserRecord *)pool;   /* BUG: 4bytes에 200bytes 구조체 사용 */
    line();
    printf("  sentinel (쓰기 전):   0x%08X\n", *sentinel);

    rec->user_id = 1001;            /* offset  0-3:  유효 범위 내 */
    memset(rec->username, 0x41, 4); /* offset  4-7:  sentinel 영역 덮어씀 */

    printf("  sentinel (쓰기 후):   0x%08X  <- 인접 메모리 오염\n", *sentinel);
    free(pool);
}

static void report_login_failure(const char *username) {
    static const char fmt[] = "print %s\n";
    char msg[256];
    snprintf(msg, sizeof(msg), fmt, username);
    fprintf(stderr, msg);   /* BUG: 사용자 입력이 포함된 msg가 포맷 문자열로 전달 */
}

static void scenario_fio30() {
    scenario_title("Scenario 2: 포맷 스트링 취약점");

    line();
    printf("  username = \"%%p %%p %%p\"  ->  fprintf(stderr, msg):\n  ");
    fflush(stdout);
    report_login_failure("%p %p %p");   /* 포맷 지시자 포함: 스택 주소 노출 */
}

static void scenario_fio45() {
    scenario_title("Scenario 3: TOCTOU 경합 조건");

    const char *logpath = "audit.log";
    remove(logpath);

    line();
    /* (1) 검사: 파일 없음 확인 */
    FILE *check = fopen(logpath, "r");
    if (check) { fclose(check); return; }
    printf("  (1) fopen(\"r\") -> 없음 확인\n");

    /* ── 공격자 개입 시뮬레이션 ─────────────────────────────── */
    { FILE *atk = fopen(logpath, "w"); fprintf(atk, "ATTACKER_INJECTED\n"); fclose(atk); }
    printf("  [공격자] %s 에 악성 내용 삽입\n", logpath);
    /* ──────────────────────────────────────────────────────── */

    /* (2) 사용: 공격자가 만든 파일을 덮어씀 (실제 공격 시 심볼릭링크로 임의 파일 덮어쓰기 가능) */
    FILE *fp = fopen(logpath, "w");
    if (!fp) return;
    fprintf(fp, "session started\n");
    fclose(fp);

    /* 결과 확인 */
    FILE *v = fopen(logpath, "r"); char buf[64] = {};
    if (fgets(buf, sizeof(buf), v)) { buf[strcspn(buf, "\n")] = 0; }
    fclose(v);
    printf("  (2) fopen(\"w\") -> 의도: 신규 생성 / 실제: 공격자 파일 덮어씀\n");
    printf("  %s 최종 내용: \"%s\"\n", logpath, buf);
    remove(logpath);
}

static std::fstream *g_leaked_fs = nullptr;

static bool write_report(const std::string &path, const std::string &data) {
    std::fstream *fs = new std::fstream(path, std::ios::out | std::ios::trunc);

    if (!fs->is_open()) { delete fs; return false; }

    if (data.empty()) {
        g_leaked_fs = fs;
        return false;       /* BUG: close/delete 없이 반환 → 파일 핸들 누수 */
    }

    *fs << data;
    fs->close();
    delete fs;
    return true;
}

static void scenario_fio51() {
    scenario_title("Scenario 4: 파일 핸들 누수");

    line();
    g_leaked_fs = nullptr;
    write_report("report.txt", "");   /* data 없음 → 누수 경로 */
    if (g_leaked_fs) {
        printf("  write_report() 반환 후 is_open() = %s\n",
               g_leaked_fs->is_open() ? "true  <- 파일 핸들 미반환" : "false");
    }
    remove("report.txt");
}

static int g_live = 0;

struct Payload {
    int  id;
    char data[256];
    Payload(int i) : id(i) { g_live++; printf("  Payload(%d) 생성 [live=%d]\n", i, g_live); }
    ~Payload()              { g_live--; printf("  ~Payload(%d) 소멸 [live=%d]\n", id, g_live); }
};

static void scenario_mem52() {
    scenario_title("Scenario 5: 할당 실패 미처리");

        /* [A] noexcept 함수에서 new[] 예외(std::bad_alloc) 미처리 -> std::terminate 가능 */
    line();
    printf("  [A] noexcept + new[] (예외 미처리)");
    auto copy_noexcept = [](const int *array, std::size_t size) noexcept {
        /* BUG: new[]는 실패 시 std::bad_alloc (또는 bad_array_new_length)을 던질 수 있음 */
        int *copy = new int[size];
        std::memcpy(copy, array, size * sizeof(*copy));
        delete[] copy;
    };

    /* 큰 size로 할당 실패 가능성을 높임(환경에 따라 실패/성공은 달라질 수 있음) */
    std::size_t huge_size = static_cast<std::size_t>(536000000);
    const int dummy[1] = {0};
    printf("  copy_noexcept(dummy, %zu) 호출(실패 시 프로그램 종료 가능)", huge_size);
    copy_noexcept(dummy, huge_size);
/* [B] new(nothrow) 결과를 nullptr 체크 없이 역참조 */
    line();
    printf("  [B] new(nothrow) int[536000000]\n");
    std::size_t giant = static_cast<std::size_t>(536000000);
    int *huge = new(std::nothrow) int[giant];
    printf("  huge = %p  (nullptr=%d)\n", (void*)huge, huge == nullptr);
    /* huge[0] = 42; */   /* 주석 해제 시: nullptr 역참조 → Segfault */
    delete[] huge;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    scenario_mem35();
    scenario_fio30();
    scenario_fio45();
    scenario_fio51();
    scenario_mem52();

    return 0;
}
