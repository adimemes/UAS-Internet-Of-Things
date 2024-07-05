///////////////////////////////////////////////////// Penerima

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Masukan Sinyal yang akan digunakan
#define WIFI_SSID "Infinix NOTE 30 Pro"
#define WIFI_PASSWORD "12345678910"

// Masukan  Firebase project API Key
#define API_KEY "AIzaSyDoeVFzyWoRKpNSY8SmjoCHZcag8hPG6Gk"
// Masukan RTDB URL
#define DATABASE_URL "https://iotmemes-default-rtdb.asia-southeast1.firebasedatabase.app/"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

int pushCount = 0;
String sendData;

/////////////////////////////////////////////////////// Define dari Pin sensor
#if defined(_AVR_ATmega328P) || defined(AVR_ATmega328PB) || defined(AVR_ATmega2560) || defined(AVR_ATmega1280_)
#define CSN 10
#define LED 12
#define CE 9
#elif defined(ESP32)
#define CSN 5
#define CE 4
#else
#error "Make Config yourself"
#endif
#define Debug_mode false

//////////////////////////////////////////////////////////////////////////// Membuat pipe unique
const uint64_t pipeIn = 0x662266;  //Harus sama dengan receiver!!!

RF24 radio(CE, CSN);  // select  CSN and CE  pins

//////////////////////////////////////////////////////////Create a struct untuk menerima  NRF24
struct MyData {
  byte suhu;
  byte humadity;
  byte jarak;
  byte gas;
};
MyData data;

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(12, OUTPUT);
  radio.begin();
  radio.setDataRate(RF24_250KBPS);   //speed  RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.openWritingPipe(pipeIn);     //Open a pipe for writing
  radio.openReadingPipe(1, pipeIn);  //Open a pipe for reading
  radio.openReadingPipe(2, pipeIn);  //Open a pipe for reading
  radio.openReadingPipe(3, pipeIn);  //Open a pipe for reading
  radio.openReadingPipe(4, pipeIn);  //Open a pipe for reading
  radio.openReadingPipe(5, pipeIn);  //Open a pipe for reading
  radio.setAutoAck(true);            // Ensure autoACK is enabled
  radio.setChannel(108);             // Set RF communication channel.
  radio.setPALevel(RF24_PA_MAX);     //translate to: RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.
  radio.enableDynamicPayloads();     //This way you don't always have to send large packets just to send them once in a while. This enables dynamic payloads on ALL pipes.
  radio.setCRCLength(RF24_CRC_16);  // Use 8-bit or 16bit CRC for performance. CRC cannot be disabled if auto-ack is enabled. Mode :RF24_CRC_DISABLED  ,RF24_CRC_8 ,RF24_CRC_16
  radio.setRetries(10, 15);         //Set the number of retry attempts and delay between retry attempts when transmitting a payload. The radio is waiting for an acknowledgement (ACK) packet during the delay between retry attempts.Mode: 0-15,0-15
  radio.startListening();           //Start listening on the pipes opened for reading.
}

unsigned long lastRecvTime = 0;

void recvData() {
  while (radio.available())  //Check whether there are bytes available to be read
  {
    radio.read(&data, sizeof(MyData));  //Read payload data from the RX FIFO buffer(s).
    lastRecvTime = millis();            //here we receive the data
  }
}

void loop() {
  recvData();

  // Update the variables with the received data
  float dataSuhu = data.suhu;
  float datahumidity = data.humadity;
  float datagas = data.gas;
  int datajarak = data.jarak;

  // Output to serial monitor
  Serial.print("Nilai Suhu: ");
  Serial.println(dataSuhu);
  analogWrite(12, dataSuhu);
  Serial.print("Nilai Humadity: ");
  Serial.println(datahumidity);
  analogWrite(13, datahumidity);
  Serial.print("Jarak: ");
  Serial.println(datajarak);
  analogWrite(14, datajarak);
  Serial.print("Nilai GAS: ");
  Serial.println(datagas);
  analogWrite(15, datagas);
  delay(1000);

  // Write data to Firebase
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    // Set Suhu data
    if (Firebase.RTDB.setFloat(&fbdo, "/Suhu", dataSuhu)) {
      Serial.println();
      Serial.print(dataSuhu);
      Serial.println("  - successfully saved to: " + fbdo.dataPath());
      Serial.println("( " + fbdo.dataType() + ")");
      pushCount++;
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Set Humidity data
    if (Firebase.RTDB.setFloat(&fbdo, "/Humadity", datahumidity)) {
      Serial.println();
      Serial.print(datahumidity);
      Serial.println("  - successfully saved to: " + fbdo.dataPath());
      Serial.println("( " + fbdo.dataType() + ")");
      pushCount++;
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Set Gas data
    if (Firebase.RTDB.setFloat(&fbdo, "/Gas", datagas)) {
      Serial.println();
      Serial.print(datagas);
      Serial.println("  - successfully saved to: " + fbdo.dataPath());
      Serial.println("( " + fbdo.dataType() + ")");
      pushCount++;
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Set Jarak data
    if (Firebase.RTDB.setInt(&fbdo, "/Jarak", datajarak)) {
      Serial.println();
      Serial.print(datajarak);
      Serial.println("  - successfully saved to: " + fbdo.dataPath());
      Serial.println("( " + fbdo.dataType() + ")");
      pushCount++;
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Set Alert Banjir
    if (datajarak < 10) {
      if (Firebase.RTDB.setString(&fbdo, "/Alert Kebanjiran", "Volume air naik")) {
        Serial.println();
        Serial.println("Volume air naik - successfully saved to: " + fbdo.dataPath());
      } else {
        Serial.println("FAILED to save flood alert");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    } else {
      if (Firebase.RTDB.setString(&fbdo, "/Alert Kebanjiran", "Volume air aman")) {
        Serial.println();
        Serial.println("Volume air aman - successfully saved to: " + fbdo.dataPath());
      } else {
        Serial.println("FAILED to save flood alert");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }

    // Set  Alert Kebakaran
    if (datagas > 4) {
      if (Firebase.RTDB.setString(&fbdo, "/Alert Kebakaran", "Terdeteksi Asap")) {
        Serial.println();
        Serial.println("Terdeteksi Asap - successfully saved to: " + fbdo.dataPath());
      } else {
        Serial.println("FAILED to save fire alert");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    } else {
      if (Firebase.RTDB.setString(&fbdo, "/Alert Kebakaran", "Keadaan Gas Aman")) {
        Serial.println();
        Serial.println("Keadaan Gas Aman - successfully saved to: " + fbdo.dataPath());
      } else {
        Serial.println("FAILED to save fire alert");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }
  }

  unsigned long now = millis();
  // Check if signal lost
  if (now - lastRecvTime > 1000) {
    // Handle signal lost scenario
  }
}
