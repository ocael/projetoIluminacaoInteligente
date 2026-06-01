#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pinos
const int pinoLDR = 34;
const int pinoLED = 2;
const int pinoBuzzer = 25;

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);

void conectarWiFi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
}

void conectarMQTT() {
  while (!client.connected()) {

    String clientId = "ESP32-LUZ-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT conectado!");
    } else {
      Serial.print("Falha MQTT: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(pinoLED, OUTPUT);
  pinMode(pinoBuzzer, OUTPUT);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha OLED");
    while (true);
  }

  display.clearDisplay();
  display.display();

  conectarWiFi();

  client.setServer(mqtt_server, 1883);

  Serial.println("=== Sistema de Iluminacao Inteligente ===");
}

void loop() {

  if (!client.connected()) {
    conectarMQTT();
  }

  client.loop();

  int valorLuz = analogRead(pinoLDR);

  // Converte para porcentagem (invertido)
  int porcentagem = map(valorLuz, 0, 4095, 100, 0);

  String estado;
  String ambiente;

  // Aciona LED e buzzer quando a luminosidade for 20% ou menor
  if (porcentagem <= 20) {

    digitalWrite(pinoLED, HIGH);

    tone(pinoBuzzer, 1000);

    estado = "Luz desligada";
    ambiente = "ESCURO";

  } else {

    digitalWrite(pinoLED, LOW);

    noTone(pinoBuzzer);

    estado = "Luz ligada";
    ambiente = "CLARO";
  }

  // Monitor Serial
  Serial.print("Luminosidade: ");
  Serial.print(porcentagem);
  Serial.print("% | Estado: ");
  Serial.println(estado);

  // MQTT
  String msgLuminosidade = "Luminosidade=" + String(porcentagem) + "%";
  String msgEstado = "Estado=" + estado;

  client.publish("iluminacao/valor", msgLuminosidade.c_str());
  client.publish("iluminacao/estado", msgEstado.c_str());

  // OLED
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.println("MONITOR DE LUZ");

  display.setCursor(0, 18);
  display.print("Luz: ");
  display.print(porcentagem);
  display.println("%");

  display.setCursor(0, 34);
  display.print("Ambiente:");

  display.setCursor(0, 46);
  display.println(ambiente);

  display.display();

  delay(1000);
}