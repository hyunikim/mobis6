#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* ============================================================================
 * 플랫폼 호환성 매크로 정의
 * Windows/MinGW와 POSIX(Linux/macOS) 환경 모두 지원
 * ============================================================================ */
#ifdef _WIN32
    #include <io.h>
    #include <direct.h>
    #define PATH_SEPARATOR "\\"
    #define MKDIR(path) _mkdir(path)
    #define RMDIR(path) _rmdir(path)
    #define S_IRUSR _S_IREAD
    #define S_IWUSR _S_IWRITE
    #define OPEN(path, flags, mode) _open(path, flags, mode)
    #define READ(fd, buf, count) _read(fd, buf, count)
    #define WRITE(fd, buf, count) _write(fd, buf, (unsigned int)(count))
    #define CLOSE(fd) _close(fd)
    #define SETENV(name, value) _putenv_s(name, value)
    #define ENV_TEMP "TEMP"
    #define ENV_HOME "USERPROFILE"
#else
    #include <unistd.h>
    #define PATH_SEPARATOR "/"
    #define MKDIR(path) mkdir(path, 0755)
    #define RMDIR(path) rmdir(path)
    #define OPEN(path, flags, mode) open(path, flags, mode)
    #define READ(fd, buf, count) read(fd, buf, count)
    #define WRITE(fd, buf, count) write(fd, buf, count)
    #define CLOSE(fd) close(fd)
    #define SETENV(name, value) setenv(name, value, 1)
    #define ENV_TEMP "TMPDIR"
    #define ENV_HOME "HOME"
#endif

/* ============================================================================
 * 상수 정의
 * ============================================================================ */
#define LOG_FILE "app_log.txt"
#define CONFIG_FILE "config.txt"
#define MAX_PATH_LEN 256
#define MAX_BUFFER 1024

/* ============================================================================
 * 전역 변수
 * ============================================================================ */
FILE* g_logFile = NULL;

/* ============================================================================
 * 로깅 함수
 * ============================================================================ */
void write_log(const char* message) {
    if (g_logFile) {
        fprintf(g_logFile, "[LOG] %s\n", message);
        fflush(g_logFile);
    }
    printf("[LOG] %s\n", message);
}

void init_logging() {
    g_logFile = fopen(LOG_FILE, "a");
    write_log("Application started");
}

void close_logging() {
    write_log("Application finished");
    if (g_logFile) {
        fclose(g_logFile);
    }
}

/* ============================================================================
 * Scenario 1: 파일 연산 (ERR33-C 위반)
 * ============================================================================ */
void scenario1_file_operations() {
    write_log("=== Scenario 1: File Operations ===");
    
    FILE* configFile = fopen(CONFIG_FILE, "r");
    
    char buffer[MAX_BUFFER];
    fgets(buffer, MAX_BUFFER, configFile);
    printf("  Config content: %s", buffer);
    
    fclose(configFile);
    
    FILE* dataFile = fopen("data.bin", "wb");
    int data[] = {1, 2, 3, 4, 5};
    fwrite(data, sizeof(int), 5, dataFile);
    fclose(dataFile);
    
    dataFile = fopen("data.bin", "rb");
    int readData[5];
    fread(readData, sizeof(int), 5, dataFile);
    
    printf("  Read data: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", readData[i]);
    }
    printf("\n");
    
    fclose(dataFile);
    
    rename("data.bin", "data_backup.bin");
    remove("data_backup.bin");
    
    write_log("File operations completed");
}

/* ============================================================================
 * Scenario 2: 숫자 변환 (ERR34-C 위반)
 * ============================================================================ */
void scenario2_numeric_conversion() {
    write_log("=== Scenario 2: Numeric Conversion ===");
    
    const char* userInputs[] = {
        "12345",
        "  -9876  ",
        "42abc",
        "99999999999999999999",
        "0x1A2B",
        "",
        "   ",
        NULL
    };
    
    for (int i = 0; userInputs[i] != NULL; i++) {
        printf("  Processing input[%d]: \"%s\"\n", i, userInputs[i]);
        
        int value = atoi(userInputs[i]);
        printf("    atoi result: %d\n", value);
        
        long longValue = atol(userInputs[i]);
        printf("    atol result: %ld\n", longValue);
        
        double doubleValue = atof(userInputs[i]);
        printf("    atof result: %f\n", doubleValue);
        printf("\n");
    }
    
    char priceStr[] = "199.99USD";
    double price = atof(priceStr);
    printf("  Product price: %.2f\n", price);
    
    char quantityStr[] = "5 items";
    int quantity = atoi(quantityStr);
    printf("  Quantity: %d\n", quantity);
    
    int total = (int)(price * quantity);
    printf("  Total: %d\n", total);
    
    write_log("Numeric conversion completed");
}

/* ============================================================================
 * Scenario 3: 파일 권한 처리 (CWE-560 위반)
 * ============================================================================ */
void scenario3_permission_handling() {
    write_log("=== Scenario 3: File Permission Handling ===");
    
    const char* secretFile = "secret_data.txt";
    
    FILE* f = fopen(secretFile, "w");
    fprintf(f, "CONFIDENTIAL: API_KEY=sk-12345abcde\n");
    fprintf(f, "DATABASE_PASSWORD=super_secret_123\n");
    fclose(f);
    
    int fd = OPEN(secretFile, O_RDWR, 0);
    
    char readBuffer[256];
    READ(fd, readBuffer, sizeof(readBuffer) - 1);
    printf("  Secret file content accessible: YES\n");
    
    CLOSE(fd);
    
    const char* configDir = "app_config";
    MKDIR(configDir);
    
    char configFilePath[MAX_PATH_LEN];
    sprintf(configFilePath, "%s%ssettings.ini", configDir, PATH_SEPARATOR);
    
    FILE* settings = fopen(configFilePath, "w");
    fprintf(settings, "[Database]\nhost=localhost\nport=3306\n");
    fclose(settings);
    
    printf("  Config directory created\n");
    printf("  Settings file created\n");
    
    write_log("Permission handling completed");
}

/* ============================================================================
 * 정리 함수
 * ============================================================================ */
void cleanup() {
    remove("secret_data.txt");
    char settingsPath[MAX_PATH_LEN];
    snprintf(settingsPath, sizeof(settingsPath), "app_config%ssettings.ini", PATH_SEPARATOR);
    remove(settingsPath);
    RMDIR("app_config");
    remove(CONFIG_FILE);
    remove(LOG_FILE);
}

/* ============================================================================
 * 메인 함수
 * ============================================================================ */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("============================================\n");
    printf("   Secure Coding Practice - VULNERABLE     \n");
    printf("   (Windows MinGW / GCC Compatible)        \n");
    printf("============================================\n\n");
    
    init_logging();
    
    scenario1_file_operations();
    printf("\n");
    
    scenario2_numeric_conversion();
    printf("\n");
    
    scenario3_permission_handling();
    printf("\n");
    
    close_logging();
    
    printf("\n============================================\n");
    printf("   All scenarios completed                  \n");
    printf("============================================\n\n");
    
    return 0;
}