#include <iostream>
#include <string>

class SensorBase {
protected:
    std::string sensorId;

public:
    SensorBase(const std::string& id) : sensorId(id) {
        std::cout << "[Base] Constructor start: " << sensorId << std::endl;

        initializeSensor();

        std::cout << "[Base] Constructor end: " << sensorId << std::endl;
    }

    ~SensorBase() {
        std::cout << "[Base] Destructor start: " << sensorId << std::endl;

        shutdownSensor();

        std::cout << "[Base] Destructor end: " << sensorId << std::endl;
    }

    virtual void initializeSensor() {
        std::cout << "[Base] initializeSensor()" << std::endl;
    }

    virtual void shutdownSensor() {
        std::cout << "[Base] shutdownSensor()" << std::endl;
    }
};

class TemperatureSensor : public SensorBase {
public:
    TemperatureSensor(const std::string& id) : SensorBase(id) {
        std::cout << "[Derived] Constructor: " << sensorId << std::endl;
    }

    ~TemperatureSensor() {
        std::cout << "[Derived] Destructor: " << sensorId << std::endl;
    }

    void initializeSensor() override {
        std::cout << "[Derived] initializeSensor()" << std::endl;
    }

    void shutdownSensor() override {
        std::cout << "[Derived] shutdownSensor()" << std::endl;
    }
};

// ============================================================================

int main() {
    std::cout << "=== Violation Example ===" << std::endl;

    TemperatureSensor sensor("TEMP_001");

    std::cout << "=== End ===" << std::endl;
    return 0;
}
