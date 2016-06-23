#include <ESP8266WiFi.h>
 
const char* ssid = "openspot";
const char* password = "1234567890";
 
int ledPin = 16; // GPIO13
WiFiServer server(80);

const String httpPost =   "POST /trigger/test/with/key/bJrdZ0ZTyGPFGd34vBPx82 HTTP/1.1\r\n"
                          "Host: maker.ifttt.com\r\n"
                          "User-Agent: Arduino/1.0\r\n"
                          "Connection: keep-alive\r\n"
                          "Content-Type: text/html; charset=utf-8;\r\n"
                          "\r\n";
 
void setup() {
  Serial.begin(115200);
  delay(10);
 
  pinMode(ledPin, INPUT);
  
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
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  int ledValue;
  ledValue = digitalRead(ledPin);
  Serial.print(ledValue);

  if (ledValue)
  {
    Serial.println(httpPost);
    client.print(httpPost);
  }
 
}
 
