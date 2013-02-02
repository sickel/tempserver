/*
 * Web Server
 *
 * A simple web server that shows the value of the analog input pins.
 */

#include <Ethernet.h>
// #include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192,168,0,177 };

Server server(80);

static const byte ledPin = 5;

#define MAXSENSORS 5
#define ONE_WIRE_BUS 3
#define TEMPERATURE_PRECISION 9
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress thermos[MAXSENSORS]; 
int ndevs;

void setup()
{
  pinMode(ledPin, OUTPUT);
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();

  // locate devices on the bus
  // Serial.print("Locating devices...");
  ndevs=sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(ndevs, DEC);
  Serial.println(" devices.");
  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

// Setting up the thermo sensors:
  for (int i=0;i<ndevs;i++){
    if (!sensors.getAddress(thermos[i], i)){
      Serial.print("Unable to find address for Device"); 
      Serial.println(i);
    }else{
      Serial.print("Sensor OK: ");
      Serial.println(i);
      printAddress(thermos[i]);
      sensors.setResolution(thermos[i], TEMPERATURE_PRECISION);
      Serial.println();
      Serial.print("resolution");
      Serial.println(sensors.getResolution(thermos[i]), DEC); 
    }
  }
  

  Ethernet.begin(mac, ip);
  server.begin();
  Serial.println("Server started");
}

void loop()
{ 
  digitalWrite(ledPin, HIGH);
  sensors.requestTemperatures();
  //Serial.println("DONE");
  //float tempin=printData(insideThermometer);
  //float tempout=printData(outsideThermometer);
  float temps[MAXSENSORS];
  for(int i=0;i<ndevs;i++){
    temps[i]=sensors.getTempC(thermos[i]);
  }
  digitalWrite(ledPin, LOW);
  Client client = server.available();
  if (client) {
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
          client.println("Content-Type: text/html");
          client.println();
          if (strncmp("GET /json", request, 8) == 0) {
            client.print("{\"temp\":[");
            for(i=0;i<ndevs;i++){
              client.print(temps[i]);
              if(i<ndevs-1){
                client.print(",");
              }
            }
            client.print("],\"millis\":");
            client.print(millis());
            client.println("}");
          }else{         
            for(i=0;i<ndevs;i++){
              client.print("sensor ");
              client.print(i);
              client.print(" : ");
              client.print(temps[i]);
              client.println("<br />");
            }
            client.print("<br /><br />");
            client.println(millis()/1000);
            client.print("<br /><a href=\"/json\">json</a>");
            client.print("<br /><a href=\"/\">refresh</a>");
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



// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();    
}




