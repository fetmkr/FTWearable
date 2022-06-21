#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

#include "EEPROM.h"
#define EEPROM_SIZE 1

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>


const uint16_t PixelCount = 24; // make sure to set this to the number of pixels in your strip
const uint16_t PixelPin = 4;  // make sure to set this to the correct pin, ignored for Esp8266


//NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma

NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);


int myID = 0;


char ssid[] = "Future_Theatre";          // your network SSID (name)
char pass[] = "FT201106";                    // your network password

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
const IPAddress outIp(192,168,0,150);        // remote IP (not needed for receive)
const unsigned int outPort = 9999;          // remote port (not needed for receive)
const unsigned int localPort = 8888;        // local port to listen for UDP packets (here's where we send the packets)


OSCErrorCode error;
unsigned int flashState = LOW;              // LOW means led is *on*

// Output
// GPIO4 LEDStrip1
// GPIO5 LEDStrip2
// GPIO19 LAT
// GPIO21 CLK
// GPIO22 SER
// GPIO23 Buzzer
// GPIO32 Relay flash

// Input
// GPIO34 Button Left
// GPIO36 Button Right

int ledstrip1pin = 4;
int ledstrip2pin = 5;
int segmentLatch = 19;
int segmentClock = 21;
int segmentData = 22;
int buzzerpin = 23;
int relayflashpin = 32;

int buttonleftpin = 34;
int buttonrightpin = 36;

boolean leftbuttonFlag = false;
boolean rightbuttonFlag = false;

boolean onairFlag = false;
boolean healthFlag = false;
boolean showDecimalPoint = false;

boolean onairLEDFlag = false;


int freq = 1000; 
int channel = 0; 
int resolution = 10;

unsigned long previousMillis = 0;        // will store last time LED was updated
//const long interval = 15;           // interval at which to blink (milliseconds)
const long interval = 50;           // interval at which to blink (milliseconds)
int animationIndex = 0;
boolean animationPolarity = true;

void setup() {

  pinMode(segmentClock, OUTPUT);
  pinMode(segmentData, OUTPUT);
  pinMode(segmentLatch, OUTPUT);

  digitalWrite(segmentClock, LOW);
  digitalWrite(segmentData, LOW);
  digitalWrite(segmentLatch, LOW);

  pinMode(buzzerpin, OUTPUT);
  digitalWrite(buzzerpin, 0);
  ledcSetup(channel, freq, resolution);  
  ledcAttachPin(buzzerpin, channel);
  
  pinMode(relayflashpin, OUTPUT);
  digitalWrite(relayflashpin, 0);   

  pinMode(buttonleftpin, INPUT_PULLUP);
  pinMode(buttonrightpin, INPUT_PULLUP);

  Serial.begin(115200);

   if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); 
    delay(1000000);
  }
  delay(500);

  // Read My ID
  myID = EEPROM.read(0);

  Serial.print("my ID from eeprom is: ");
  Serial.println(myID);
  
  showNumber(myID, false);
  ledcWriteTone(channel, 1200);
  delay(500);
  ledcWrite(channel, 0);
  delay(250); 
  
  showNumber(' ', false);
  ledcWriteTone(channel, 1200);
  delay(250);
  ledcWrite(channel, 0);
  delay(250); 

  showNumber(myID, false);
  ledcWriteTone(channel, 1200);
  delay(250);
  ledcWrite(channel, 0);
  delay(250); 
  
  showNumber(' ', false);
  ledcWriteTone(channel, 1200);
  delay(250);
  ledcWrite(channel, 0);
  delay(250);

  showNumber(myID, false);
  ledcWriteTone(channel, 1200);
  delay(250);
  ledcWrite(channel, 0);
  delay(250);

  // Set My ID

  while(digitalRead(buttonrightpin) == HIGH){
    if(digitalRead(buttonleftpin) == LOW )
    {
      if(leftbuttonFlag == false)
      {
        leftbuttonFlag = true;
        myID = myID+1;
        myID = myID % 6;
        //myID = myID+1;
        showNumber(myID, false);
        ledcWriteTone(channel, 880);
        delay(500);
        ledcWrite(channel, 0);
        delay(50);  
        Serial.println(myID);
      }
    }
    else {
      if(leftbuttonFlag == true ) leftbuttonFlag = false;
    }
  }

  showNumber(myID, false);
  ledcWriteTone(channel, 1200);
  EEPROM.write(0, myID);
  EEPROM.commit();
  delay(1000);
  Serial.print("my new ID ");
  Serial.print(myID);
  Serial.println(" is saved");

  ledcWrite(channel, 0);
  delay(250);  

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
#ifdef ESP32
  Serial.println(localPort);
#else
  Serial.println(Udp.localPort());
#endif

  strip.Begin();
  strip.Show();
   
}


void flash(OSCMessage &msg) {
  flashState = (int)msg.getFloat(0);
  // need get float as TouchOSC only sends floats
  digitalWrite(relayflashpin, flashState);
  
  Serial.print("/flash: ");
  Serial.println(flashState);
}

void health_check(OSCMessage &msg) {

  if(msg.getFloat(0) > 0.5f ){
    healthFlag = true;
    //showNumber(myID, false);
    showDecimalPoint = true;
    Serial.print("health check from ");
    Serial.print(Udp.remoteIP());
    Serial.print(": ");
    Serial.println(Udp.remotePort());
  }
}

void onair(OSCMessage &msg) {

  if(msg.getFloat(0) > 0.5f ){
    Serial.println("ID " + String(myID)+ " on air");
    onairFlag = true;
    onairLEDFlag = true;
  }
  else {
    Serial.println("ID " + String(myID)+ " off air");
    onairFlag = false;
    onairLEDFlag = true;
  }
}

void loop() {

  if(millis() - previousMillis > interval) {
    previousMillis = millis();
    //Serial.println("interval timer working");
    if(onairFlag)
    {
      if (animationIndex > 10) {
        animationIndex = 0;
        if(animationPolarity)animationPolarity = false; else animationPolarity = true;
      }

      if(animationPolarity){
      
      strip.SetPixelColor(animationIndex, RgbwColor(255,0,0,0));
      strip.SetPixelColor(animationIndex+1, RgbwColor(255,0,0,0));
      
      strip.SetPixelColor(23-animationIndex, RgbwColor(255,0,0,0));
      strip.SetPixelColor(22-animationIndex, RgbwColor(255,0,0,0));
      }
      else
      {
      strip.SetPixelColor(animationIndex, RgbwColor(0,0,0,0));
      strip.SetPixelColor(animationIndex+1, RgbwColor(0,0,0,0));
      
      strip.SetPixelColor(23-animationIndex, RgbwColor(0,0,0,0));
      strip.SetPixelColor(22-animationIndex, RgbwColor(0,0,0,0));
      }
      
      strip.Show();

      animationIndex++;

    }

    if(showDecimalPoint) {
      showDecimalPoint = false;
      showNumber(myID,true);

    }
    else {
      showNumber(myID,false);
    }
  }
  
  OSCBundle bundle;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) {
      bundle.fill(Udp.read());
    }
    if (!bundle.hasError()) {

      String flash_addr_str = "/p"+String(myID)+"/flash"; 
      const char * flash_addr = flash_addr_str.c_str();

      String health_addr_str = "/p"+String(myID)+"/ok"; 
      const char * health_addr = health_addr_str.c_str();

      String onair_addr_str = "/p"+String(myID)+"/onair"; 
      const char * onair_addr = onair_addr_str.c_str();

      bundle.dispatch(flash_addr, flash);
      bundle.dispatch(health_addr, health_check);
      bundle.dispatch(onair_addr, onair);

      
    } else {
      error = bundle.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }

  if (digitalRead(buttonleftpin) == LOW)
  {
    if(leftbuttonFlag == false)
    {
      leftbuttonFlag = true;

      Serial.println("left button pressed");
    }
    
  }
  else {
    if(leftbuttonFlag == true ) leftbuttonFlag = false;
  }

  if (digitalRead(buttonrightpin) == LOW)
  {
    if(rightbuttonFlag == false)
    {
      rightbuttonFlag = true;

      Serial.println("right button pressed");
    }
    
  }
  else {
    if(rightbuttonFlag == true ) rightbuttonFlag = false;
  }

  if(onairFlag) {

    if(onairLEDFlag){
      onairLEDFlag = false;
//      int i;
//      for (i = 0 ; i < PixelCount; i++) {
//        strip.SetPixelColor(i, RgbwColor(255,0,0,0));
//      }
//      strip.Show();
    }
  }
  else{
    if(onairLEDFlag)
    {
      onairLEDFlag = false;
      animationIndex = 0;
      animationPolarity = true;
      int i;
      for (i = 0 ; i < PixelCount; i++) {
        strip.SetPixelColor(i, RgbwColor(0));
      }
      strip.Show();
    }
  }

  if(healthFlag) 
  {
    healthFlag = false;

    String addr_str = "/p"+String(myID)+"/yes"; 
    const char * addr = addr_str.c_str();

    OSCMessage msg(addr);
    msg.add(1.0);
    Udp.beginPacket(Udp.remoteIP(),9999);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
  }

}


void showNumber(byte val, boolean decimal)
{
  postNumber(val, decimal);
  //Latch the current segment data
  digitalWrite(segmentLatch, LOW);
  digitalWrite(segmentLatch, HIGH); //Register moves storage register on the rising edge of RCK
}


//Given a number, or '-', shifts it out to the display
void postNumber(byte number, boolean decimal)
{
  //    -  A
  //   / / F/B
  //    -  G
  //   / / E/C
  //    -. D/DP

#define a  1<<0
#define b  1<<6
#define c  1<<5
#define d  1<<4
#define e  1<<3
#define f  1<<1
#define g  1<<2
#define dp 1<<7

  byte segments;

  switch (number)
  {
    case 1: segments = b | c; break;
    case 2: segments = a | b | d | e | g; break;
    case 3: segments = a | b | c | d | g; break;
    case 4: segments = f | g | b | c; break;
    case 5: segments = a | f | g | c | d; break;
    case 6: segments = a | f | g | e | c | d; break;
    case 7: segments = a | b | c; break;
    case 8: segments = a | b | c | d | e | f | g; break;
    case 9: segments = a | b | c | d | f | g; break;
    case 0: segments = a | b | c | d | e | f; break;
    case ' ': segments = 0; break;
    case 'c': segments = g | e | d; break;
    case '-': segments = g; break;
  }

  if (decimal) segments |= dp;

  //Clock these bits out to the drivers
  for (byte x = 0 ; x < 8 ; x++)
  {
    digitalWrite(segmentClock, LOW);
    digitalWrite(segmentData, segments & 1 << (7 - x));
    digitalWrite(segmentClock, HIGH); //Data transfers to the register on the rising edge of SRCK
  }
}
