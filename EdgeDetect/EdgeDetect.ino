#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <SPI.h>

#include <math.h>

// PIN CONSTANTS ==================================================
const int TRIG_PIN_A = 2;
const int ECHO_PIN_A = 3;
const int RED_LED = 4;   // define red LED
const int GREEN_LED = 5; // define green LED pin

const int SPEAKER_PIN = 6;
const int TRIG_PIN_B = 7;
const int ECHO_PIN_B = 8;

const int RST_PIN = 9;
const int SS_PIN = 10;
// END PIN CONSTANTS ==================================================


struct State
{
  float dist_a;
  float dist_b;
  bool cardDetected;
  bool armed;
};

byte PSWBuff[] = {0x60, 0xEC, 0x71, 0xA5};
const float max_distance_a = 35.0;
const float max_distance_b = 20.0;
State state = {0};
MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance.


float pollSensor(int trigger_pin, int echo_pin)
{
  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float distance;

  // Hold the trigger pin high for at least 10 us
  digitalWrite(trigger_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger_pin, LOW);

  // Wait for pulse on echo pin
  while (digitalRead(echo_pin) == 0)
    ;

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min
  t1 = micros();
  while (digitalRead(echo_pin) == 1)
    ;
  t2 = micros();
  pulse_width = t2 - t1;

  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed
  // of sound in air at sea level (~340 m/s).
  distance = pulse_width / 58.0;
  return distance;
}

void displayState()
{
  Serial.print("Distance A: ");
  Serial.print(state.dist_a);
  Serial.print("cm ");
  Serial.print("Distance B: ");
  Serial.print(state.dist_b);
  Serial.println("cm ");
}

String writeHex(byte *buffer, byte bufferSize)
{
  String acc = "";
  for (byte i = 0; i < bufferSize; i++)
  {
    acc.concat(buffer[i] < 0x10 ? " 0" : " ");
    acc.concat(String(buffer[i], HEX));
  }
  return acc;
}

void setup()
{

  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN_A, OUTPUT);
  digitalWrite(TRIG_PIN_A, LOW);

  pinMode(TRIG_PIN_B, OUTPUT);
  digitalWrite(TRIG_PIN_B, LOW);

  // Set Echo pin as input to measure the duration of
  // pulses coming back from the distance sensor
  pinMode(ECHO_PIN_A, INPUT);
  pinMode(ECHO_PIN_B, INPUT);

  // Setup the speaker
  pinMode(SPEAKER_PIN, OUTPUT);

  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init(); // Initiate MFRC522

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void loop()
{

  state.dist_a = pollSensor(TRIG_PIN_A, ECHO_PIN_A); //run ultra sensors
  state.dist_b = pollSensor(TRIG_PIN_B, ECHO_PIN_B);
  displayState();
  if ((state.dist_a > max_distance_a && state.dist_b > max_distance_b) && state.armed == true) { //If detecting empty space while armed, start beeping
  {
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(100);
    digitalWrite(SPEAKER_PIN, LOW);
    delay(100);
  }
  }

  if (state.dist_a < max_distance_a && state.dist_b < max_distance_b) { //If detecting object space while unarmed, arm device
  {
    digitalWrite(RED_LED, HIGH); 
    delay(300);
    state.armed = true;
  }
  }

  // displayState();

  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent())
    return;
  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial())
    return;
  // Show UID on serial monitor
  String content = writeHex(rfid.uid.uidByte, rfid.uid.size);
  content.toUpperCase();
  Serial.println(content);

  // Check if card matches password
  bool matches = true;
  for(int i = 0; i < rfid.uid.size; i++) {
    if(rfid.uid.uidByte[i] != PSWBuff[i]) {
      matches = false;
    }
  }

  Serial.print("Message : ");
  if (matches) // Password matches
  {
   
    state.armed = false;
    Serial.println("DISARMED");
    Serial.println();
    Serial.println("Secure the package");
    Serial.println();
    delay(500);
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(500);
    digitalWrite(SPEAKER_PIN, LOW);
    delay(100);
    digitalWrite(GREEN_LED, HIGH); // turn green light on
    delay(300);
    digitalWrite(RED_LED, LOW); // red light off
    while(state.dist_a < max_distance_a && state.dist_b < max_distance_b) {
       state.dist_a = pollSensor(TRIG_PIN_A, ECHO_PIN_A); //run ultra sensors
      state.dist_b = pollSensor(TRIG_PIN_B, ECHO_PIN_B);
    };
    digitalWrite(GREEN_LED, LOW); // turn green light off
    delay(2000);
  }
  else
  {
    state.cardDetected = false;
    Serial.println(" Access denied");
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(50);
    digitalWrite(SPEAKER_PIN, LOW);    
    delay(50);
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(50);
    digitalWrite(SPEAKER_PIN, LOW);

    digitalWrite(RED_LED, HIGH);
    delay(300);
    digitalWrite(GREEN_LED, LOW);
    delay(1000);
  }

  // Wait at least 60ms before next measurement
  delay(60);
}