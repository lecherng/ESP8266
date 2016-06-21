
#include <ESP8266WiFi.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>

//wifi setting
const char* ssid = "openspot";
const char* password = "1234567890";

//ifttt setting
const char iftttkey[] = "zzzz";
const char iftttevent[] = "test";
const char url_server[] = "maker.ifttt.com";
const char dweet_server[] = "dweet.io";

const String httpPost =   "GET /get/latest/dweet/for/lecherngtest HTTP/1.1\r\n"
                          "Host: dweet.io\r\n"
                          "User-Agent: Arduino/1.0\r\n"
                          "\r\n";
 
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
  delay(10);

  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
}
  

