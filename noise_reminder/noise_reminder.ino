/*
 * 噪音提醒装置
 * 功能：拍手 → 红灯亮 → 按按钮 → 蓝灯亮5秒 → 恢复
 * LCD 背光通过短接常亮
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD 地址 0x27（根据你的扫描结果）
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 引脚定义
const int MIC_PIN = A0;          // MAX9814 声音传感器
const int RED_LED = 3;           // 红色 LED
const int BLUE_LED = 4;          // 蓝色 LED
const int BUTTON_PIN = 2;        // 按钮

// 可调参数
int SOUND_THRESHOLD = 400;       // 声音阈值（安静300，拍手500）
const int NOISE_DURATION = 300;  // 噪音持续多久才触发（毫秒）
const int BLUE_HOLD = 5000;      // 蓝灯亮多久后熄灭（毫秒）

// 状态变量
bool isRedOn = false;
bool isBlueOn = false;
unsigned long noiseStartTime = 0;
unsigned long blueStartTime = 0;

void setup() {
  // 设置引脚模式
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 初始状态：全灭
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

  // 初始化 LCD（背光已通过短接常亮，不需要 lcd.backlight()）
  lcd.init();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Noise Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Ready...");
  delay(2000);
  lcd.clear();

  // 串口调试
  Serial.begin(9600);
  Serial.println("系统就绪 - 拍手触发红灯，按按钮蓝灯5秒");
}

void loop() {
  // ===== 1. 读取声音信号（峰值检测）=====
  unsigned long start = millis();
  int maxVal = 0;
  while (millis() - start < 50) {
    int val = analogRead(MIC_PIN);
    if (val > maxVal) maxVal = val;
  }
  Serial.println(maxVal);

  // ===== 2. 更新 LCD 显示 =====
  lcd.setCursor(0, 0);
  lcd.print("Vol:");
  lcd.print(maxVal);
  lcd.print("   ");

  lcd.setCursor(0, 1);
  if (isRedOn) {
    lcd.print("RED ON - Press!");
  } else if (isBlueOn) {
    lcd.print("BLUE ON - OK   ");
  } else {
    lcd.print("Waiting...      ");
  }

  // ===== 3. 读取按钮状态 =====
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);

  // ===== 4. 噪音检测（红灯和蓝灯都未亮时）=====
  if (!isRedOn && !isBlueOn) {
    if (maxVal > SOUND_THRESHOLD) {
      if (noiseStartTime == 0) {
        noiseStartTime = millis();
      } else if (millis() - noiseStartTime >= NOISE_DURATION) {
        digitalWrite(RED_LED, HIGH);
        isRedOn = true;
        noiseStartTime = 0;
        Serial.println("触发！红灯亮");
      }
    } else {
      noiseStartTime = 0;
    }
  }

  // ===== 5. 按钮处理（红灯亮时按下，关闭红灯，开启蓝灯）=====
  if (isRedOn && buttonPressed) {
    digitalWrite(RED_LED, LOW);
    isRedOn = false;

    digitalWrite(BLUE_LED, HIGH);
    isBlueOn = true;
    blueStartTime = millis();

    Serial.println("按钮按下！红灯灭，蓝灯亮");

    // 防抖：等待按钮松开
    delay(50);
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }
  }

  // ===== 6. 蓝灯自动熄灭 =====
  if (isBlueOn && (millis() - blueStartTime >= BLUE_HOLD)) {
    digitalWrite(BLUE_LED, LOW);
    isBlueOn = false;
    Serial.println("蓝灯熄灭，系统恢复");
  }
}