
#include <ESP8266WiFi.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

//wifi setting
const char* ssid = "openspot";
const char* password = "1234567890";

//ifttt setting
const char iftttkey[] = "zzzz";
const char iftttevent[] = "test";
const char url_server[] = "maker.ifttt.com";

//dweet setting
const char dweet_server[] = "dweet.io";
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response
const String httpPost =   "GET /get/latest/dweet/for/lecherngtest HTTP/1.1\r\n"
                          "Host: dweet.io\r\n"
                          "User-Agent: Arduino/1.0\r\n"
                          "Accept: */*\r\n"
                          "\r\n";
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server


//ntp setting
unsigned int localPort = 2390;
IPAddress timeServerIP;
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE =48;
byte packetBuffer[NTP_PACKET_SIZE];

//UDP
WiFiUDP udp;

struct dweet_resp{
  char thing[128];
  char created[128];
  char content1[64];
};

struct dtime{
  //extra char for null termination
  char tYear[5];
  char tMonth[3];
  char tDay[3];
  char tHour[3];
  char tMin[3];
  char tSec[3];
};

//systime
unsigned long datebootup = 0;
unsigned long timebootup = 0;
unsigned long last_response = 0;


struct ontime{
  char date[11];
  char tm[12];
};

//gpio setting
int ledPin = 16; // GPIO13

//http server setup.
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);
 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

 
  // Connect to WiFi network
  Serial.println();
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
 
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP);
  delay(1000);
  while(true){
    int cb = udp.parsePacket();
    if (!cb){
      Serial.println("no packet yet");
      delay(1000);
    }
    else{
      //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/NTPClient/NTPClient.ino
      
      Serial.println("packet received, length= ");
      Serial.println(cb);
      udp.read(packetBuffer, NTP_PACKET_SIZE);
  
      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:
      
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      Serial.print("Seconds since Jan 1 1900 = " );
      Serial.println(secsSince1900);
  
      // now convert NTP time into everyday time:
      Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      // print Unix time:
      Serial.println(epoch);

  
      // print the hour, minute and second:
      datebootup = epoch % 1314000L;           // print the date (1314000 is equals one whole year)
      timebootup = datebootup % 86400L;       // print the hour (86400 equals secs per day)
      Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
      Serial.print(timebootup / 3600);
      Serial.print(':');
      if ( ((epoch % 3600) / 60) < 10 ) {
        // In the first 10 minutes of each hour, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
      Serial.print(':');
      if ( (epoch % 60) < 10 ) {
        // In the first 10 seconds of each minute, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.println(epoch % 60); // print the second
      break;
    } 
  }

}
 
void loop() {
  delay(10000);
  digitalWrite(ledPin, HIGH);
  
  Serial.print("connecting to ");
  Serial.println(dweet_server);

  WiFiClient client;
  if (!client.connect(dweet_server, 80)){
    Serial.println("connection failed");
    return;
  }

  client.print(httpPost);
  
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);
  
  if (!ok) {
    Serial.println("No response or invalid response!");
  }
  
  delay(10);
  
  char response[MAX_CONTENT_SIZE];
  size_t length = client.readBytes(response, sizeof(response));
  response[length] = 0;

  bool trimmed = false;
  char response_with[256];
  int i=0;

  for (int j = 0; j < sizeof(response); j++){
    if (response[j] == char('[')){
      trimmed = true;
      continue;
    }
    if (response[j] == char(']')){
      for (i ; i < sizeof(response_with); i++){
        response_with[i] = 0;
      }
      break;
    }
    if (trimmed){
      response_with[i] = response[j];
      i++;
      //Serial.print(response[j]);
    }
  }
  //Serial.println(response_with);

  //only process the trimmed responses,
  dweet_resp dweet_resp;
  dtime dtime;
  if (parseDweetRespWith(response_with, &dweet_resp)){
    //String created(dweet_resp.created);
    //if (created != last_response){
    constructtime(&dweet_resp, &dtime);
    unsigned long current_response = ((atoi(dtime.tHour) * 3600) +
                                      (atoi(dtime.tMin) * 60) +
                                      (atoi(dtime.tSec)));
    if ((current_response > timebootup) && (current_response > last_response)){
      last_response = current_response;
      Serial.println("Get new response from Dweet");
      printDweetResp(&dweet_resp);
      Serial.println("HIGH LED");
      digitalWrite(ledPin, LOW);
    }
    else{
      Serial.println("No new response from Dweet");
    }
  }

  Serial.println();
  Serial.println("closing connection");
}

bool parseDweetRespWith(char* content, struct dweet_resp* dweet_resp) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  
  const size_t BUFFER_SIZE =
      JSON_OBJECT_SIZE(3) +    // root object has 3 elements
      JSON_OBJECT_SIZE(1);     // content object has 1 element

    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(content);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  // Here were copy the strings we're interested in
  strcpy(dweet_resp->thing, root["thing"]);
  strcpy(dweet_resp->created, root["created"]);
  strcpy(dweet_resp->content1, root["content"]["test"]);

  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string

  return true;
}

void constructtime(const struct dweet_resp* dweet_resp, struct dtime* dtime) {
  strncpy( dtime->tYear, dweet_resp->created, 4);
  dtime->tYear[4] = '\0';
  strncpy( dtime->tMonth, dweet_resp->created+5, 2);
  dtime->tMonth[2] = '\0';
  strncpy( dtime->tDay, dweet_resp->created+8, 2); 
  dtime->tDay[2] = '\0';
  strncpy( dtime->tHour, dweet_resp->created+11, 2);
  dtime->tHour[2] = '\0';
  strncpy( dtime->tMin, dweet_resp->created+14, 2);
  dtime->tMin[2] = '\0';
  strncpy( dtime->tSec, dweet_resp->created+17, 2);
  dtime->tSec[2] = '\0';
}

void printDweetResp(const struct dweet_resp* dweet_resp) {
  Serial.print("thing = ");
  Serial.println(dweet_resp->thing);
  Serial.print("created = ");
  Serial.println(dweet_resp->created);
  Serial.print("content1 = ");
  Serial.println(dweet_resp->content1);

}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
