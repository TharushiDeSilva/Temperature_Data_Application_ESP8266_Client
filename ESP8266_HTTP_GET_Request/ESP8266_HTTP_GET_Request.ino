#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#include "DHT.h"
#include "TimeLib.h"
#include "WiFiUdp.h"

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "THARUSHI_DE_SILVA";
const char* password = "12345678";
const int sensorID = 1;

static const char ntpServerName[] = "us.pool.ntp.org";    //the NTP server name
const double timeZone = 5.50;                             //the timeZone value for Sri Lanka.

float temperature;
float humidity;
float heatIndex;
String DATE;
String TIME;

//parameters related to the DateTimeStamp;
WiFiUDP udp;
unsigned int localPort = 8888;
time_t getNtpTime();
void sendNtpPacket(IPAddress &address);
String getReadings();
String createDateString();
String createTimeString();
String formatDigits(int digit);

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
    //http.begin("http://jsonplaceholder.typicode.com/users/1");
    String query = getReadings();
    //http.begin("http://10.17.62.12:8080/temperature-data-application/weather-data?sensorID=1");
    http.begin(query);
    int httpCode = http.GET();
    Serial.println(httpCode);
    if(httpCode>0){
      String payload = http.getString();
      Serial.println(payload);
    }
    http.end(); 
  }
  delay(5000);

}

time_t prevDisplay= 0;

String getReadings(){
  String webString = "http://10.17.62.12:8080/temperature-data-application/weather-data?";
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

    //prepare the get request query
    webString += "sensorID=";
    webString += sensorID;
    webString += "&date=";
    webString += DATE;
    webString += "&time=";
    webString += TIME;
    webString += "&temperature=";
    webString += temperature;
    webString += "&humidity=";
    webString += humidity;
    webString += "&heatIndex=";
    webString += heatIndex;

    return webString;
   }
  }
}


String createDateString() {
  String dateString = "";
  dateString += formatDigits(day());
  dateString += "/";
  dateString += formatDigits(month());
  dateString += "/";
  dateString += year();
  return dateString;
  }
  
String createTimeString() {
  String timeString = "";
  timeString += formatDigits(hour());
  timeString += ":";
  timeString += formatDigits(minute());
  timeString += ":";
  timeString += formatDigits(second());
  return timeString;
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
  
