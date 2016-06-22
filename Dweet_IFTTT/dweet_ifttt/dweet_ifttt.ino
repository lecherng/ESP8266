
#include <ESP8266WiFi.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

//wifi setting
const char* ssid = "openspot";
const char* password = "1234567890";
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server

//ifttt setting
const char iftttkey[] = "zzzz";
const char iftttevent[] = "test";
const char url_server[] = "maker.ifttt.com";
const char dweet_server[] = "dweet.io";
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response

const String httpPost =   "GET /get/latest/dweet/for/lecherngtest HTTP/1.1\r\n"
                          "Host: dweet.io\r\n"
                          "User-Agent: Arduino/1.0\r\n"
                          "Accept: */*\r\n"
                          "\r\n";

struct dweet_resp{
  char thing[128];
  char created[128];
  char content1[64];
};

//gpio setting
int ledPin = 16; // GPIO13

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
}
 
void loop() {
  delay(10000);
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
  if (parseDweetRespWith(response_with, &dweet_resp)){
    printDweetResp(&dweet_resp);
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

void printDweetResp(const struct dweet_resp* dweet_resp) {
  Serial.print("thing = ");
  Serial.println(dweet_resp->thing);
  Serial.print("created = ");
  Serial.println(dweet_resp->created);
  Serial.print("content1 = ");
  Serial.println(dweet_resp->content1);
}
