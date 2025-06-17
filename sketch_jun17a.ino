#include <WiFi.h>
#include <PubSubClient.h>       // Bibliothèque MQTT
#include <LiquidCrystal.h>
#include <DFRobot_MAX30102.h>

// LCD
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);

// Capteur
DFRobot_MAX30102 particleSensor;
int32_t SPO2;
int8_t SPO2Valid;
int32_t heartRate;
int8_t heartRateValid;

// Buzzer
const int buzzerPin = 26;

// WiFi
const char* ssid = "alpha diallo";
const char* password = "diallo1998";

// MQTT Broker
const char* mqtt_server = "broker.hivemq.com";    // Adresse IP ou domaine du broker MQTT
const int mqtt_port = 1883;                   // Port MQTT par défaut

WiFiClient espClient;
PubSubClient client(espClient);

// Fonction pour se connecter au broker MQTT sans auth
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connexion au broker MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(WiFi.macAddress());

    if (client.connect(clientId.c_str())) {
      Serial.println("connecté");
    } else {
      Serial.print("Erreur de connexion, rc=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.print("Initialisation");
  delay(1000);

  while (!particleSensor.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erreur capteur");
    delay(1000);
  }

  particleSensor.sensorConfiguration(50, SAMPLEAVG_4, MODE_MULTILED, SAMPLERATE_100, PULSEWIDTH_411, ADCRANGE_16384);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connexion WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connecté");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  particleSensor.heartrateAndOxygenSaturation(&SPO2, &SPO2Valid, &heartRate, &heartRateValid);

  lcd.clear();
  lcd.setCursor(0, 0);
  if (heartRateValid) {
    lcd.print("FC:");
    lcd.print(heartRate);
    lcd.print(" bpm");

    if (heartRate > 110 || heartRate < 50 ) {
      digitalWrite(buzzerPin, HIGH);
      delay(500);
      digitalWrite(buzzerPin, LOW);
      delay(100);
    }
  } else {
    lcd.print("NON DETECTE !");
    String payload = "{";
    payload += "\"hr\":" + String("null") + ",";
    payload += "\"spo2\":" + String("null");
    payload += "}";
    client.publish("sante/oxymetre", payload.c_str());
    Serial.println("Publié MQTT : " + payload);
  }

  lcd.setCursor(0, 1);
  if (SPO2Valid) {
    lcd.print("SPO2:");
    lcd.print(SPO2);
    lcd.print("%");
  } else {
    lcd.print("SPO2:___");
    String payload = "{";
    payload += "\"hr\":" + String("null") + ",";
    payload += "\"spo2\":" + String("null");
    payload += "}";
    client.publish("sante/oxymetre", payload.c_str());
    Serial.println("Publié MQTT : " + payload);
  }

  if (heartRateValid && SPO2Valid) {
    String payload = "{";
    payload += "\"hr\":" + String(heartRate) + ",";
    payload += "\"spo2\":" + String(SPO2);
    payload += "}";
    client.publish("sante/oxymetre", payload.c_str());
    Serial.println("Publié MQTT : " + payload);
  }

  delay(2000);
}
