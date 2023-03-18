#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

/*GPS*/
#include <SparkFun_I2C_GPS_Arduino_Library.h> //Use Library Manager or download here: https://github.com/sparkfun/SparkFun_I2C_GPS_Arduino_Library
I2CGPS myI2CGPS; //Hook object to the library

#include <TinyGPS++.h> //From: https://github.com/mikalhart/TinyGPSPlus
TinyGPSPlus gps; //Declare gps object

/*GPS variables*/
double lat;
double lng;
double location;

double month;
double day;
double year;

double hour;
double minute;
double second;



//Button
const int buttonPin = 2;
int buttonState; //state of button
unsigned long timePress = 0; //time pressed
unsigned long timePressLimit = 0; //limit to wait for Hold click
int clicks = 0; //number of clicks

//Password for internet
const char* ssid = "";
const char* password = "";

//the mqtt server
const char* mqtt_server = "mqtt.flespi.io";

//wifi client
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


// defines pins numbers for Distance sensor (HC-SR04) and Motor
int trigPin = 13;
int echoPin = 14;
int motorPin = 25; // in1 pin on Relay


long duration, cm, inches;

int currentState;     // the current reading from the input pin

void setup() {
  /*GPS*/
  Serial.begin(115200);
  Serial.println("GTOP Read Example");

  if (myI2CGPS.begin() == false)
  {
    Serial.println("Module failed to respond. Please check wiring.");
    while (1); //Freeze!
  }
  Serial.println("GPS module found!");
  /*GPS*/



  /* MQTT and button*/
  pinMode(2, INPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);


  //Serial Port begin
  Serial.begin (115200);

  Serial.println("Setup started");

  //Define inputs and outputs for Distance Sensor and Motor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(motorPin, OUTPUT);

}

/*Wifi connection Setting */
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client","T0jLbGxLz6LQVQPXDKFJNPIs17LM1DUKt3lvzG4ZBFDmmi9NQDkriSJ9PlJGOsh5","")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

 //displayInfo() function is used to print GPS data to the serial monitor.
void displayInfo()
{
  Serial.println();
  //first checks if the GPS data contains valid time information , prints the date and time information to the serial monitor 
  if (gps.time.isValid())
  {
    Serial.print(F("Date: "));
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());

    Serial.print((" Time: "));
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());

    Serial.println(); //Done printing time
  }
  //If the GPS data does not contain valid time information, the function prints the message "Time not yet valid" to the serial monitor.
  else
  {
    Serial.println(F("Time not yet valid"));
  }

  //If the location information is valid, the function prints the latitude and longitude information to the serial monitor with 6 decimal 
  if (gps.location.isValid())
  {
    Serial.print("Location: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(", "));
    Serial.print(gps.location.lng(), 6);
    Serial.println();
  }
  //If the location information is not yet valid, the function prints the message "Location not yet valid" to the serial monitor.
  else
  {
    Serial.println(F("Location not yet valid"));
  }
} /*GPS*/


void loop() {
  
  /*GPS*/
    while (myI2CGPS.available()) //available() returns the number of new bytes available from the GPS module
  {
    gps.encode(myI2CGPS.read()); //Feed the GPS parser
  }

  if (gps.time.isUpdated()) //Check to see if new GPS info is available
  {
    displayInfo();
  }
   /*GPS*/
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.

  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:   

  digitalWrite(trigPin, LOW);

  delayMicroseconds(10);

  digitalWrite(trigPin, HIGH);

  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  // Read the signal from the sensor: a HIGH pulse whose

  // duration is the time (in microseconds) from the sending

  // of the ping to the reception of its echo off of an object.

  pinMode(echoPin, INPUT);

  duration = pulseIn(echoPin, HIGH);

  // Convert the time into a distance

  cm = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343

  inches = (duration/2) / 74;   // Divide by 74 or multiply by 0.0135

 
  delay(100);

// switch
  
/*Motor logic*/
    if (cm <= 100){
      digitalWrite(motorPin, LOW);
    } else if (cm >= 101){
      digitalWrite(motorPin, HIGH);
    }

/*all the buttom Logic*/
buttonState = digitalRead(buttonPin); //informs buttonstate of buttonpin
  
  if (buttonState == HIGH){
    delay(200);
    
    //detects clicks
    if (clicks == 0) {
    timePress = millis();
    timePressLimit = timePress + 500;    
    clicks = 1;
    }

    //hold press
    else if (clicks == 1 && millis() < timePressLimit){


    //find location
    lat = gps.location.lat();
    lng = gps.location.lng();

    month = gps.date.month();
    day = gps.date.day();
    year = gps.date.year();

    hour = gps.time.hour();
    minute = gps.time.minute();
    second = gps.time.second();
    

     String locationStr = "";
     String latStr = "";
     String lngStr = "";
     String monthStr = "";
     String dayStr = "";
     String yearStr = "";
     String hourStr = "";
     String minuteStr = "";
     String secondStr = "";

     latStr = String(lat, 10);
     lngStr = String(lng, 10);
     monthStr = String(month, 0);
     dayStr = String(day, 0);
     yearStr = String(year, 0);
     hourStr = String(hour, 0);
     minuteStr = String(minute, 0);
     secondStr = String(second, 0);     
     locationStr = "Big danger in area, " + latStr + " " + lngStr + ", " + dayStr + "/" + monthStr + "/" + yearStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;
     int len = locationStr.length();
     char char_array[len];
     locationStr.toCharArray(char_array, len);
     Serial.println(locationStr);     

      //hold Press Action
      Serial.println("Button Pressed twice");
      client.publish("BlindData/warningLocation", char_array);
      //client.publish("BlindData/location","Location");
     
     //set variables back to 0
      timePress = 0;
      timePressLimit = 0;
      clicks = 0;
      delay(3000);      
    }    
  }

  //single press
  if (clicks == 1 && timePressLimit != 0 && millis() > timePressLimit){

    //find location
    lat = gps.location.lat();
    lng = gps.location.lng();
     //find location
    lat = gps.location.lat();
    lng = gps.location.lng();

    month = gps.date.month();
    day = gps.date.day();
    year = gps.date.year();

    hour = gps.time.hour();
    minute = gps.time.minute();
    second = gps.time.second();
    

     String locationStr = "";
     String latStr = "";
     String lngStr = "";
     String monthStr = "";
     String dayStr = "";
     String yearStr = "";
     String hourStr = "";
     String minuteStr = "";
     String secondStr = "";

     latStr = String(lat, 10);
     lngStr = String(lng, 10);
     monthStr = String(month, 0);
     dayStr = String(day, 0);
     yearStr = String(year, 0);
     hourStr = String(hour, 0);
     minuteStr = String(minute, 0);
     secondStr = String(second, 0);     
     locationStr = "Small danger in area, " + latStr + " " + lngStr + ", " + dayStr + "/" + monthStr + "/" + yearStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;
     int len = locationStr.length();
     char char_array[len];
     locationStr.toCharArray(char_array, len);
     Serial.println(locationStr);
     
     //Single Press Action
      Serial.println("Button Pressed Once");
     client.publish("BlindData/warningLocation", char_array);
     //client.publish("BlindData/location",);

     //set variables back to 0
     timePress = 0;
     timePressLimit = 0;
     clicks = 0;
     delay(3000);
  }


if (!client.connected()) {
    reconnect();
  }
  client.loop();
}