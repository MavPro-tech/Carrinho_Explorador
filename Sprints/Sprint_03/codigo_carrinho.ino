/*
        DIY Arduino based RC Receiver - Car Example 1
  by Dejan Nedelkovski, www.HowToMechatronics.com
  Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Define connection pins for L298N Motor Driver
const int enA = 3;
const int in1 = 4;
const int in2 = 5;
const int enB = 9;
const int in3 = 6;
const int in4 = 7;

RF24 radio(3, 2);   // nRF24L01 (CE, CSN)
const byte address[6] = "00001";

// Max size of this struct is 32 bytes - NRF24L01 buffer limit
struct Data_Package {
  byte j1PotX;
  byte j1PotY;
  byte j1Button;
  byte j2PotX;
  byte j2PotY;
  byte j2Button;
  byte pot1;
  byte pot2;
  byte tSwitch1;
  byte tSwitch2;
  byte button1;
  byte button2;
  byte button3;
  byte button4;
};

Data_Package data;

unsigned long lastReceiveTime = 0;
unsigned long currentTime = 0;

int steering, throttle;
int motorSpeedA = 0;
int motorSpeedB = 0;

void setup() {
  // Set all the motor control pins to outputs
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  // Initialize the radio communication
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();  // Set the module as receiver
  
  resetData();
}

void loop() {
  // Check whether we keep receiving data or if we lost connection
  currentTime = millis();
  if (currentTime - lastReceiveTime > 1000) { // If more than 1 second has passed
    resetData(); // If connection is lost, reset the data (stops the car)
  }

  // Check whether there is data to be received
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package)); // Read the whole data package
    lastReceiveTime = millis(); // Save the last time data was received
  }

  // Parse the data from Joystick 1 to throttle and steering variables
  throttle = data.j1PotY;
  steering = data.j1PotX;

  // === Motor Control Logic === //
  
  // Going backward
  if (throttle < 110) {
    // Set Motor A backward
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    // Set Motor B backward
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    // Convert the declining throttle readings for going backward from 110 to 0 into 0 to 255 PWM value
    throttle = map(throttle, 110, 0, 0, 255);
    motorSpeedA = throttle;
    motorSpeedB = throttle;
  }
  // Going forward
  else if (throttle > 140) {
    // Set Motor A forward
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    // Set Motor B forward
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    // Convert the increasing throttle readings for going forward from 140 to 255 into 0 to 255 PWM value
    throttle = map(throttle, 140, 255, 0, 255);
    motorSpeedA = throttle;
    motorSpeedB = throttle;
  }
  // Stopped/Neutral position
  else {
    motorSpeedA = 0;
    motorSpeedB = 0;
  }

  // Steering control (Left/Right)
  if (steering < 110) {
    // Move to left - decrease left motor speed, increase right motor speed
    int xMapped = map(steering, 110, 0, 0, 255);
    motorSpeedA = motorSpeedA - xMapped;
    motorSpeedB = motorSpeedB + xMapped;
    // Confine the range from 0 to 255
    if (motorSpeedA < 0) {
      motorSpeedA = 0;
    }
    if (motorSpeedB > 255) {
      motorSpeedB = 255;
    }
  }
  if (steering > 140) {
    // Move to right - decrease right motor speed, increase left motor speed
    int xMapped = map(steering, 140, 255, 0, 255);
    motorSpeedA = motorSpeedA + xMapped;
    motorSpeedB = motorSpeedB - xMapped;
    // Confine the range from 0 to 255
    if (motorSpeedA > 255) {
      motorSpeedA = 255;
    }
    if (motorSpeedB < 0) {
      motorSpeedB = 0;
    }
  }

  // Prevent buzzing at low speeds (adjust threshold according to your motors)
  if (motorSpeedA < 70) motorSpeedA = 0;
  if (motorSpeedB < 70) motorSpeedB = 0;

  // Send PWM signals to the L298N driver
  analogWrite(enA, motorSpeedA);
  analogWrite(enB, motorSpeedB);
}

void resetData() {
  // Reset initial default values when connection is lost
  data.j1PotX = 127;
  data.j1PotY = 127;
  data.j2PotX = 127;
  data.j2PotY = 127;
  data.j1Button = 1;
  data.j2Button = 1;
  data.pot1 = 1;
  data.pot2 = 1;
  data.tSwitch1 = 1;
  data.tSwitch2 = 1;
  data.button1 = 1;
  data.button2 = 1;
  data.button3 = 1;
  data.button4 = 1;
  
  motorSpeedA = 0;
  motorSpeedB = 0;
}
