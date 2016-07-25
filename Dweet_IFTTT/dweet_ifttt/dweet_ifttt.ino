
#include <ESP8266WiFi.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

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

//general struct for time
struct time_struct{
  //extra char for null termination
  char tYear[5];
  char tMonth[3];
  char tDay[3];
  char tHour[3];
  char tMin[3];
  char tSec[3];
};

//systime
time_struct bootTime;
time_struct prevResponse;
time_t prevDisplay = 0; // when the digital clock was displayed

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

  //Getting time from NTP server.
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  setSyncProvider(getNtpTime);

  //set boottime
  SetBootTime();
}
void loop() {

  //this function run every 5s
  delay(5000);
  Serial.println("======================================");
  
  //show the current time...
  //can be disable in future...
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();  
    }
  }

  digitalWrite(ledPin, HIGH);
  
  Serial.print("connecting to ");
  Serial.println(dweet_server);

  WiFiClient client;
  if (!client.connect(dweet_server, 80)){
    Serial.println("connection failed\n\n");
    return;
  }


  //get Dweet Response
  client.print(httpPost);
  
  char endOfHeaders[] = "\r\n\r\n";
  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);
  
  if (!ok) {
    Serial.println("No response or invalid response!\n\n");
    return;
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
  time_struct dtime;
  if (parseDweetRespWith(response_with, &dweet_resp)){
    //String created(dweet_resp.created);
    //if (created != last_response){
    constructtime(&dweet_resp, &dtime);
    if (IsNewerDweetResponse(&dtime, &prevResponse)){
      prevResponse = dtime;
      Serial.println("Get new response from Dweet");
      printDweetResp(&dweet_resp);
      Serial.println("HIGH LED");
      digitalWrite(ledPin, LOW);
    }
    else{
      Serial.println("No new response from Dweet");
    }
  }
  Serial.println("closing connection\n\n");
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.printf("%02d:%02d.%02d %02d-%02d-%04d\n", hour(), minute(), second(), day(), month(), year());
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

void constructtime(const struct dweet_resp* dweet_resp, struct time_struct* dtime) {
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

time_t getNtpTime()
{
  WiFi.hostByName(ntpServerName, timeServerIP);
  delay(1000);
  while(true){
    sendNTPpacket(timeServerIP);
    int cb = udp.parsePacket();
    if (!cb){
      Serial.println("no packet yet");
      delay(1000);
    }
    else{
      //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/NTPClient/NTPClient.ino
      //https://github.com/PaulStoffregen/Time/blob/master/examples/TimeNTP/TimeNTP.ino
      
      Serial.println(cb);
      udp.read(packetBuffer, NTP_PACKET_SIZE);
  
      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:
      
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
  
      // now convert NTP time into everyday time:
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      // print Unix time:
      Serial.println(epoch);
      return epoch;
    } //if
  }  //while
  
} //gettimeNtp

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

void SetBootTime(){
  sprintf(bootTime.tYear,  "%02d", year());
  sprintf(bootTime.tMonth, "%02d", month());
  sprintf(bootTime.tDay,   "%02d", day());
  sprintf(bootTime.tHour,  "%02d", hour());
  sprintf(bootTime.tMin,   "%02d", minute());
  sprintf(bootTime.tSec,   "%02d", second());
  prevResponse = bootTime;
}

void printFromTimeStruct(struct time_struct* time_struct){
  Serial.printf("%02d:%02d.%02d %02d-%02d-%04d\n", atoi(time_struct->tHour), atoi(time_struct->tMin), atoi(time_struct->tSec),
                                                 atoi(time_struct->tDay), atoi(time_struct->tMonth), atoi(time_struct->tYear));
}

boolean IsNewerDweetResponse(struct time_struct* dweet_time, struct time_struct* time_to_compare){
  if (atoi(dweet_time->tYear) > atoi(time_to_compare->tYear))
      return true;
  if (atoi(dweet_time->tMonth) > atoi(time_to_compare->tMonth))
      return true;
  if (atoi(dweet_time->tDay) > atoi(time_to_compare->tDay))
      return true;
  if (atoi(dweet_time->tHour) > atoi(time_to_compare->tHour))
      return true;      
  if (atoi(dweet_time->tHour) == atoi(time_to_compare->tHour)){
      if (atoi(dweet_time->tMin) > atoi(time_to_compare->tMin))
          return true;
      if (atoi(dweet_time->tMin) == atoi(time_to_compare->tMin)){
          if (atoi(dweet_time->tSec) > atoi(time_to_compare->tSec))
              return true;
      }
  }
  return false;
}


