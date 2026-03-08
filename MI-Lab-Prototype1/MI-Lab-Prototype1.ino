#include <BLEDevice.h>
  #include <BLEUtils.h>
  #include <BLEServer.h>

  #define MOTOR_PIN 5
  #define US_TRIG_PIN 2
  #define US_ECHO_PIN 3

  #define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
  #define CHARACTERISTIC_UUID "abcd1234-5678-5678-5678-abcdef123456"

  const int BLE_MIN_POWER = 10;            // lower than before for more dynamic range
  const int BLE_MAX_POWER = 255;

  const bool SENSOR_OVERRIDE_ENABLED = true;
  const float PROX_THRESHOLD_CM = 22.0f;   // alarm activates when object is closer than
  const unsigned long SENSOR_READ_MS = 45; // ultrasonic update interval

  const uint8_t ALARM_PWM = 255;
  const uint16_t ALARM_ON_MS = 28;          // short buzz
  const uint16_t ALARM_OFF_MS_FAR = 220;    // slower when farther
  const uint16_t ALARM_OFF_MS_NEAR = 20;    // very fast when very close
  const float ALARM_NEAR_CM = 6.0f;


  BLECharacteristic *pCharacteristic = nullptr;

  volatile int g_bleRequestedPwm = 0;
  float g_lastDistanceCm = -1.0f;
  bool g_alarmActive = false;
  bool g_alarmPulseOn = false;
  unsigned long g_alarmPhaseStartedAt =0;
  unsigned long g_lastSensorReadAt = 0;

  class MotorCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) override {
      std::string value = characteristic->getValue();
      if (value.empty()) return;

      int power = atoi(value.c_str());
      power = constrain(power, 0, 255);
      g_bleRequestedPwm = power;
    }
  };

  int mapBleToMotorPwm(int blePower) {
    blePower = constrain(blePower, 0, 255);
    if (blePower <= 0) return 0;
    return map(blePower, 1, 255, BLE_MIN_POWER, BLE_MAX_POWER);
  }

  float readUltrasonicCm() {
    digitalWrite(US_TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(US_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(US_TRIG_PIN, LOW);

    // ~25ms timeout (~4m). Returns 0 on timeout.
    unsigned long durationUs = pulseIn(US_ECHO_PIN, HIGH, 25000UL);
    if (durationUs == 0) return -1.0f;

    return (durationUs * 0.0343f) * 0.5f;
  }

  uint16_t computeAlarmOffMs(float distanceCm) {
    float farCm = PROX_THRESHOLD_CM;
    float nearCm = ALARM_NEAR_CM;

    if (distanceCm <= 0.0f) return ALARM_OFF_MS_FAR;

    float t= 1.0f - constrain((distanceCm - nearCm) / (farCm - nearCm), 0.0f, 1.0f);
    return (uint16_t) (ALARM_OFF_MS_FAR + (ALARM_OFF_MS_NEAR - ALARM_OFF_MS_FAR) * t);
   }

  void updateSensorState(unsigned long nowMs) {
    if (!SENSOR_OVERRIDE_ENABLED) {
      g_alarmActive = false;
      return;
    }

    if (nowMs - g_lastSensorReadAt < SENSOR_READ_MS) return;
    g_lastSensorReadAt = nowMs;

    float cm = readUltrasonicCm();
    if (cm > 0.0f) {
      g_lastDistanceCm = cm;
    }

    bool shouldAlarm = (g_lastDistanceCm > 0.0f && g_lastDistanceCm <=
  PROX_THRESHOLD_CM);

    if (shouldAlarm && !g_alarmActive) {
      g_alarmActive = true;
      g_alarmPulseOn = true;
      g_alarmPhaseStartedAt = nowMs;
    } else if (!shouldAlarm && g_alarmActive) {
      g_alarmActive = false;
      g_alarmPulseOn = false;
    }
  }

  int updateAlarmPwm(unsigned long nowMs) {
    if (!g_alarmActive) return 0;

    uint16_t offMs = computeAlarmOffMs(g_lastDistanceCm);
    uint16_t phaseMs = g_alarmPulseOn ? ALARM_ON_MS : offMs;

    if (nowMs - g_alarmPhaseStartedAt >= phaseMs) {
      g_alarmPulseOn = !g_alarmPulseOn;
      g_alarmPhaseStartedAt = nowMs;
    }

    return g_alarmPulseOn ? ALARM_PWM : 0;
  }

  void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(US_TRIG_PIN, OUTPUT);
    pinMode(US_ECHO_PIN, INPUT);

    ledcSetup(0, 200, 8);
    ledcAttachPin(MOTOR_PIN, 0);
    ledcWrite(0, 0);

    BLEDevice::init("MagnetHand");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCharacteristic->setCallbacks(new MotorCallbacks());

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->start();

    Serial.println("BLE Ready. Sensor override enabled.");
  }

  void loop() {
    unsigned long nowMs = millis();
    updateSensorState(nowMs);

    int outputPwm = 0;

    if (g_alarmActive) {
      // Physical-world proximity alarm overrides BLE magnet buzz.
      outputPwm = updateAlarmPwm(nowMs);
    } else {
      int requested = g_bleRequestedPwm;
      outputPwm = mapBleToMotorPwm(requested);
    }

    ledcWrite(0, outputPwm);
  }