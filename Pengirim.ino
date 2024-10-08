/*
Pengirim
*/

// ----------------Masukan Libary--------------//
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>
#include <DHT.h>
#include <MQUnifiedsensor.h>

// ----------------Deklarasi Variabel ----------//
const int pinAlarm = 12;
#define RL 10
#define m -0.44953
#define b 1.23257
#define Ro 2.7

float VRL;
float Rs;
float ratio;
float ppm;

//---------------Define Pin---------------//
#define MQ2pin (35)
#define TRIG_PIN 15
#define ECHO_PIN 27
#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

/*//////////////////////////////////////////////////////*/

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#define CSN 8
#define CE 7
#elif defined(ESP32)
#define CSN 5
#define CE 4
#else
#error "Make Config you're self"
#endif
#define Debug_mode false


/*Create a unique pipe out. The receiver has to
  wear the same unique code*/
const uint64_t pipeOut = 0x662266;  //IMPORTANT: The same as in the receiver!!!
/*//////////////////////////////////////////////////////*/

/*Create the data struct we will send
  The sizeof this struct should not exceed 32 bytes
  This gives us up to 32 8 bits channals */

RF24 radio(CE, CSN);  // select  CSN and CE  pins


/*//////////////////////////////////////////////////////*/
//Create a struct to send over NRF24
struct MyData {
  byte suhu;
  byte humadity;
  byte jarak;
  byte gas;
};
MyData data;
byte count = 0;
/*//////////////////////////////////////////////////////*/



//This function will only set the value to  0 if the connection is lost...
void resetData() {
  data.suhu;
  data.humadity;
  data.jarak;
  data.gas;
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  dht.begin();
  pinMode(MQ2pin, INPUT);
  pinMode(pinAlarm, OUTPUT);
  if (Debug_mode)
    printf_begin();
  radio.begin();
  radio.setDataRate(RF24_250KBPS);    //speed  RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.openWritingPipe(pipeOut);     //Open a pipe for writing
  radio.openReadingPipe(0, pipeOut);  //Open a pipe for reading
  radio.setChannel(108);              // Set RF communication channel.
  radio.setPALevel(RF24_PA_MAX);      //translate to: RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.
  radio.enableDynamicPayloads();      //This way you don't always have to send large packets just to send them once in a while. This enables dynamic payloads on ALL pipes.
  //radio.disableDynamicPayloads();//This disables dynamic payloads on ALL pipes. Since Ack Payloads requires Dynamic Payloads, Ack Payloads are also disabled. If dynamic payloads are later re-enabled and ack payloads are desired then enableAckPayload() must be called again as well.
  radio.setCRCLength(RF24_CRC_16);  // Use 8-bit or 16bit CRC for performance. CRC cannot be disabled if auto-ack is enabled. Mode :RF24_CRC_DISABLED  ,RF24_CRC_8 ,RF24_CRC_16
  radio.setRetries(10, 15);         //Set the number of retry attempts and delay between retry attempts when transmitting a payload. The radio is waiting for an acknowledgement (ACK) packet during the delay between retry attempts.Mode: 0-15,0-15
  radio.setAutoAck(true);           // Ensure autoACK is enabled
  radio.stopListening();            //Stop listening for incoming messages, and switch to transmit mode.
  resetData();
}

/**************************************************/

void loop() {
  //---------------Ultrasonic---------------//
  long duration, distance;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) / 29.1;
  // Serial.print("Distance (cm): ");
  // Serial.println(distance);

  //---------------GAS---------------//
  VRL = analogRead(MQ2pin) * (3.3 / 4095.0);
  Rs = ((3.3 * RL) / VRL) - RL;
  ratio = Rs / Ro;
  ppm = pow(10, ((log10(ratio) - b) / m));
  float Mq2gas = (analogRead(MQ2pin));
  // Serial.print("GAS   : ");
  // Serial.println(ppm);

  //---------------Temperature---------------//
  float t = dht.readTemperature();
  // Serial.print("Temperature: ");
  // Serial.print(t);
  // Serial.print(" *C\t");

  //----------------Humidity----------------//
  float h = dht.readHumidity();
  // Serial.print("Humidity: ");
  // Serial.print(h);
  // Serial.println(" %");
  // Serial.println("-----------------------------");

  //---------------Perkondisian Gas---------------//
    if (ppm > 8.0) {
    digitalWrite(pinAlarm, HIGH);
    delay(5000);
    digitalWrite(pinAlarm, LOW);
    delay(5);
  } else {
    digitalWrite(pinAlarm, LOW);
  }

  //---------------Perkondisian Jarak---------------//
  if (distance < 20) {
    digitalWrite(pinAlarm, HIGH);
    delay(5000);
    digitalWrite(pinAlarm, LOW);
    delay(5);
  } else {
    digitalWrite(pinAlarm, LOW);
  }


  data.suhu = t;
  Serial.print("Suhu (C): ");
  Serial.println(t);
  data.humadity = h;
  Serial.print("Humadity:");
  Serial.println(h);
  Serial.println("%");
  Serial.println("-----------------------------");
  data.jarak = distance;
  Serial.print("Jarak (cm): ");
  Serial.println(distance);
  data.gas = ppm;
  Serial.print("Gas: ");
  Serial.println(ppm);
   
  NRF24_Transmit();  //Transmit MyData

  delay(1000);
}

void NRF24_Transmit() {
  radio.writeFast(&data, sizeof(MyData));  //Transmit Data. use one of this two: write() or writeFast()
  if (Debug_mode)
    radio.printDetails();       //Show debug data
  bool OK = radio.txStandBy();  //Returns 0 if failed. 1 if success.
  delayMicroseconds(50);
  radio.flush_tx();  //Empty all 3 of the TX (transmit) FIFO buffers
}
