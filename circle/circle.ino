
//Include the required libraries
#include <qbcan.h>
#include <Wire.h>
#include <SPI.h>
#include <SoftwareSerial.h>

SoftwareSerial ss(11, 5); // RX, TX
BMP180 bmp;

//Radio Parameters
#define NODEID        2    //unique for each node on same network
#define NETWORKID     135  //the same on all nodes that talk to each other
#define GATEWAYID     1    //Receiving node
#define ENCRYPTKEY    "alphabettebahpla" //exactly the same 16 characters/bytes on all nodes!

//Radio object
char payload[80];
RFM69 radio;

int led = 17;
unsigned long next_update = 0;

// Blink the LED for n milliseconds.
void blink(int n)
{
    digitalWrite(led, LOW); // For QBCan, LOW means HIGH
    delay(n);               // Wait the specified time
    digitalWrite(led, HIGH); // For QBCan, HIGH means LOW
}

void setup()
{
    // initialize the digital pin as an output
    pinMode(led, OUTPUT);

    // initialize serial connections
    Serial.begin(9600);
    ss.begin(9600);

    // initialize radio
    radio.initialize(FREQUENCY,NODEID,NETWORKID);
    radio.setHighPower(); //To use the high power capabilities of the RFM69HW
    radio.encrypt(ENCRYPTKEY);
    Serial.println("Transmitting at 433 Mhz");

    // wait 1 second for the rectangle board to initialize
    blink(1000);

    // Send start message.
    Serial.println("START");
    ss.println("START");

    bmp.begin();
}

void loop()
{
    // Get rectangle data and then echo down to the ground.
    while (ss.available() > 0)
    {
        String data = ss.readStringUntil('\n');
        data.trim();
        radio.send(GATEWAYID, data.c_str(), (data.length() + 1));
        Serial.print("Received data: "); Serial.println(data);
    }

    // Check that the timing is correct.
    if (millis() < next_update)
    {
        return;
    }
    next_update += 500;

    // Generate provisional data.
    double T,P;
    bmp.getData(T,P);
    char provisional[80];
    sprintf(payload,"PROV,%d,%d,",(int)T,(int)P);
    //char *tmp[6], *prs[6];
    //dtostrf(T, 5, 1, tmp);
    //dtostrf(P, 5, 1, prs);
    //String provn = "PROV" + "," + tmp + "," + prs;

    // Send provisional data to the ground and via serial.
    //Serial.println(payload);
    ss.println(payload);
    radio.send(GATEWAYID, payload, 80);

    // Blink LED to show data has been updated.
    blink(50);
}

