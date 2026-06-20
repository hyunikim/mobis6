#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <windows.h>
#include <process.h>

using namespace std;

string getCurrentTimestamp() {
    auto now = chrono::system_clock::now();
    auto time_t_now = chrono::system_clock::to_time_t(now);
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    stringstream ss;
    ss << put_time(localtime(&time_t_now), "%H:%M:%S") << "." << setfill('0') << setw(3) << ms.count();
    return ss.str();
}

void logMessage(const string& testName, const string& threadId, const string& message) {
    cout << "[" << getCurrentTimestamp() << "][" << testName << "][Thread-" << threadId << "] " << message << endl;
}

struct BankAccount {
    int accountId;
    double balance;
    string ownerName;
    int transactionCount;
};

BankAccount sharedAccount;
bool accountInitialized = false;
int totalDeposits = 0;
int totalWithdrawals = 0;

class ResourceManager {
private:
    int* data;
    size_t size;
    string resourceName;
    
public:
    ResourceManager(const string& name, size_t sz) : data(nullptr), size(sz), resourceName(name) {
        data = new int[size];
        for (size_t i = 0; i < size; i++) {
            data[i] = static_cast<int>(i);
        }
    }
    
    ~ResourceManager() {
        delete[] data;
    }
    
    int getElement(size_t index) {
        if (index >= size) {
            throw out_of_range("Index " + to_string(index) + " out of range for resource " + resourceName);
        }
        return data[index];
    }
    
    void setElement(size_t index, int value) {
        if (index >= size) {
            throw out_of_range("Index " + to_string(index) + " out of range for resource " + resourceName);
        }
        data[index] = value;
    }
    
    size_t getSize() { return size; }
    string getName() { return resourceName; }
};

CRITICAL_SECTION resourceMutex;
bool mutexInitialized = false;
ResourceManager* sharedResource = nullptr;
int successfulOperations = 0;
int failedOperations = 0;
bool simulateException = true;

void initMutex() {
    if (!mutexInitialized) {
        InitializeCriticalSection(&resourceMutex);
        mutexInitialized = true;
    }
}

void destroyMutex() {
    if (mutexInitialized) {
        DeleteCriticalSection(&resourceMutex);
        mutexInitialized = false;
    }
}

void initializeAccount() {
    sharedAccount.accountId = 1001;
    sharedAccount.balance = 10000.0;
    sharedAccount.ownerName = "Test User";
    sharedAccount.transactionCount = 0;
    accountInitialized = true;
    totalDeposits = 0;
    totalWithdrawals = 0;
}

struct ThreadParams {
    int threadNum;
    int iterations;
};

unsigned __stdcall depositThread(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int threadNum = params->threadNum;
    int iterations = params->iterations;
    string tid = to_string(threadNum);
    
    for (int i = 0; i < iterations; i++) {
        if (accountInitialized) {
            double currentBalance = sharedAccount.balance;
            Sleep(rand() % 10);
            double depositAmount = (rand() % 100) + 1;
            sharedAccount.balance = currentBalance + depositAmount;
            sharedAccount.transactionCount++;
            totalDeposits++;
            
            if (i % 50 == 0) {
                logMessage("CON43-C", tid, "Deposit #" + to_string(i) + 
                          " Amount: " + to_string(depositAmount) + 
                          " Balance: " + to_string(sharedAccount.balance));
            }
        }
    }
    logMessage("CON43-C", tid, "Deposit thread completed. Total deposits by this thread: " + to_string(iterations));
    return 0;
}

unsigned __stdcall withdrawThread(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int threadNum = params->threadNum;
    int iterations = params->iterations;
    string tid = to_string(threadNum);
    
    for (int i = 0; i < iterations; i++) {
        if (accountInitialized) {
            double currentBalance = sharedAccount.balance;
            Sleep(rand() % 10);
            double withdrawAmount = (rand() % 50) + 1;
            
            if (currentBalance >= withdrawAmount) {
                sharedAccount.balance = currentBalance - withdrawAmount;
                sharedAccount.transactionCount++;
                totalWithdrawals++;
            }
            
            if (i % 50 == 0) {
                logMessage("CON43-C", tid, "Withdraw #" + to_string(i) + 
                          " Balance: " + to_string(sharedAccount.balance));
            }
        }
    }
    logMessage("CON43-C", tid, "Withdraw thread completed.");
    return 0;
}

unsigned __stdcall balanceCheckerThread(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int threadNum = params->threadNum;
    int iterations = params->iterations;
    string tid = to_string(threadNum);
    int negativeBalanceCount = 0;
    int inconsistentCount = 0;
    
    for (int i = 0; i < iterations; i++) {
        if (accountInitialized) {
            double balance1 = sharedAccount.balance;
            int txCount1 = sharedAccount.transactionCount;
            Sleep(1);
            double balance2 = sharedAccount.balance;
            int txCount2 = sharedAccount.transactionCount;
            
            if (balance1 < 0 || balance2 < 0) {
                negativeBalanceCount++;
                logMessage("CON43-C", tid, "ANOMALY: Negative balance detected! " + to_string(balance1));
            }
            
            if (txCount1 == txCount2 && balance1 != balance2) {
                inconsistentCount++;
            }
        }
    }
    logMessage("CON43-C", tid, "Balance checker completed. Anomalies: " + 
              to_string(negativeBalanceCount) + " negative, " + 
              to_string(inconsistentCount) + " inconsistent");
    return 0;
}

void processResourceUnsafe(int threadNum, int iterations) {
    string tid = to_string(threadNum);
    
    for (int i = 0; i < iterations; i++) {
        EnterCriticalSection(&resourceMutex);
        logMessage("CON51-CPP", tid, "Acquired mutex, iteration " + to_string(i));
        
        try {
            if (sharedResource == nullptr) {
                throw runtime_error("Resource not initialized");
            }
            
            size_t index = rand() % (sharedResource->getSize() + 5);
            
            if (simulateException && index >= sharedResource->getSize()) {
                logMessage("CON51-CPP", tid, "Attempting invalid index: " + to_string(index));
                int value = sharedResource->getElement(index);
                (void)value;
            }
            
            if (index < sharedResource->getSize()) {
                sharedResource->setElement(index, threadNum * 1000 + i);
                successfulOperations++;
            }
            
            LeaveCriticalSection(&resourceMutex);
            logMessage("CON51-CPP", tid, "Released mutex normally");
            
        } catch (const exception& e) {
            failedOperations++;
            logMessage("CON51-CPP", tid, "EXCEPTION: " + string(e.what()));
            logMessage("CON51-CPP", tid, "Exception occurred - mutex state unknown!");
            throw;
        }
        
        Sleep(10);
    }
}

unsigned __stdcall workerThread(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int threadNum = params->threadNum;
    int iterations = params->iterations;
    string tid = to_string(threadNum);
    
    logMessage("CON51-CPP", tid, "Worker thread started");
    
    try {
        processResourceUnsafe(threadNum, iterations);
    } catch (const exception& e) {
        logMessage("CON51-CPP", tid, "Thread caught exception: " + string(e.what()));
    }
    
    logMessage("CON51-CPP", tid, "Worker thread ending");
    return 0;
}

void runCON43CTest() {
    cout << "\n" << string(70, '=') << endl;
    cout << "CON43-C TEST: Data Race in Multi-threaded Bank Account System" << endl;
    cout << string(70, '=') << endl;
    
    initializeAccount();
    double initialBalance = sharedAccount.balance;
    
    logMessage("CON43-C", "MAIN", "Initial balance: " + to_string(initialBalance));
    logMessage("CON43-C", "MAIN", "Starting 6 threads (2 deposit, 2 withdraw, 2 checker)...");
    
    HANDLE threads[6];
    ThreadParams params[6];
    
    params[0] = {1, 200};
    params[1] = {2, 200};
    params[2] = {3, 200};
    params[3] = {4, 200};
    params[4] = {5, 500};
    params[5] = {6, 500};
    
    threads[0] = (HANDLE)_beginthreadex(NULL, 0, depositThread, &params[0], 0, NULL);
    threads[1] = (HANDLE)_beginthreadex(NULL, 0, depositThread, &params[1], 0, NULL);
    threads[2] = (HANDLE)_beginthreadex(NULL, 0, withdrawThread, &params[2], 0, NULL);
    threads[3] = (HANDLE)_beginthreadex(NULL, 0, withdrawThread, &params[3], 0, NULL);
    threads[4] = (HANDLE)_beginthreadex(NULL, 0, balanceCheckerThread, &params[4], 0, NULL);
    threads[5] = (HANDLE)_beginthreadex(NULL, 0, balanceCheckerThread, &params[5], 0, NULL);
    
    WaitForMultipleObjects(6, threads, TRUE, INFINITE);
    
    for (int i = 0; i < 6; i++) {
        CloseHandle(threads[i]);
    }
    
    logMessage("CON43-C", "MAIN", "All threads completed.");
    logMessage("CON43-C", "MAIN", "Final balance: " + to_string(sharedAccount.balance));
    logMessage("CON43-C", "MAIN", "Total transactions: " + to_string(sharedAccount.transactionCount));
    logMessage("CON43-C", "MAIN", "Total deposits: " + to_string(totalDeposits) + 
              ", Total withdrawals: " + to_string(totalWithdrawals));
    
    cout << "\n[RESULT] Check if final balance and transaction counts are consistent." << endl;
    cout << "[RESULT] Expected transactions: 400 deposits + up to 400 withdrawals" << endl;
    cout << "[RESULT] Actual transaction count: " << sharedAccount.transactionCount << endl;
}

void runCON51CPPTest() {
    cout << "\n" << string(70, '=') << endl;
    cout << "CON51-CPP TEST: Mutex Lock Release on Exception" << endl;
    cout << string(70, '=') << endl;
    
    initMutex();
    sharedResource = new ResourceManager("TestResource", 10);
    successfulOperations = 0;
    failedOperations = 0;
    
    logMessage("CON51-CPP", "MAIN", "Resource initialized with size 10");
    logMessage("CON51-CPP", "MAIN", "Starting 3 worker threads...");
    
    HANDLE threads[3];
    ThreadParams params[3];
    
    params[0] = {1, 5};
    params[1] = {2, 5};
    params[2] = {3, 5};
    
    threads[0] = (HANDLE)_beginthreadex(NULL, 0, workerThread, &params[0], 0, NULL);
    threads[1] = (HANDLE)_beginthreadex(NULL, 0, workerThread, &params[1], 0, NULL);
    threads[2] = (HANDLE)_beginthreadex(NULL, 0, workerThread, &params[2], 0, NULL);
    
    WaitForMultipleObjects(3, threads, TRUE, INFINITE);
    
    for (int i = 0; i < 3; i++) {
        CloseHandle(threads[i]);
    }
    
    logMessage("CON51-CPP", "MAIN", "All threads completed (or deadlocked).");
    logMessage("CON51-CPP", "MAIN", "Successful operations: " + to_string(successfulOperations));
    logMessage("CON51-CPP", "MAIN", "Failed operations: " + to_string(failedOperations));
    
    delete sharedResource;
    destroyMutex();
    
    cout << "\n[RESULT] If program hangs, it indicates mutex was not released on exception." << endl;
    cout << "[RESULT] This demonstrates the danger of not ensuring mutex release." << endl;
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(nullptr)));
    
    cout << "\n" << string(70, '*') << endl;
    cout << "* CERT Secure Coding Rules - Concurrency Violation Examples" << endl;
    cout << "* CON43-C: Data Race Prevention" << endl;
    cout << "* CON51-CPP: Mutex Lock Release on Exception" << endl;
    cout << string(70, '*') << endl;
    
    int testChoice = 0;
    
    if (argc > 1) {
        testChoice = atoi(argv[1]);
    }
    
    if (testChoice == 0 || testChoice == 1) {
        runCON43CTest();
    }
    
    if (testChoice == 0 || testChoice == 2) {
        runCON51CPPTest();
    }
    
    cout << "\n" << string(70, '*') << endl;
    cout << "* All tests completed." << endl;
    cout << string(70, '*') << endl;
    
    return 0;
}