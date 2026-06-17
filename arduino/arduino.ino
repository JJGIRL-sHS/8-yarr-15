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
#define SOIL_A0 A0    // 토양 수분 센서

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
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
}

void loop() {
  // 1. 센서 값 읽기 (수분과 온도만)
  int soilValue = analogRead(SOIL_A0);
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    temp = 0;
  }

  // 2. LCD 첫 줄: 수분과 온도 표시
  lcd.setCursor(0, 0);
  lcd.print("S:");
  lcd.print(soilValue);
  lcd.print("  T:");
  lcd.print((int)temp);  // 온도는 정수로 깔끔하게 표시
  lcd.print("C   ");

  // 3. 수분 상태 체크 및 LED/LCD 둘째 줄 표시
  lcd.setCursor(0, 1);

  if (soilValue > 950) {
    // 너무 건조할 때 (노란불)
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    lcd.print("Status: THIRSTY ");
  } else if (soilValue < 300) {
    // 너무 축축할 때 (빨간불)
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    lcd.print("Status: OVERFLOW");
  } else {
    // 딱 좋을 때 (초록불)
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    lcd.print("Status: FINE    ");
  }

  delay(1000);  // 1초마다 업데이트

  String statusText;

  if (soilValue > 950) {
    statusText = "THIRSTY";
  } else if (soilValue < 300) {
    statusText = "OVERFLOW";
  } else {
    statusText = "FINE";
  }

  Serial.print(soilValue);
  Serial.print(",");
  Serial.print(temp);
  Serial.print(",");
  Serial.println(statusText);
}