#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "ENTER YOUR WIFI NAME";
const char* password = "ENTER YOUR WIFI PASSWORD";

// MQTT server details
const char* mqtt_server = "ENTER MQTT SERVER IP";
const int mqtt_port = 1883;
const char* mqtt_user = "ENTER MQTT USER";  // Leave as empty quotes if no username
const char* mqtt_password = "ENTER MQTT PASSWORD";  // Leave as empty quotes if no password
const char* client_id = "ESP8266_Mouse";  // MQTT client ID

// MQTT topic and message
const char* mqtt_topic = "home/mouse/set";
const char* mqtt_message = "1";

// Initialize WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Flag to track if MQTT message was sent successfully
bool messageSent = false;

// Function to connect to WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Disable WiFi persistence to reduce flash wear
  WiFi.persistent(false);
  
  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    // Don't try for more than 15 seconds
    if (millis() - startAttemptTime > 15000) {
      Serial.println("Failed to connect to WiFi, going to sleep...");
      goToSleep();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect to MQTT
bool connectMQTT() {
  Serial.print("Attempting MQTT connection...");
  
  // Set shorter timeout for MQTT connection attempts
  client.setSocketTimeout(5);
  
  // Connect with or without credentials based on whether they're provided
  bool connected = false;
  if (mqtt_user[0] != '\0') {
    connected = client.connect(client_id, mqtt_user, mqtt_password);
  } else {
    connected = client.connect(client_id);
  }
  
  if (connected) {
    Serial.println("connected");
    return true;
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
    return false;
  }
}

// Function to put the ESP8266 into indefinite deep sleep
void goToSleep() {
  Serial.println("Task complete, entering deep sleep until next power cycle");
  delay(100); // Brief delay to allow serial to complete
  
  // 0 = indefinite sleep, only wake on reset/power cycle
  ESP.deepSleep(0);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP8266 starting up");
  
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  
  // Try to connect to MQTT server (with timeout)
  unsigned long mqttStartTime = millis();
  bool mqttConnected = false;
  
  while (!mqttConnected && (millis() - mqttStartTime < 10000)) {
    mqttConnected = connectMQTT();
    if (!mqttConnected) {
      delay(1000);
    }
  }
  
  if (mqttConnected) {
    // Publish MQTT message
    Serial.print("Publishing message: ");
    Serial.print(mqtt_message);
    Serial.print(" to topic: ");
    Serial.println(mqtt_topic);
    
    if (client.publish(mqtt_topic, mqtt_message)) {
      Serial.println("Message published successfully");
      messageSent = true;
    } else {
      Serial.println("Failed to publish message");
    }
    
    // Give MQTT a moment to complete the transmission
    delay(200);
    
    // Disconnect from MQTT gracefully
    client.disconnect();
  } else {
    Serial.println("Could not connect to MQTT server");
  }
  
  // Disconnect WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // Go to deep sleep
  goToSleep();
}

void loop() {
  // This won't be used since we're going to deep sleep in setup()
  // But we'll add a failsafe just in case
  if (!messageSent) {
    if (!client.connected()) {
      connectMQTT();
    }
    
    if (client.connected() && client.publish(mqtt_topic, mqtt_message)) {
      Serial.println("Message published successfully in loop");
      messageSent = true;
      
      // Disconnect gracefully
      client.disconnect();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      
      // Go to deep sleep
      goToSleep();
    }
  } else {
    goToSleep();
  }
  
  delay(100);
}