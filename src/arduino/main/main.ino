#include <Wire.h>

// Пин подключения кнопки
#define BUTTON_PIN 2

// Пины для подключения сдвигового регистра 74HC595
#define DATA_PIN 11    // DS (pin 14 на 74HC595)
#define LATCH_PIN 12   // ST_CP (pin 12 на 74HC595)
#define CLOCK_PIN 13   // SH_CP (pin 11 на 74HC595)

// Массив для отображения цифр на 7-сегментном индикаторе (общий анод)
// Биты: DP A B C D E F G 
const byte digitPatterns[10] = {
  B11000000, // 0
  B11111001, // 1
  B10100100, // 2
  B10110000, // 3
  B10011001, // 4
  B10010010, // 5
  B10000010, // 6
  B11111000, // 7
  B10000000, // 8
  B10010000  // 9
};

// Переменные для хранения времени
byte hours = 12;
byte minutes = 0;
byte seconds = 0;

// Переменные для управления кнопкой
bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Переменные для режима настройки
bool settingMode = false;
byte settingPosition = 0; // 0 - часы, 1 - минуты
unsigned long lastSettingChange = 0;
unsigned long settingTimeout = 10000; // 10 секунд на настройку

// Переменные для динамической индикации
unsigned long lastUpdateTime = 0;
const byte updateInterval = 5; // интервал обновления индикатора в мс
byte currentDigit = 0;
byte digits[4] = {0}; // массив для хранения цифр для отображения

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  
  // Инициализация времени
  updateDisplayData();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Обработка кнопки
  bool reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = currentMillis;
  }
  
  if ((currentMillis - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      if (buttonState == LOW) {
        handleButtonPress();
      }
    }
  }
  
  lastButtonState = reading;
  
  // Обновление времени
  if (!settingMode && currentMillis - lastUpdateTime >= 1000) {
    lastUpdateTime = currentMillis;
    updateTime();
    updateDisplayData();
  }
  
  // Динамическая индикация
  if (currentMillis - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentMillis;
    displayDigit();
  }
  
  // Выход из режима настройки по таймауту
  if (settingMode && currentMillis - lastSettingChange > settingTimeout) {
    settingMode = false;
    settingPosition = 0;
  }
}

void updateTime() {
  seconds++;
  if (seconds >= 60) {
    seconds = 0;
    minutes++;
    if (minutes >= 60) {
      minutes = 0;
      hours++;
      if (hours >= 24) {
        hours = 0;
      }
    }
  }
}

void updateDisplayData() {
  digits[0] = hours / 10;
  digits[1] = hours % 10;
  digits[2] = minutes / 10;
  digits[3] = minutes % 10;
}

void displayDigit() {
  // Отключаем все сегменты перед обновлением
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, B11111111);
  digitalWrite(LATCH_PIN, HIGH);
  
  byte pattern;
  
  if (settingMode) {
    // В режиме настройки мигаем редактируемым разрядом
    if ((millis() / 500) % 2 == 0 || 
        (settingPosition == 0 && currentDigit >= 2) || 
        (settingPosition == 1 && currentDigit < 2)) {
      pattern = digitPatterns[digits[currentDigit]];
    } else {
      pattern = B11111111; // все сегменты выключены
    }
  } else {
    pattern = digitPatterns[digits[currentDigit]];
  }
  
  // Добавляем точку для разделения часов и минут на 3-м разряде
  if (currentDigit == 1) {
    pattern &= ~B10000000; // включаем точку (для общего анода)
  }
  
  // Выбираем разряд (общий анод)
  byte digitSelect = ~(1 << (3 - currentDigit));
  
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, pattern);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, digitSelect);
  digitalWrite(LATCH_PIN, HIGH);
  
  currentDigit = (currentDigit + 1) % 4;
}

void handleButtonPress() {
  unsigned long currentMillis = millis();
  lastSettingChange = currentMillis;
  
  if (!settingMode) {
    // Вход в режим настройки
    settingMode = true;
    settingPosition = 0;
  } else {
    // В режиме настройки
    if (settingPosition == 0) {
      // Настройка часов
      hours = (hours + 1) % 24;
    } else {
      // Настройка минут
      minutes = (minutes + 1) % 60;
      seconds = 0;
    }
    
    updateDisplayData();
    
    // Переключение между настройкой часов и минут при длительном удержании
    if (currentMillis - lastSettingChange < 1000) {
      settingPosition = (settingPosition + 1) % 2;
    }
  }
}