#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <DHT.h>

#define CE_PIN 7
#define CSN_PIN 8
#define DHTPIN 2
#define DHTTYPE DHT11
#define RELAY_PIN 3  // Пін реле

DHT dht(DHTPIN, DHTTYPE);
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Реле вимкнене за замовчуванням

  radio.begin();
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  // Читання даних з датчика
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Помилка датчика!");
    return;
  }

  // Логіка керування реле за показниками вологості
  if (humidity < 50) {
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Реле увімкнено: низька вологість");
  } else if (humidity > 75) {
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Реле вимкнено: висока вологість");
  }

  // Відправка даних до ESP32
  if (radio.available()) {
    String data = String(temperature) + "," + String(humidity);
    radio.write(data.c_str(), data.length());
  }

  // Прийом команди від ESP32
  if (radio.available()) {
    char command[32] = "";
    radio.read(&command, sizeof(command));
    if (String(command) == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Реле увімкнено за командою ESP32");
    } else if (String(command) == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Реле вимкнено за командою ESP32");
    }
  }
}
