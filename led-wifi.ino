// Import required libraries
#include <WiFi.h>
#include <SPIFFS.h>

// WiFi parameters
const char* ssid = "Sonnenstation";
const char* password = "FamilieKrausse";

// The port to listen for incoming TCP connections
#define LISTEN_PORT 80

// The used links
#define FAVICON "/favicon.ico"
#define INDEX "/index.html"
#define CSS "/styles.css"
#define SCRIPTS "/script.js"

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

//ESP32 LED
int esp32LED = 1;

bool blink = false;

//Function Declaration
void initializeServer();
bool initializeSPIFFS();

String checkRequest(WiFiClient& client);
void handleRequest(WiFiClient& client);

void prepareHTTPMessage(WiFiClient& client, unsigned code, const String& filename = "");

File getFile(const String& filename, const char* mode);
void addFileContent(WiFiClient& client, File file);

void createTask(TaskFunction_t task, const String& name, unsigned short core);
void wifiTask(void* pvParameters);

// the setup function runs once when you press reset or power the board
void setup() {

  //Set Serial Rate for debugging  
  Serial.begin(112500);

  Serial.println("");
  Serial.println("Starting Initialization.");

  // digital pin LED_BUILTIN as an output.
  pinMode(esp32LED, OUTPUT);

  // Initialize
  Serial.println("");
  Serial.println(" ------------------------------------- ");
  Serial.println("  FastLED Infinite table Wifi Project  ");
  Serial.println("       Written by Tobias K.            ");
  Serial.println(" ------------------------------------- ");

  // Initialize SPIFFS
  if(initializeSPIFFS())
  {
    // initialize Server
    initializeServer();
  }
}

// the loop function runs over and over again forever
void loop() { 

  if(blink)
  {
    Serial.println("Blink ON.");
    digitalWrite(esp32LED, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                    // wait for a second
    digitalWrite(esp32LED, LOW);
    Serial.println("Blink OFF.");    // turn the LED off by making the voltage LOW
    delay(1000);                    // wait for a second   
  }
  else
    delay(1);
}

bool initializeSPIFFS(){

  Serial.println("");
  Serial.print("Initialize SPIFFS at core: " );
  Serial.println(xPortGetCoreID());  
  Serial.println("");

  if(!SPIFFS.begin(true)){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return false;
  }
  Serial.println("SPIFFS mounted.");
  // Check if Index.html is available 
  File file = getFile(INDEX, FILE_READ);
  if(!file)
    return false; 
  Serial.println("index.html found:");
  //Serial.println(file.readString());
  file.close();
  // Check if styles.css is available 
  file = getFile(CSS, FILE_READ);
  if(!file)
    return false;  
  Serial.println("styles.css found.");
  //Serial.println(file.readString());
  file.close();

  // Check if script.js is available 
  file = getFile(SCRIPTS, FILE_READ);
  if(!file)
    return false;  
  Serial.println("script.js found.");
  //Serial.println(file.readString());
  file.close();

  // Check if favicon is available 
  file = getFile(FAVICON, FILE_READ);
  if(!file)
    return false;  
  Serial.println("favicon.ico found.");
  //Serial.println(file.readString());
  file.close();

  Serial.println("SPIFFS Initialized and working.");
  Serial.println("");

  return true;
}

void initializeServer(){

  Serial.println("");
  Serial.print("Initialize Server at core: " );
  Serial.println(xPortGetCoreID());  
  Serial.println("");

  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to Wifi
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wifi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  server.begin();

  // Create Request handler Task
  createTask(wifiTask, "WifiRequestTask", 0);

  Serial.println("Server Initialized and working.");
  Serial.println("");
}

String checkRequest(WiFiClient& client){

  String header = "";
  if(client)
  {
    //New Client
    Serial.println("");
    Serial.print("New Client: "); 
    Serial.println(client.localIP());

    while(client.connected())
    {
      //Read message from client while connected
      if(client.available())
      {
        char c = client.read();

        //extend the header
        header += c;

        //Check for Message end
        if(c == '\n')        
          break;        
      }
    }
  }
  return header;
}

void handleRequest(WiFiClient& client){

  if(!client)
    return;

  String header = checkRequest(client);

  Serial.print("Request Accepted: ");
  Serial.print(header);

  if(header.indexOf("GET /color") >= 0)
  {
    Serial.println("Color checked.");
    blink = !blink;
    prepareHTTPMessage(client, 200, INDEX);
  }
  else if(header.indexOf("GET /brightness") >= 0)
  {
    Serial.println("Brightness checked.");
    prepareHTTPMessage(client, 200, INDEX);
  }
  else if(header.indexOf("GET /time") >= 0)
  {
    Serial.println("Time checked.");
    prepareHTTPMessage(client, 200), INDEX;
  }
  else if(header.indexOf("GET /styles.css") >= 0)
  {
    Serial.println("load CSS.");
    prepareHTTPMessage(client, 200, CSS);
  }
  else if(header.indexOf("GET /script.js") >= 0)
  {
    Serial.println("load JavaScript.");
    prepareHTTPMessage(client, 200, SCRIPTS);
  }
  else if(header.indexOf("GET /favicon.ico") >= 0)
  {
    Serial.println("load Favicon.");
    prepareHTTPMessage(client,200, FAVICON);
  }
  else if(header.length() == 16) // empty GET
  {
    Serial.println("load Mainpage.");
    prepareHTTPMessage(client, 200, INDEX);
  }
  else
  {
    Serial.print("404 Not Found ");
    prepareHTTPMessage(client, 404);
  }

  Serial.print("Request handled.");
  Serial.print(header);
}

void prepareHTTPMessage(WiFiClient& client, unsigned code, const String& filename /*= ""*/){

  switch(code)
  {
    case 200: client.println("HTTP/1.1 200 OK"); break;
    case 404: client.println("HTTP/1.1 404 Not Found"); break;
    default : client.println("HTTP/1.1 500 Internal Server Error"); break;
  }

  client.println("Server: ESP32");

  //Check file ending
  if(filename.indexOf(".html") >= 0)
    client.print("Content-type:text/html;");
  else if(filename.indexOf(".css") >= 0)
    client.print("Content-type:text/css;");
  else if(filename.indexOf(".js") >= 0)
    client.print("Content-type:text/javascript;");
  else if(filename.indexOf(".ico") >= 0)
    client.print("Content-type:image/x-icon;");
  else
    client.print("Content-type:text/plain;");
  client.println(" charset=utf-8");

  // Get File and add header
  File file = filename.length() > 0 ? getFile(filename, FILE_READ) : File();
  if(file && file.size() > 0)
  {
    // Add Content length
    client.print("Content-Length: ");
    client.println(String(file.size() + 1)); //Need to add 1 to the content size
  }

  // Msg Closed
  client.println("Connection: closed");

  // Content Security Header
  String csp = "default-src 'none'; img-src 'self'; script-src 'self'; style-src 'self'";
  client.print("Content-Security-Policy: "  ); client.println(csp);
  client.print("X-Content-Security-Policy: "); client.println(csp);
  client.print("X-WebKit-CSP: "             ); client.println(csp);

  //Close Header Block
  client.println();
  //client.println();

  //Add Content
  if(file)  
    addFileContent(client, file);
  else
    client.println();
}

File getFile(const String& filename, const char* mode){

  File file = SPIFFS.open(filename, mode);
  if(!file){
    Serial.print("An Error has occurred couldn't open ");
    Serial.println(filename);
  }
  return file;
}

void addFileContent(WiFiClient& client, File file){

  //Check File
  if(!file)
    return;

  //Get Content
  while(file.available())
    client.write(file.read());
  file.close();

  //Close Content Block
  client.println();
  client.println();
}

void createTask(TaskFunction_t task, const char* name, unsigned short core){

  Serial.print("Creating Task: ");
  Serial.print(name);
  Serial.print(" at Core ");
  Serial.println(core);
  
  xTaskCreatePinnedToCore(
                      task,  // Function to implement
                      name,  // Name of the Task  
                      10000, // Stack size in words
                      NULL,  // Task input parameters
                      0,     // Priority
                      NULL,  // Task Handle
                      core); // Core of the Task
}

void wifiTask(void* pvParameters){
  
  Serial.print("Wifi Task started at core: ");
  Serial.println(xPortGetCoreID());

  while(true)
  {
    //Create Client
    WiFiClient client = server.available();

    //Check Request
    handleRequest(client);

    //Stop Client
    client.stop();
  }
}