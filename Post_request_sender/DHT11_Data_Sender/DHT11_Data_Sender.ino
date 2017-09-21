#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#include "DHT.h"
#include "TimeLib.h"
#include "WiFiUdp.h"
#include "aJSON.h"
#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "THARUSHI_DE_SILVA";
const char* password = "12345678";
const char* sensorID = "1";

static const char ntpServerName[] = "us.pool.ntp.org";    //the NTP server name
const double timeZone = 5.50;                             //the timeZone value for Sri Lanka.
const char* serverEndpoint = "http://10.17.62.12:8080/temperature-data-application/weather-data";
double temperature;
double humidity;
double heatIndex;
String DATE;
String TIME;

char date_array[11];
char time_array[9];
char temperature_array[6];
char humidity_array[6];
char heatIndex_array[6];

//parameters related to the DateTimeStamp;
WiFiUDP udp;
unsigned int localPort = 8888;
time_t getNtpTime();
void sendNtpPacket(IPAddress &address);
void getReadings();
String createDateString();
String createTimeString();
String formatDigits(int digit);
void convertNumbersToString();
void setup() {
  
  Serial.begin(9600);
  delay(2000);  //delay of 2 seconds
  dht.begin();
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting");
  }
  //Ringing the Buzzer
  tone(5, 1000);      //Buzzer pin and the frequency, buzzer activated
  delay(1500);        //Buzzer on for 1000ms. 
  noTone(5);          // not ringing
  
  Serial.print("Connected to WiFi network: ");
  Serial.println(ssid);
  Serial.println("-----------------------------------------------------");

  udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  Serial.println("Date Time Server Connection activated............................");
}



void loop() {
  if (WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    getReadings();
    http.begin(serverEndpoint);
    http.addHeader("content-type", "text/plain");
    convertObjectsToChar();
    aJsonObject *root;
    root = aJson.createObject();
    aJson.addStringToObject(root, "sensorID", sensorID);
    aJson.addStringToObject(root, "date", date_array);
    aJson.addStringToObject(root, "time", time_array);
    aJson.addStringToObject(root, "temperature", temperature_array);
    aJson.addStringToObject(root, "humidity", humidity_array);
    aJson.addStringToObject(root, "heatIndex", heatIndex_array);
    char *payload = aJson.print(root);
    int httpCode = http.POST((uint8_t *)payload, strlen(payload));
    Serial.println(httpCode);
    Serial.println(payload);

    http.end(); 
  }else{
    Serial.println("Error in WiFi connection!");
    }
  delay(10000);

}

time_t prevDisplay= 0;

void getReadings(){
  if(timeStatus() != timeNotSet){
    if(now() != prevDisplay) {        //update the display only if time has changed.
      prevDisplay = now();
      DATE = createDateString();
      TIME = createTimeString();
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
  
      if(isnan(humidity)|| isnan(temperature)){
        Serial.println("Failed to read from DHT Sensor");
        //return;
      }
      heatIndex = dht.computeHeatIndex(temperature, humidity, false);
   }
  }
}


String createDateString() {
  String dateString = "";
  dateString += formatDigits(day());
  dateString += "-";
  dateString += formatDigits(month());
  dateString += "-";
  dateString += year();
  return dateString;
  }
  
String createTimeString() {
  String timeString = "";
  timeString += formatDigits(hour());
  timeString += "-";
  timeString += formatDigits(minute());
  timeString += "-";
  timeString += formatDigits(second());
  return timeString;
  }

void convertObjectsToChar() {
  String temp_string = String(temperature);
  String humd_string = String(humidity);
  String heatIndex_string = String(heatIndex);
  
  DATE.toCharArray(date_array, 11);
  TIME.toCharArray(time_array, 9);
  temp_string.toCharArray(temperature_array, 6);
  humd_string.toCharArray(humidity_array, 6);
  heatIndex_string.toCharArray(heatIndex_array, 6);
  }

String formatDigits(int digit){
  String formatString = "";
  if (digit<10){
    formatString += "0";
    }
  formatString += digit;
  return formatString;
  }
  
//---------- NTP code -------------------------------------
const int NTP_PACKET_SIZE = 48;     // NTP time is the 1st 38 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets. 
time_t getNtpTime() {
  IPAddress ntpServerIP;    //NTP server's IP address.
  while(udp.parsePacket() > 0);   //discarding any previously received packets. 
    WiFi.hostByName(ntpServerName, ntpServerIP);
    sendNTPpacket(ntpServerIP);
    uint32_t beginWait = millis();
    while(millis() - beginWait <1500){
      int size = udp.parsePacket();
      if(size>= NTP_PACKET_SIZE){
        udp.read(packetBuffer, NTP_PACKET_SIZE);
        unsigned long secsSince1900;
        //convert four bytes starting at location 40 to a long integer.
        secsSince1900 = (unsigned long)packetBuffer[40] <<24;
        secsSince1900 |= (unsigned long)packetBuffer[41] <<16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
        }
      }
      Serial.println("No NTP response");
      return 0;       //return 0 if unable to get the time. 
}

//send an NTP request tp the time server at the given address. 
void sendNTPpacket(IPAddress &address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);   //set all the bytes in the buffer to 0
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;        //Stratum or type of clock
  packetBuffer[2] = 6;    //polling interval
  packetBuffer[3] = 0xEC; //peer clock precision. 

  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  }
  
