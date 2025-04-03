#include <WiFi.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

const char* ssid = "Ваш_SSID";
const char* password = "Ваш_Пароль";

WiFiServer server(80);

String receivedData = "";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi підключено!");
  Serial.print("IP-адреса: ");
  Serial.println(WiFi.localIP());

  server.begin();
  radio.begin();
  radio.openWritingPipe(address);
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  // Прийом даних від RF-Nano
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    receivedData = String(text);
    Serial.println("Отримано: " + receivedData);
  }

  // Веб-інтерфейс для керування реле
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String request = client.readStringUntil('\r');
        client.flush();

        if (request.indexOf("/ON") != -1) {
          radio.stopListening();
          const char command[] = "ON";
          radio.write(&command, sizeof(command));
          radio.startListening();
          Serial.println("Команда: Увімкнути реле");
        } else if (request.indexOf("/OFF") != -1) {
          radio.stopListening();
          const char command[] = "OFF";
          radio.write(&command, sizeof(command));
          radio.startListening();
          Serial.println("Команда: Вимкнути реле");
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        client.println("<!DOCTYPE HTML>");
        client.println("<html>");
        client.println("<h1>Керування реле</h1>");
        client.println("<p>Отримані дані: " + receivedData + "</p>");
        client.println("<a href=\"/ON\">Увімкнути</a><br>");
        client.println("<a href=\"/OFF\">Вимкнути</a>");
        client.println("</html>");
        break;
      }
    }
    client.stop();
  }
}
