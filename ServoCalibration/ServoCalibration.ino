#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
 
void setup() 
{ 
	myservo.attach(9);  // attaches the servo on pin 9 to the servo object 
	Serial.begin(115200);
} 
 
 
void loop() 
{ 
	myservo.write(0);
	delay(150);
	Serial.println("Servo at 0%.  Press any key to begin.");
	while(!Serial.available()) ; 
	int foo = Serial.read();

	myservo.write(140);
	delay(150);
	Serial.println("Servo at 100%.  Connect ESC and press a key.");

	while(!Serial.available()) ; 
	foo = Serial.read();
} 