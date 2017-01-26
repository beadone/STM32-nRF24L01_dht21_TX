/*

 Sensor Receiver.
 Each sensor modue has a unique ID.
 
 TMRh20 2014 - Updates to the library allow sleeping both in TX and RX modes:
      TX Mode: The radio can be powered down (.9uA current) and the Arduino slept using the watchdog timer
      RX Mode: The radio can be left in standby mode (22uA current) and the Arduino slept using an interrupt pin
 */
 /* nrf24 pins
 * |v+  |gnd | 
 * |csn |ce  |
 * |Mosi|sck |
 * |irq |miso|
 * stm32 mapping
 * v+  -> 3.3v
 * grn -> grd
 * csn -> PB1
 * ce  -> PB0
 * IRQ -> PB10
 * sck -> PA5
 * MISO-> PA6
 * MOSI-> PA7
 * 
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Set up nRF24L01 radio on SPI-1 bus (MOSI-PA7, MISO-PA6, SCLK-PA5) ... IRQ not used?
RF24 radio(PB0,PB1);

const uint64_t pipes[2] = { 0xF0F0F0F0E5LL, 0xF0F0F0F0e2LL };   // Radio pipe addresses for the 2 nodes to communicate.
//const uint8_t addresses[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};

byte counter = 1; 
#include "DHT.h"
#define DHTPIN PB12
#define DHTTYPE DHT21   // DHT 21
DHT dht(DHTPIN, DHTTYPE);
char  message[10];  //initialise
int i;
int j;
int retry_count = 0;
bool send_ok = 0;

char  gtemp[10];
char  ghum[10];
float h;
float t;
float hic;



bool blinky = false;

void setup(){
     Serial.begin(115200);
  delay(1000);
 dht.begin();
   pinMode(PC13, OUTPUT);
  Serial.println("\n\rRF24 Sensor Receiver");
  
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  // Setup and configure rf radio

  radio.begin();


  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);
  radio.setChannel(0x4c);  //channel 76
  //radio.setPALevel(RF24_PA_LOW);   //RF_SETUP        = 0x03
  radio.setPALevel(RF24_PA_MAX);   //RF_SETUP        = 0x07
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  
//delay(10000);
// wait for the serial port

  // Open pipes to other nodes for communication

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

//  radio.openWritingPipe(addresses[0]); // transmitt
//  radio.openReadingPipe(1,addresses[1]);

  radio.openWritingPipe(pipes[0]); // transmitt
  radio.openReadingPipe(1,pipes[1]);

  // Start listening
  radio.startListening();

  // Dump the configuration of the rf unit for debugging
  radio.printDetails();
}

void loop(){
  
payload_t packet;

 byte gotByte;                                           // Initialize a variable for the incoming response
    //>>>>>>>>>>>>>>>>start of temp code >>>>>>>>>>>>>>>>>>

 
   delay(2000);
//radio.printDetails();
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
 // float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
 /* if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  } */
  

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    //return;
  } else { // send the infomosquitto_sub -h 192.168.0.12 -i nrfubuntu51 -t led/state
    

  

  // Compute heat index in Fahrenheit (the default)
  //float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity:");
  Serial.print(h);
  Serial.print("%\t");
  Serial.print(" Temperature:");
  Serial.print(t);
  Serial.print("*C ");
  //Serial.print(f);
  //Serial.print(" *F\t");
  Serial.print(" Heat index:");
  Serial.print(hic);
  Serial.println("*C ");
  //Serial.print(hif);
  //Serial.println(" *F");

sprintf(gtemp,"%f",t);
sprintf(ghum,"%f",h);
Serial.println(gtemp);
Serial.println(ghum);

    for ( i = 0; i < 5; i++ ) {      
        message[i]= gtemp[i];
    }
  message[i] = ','; 
      for ( j = 0; j < 5; j++ ) {
        i++;
        message[i]= ghum[j];
    }
  message[i+1] = '\0';

  
  //Serial.print(",\t");
  Serial.print(message);
  Serial.print(",\t");
  Serial.println(sizeof(message));
  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    radio.stopListening();   
  //radio.printDetails();
        blinky = !blinky;
    if (blinky){
      digitalWrite(PC13, HIGH);
    }else {
      digitalWrite(PC13, LOW);
    }

  
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ");
   if ( radio.write(&message,sizeof(message)) ){                         // Send the temperature to the other radio
                      
        if(!radio.available()){                             // If nothing in the buffer, we got an ack but it is blank
            Serial.print(F("Got blank response. round-trip delay: "));
    
            Serial.println(F(" microseconds")); 
            delay(30000);
           radio.printDetails();    
        }else{      
            while(radio.available() ){                      // If an ack with payload was received
                radio.read( &gotByte, 1 );                  // Read it, and display the response time
                unsigned long timer = micros();
                
                Serial.print(F("Got response "));
                Serial.print(gotByte);
                Serial.print(F(" round-trip delay: "));
               
                Serial.println(F(" microseconds"));
               
                delay(30000);
            }
        }
   }
     }//else dht loop
}// main loop


