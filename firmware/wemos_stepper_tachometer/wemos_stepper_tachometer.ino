/*
 * Wemos D1 mini / NodeMCU ESP8266 face-lock tachometer.
 *
 * Receives an absolute angle over MQTT and moves a 28BYJ-48 stepper through
 * an ULN2003 driver. The Python face-lock client publishes 0..180 degrees:
 * left = 0, center = 90, right = 180.
 *
 * Install from Arduino IDE Library Manager:
 *   - PubSubClient by Nick O'Leary
 *   - AccelStepper by Mike McCauley
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <AccelStepper.h>
#include <math.h>

// Set these before uploading. Do not commit real credentials.
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Use the same broker as the PC. Replace with a LAN broker IP if preferred.
const char* MQTT_BROKER = "broker.emqx.io";
const uint16_t MQTT_PORT = 1883;

const char* TOPIC_SUB_ANGLE = "tachometer/angle";
const char* TOPIC_PUB_STATUS = "tachometer/status";

// Wemos D1 mini / NodeMCU pin labels:
//   D1 / GPIO5  -> ULN2003 IN1
//   D2 / GPIO4  -> ULN2003 IN2
//   D5 / GPIO14 -> ULN2003 IN3
//   D6 / GPIO12 -> ULN2003 IN4
#define IN1 5
#define IN2 4
#define IN3 14
#define IN4 12

// Tune this to the actual mechanical sweep of the installed tachometer.
const long TOTAL_STEPS_180_DEG = 1024;
const bool INVERT_DIRECTION = false;

AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

long targetStep = TOTAL_STEPS_180_DEG / 2;
int lastReceivedAngle = 90;
unsigned long nextWifiAttempt = 0;
unsigned long nextMqttAttempt = 0;
unsigned long lastStatusTime = 0;

long angleToSteps(float angleDeg) {
  if (angleDeg < 0.0f) angleDeg = 0.0f;
  if (angleDeg > 180.0f) angleDeg = 180.0f;
  if (INVERT_DIRECTION) angleDeg = 180.0f - angleDeg;
  return lround((angleDeg / 180.0f) * TOTAL_STEPS_180_DEG);
}

void requestAngle(float angleDeg) {
  targetStep = angleToSteps(angleDeg);
  lastReceivedAngle = (int)lround(angleDeg);
  stepper.moveTo(targetStep);

  Serial.print("[MQTT] angle=");
  Serial.print(angleDeg);
  Serial.print(" target_step=");
  Serial.println(targetStep);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_SUB_ANGLE) != 0) return;

  char message[24];
  if (length >= sizeof(message)) length = sizeof(message) - 1;
  memcpy(message, payload, length);
  message[length] = '\0';

  char* end = nullptr;
  float angle = strtof(message, &end);
  if (end == message || *end != '\0' || isnan(angle) || isinf(angle)) {
    Serial.print("[MQTT] invalid angle: ");
    Serial.println(message);
    return;
  }

  requestAngle(angle);
}

void beginWifiAttempt() {
  if (millis() < nextWifiAttempt) return;
  nextWifiAttempt = millis() + 10000;

  Serial.print("[WiFi] connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  beginWifiAttempt();
}

void ensureMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return;
  if (millis() < nextMqttAttempt) return;
  nextMqttAttempt = millis() + 3000;

  String clientId = "Wemos-Face-Tachometer-" + String(ESP.getChipId(), HEX);
  Serial.print("[MQTT] connecting to ");
  Serial.println(MQTT_BROKER);

  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("[MQTT] connected");
    mqttClient.subscribe(TOPIC_SUB_ANGLE);
    mqttClient.publish(TOPIC_PUB_STATUS, "Wemos ESP8266 tachometer online");
  } else {
    Serial.print("[MQTT] failed, state=");
    Serial.println(mqttClient.state());
  }
}

void publishStatus() {
  if (!mqttClient.connected()) return;
  if (millis() - lastStatusTime < 5000) return;
  lastStatusTime = millis();

  char status[80];
  snprintf(
      status,
      sizeof(status),
      "angle=%d step=%ld/%ld",
      lastReceivedAngle,
      stepper.currentPosition(),
      TOTAL_STEPS_180_DEG);
  mqttClient.publish(TOPIC_PUB_STATUS, status);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Wemos ESP8266 MQTT tachometer");

  stepper.setMaxSpeed(1200.0);
  stepper.setAcceleration(2500.0);

  // The system has no limit switch. Start with the pointer physically at 90°.
  stepper.setCurrentPosition(TOTAL_STEPS_180_DEG / 2);
  stepper.moveTo(TOTAL_STEPS_180_DEG / 2);

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  ensureWifi();
}

void loop() {
  ensureWifi();
  ensureMqtt();

  if (mqttClient.connected()) mqttClient.loop();
  stepper.run();
  publishStatus();
}
