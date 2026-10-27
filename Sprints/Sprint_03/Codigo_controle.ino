/*
        DIY Arduino based RC Transmitter
  by Dejan Nedelkovski, www.HowToMechatronics.com
  Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>

// Define the digital inputs
#define jB1 1  // Joystick button 1
#define jB2 0  // Joystick button 2
#define t1 7   // Toggle switch 1
#define t2 4   // Toggle switch 1
#define b1 8   // Button 1
#define b2 9   // Button 2
#define b3 2   // Button 3
#define b4 3   // Button 4

const int MPU = 0x68; // MPU6050 I2C address
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY;
float angleX, angleY;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY;
float elapsedTime, currentTime, previousTime;
int c = 0;

RF24 radio(5, 6);   // nRF24L01 (CE, CSN)
const byte address[6] = "00001"; // Address

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

Data_Package data; //Create a variable with the above structure

void setup() {
  Serial.begin(9600);
  
  // Initialize interface to the MPU6050
  initialize_MPU6050();
  
  // Define the radio communication
  radio.begin();
  radio.openWritingPipe(address);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  
  // Activate the Arduino internal pull-up resistors
  pinMode(jB1, INPUT_PULLUP);
  pinMode(jB2, INPUT_PULLUP);
  pinMode(t1, INPUT_PULLUP);
  pinMode(t2, INPUT_PULLUP);
  pinMode(b1, INPUT_PULLUP);
  pinMode(b2, INPUT_PULLUP);
  pinMode(b3, INPUT_PULLUP);
  pinMode(b4, INPUT_PULLUP);
  
  // Set initial default values
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
}

void loop() {
  // Read all analog inputs and map them to one Byte value
  data.j1PotX = map(analogRead(A1), 0, 1023, 0, 255); 
  data.j1PotY = map(analogRead(A0), 0, 1023, 0, 255);
  data.j2PotX = map(analogRead(A2), 0, 1023, 0, 255);
  data.j2PotY = map(analogRead(A3), 0, 1023, 0, 255);
  data.pot1 = map(analogRead(A7), 0, 1023, 0, 255);
  data.pot2 = map(analogRead(A6), 0, 1023, 0, 255);
  
  // Read all digital inputs
  data.j1Button = digitalRead(jB1);
  data.j2Button = digitalRead(jB2);
  data.tSwitch2 = digitalRead(t2);
  data.button1 = digitalRead(b1);
  data.button2 = digitalRead(b2);
  data.button3 = digitalRead(b3);
  data.button4 = digitalRead(b4);
  
  // If toggle switch 1 is switched on
  if (digitalRead(t1) == 0) {
    read_IMU();    // Use MPU6050 instead of Joystick 1
  }
  
  // Send the whole data from the structure to the receiver
  radio.write(&data, sizeof(Data_Package));
}

void initialize_MPU6050() {
  Wire.begin();                      
  Wire.beginTransmission(MPU);       
  Wire.write(0x6B);                  
  Wire.write(0x00);                  
  Wire.endTransmission(true);        
  
  Wire.beginTransmission(MPU);
  Wire.write(0x1C);                  
  Wire.write(0x10);                  
  Wire.endTransmission(true);
  
  Wire.beginTransmission(MPU);
  Wire.write(0x1B);                   
  Wire.write(0x10);                   
  Wire.endTransmission(true);
}

void read_IMU() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); 
  
  AccX = (Wire.read() << 8 | Wire.read()) / 4096.0; 
  AccY = (Wire.read() << 8 | Wire.read()) / 4096.0; 
  AccZ = (Wire.read() << 8 | Wire.read()) / 4096.0; 
  
  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI) + 1.15; 
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI) - 0.52; 
  
  previousTime = currentTime;        
  currentTime = millis();            
  elapsedTime = (currentTime - previousTime) / 1000;   
  
  Wire.beginTransmission(MPU);
  Wire.write(0x43); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 4, true); 
  
  GyroX = (Wire.read() << 8 | Wire.read()) / 32.8; 
  GyroY = (Wire.read() << 8 | Wire.read()) / 32.8;
  GyroX = GyroX + 1.85; 
  GyroY = GyroY - 0.15; 
  
  gyroAngleX = GyroX * elapsedTime;
  gyroAngleY = GyroY * elapsedTime;
  
  angleX = 0.98 * (angleX + gyroAngleX) + 0.02 * accAngleX;
  angleY = 0.98 * (angleY + gyroAngleY) + 0.02 * accAngleY;
  
  data.j1PotX = map(angleX, -90, +90, 255, 0);
  data.j1PotY = map(angleY, -90, +90, 0, 255);
}

Codigo Controle
