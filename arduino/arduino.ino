#include <ArduinoJson.h>

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>



// 1. 핀 번호 설정 (정리됨)
#define DHTPIN 2  // 온습도 센서
#define DHTTYPE DHT11
#define RED_LED 3     // 너무 축축함 (OVERFLOW)
#define YELLOW_LED 4  // 너무 건조함 (THIRSTY)
#define GREEN_LED 5   // 상태 좋음 (FINE)
#define SOIL_PIN A0    // 토양 수분 센서
#define RELAY_PIN 10 // 릴레이 제어 핀 (펌프 ON/OFF)
#define PUMP_ON  HIGH   // 능동-LOW 모듈이면 LOW
#define PUMP_OFF LOW    // 능동-LOW 모듈이면 HIGH

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long lastLcdUpdateTime;
unsigned long lastSentTime;
const unsigned long interval = 1000;

enum class PlantStatus {
  THIRSTY,
  OVERFLOW,
  FINE
};

unsigned long pumpStartTime = 0;
const unsigned long pumpRunTime = 2000; // 2초 동안 물을 줍니다.
bool isPumpRunning = false;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);

  dht.begin();

  // LCD 초기화
  lcd.init();
  lcd.backlight();

  // LED 핀들을 출력으로 설정
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // 시작 화면
  lcd.setCursor(0, 0);
  lcd.print("Smart Garden");
  lcd.setCursor(0, 1);
  lcd.print("System Ready!");
  delay(1500);
  lcd.clear();

  // 펌프 초기화
  pinMode(RELAY_PIN, OUTPUT); // 릴레이 핀 출력 설정
  digitalWrite(RELAY_PIN, PUMP_OFF); // 처음엔 펌프 OFF
}

void loop() {

  // 0. 펌프 기능
  // 물이 넘치면 안되기 때문에 맨 위에 배치합니다!!

  // 수신된 메시지를 확인하고 펌프를 작동시키거나 멈춥니다.
  checkReceivedMessage();

  if (isPumpRunning) {
    unsigned long currentTime = millis();
    if (currentTime - pumpStartTime >= pumpRunTime) {
      stopPump();
    }
  }

  // 1. 센서 값 읽기 (수분과 온도만)
  int soilMoisture = analogRead(SOIL_PIN);
  float temperature = dht.readTemperature();
  if (isnan(temperature)) {
    temperature = 0.0;
  }

  // 2. 상태 판별하기
  PlantStatus currentStatus = checkPlantStatus(soilMoisture);

  updateLcd(soilMoisture, temperature, currentStatus);
  
  updateLed(currentStatus);

  sendMessage(soilMoisture, temperature, currentStatus);
}

PlantStatus checkPlantStatus(int soilMoisture) {
  if (soilMoisture > 950) {
      // 너무 건조할 때 (노란불)
      return PlantStatus::THIRSTY;
    } else if (soilMoisture < 300) {
      // 너무 축축할 때 (빨간불)
      return PlantStatus::OVERFLOW;
    } else {
      // 딱 좋을 때 (초록불)
      return PlantStatus::FINE;
    }
}

void checkReceivedMessage() {
    if (Serial.available() > 0) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, Serial);

    if (!error) {
      if (doc.containsKey("type")) {
        String type = doc["type"];

        if (type == "pump") {
          String action = doc["action"];

          if (action == "on" && !isPumpRunning) {
            startPump();
          } else if (action == "off") {
            stopPump();
          }
        }
        
      }
    }
  }
}

void startPump() {
  if (!isPumpRunning) {
    isPumpRunning = true;
    pumpStartTime = millis();
    digitalWrite(RELAY_PIN, PUMP_ON);
  }
}

void stopPump() {
  digitalWrite(RELAY_PIN, PUMP_OFF);
  isPumpRunning = false;
}

void updateLcd(int soilMoisture, float temperature, PlantStatus currentStatus) {
  unsigned long currentTime = millis();
  if (currentTime - lastLcdUpdateTime >= interval) {
    // 2. LCD 첫 줄: 수분과 온도 표시
    lcd.setCursor(0, 0);
    lcd.print("S:");
    lcd.print(soilMoisture);
    lcd.print("  T:");
    lcd.print((int)temperature);  // 온도는 정수로 깔끔하게 표시
    lcd.print("C   ");

    // 3. 수분 상태 체크 및 LED/LCD 둘째 줄 표시
    lcd.setCursor(0, 1);

    if (currentStatus == PlantStatus::THIRSTY) {
      // 너무 건조할 때 (노란불)
      lcd.print("Status: THIRSTY ");
    } else if (currentStatus == PlantStatus::OVERFLOW) {
      // 너무 축축할 때 (빨간불)
      lcd.print("Status: OVERFLOW");
    } else {
      // 딱 좋을 때 (초록불)
      lcd.print("Status: FINE    ");
    }

    lastLcdUpdateTime = currentTime;
  }
}

void updateLed(PlantStatus status) {
  switch (status) {
    case PlantStatus::THIRSTY:
      turnOnYellowLed();
      break;
    
    case PlantStatus::OVERFLOW:
      turnOnRedLed();
      break;
    
    case PlantStatus::FINE:
      turnOnGreenLed();
      break;
  }
}

void turnOnYellowLed() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);
}

void turnOnRedLed() {
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
}

void turnOnGreenLed() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(YELLOW_LED, LOW);
}

void sendMessage(int soilMoisture, float temperature, PlantStatus status) {
  unsigned long currentTime = millis();
  if (currentTime - lastSentTime >= interval) {
    JsonDocument doc;

    doc["soil_moisture"] = soilMoisture;
    doc["temperature"] = temperature;
    
    doc["status"] = getStatusString(status);

    serializeJson(doc, Serial);
    Serial.println();

    lastSentTime = currentTime;
  }
}

String getStatusString(PlantStatus status) {
  switch (status) {
    case PlantStatus::THIRSTY:
      return "THIRSTY";
    case PlantStatus::OVERFLOW:
      return "OVERFLOW";
    case PlantStatus::FINE:
      return "FINE";
    default:
      return "UNKNOWN";
  }
}
