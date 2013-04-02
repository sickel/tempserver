/*
 * Web Server
 *
 * A web server that shows the temperatures of connected DS18B20s.
 * It may either be called directly, and will then show a basic 
 * web page
 * http://192.168.0.177
 * Or it mvay be called as
 * http://192.168.0.177/json 
 * and will then return the temperatures as a json encoded string
 *
 * The html is pretty messy, but it is on the limit of how much 
 * text the arduino can serve.
 *
 * (c) Morten Sickel 2013
 * Based on examples from the DallasTemperature and Ethernet
 * (ENC28J60) libraries
 * licenced under the GPL v2 or later
 *
 * Version 1.0 - 2nd February 2013
 */

#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ip must be set to fit into the local network
byte ip[] = { 192,168,0,177 };
// mac must be unique within the local network (probably OK if there
// are no other arduinoes around
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// maxsensors is the highest number of connected sensors
#define MAXSENSORS 7
/* It seems to work with 7 sensors, with 8 or more, the sketch is 
   hanging -it may be possible to fit in more sensors by stripping 
   out all unneeded code (i.e. Serial.print calls for debugging and 
   testing or unneeded web page code.) (there are also some ways of
   using the memory better, but I have so far not gone into that)
*/
 
// The pin that gets the signal from the sensors:
#define ONE_WIRE_PIN 3

// There should be no need to change anything below here ...

Server server(80);
#define TEMPERATURE_PRECISION 9
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_PIN);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress thermos[MAXSENSORS]; 
int ndevs;// Actual number of devices on the bus

void setup()
{
  // start serial port
 // Serial.begin(9600);
 
  // Start up the library
  sensors.begin();

  // locate devices on the bus
  // Serial.print("Locating devices...");
  ndevs=sensors.getDeviceCount();
 // Serial.print("Found ");
 // Serial.print(ndevs, DEC);
 // Serial.println(" devices.");
  // report parasite power requirements
 // Serial.print("Parasite power : "); 
 // if (sensors.isParasitePowerMode()) Serial.println("ON");
 // else Serial.println("OFF");

// Setting up the thermo sensors:
  for (int i=0;i<ndevs;i++){
    if (!sensors.getAddress(thermos[i], i)){
    //  Serial.print("Unable to find device "); 
    //  Serial.println(i);
    }else{
  //    Serial.print("Sensor OK: ");
   //   Serial.println(i);
    //  printAddress(thermos[i]);
      sensors.setResolution(thermos[i], TEMPERATURE_PRECISION);
    }
  }
  
  Ethernet.begin(mac, ip);
  server.begin();
 // Serial.println("Server started");
}

void loop()
{ 
  // Waits for a client to connect
  Client client = server.available();
  if (client) {
    // reads and stores the temperatures
    sensors.requestTemperatures();
    float temps[MAXSENSORS];
    for(int i=0;i<ndevs;i++){
      temps[i]=sensors.getTempC(thermos[i]);
    }
    char request[10];
    int i = 0;
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read(); if (i < 9) {
          request[i] = c;
          i++;
        }
        // if we've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so we can send a reply
        if (c == '\n' && current_line_is_blank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          if (strncmp("GET /json", request, 8) == 0) {
            // returns json encoded data
            client.println("Content-Type: application/json");
            client.println();
            client.print("{\"temp\":[");
            for(i=0;i<ndevs;i++){
              client.print(temps[i]);
              if(i<ndevs-1){
                client.print(",");
              }
            }
            // the addresses to make it possible to identify sensors
            client.print("],\"address\":[");
            for(i=0;i<ndevs;i++){
              client.print("\"");
              printAddress(thermos[i],client);
              client.print("\"");
              if(i<ndevs-1){
                client.print(",");
              }
            }
            // runtime in millisecouns since boot:
            client.print("],\"millis\":");
            client.print(millis());
            client.println("}");
          }else{         
            client.println("Content-Type: text/html");
            client.println();
            for(i=0;i<ndevs;i++){
              client.print("sensor ");
              client.print(i);
              client.print(" : ");
              client.print(temps[i]);
              client.println(" deg C<br />");
            }
            client.print("<hr />");
            client.println("<a href=\"/\">refresh</a> - ");
            client.println("<a href=\"/json\">json</a><br />");
             // to make it clear that the page updates:
            client.print(millis()/1000);
            client.println(" s uptime");
          }
          break;
        }
        if (c == '\n') {
          // we're starting a new line
          current_line_is_blank = true;
        } else if (c != '\r') {
          // we've gotten a character on the current line
          current_line_is_blank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
  }
}



// function to print a device address on the serial device
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print a device address to the client
void printAddress(DeviceAddress deviceAddress,Client client)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) client.print("0");
    client.print(deviceAddress[i], HEX);
  }
}






