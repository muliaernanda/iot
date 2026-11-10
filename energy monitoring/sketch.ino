#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <esp_timer.h>

// Smart Energy Monitoring System - ESP32 / Wokwi
// Educational simulation only. No connection to mains electricity.

constexpr int PIN_VOLTAGE_POT = 34; // ADC1 input-only GPIO
constexpr int PIN_CURRENT_POT = 35; // ADC1 input-only GPIO
constexpr int PIN_LED_GREEN   = 18;
constexpr int PIN_LED_YELLOW  = 19;
constexpr int PIN_LED_RED     = 23;
constexpr int PIN_BUZZER      = 25;

constexpr float MAX_SIM_VOLTAGE = 250.0f; // 0..250 V (simulated)
constexpr float MAX_SIM_CURRENT = 10.0f;  // 0..10 A (simulated)
constexpr float INVALID_LOW_VOLTAGE = 1.0f;
constexpr float INVALID_LOW_CURRENT = 0.02f;
constexpr float HIGH_POWER_LIMIT = 1000.0f;

constexpr uint32_t SAMPLE_INTERVAL_MS = 500;
constexpr uint32_t LCD_PAGE_INTERVAL_MS = 2000;

LiquidCrystal_I2C lcd(0x27, 16, 2);

uint32_t lastSampleMs = 0;
uint32_t lastLcdPageMs = 0;
uint64_t lastEnergyUpdateUs = 0;
double energyWh = 0.0;
float voltage = 0.0f;
float current = 0.0f;
float powerW = 0.0f;
bool sensorValid = false;
uint8_t lcdPage = 0;

void setOutputs(bool green, bool yellow, bool red, bool buzzer) {
  digitalWrite(PIN_LED_GREEN, green ? HIGH : LOW);
  digitalWrite(PIN_LED_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(PIN_LED_RED, red ? HIGH : LOW);
  digitalWrite(PIN_BUZZER, buzzer ? HIGH : LOW);
}

void updateOutputs() {
  if (!sensorValid) {
    setOutputs(false, false, true, false);
  } else if (powerW < 500.0f) {
    setOutputs(true, false, false, false);
  } else if (powerW < HIGH_POWER_LIMIT) {
    setOutputs(false, true, false, false);
  } else {
    setOutputs(false, false, true, true);
  }
}

void showError() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SENSOR ERROR");
  lcd.setCursor(0, 1);
  lcd.print("Check input pots");
}

void showPage() {
  if (!sensorValid) {
    showError();
    return;
  }

  lcd.clear();
  if (lcdPage == 0) {
    lcd.setCursor(0, 0);
    lcd.print("V:");
    lcd.print(voltage, 1);
    lcd.print(" I:");
    lcd.print(current, 2);
    lcd.setCursor(0, 1);
    lcd.print("Sensor readings");
  } else if (lcdPage == 1) {
    lcd.setCursor(0, 0);
    lcd.print("POWER: ");
    lcd.print(powerW, 0);
    lcd.print("W");
    lcd.setCursor(0, 1);
    lcd.print("ENERGY:");
    // Keep the numeric field short enough for a 16x2 display.
    if (energyWh < 1000.0) {
      lcd.print(energyWh, 1);
    } else {
      lcd.print(energyWh, 0);
    }
    lcd.print("Wh");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Energy Monitor");
    lcd.setCursor(0, 1);
    if (powerW >= HIGH_POWER_LIMIT) {
      lcd.print("WARNING HIGH PWR");
    } else if (powerW >= 500.0f) {
      lcd.print("LOAD: MEDIUM");
    } else {
      lcd.print("LOAD: NORMAL");
    }
  }
}

void readSensorsAndCalculate(uint32_t nowMs) {
  const int rawVoltage = analogRead(PIN_VOLTAGE_POT);
  const int rawCurrent = analogRead(PIN_CURRENT_POT);

  // ESP32 ADC is configured for 12-bit readings: 0..4095.
  voltage = (rawVoltage / 4095.0f) * MAX_SIM_VOLTAGE;
  current = (rawCurrent / 4095.0f) * MAX_SIM_CURRENT;

  sensorValid = (rawVoltage >= 1 && rawCurrent >= 1 &&
                 voltage >= INVALID_LOW_VOLTAGE &&
                 current >= INVALID_LOW_CURRENT);

  if (sensorValid) {
    powerW = voltage * current;

    // Integrate using the actual elapsed time, not the nominal sample period.
    const uint64_t nowUs = esp_timer_get_time();
    if (lastEnergyUpdateUs != 0) {
      const double elapsedHours =
          static_cast<double>(nowUs - lastEnergyUpdateUs) / 3600000000.0;
      energyWh += static_cast<double>(powerW) * elapsedHours;
    }
    lastEnergyUpdateUs = nowUs;
  } else {
    powerW = 0.0f;
    // Reset integration baseline while invalid to avoid counting the gap.
    lastEnergyUpdateUs = 0;
  }

  (void)nowMs;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  setOutputs(false, false, false, false);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_VOLTAGE_POT, ADC_11db);
  analogSetPinAttenuation(PIN_CURRENT_POT, ADC_11db);

  Wire.begin(21, 22); // SDA, SCL
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SMART ENERGY");
  lcd.setCursor(0, 1);
  lcd.print("MONITOR START");
  lastSampleMs = millis();
  lastLcdPageMs = millis();
}

void loop() {
  const uint32_t nowMs = millis();

  if (nowMs - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = nowMs;
    readSensorsAndCalculate(nowMs);
    updateOutputs();

    Serial.print("V=");
    Serial.print(voltage, 1);
    Serial.print(" V, I=");
    Serial.print(current, 2);
    Serial.print(" A, P=");
    Serial.print(powerW, 1);
    Serial.print(" W, E=");
    Serial.print(energyWh, 4);
    Serial.println(" Wh");
  }

  if (nowMs - lastLcdPageMs >= LCD_PAGE_INTERVAL_MS) {
    lastLcdPageMs = nowMs;
    lcdPage = (lcdPage + 1) % 3;
    showPage();
  }
}
