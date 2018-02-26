#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi.h>
#include <WifiIPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>

// your network name also called SSID
char ssid[] = "MerahPutih";
// your network password
char password[] = "chipsahoy";

// IBM IoT Foundation Cloud Settings
// When adding a device on internetofthings.ibmcloud.com the following
// information will be generated:
// org=<org>
// type=iotsample-ti-energia
// id=<mac>
// auth-method=token
// auth-token=<password>

#define MQTT_MAX_PACKET_SIZE 100
#define IBMSERVERURLLEN  64
#define IBMIOTFSERVERSUFFIX "messaging.internetofthings.ibmcloud.com"

//char organization[] = "zbog8o";
char organization[] = "y6badt";
char typeId[] = "CC3200";
char pubtopic[] = "iot-2/evt/status/fmt/json";
char subTopic[] = "iot-2/cmd/+/fmt/json";
char deviceId[] = "d4f51303f075";
char clientId[64];

char mqttAddr[IBMSERVERURLLEN];
int mqttPort = 1883;

// Authentication method. Should be use-toke-auth
// When using authenticated mode
char authMethod[] = "use-token-auth";
// The auth-token from the information above
char authToken[] = "WYrBJSh40P9ZW*SPy6";

MACAddress mac;

// getTemp() function for cc3200
#ifdef TARGET_IS_CC3101
#include <Wire.h>
#include "Adafruit_TMP006.h"
Adafruit_TMP006 tmp006(0x41);
#endif
  
WifiIPStack ipstack;  
MQTT::Client<WifiIPStack, Countdown, MQTT_MAX_PACKET_SIZE> client(ipstack);

int ledPin = RED_LED;

//getOccupancy - PIR sensor
int pirPIN = 4;
int val =0;
String roomstatus = "";



 

// The function to call when a message arrives
void callback(char* topic, byte* payload, unsigned int length);
void messageArrived(MQTT::MessageData& md);

void setup() {
  //PIR pin
  pinMode(pirPIN, INPUT); // declare pir as input
 
  
  uint8_t macOctets[6];
  
  Serial.begin(115200);
  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to Network named: ");
  // print the network name (SSID);
  Serial.println(ssid); 
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED) {
    // print dots while we wait to connect
    Serial.print(".");
    delay(300);
  }
  
  Serial.println("\nYou're connected to the network");
  Serial.println("Waiting for an ip address");
  
  while (WiFi.localIP() == INADDR_NONE) {
    // print dots while we wait for an ip addresss
    Serial.print(".");
    delay(300);
  }

  // We are connected and have an IP address.
  Serial.print("\nIP Address obtained: ");
  Serial.println(WiFi.localIP());

  mac = WiFi.macAddress(macOctets);
  Serial.print("MAC Address: ");
  Serial.println(mac);
  
  // Use MAC Address as deviceId
  sprintf(deviceId, "%02x%02x%02x%02x%02x%02x", macOctets[0], macOctets[1], macOctets[2], macOctets[3], macOctets[4], macOctets[5]);
  Serial.print("deviceId: ");
  Serial.println(deviceId);

  sprintf(clientId, "d:%s:%s:%s", organization, typeId, deviceId);
  sprintf(mqttAddr, "%s.%s", organization, IBMIOTFSERVERSUFFIX);

  #ifdef __CC3200R1M1RGC__
  if (!tmp006.begin()) {
    Serial.println("No sensor found");
    while (1);
  }
  #endif
  


}

void loop() {

  int rc = -1;
  if (!client.isConnected()) {
    Serial.print("Connecting to ");
    Serial.print(mqttAddr);
    Serial.print(":");
    Serial.println(mqttPort);
    Serial.print("With client id: ");
    Serial.println(clientId);
    
    while (rc != 0) {
      rc = ipstack.connect(mqttAddr, mqttPort);
    }

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    connectData.MQTTVersion = 3;
    connectData.clientID.cstring = clientId;
    connectData.username.cstring = authMethod;
    connectData.password.cstring = authToken;
    connectData.keepAliveInterval = 10;
    
    rc = -1;
    while ((rc = client.connect(connectData)) != 0)
      ;
    Serial.println("Connected\n");
    
    Serial.print("Subscribing to topic: ");
    Serial.println(subTopic);
    
    // Unsubscribe the topic, if it had subscribed it before.
    client.unsubscribe(subTopic);
    // Try to subscribe for commands
    if ((rc = client.subscribe(subTopic, MQTT::QOS0, messageArrived)) != 0) {
      Serial.print("Subscribe failed with return code : ");
      Serial.println(rc);
    } else {
      Serial.println("Subscribe success\n");
    }
  }


  char mqttjson[100];
  mqttjson = jsonMsg();


  Serial.print("Publishing: ");
  Serial.println(mqttjson);
  MQTT::Message message;
  message.qos = MQTT::QOS0; 
  message.retained = false;
  message.payload = mqttjson; 
  message.payloadlen = strlen(mqttjson);
  rc = client.publish(pubtopic, message);
  if (rc != 0) {
    Serial.print("Message publish failed with return code : ");
    Serial.println(rc);
  }
  
  // Wait for one second before publishing again
  // This will also service any incoming messages
  client.yield(4000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message has arrived");
  
  char * msg = (char *)malloc(length * sizeof(char));
  int count = 0;
  for(count = 0 ; count < length ; count++) {
    msg[count] = payload[count];
  }
  msg[count] = '\0';
  Serial.println(msg);
  
  if(length > 0) {
    digitalWrite(ledPin, HIGH);
    delay(1000);
    digitalWrite(ledPin, LOW);  
  }

  free(msg);
}

void messageArrived(MQTT::MessageData& md) {
  Serial.print("Message Received\t");
    MQTT::Message &message = md.message;
    int topicLen = strlen(md.topicName.lenstring.data) + 1;
//    char* topic = new char[topicLen];
    char * topic = (char *)malloc(topicLen * sizeof(char));
    topic = md.topicName.lenstring.data;
    topic[topicLen] = '\0';
    
    int payloadLen = message.payloadlen + 1;
//    char* payload = new char[payloadLen];
    char * payload = (char*)message.payload;
    payload[payloadLen] = '\0';
    
    String topicStr = topic;
    String payloadStr = payload;
    
    //Command topic: iot-2/cmd/blink/fmt/json

    if(strstr(topic, "/cmd/blink") != NULL) {
      Serial.print("Command IS Supported : ");
      Serial.print(payload);
      Serial.println("\t.....");
      
      pinMode(ledPin, OUTPUT);
      
      //Blink twice
      for(int i = 0 ; i < 2 ; i++ ) {
        digitalWrite(ledPin, HIGH);
        delay(250);
        digitalWrite(ledPin, LOW);
        delay(250);
      }
    } else {
      Serial.println("Command Not Supported:");            
    }
}

// getTemp() function for MSP430F5529
#if defined(__MSP430F5529)
// Temperature Sensor Calibration-30 C
#define CALADC12_15V_30C  *((unsigned int *)0x1A1A)
// Temperature Sensor Calibration-85 C
#define CALADC12_15V_85C  *((unsigned int *)0x1A1C)

double getTemp() {
 return (float)(((long)analogRead(TEMPSENSOR) - CALADC12_15V_30C) * (85 - 30)) /
        (CALADC12_15V_85C - CALADC12_15V_30C) + 30.0f;
}

// getTemp() function for Stellaris and TivaC LaunchPad
#elif defined(TARGET_IS_SNOWFLAKE_RA0) || defined(TARGET_IS_BLIZZARD_RB1)

double getTemp() {
  return (float)(147.5 - ((75 * 3.3 * (long)analogRead(TEMPSENSOR)) / 4096));
}

// getTemp() function for cc3200
#elif defined(__CC3200R1M1RGC__)
double getTemp() {
  return (double)tmp006.readObjTempC();
}
#else
double getTemp() {
  return 21.05;
}
#endif

//get room Occupancy - PIR sensor 
String getOccupancy(){
  val = digitalRead(pirPIN); // read input value
  if (val == HIGH){
    roomstatus = "OCCUPIED";
    Serial.println("room status: occupied");
  }
  else{
    roomstatus = "UNOCCUPIED";
    Serial.println("room status: occupied");
  }
  
  return roomstatus;
}

//Construct JSON
char jsonMsg(){
  
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    char jsonchar[200];
  
    root["d"] = "d";
    JsonObject& values = root.createNestedObject();
    values["myName"] = "TILaunchPad";
    values["temperature"] = "25";
    values["occupancy"] = "occupied";
    
    root.printTo(jsonchar);
    
    return jsonchar;
    
}
