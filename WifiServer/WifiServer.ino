
#include <ESP8266WiFi.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>

//wifi setting
const char* ssid = "openspot";
const char* password = "1234567890";

//ifttt setting
const char iftttkey[] = "bJrdZ0ZTyGPFGd34vBPx82";
const char iftttevent[] = "test";
const char url_server[] = "maker.ifttt.com";

//prototype
void constructHttpRequest(char* httpPost,char* httpData, char value1[], char value2[], char value3[]);

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
  // Check if a client has connected
  
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println("Getting request");
  Serial.println(request);
  client.flush();
 
  // Match the request
 
  int value = HIGH;
  bool sendingIFTTT = false;
  if (request.indexOf("/LED=OFF") != -1)  {
    digitalWrite(ledPin, HIGH);
    value = HIGH;
  }
  if (request.indexOf("/LED=ON") != -1)  {
    digitalWrite(ledPin, LOW);
    value = LOW;
    sendingIFTTT = true;
  } 
 
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  
  //client.print("Led pin is now: %s", value==HIGH?"Off":"On");
 
  if(value == HIGH) {
    client.print("Off");
  } else {
    client.print("On");
  }
  
  client.println("<br><br>");
  client.println("<a href=\"/LED=ON\"\"><button>Turn On </button></a>");
  client.println("<a href=\"/LED=OFF\"\"><button>Turn Off </button></a><br />");  
  client.println("</html>");
 
  delay(1);
  Serial.println("Client disonnected");

  if (client.connect(url_server, 80) && sendingIFTTT == true)
  {
    char *httpData = (char*) malloc(128*sizeof(char));
    char *httpPost = (char*) malloc(1024*sizeof(char));
    
    constructHttpRequest(httpPost, httpData, "test", "le", "cherng");
    Serial.print("Connected to maker channel\n");
    Serial.println(httpPost);
    Serial.println(httpData);
    client.print(httpPost);
    client.print(httpData);

    free(httpData);
    free(httpPost);
  }
  Serial.println("");
 
}

void constructHttpRequest(char* httpPost, char* httpData, char value1[], char value2[], char value3[])
{
  int lengthHttpData;
  int lengthHttpResp;

  lengthHttpData = sprintf(httpData, "{\"value1\":\"%s\", \"value2\":\"%s\", \"value3\":\"%s\"}", value1, value2, value3);
  lengthHttpResp = sprintf(httpPost, "POST /trigger/%s/with/key/%s HTTP/1.1\r\nUser-Agent: Arduino/1.0\r\nHost: %s\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n", iftttevent, iftttkey, url_server, lengthHttpData);
}
 
