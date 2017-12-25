//
//              Created by Meinhard Kissich 2017
//              www.meinhard-kissich.at
//
//    This sketch is work in progress (!). It's not finished
//    and there might be some faults. For further information
//    check out my website.
//
//  You may do whatever you want with this code, as long as:
//    a. share it and keep it FREE. 
//    b. keep the original credits.


#include "Adafruit_FONA.h"
#include "LedControl.h"
#include "Adafruit_Thermal.h"
#include <stdio.h>
#include <SoftwareSerial.h>

#define GMS_RX 10
#define GMS_TX 11
#define GMS_RST 2

#define TX_PRINTER 4      // TX to thermal printer
#define RX_PRINTER 3      // RX from thermal printer, but just an open pin,
                          // because the printer does not respond

// buffer handling not good, better improve it.
char fonaNotificationBuffer[64];
char smsBuffer[156];
char replybuffer[255];
char printerBuffer[300];

// software serial for GSM module and thermal printer
// Note: just one software serial can be used at once
SoftwareSerial fonaSS = SoftwareSerial(GMS_TX, GMS_RX);
SoftwareSerial printerSS = SoftwareSerial(RX_PRINTER, TX_PRINTER);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_Thermal printer(&printerSS);
Adafruit_FONA fona = Adafruit_FONA(GMS_RST);

//init 7-seg display module
LedControl dout = LedControl(6,8,7,1);

//counter: number of received SMS since start-up
long gl_sms_cnt = 0;

void setup() {
  while (!Serial);
  Serial.begin(115200);
  printerSS.begin(9600);
  
  Serial.println(F("Initializing...."));

  //check SIM800
  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find SIM800L"));
    while(1);
  }
  Serial.println(F("SIM800L OK"));

  //set up the FONA to send a +CMTI notification when an SMS is received
  fonaSerial->print("AT+CNMI=2,1\r\n");  

  Serial.println("SIM800L Ready.");

  //init 7-seg display
  dout.shutdown(0, false);
  dout.setIntensity(0,8);
  dout.clearDisplay(0);

  updateSMScnt(); //write gl_sms_cnt to display
    
  Serial.println("7-seg initialised.");

  //thermal printer
  printer.begin();
  printer.inverseOff();

  Serial.println("printer initialized");  
}

void loop() {
  char* bufPtr = fonaNotificationBuffer;    //handy buffer pointer
  
  if (fona.available())      //any data available from GSM?
  {
    Serial.println("new data ...");
    
    int slot = 0;
    int charCount = 0;
    
    //Read the notification into fonaInBuffer
    do  {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaNotificationBuffer)-1)));
    
    //Add a terminal NULL to the notification string
    *bufPtr = 0;


    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)) 
    {
      Serial.print("Slot: "); Serial.println(slot);
      
      char callerIDbuffer[32];  //we'll store the SMS sender number in here
      
      // Retrieve SMS sender address/phone number.
      if (! fona.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);

      // Retrieve SMS value.
      uint16_t smslen;
        
      if (fona.readSMS(slot, smsBuffer, 250, &smslen)) 
      {
        Serial.println(smsBuffer);
        gl_sms_cnt++;
        updateSMScnt();

        //change softwareSerial to printer connection
        printerSS.begin(9600);

        sprintf(printerBuffer, "from *%c%c%c%c%c%c**: %s", callerIDbuffer[3], callerIDbuffer[4], callerIDbuffer[5], 
                                                           callerIDbuffer[6], callerIDbuffer[7], callerIDbuffer[8], 
                                                           smsBuffer);

        //print thermal
        printer.println(printerBuffer);   
        printer.println(" ");
        printer.println(" ");
        printer.println(" ");

        //change softwareSerial back to gsm connection
        fonaSerial->begin(4800);

        //delete all SMS
        Serial.println("Deleting all SMS.");
        fona.sendCheckReply("AT+CMGD=1,4", replybuffer);
        Serial.print(replybuffer);
      }
    }
  }
}



/*
 * writing the gl_sms_cnt value to the 7-seg display.
 */
void updateSMScnt()
{
  dout.setDigit(0,0,gl_sms_cnt % 10,false);
  dout.setDigit(0,1,(gl_sms_cnt / 10) % 10,false);
  dout.setDigit(0,2,(gl_sms_cnt / 100) % 10,false);
  dout.setDigit(0,3,(gl_sms_cnt / 1000) % 10,false);
  dout.setDigit(0,4,(gl_sms_cnt / 10000) % 10,false);
  dout.setDigit(0,5,(gl_sms_cnt / 100000) % 10,false);
  dout.setDigit(0,6,(gl_sms_cnt / 1000000) % 10,false);
  dout.setDigit(0,7,(gl_sms_cnt / 10000000) % 10,false);
}




