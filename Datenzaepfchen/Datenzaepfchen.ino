/* Datenzaepfchen
 *  
 * This is the Arduino-sketch code for the Datenzaepfchen. 
 * 
 * The program is written for a NodeMCU board. Additional 
 * hardware is a SD-card slot.
 * 
 * We use both the LED on the ESP-12 and the one on the
 * NodeMCU as status indicators. 
 *
 * The (blue) LED on the ESP lights
 * when a client request is being served.
 *
 * The (red) LED on the NodeMCU board blinks when the Datenzaepfchen 
 * is ready and working.
 * 
 * Marian Aldenhövel <marian.aldenhoevel@marian-aldenhoevel.de>
 * Published under the MIT license. See license.html or 
 * https://opensource.org/licenses/MIT
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Ticker.h>

// ***************************************************************************
// Setup:
// ***************************************************************************

// There are two LEDs on my NodeMCU board, a blue one on the ESP module
// itself and a red one as part of the added NodeMCU hardware. Declare constants
// for each pin and then select LED_PIN for the rest of the code to use. 
const int LED_PIN_ESP = 2; 
const int LED_PIN_NODEMCU = 16; 

// Blink-interval of the LED that indicates alive in milliseconds.
const long LED_BLINK_INTERVAL = 1000;       

// ***************************************************************************
// Global variables:
// ***************************************************************************

String hostName;

// Global variable for the servers.
ESP8266WebServer server(80);

const byte DNS_PORT = 53;
DNSServer dnsServer;

// Setup for captive portal. 
IPAddress softAP_IP(192, 168, 4, 1);
IPAddress softAP_NetMask(255, 255, 255, 0);

// Ticker object for the alive-indication.
Ticker alive;

// Handles blinking of the "I am alive" LED. Called from a Ticker object.
void toggleLED() {  
  digitalWrite(LED_PIN_NODEMCU, !digitalRead(LED_PIN_NODEMCU));     
}

// Debug: Output a list of all the files stored in SPIFFS.
void dumpFileSystem() {
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("  %s %d bytes\n", fileName.c_str(), fileSize);
  }
}

String methodToString(HTTPMethod method) {
  switch (method) {
    case HTTP_GET: return "GET";
    case HTTP_POST: return "POST";
    case HTTP_DELETE: return "DELETE";
    case HTTP_OPTIONS: return "OPTIONS";
    case HTTP_PUT: return "PUT";
    case HTTP_PATCH: return "PATCH";
    default: return String(method);
  }
}

// See: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer/src/detail/RequestHandler.h
class StaticFileWithLEDHandler: public RequestHandler {

  String getFilenameFromUri(String uri) {
    return uri; // TODO: Strip query-params etc.
  }
  
  bool canHandle(HTTPMethod method, String uri) {
    String filename = getFilenameFromUri(uri);
    return (SPIFFS.exists(filename +  ".gz") || SPIFFS.exists(filename));
  }

  bool handle(ESP8266WebServer &server, HTTPMethod method, String uri) {   
    digitalWrite(LED_PIN_ESP, LOW);

    logRequest();
    
    bool result = false;
    String filename = uri; // TODO: Strip query-params etc.
 
    File content;
    if (SPIFFS.exists(filename + ".gz")) {
      Serial.printf("(serving .gz) ");
      content = SPIFFS.open(filename + ".gz", "r");
    } else {
      content = SPIFFS.open(filename, "r");
    }

    if (content) { 
      // Create a cache-tag for the file. We do not have timestamps on
      // SPIFFS, so we use the size of the file instead.
      String etag = "\"" + String(content.size()) + "\"";
      server.sendHeader("Cache-Control", "max-age=3600");
      server.sendHeader("ETag", etag);

      // Has the client provided If-None-Match?
      if (server.hasHeader("If-None-Match")) {
        // Yes. And is it the same as we think it should be?
        String clientEtag = server.header("If-None-Match");
        if (etag == clientEtag) {
          // Yes. Don't send content, but a 304 status.
          server.send(304, "text/plain", "304 Not modified");
          Serial.printf("sent not modified.\n");
          result = true;
        }
      }

      // Handled yet? Maybe with a 304?
      if (!result) {
        // No. Serve the file.
        String contentType = getContentType(filename);
        
        size_t sent = server.streamFile(content, contentType);
        Serial.printf("%s sent %d bytes.\n", contentType.c_str(), sent);
        result = true;
      }

      content.close();
    } else {
      Serial.printf("error opening file.\n");
      server.send(500, "text/plain", "500 Internal server error (error opening file)");
      result = true;
    }

    digitalWrite(LED_PIN_ESP, HIGH);

    return result;
  }
      
} staticFileWithLEDHandler;

String getContentType(String filename){
  if(filename.endsWith(".htm"))        return "text/html";
  else if(filename.endsWith(".html"))  return "text/html";
  else if(filename.endsWith(".txt"))   return "text/plain";
  else if(filename.endsWith(".css"))   return "text/css";
  else if(filename.endsWith(".xml"))   return "text/xml";
  else if(filename.endsWith(".png"))   return "image/png";
  else if(filename.endsWith(".gif"))   return "image/gif";
  else if(filename.endsWith(".jpg"))   return "image/jpeg";
  else if(filename.endsWith(".ico"))   return "image/x-icon";
  else if(filename.endsWith(".svg"))   return "image/svg+xml";
  else if(filename.endsWith(".mp3"))   return "audio/mpeg";
  else if(filename.endsWith(".woff"))  return "application/font-woff";
  else if(filename.endsWith(".woff2")) return "application/font-woff2";
  else if(filename.endsWith(".ttf"))   return "application/x-font-ttf";
  else if(filename.endsWith(".eot"))   return "application/vnd.ms-fontobject";
  else if(filename.endsWith(".js"))    return "application/javascript";
  else if(filename.endsWith(".pdf"))   return "application/x-pdf";
  else if(filename.endsWith(".zip"))   return "application/x-zip";
  else if(filename.endsWith(".gz"))    return "application/x-gzip";
  else return "application/octet-stream";
}

void logRequest() {
  Serial.printf("%s %s ", methodToString(server.method()).c_str(), server.uri().c_str());
}

void handleRoot() {
  logRequest();

  server.sendHeader("Location", "/index.html");
  server.send(302, "text/plain", "302 Found");
  
  Serial.printf("sent redirect.\n");        
}

bool looksLikeIP(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

void handleNotFound(){
  digitalWrite(LED_PIN_ESP, LOW);

  logRequest();
  Serial.println("- Not found.");

  // If running as captive portal redirect anything not 
  // found to the setup page.
  if (!looksLikeIP(server.hostHeader()) && server.hostHeader() != (hostName +".local")) {
    Serial.printf("Request redirected to captive portal page.\n");
    server.sendHeader("Location", "http://" + server.client().localIP().toString());
    server.send(302, "text/plain", "302 Found");
  }
    
  digitalWrite(LED_PIN_ESP, HIGH);
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  Serial.printf("handleFileUpload() - %s.\n", upload.filename.c_str());
  /*
  File f = SPIFFS.open(String("/" + Test ), "w");
  f.print(upload.buf);
  f.close();
  */
}

void setup() {
  // Setup serial comms for logging.
  Serial.begin(115200);
  Serial.printf("\nDatenzäpfchen reporting for duty!\n\n");
  
  // Serial.setDebugOutput(true);

  // Setup LED-pin to output, pull LOW.
  pinMode(LED_PIN_NODEMCU, OUTPUT);
  digitalWrite(LED_PIN_NODEMCU, LOW);
  
 // Set up filesystem.
  if (SPIFFS.begin()) {
    Serial.printf("SPIFFS initialized.\n");
  } else {
    Serial.printf("Error initializing SPIFFS.\n");
  }
  
  // dumpFileSystem();
  
  Serial.printf("\n");
  
  // Setup Wifi-LED-pin.
  pinMode(LED_PIN_ESP, OUTPUT);

  // Pointless blinking of the blue LED.
  digitalWrite(LED_PIN_ESP, LOW);
  delay(500);
  digitalWrite(LED_PIN_ESP, HIGH);
  delay(500);
  digitalWrite(LED_PIN_ESP, LOW);

  hostName = "Datenzaepfchen_" + String(ESP.getChipId(), HEX);

  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.hostname(hostName);
  delay(100);

  // Create an AP users can connect to.
  Serial.printf("Configuring as access point SSID \"%s\"...\n", hostName.c_str());

  WiFi.mode(WIFI_AP);
  delay(100);

  WiFi.softAPConfig(softAP_IP, softAP_IP, softAP_NetMask);
  if (!WiFi.softAP(hostName.c_str(), "")) {
    Serial.printf("WiFi.softAP() failed.\n");
  }

  Serial.printf("IP address: %s\n", WiFi.softAPIP().toString().c_str());

  Serial.printf("Setting up DNS server for captive portal...\n");
  /* Setup the DNS server redirecting all the domains to the softAP_IP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", softAP_IP);  
  digitalWrite(LED_PIN_ESP, HIGH);
  
  Serial.printf("Starting MDNS responder...\n");
  if (MDNS.begin(hostName.c_str(), WiFi.localIP())) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
  }
  
  Serial.printf("Starting HTTP server...\n");
  server.on("/", handleRoot);
  server.addHandler(&staticFileWithLEDHandler);
  server.onNotFound(handleNotFound);
  server.onFileUpload(handleFileUpload);
      
  const char * headerkeys[] = {"ETag", "If-None-Match"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize );
  
  server.begin();
 
  Serial.printf("WIFI diagnostics:\n");
  WiFi.printDiag(Serial);
  Serial.printf("\n");

  // Enable ticker to blink the "am alive" LED.
  alive.attach_ms(LED_BLINK_INTERVAL, toggleLED);

  Serial.printf("Setup finished, at your service.\n\n");
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
  MDNS.update();
}

