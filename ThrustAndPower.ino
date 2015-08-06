// LCD headers
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Hardware-specific library

// Load Cell Amp headers
#include "HX711.h"

// SD Card headers
#include <SdFat.h>
SdFat SD;          // SdFat to SD mapping object

// Servo Headers
#include <Servo.h> 

// Initialize the TFT device
#define TFT_CS     10
#define TFT_RST    9
#define TFT_DC     8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// Initialize the HX711 scale on Pins 2 (CLK) and 3 (DAT)
HX711 scale(3, 2); // parameter "gain" is ommited; the default value 128 is used by the library

Servo myservo;
char tmp[5];       // string storage
int pos = 0;       // variable to store the servo position 

void setup() {
	myservo.attach(5);  // attaches the servo on pin 9 to the servo object 
	myservo.write(0); // stop the servo.  First thing. 
	// initialize the 1.8" TFT LCD
	tft.initR(INITR_18BLACKTAB);
	// and fill it with black pixels
	tft.fillScreen(ST7735_BLACK);

	tft.setTextWrap(true);
	tft.setCursor(0, 0);
	tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
	tft.setTextSize(1);
  
	tft.println("display OK");  
  
	// SD init
	// make sure that the default chip select pin is set to
	// output, even if you don't use it:
	pinMode(10, OUTPUT);

	// see if the card is present and can be initialized:
	while(!SD.begin(7)) {
		tft.setCursor(0,0);
		tft.println("Please insert card");
		delay(100);
	}

	// wipe out the old data while we're at it.
	if(SD.exists("output.txt")) {
		SD.remove("output.txt");
		tft.println("deleted output.txt");
	}
	// I just deleted it, so lay down the headers again.
	FileHeader();
 float scale_setting=459.f; 
	// Check the HX711 demo code for more information on the difference between scale.read, scale.read_average, scale.get_value and scale.get_units.
	// scale.read() = raw ADC from HX711
	// scale.read_average(x) = average of (x) readings of HX711 ADC
	// scale.get_value(x) = Average of x readings from ADC minus tare weight.
	// scale.get_units(x) = average of x readings from ADS minus tare weight divided by scale.set_scale() value
	tft.print("scale at ");
	tft.println(scale_setting);
	//	scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
// target: 617g
//	scale.set_scale(411.f); // HI  673g
//	scale.set_scale(452.f); // HI  645.93 
	scale.set_scale(scale_setting); // closer, maybe? 460 was consistently slightly low.
//	scale.set_scale(460.f); // Close enough - 610-616, depending on how you tap the scale.  The load cell is "sticky" on that damn thing due to the paint.
//	scale.set_scale(462.f); // LOW 606g
//	scale.set_scale(472.f); // LOW 592.56
	tft.println("tare");
	scale.tare();				        // reset the scale to 0

	tft.fillScreen(ST7735_BLACK);
}

void loop() {
	// ----------------------------------------
	// Countdown from 3 for safety, so people aren't close to the device when it kicks in.
	Countdown(3);
	
	// ----------------------------------------
	// Test that motor!
	Testing(10);
	ServoSafe(); // Stop the ESC so I'm not trying to unplug it while it's thrashing off the desk.

	tft.setTextSize(2);
	tft.setCursor(0,0);
	tft.print("  0%");
	tft.setCursor(60,0); // overwrite the cycle count.
	tft.println("Done.");

	while( true ); // halt.  do nothing.  stop.

/* Load cell calibration:
	tft.setCursor(0,0);
	tft.println(scale.get_values(10));
*/
	
}

void Countdown(int times) {
	tft.fillScreen(ST7735_BLACK);
	tft.setTextSize(2);
	tft.setCursor(12,0);
	tft.print("Starting");
	tft.setCursor(48,21);
	tft.print("in");
	tft.setTextSize(3);
	for(int x = times; x>0; x--) {
		tft.setCursor(48,63);
		tft.print(x);
		delay(1000);
	}
	tft.fillScreen(ST7735_BLACK);
}
// -------------------------------------
// A subroutine for testing the motor and prop.
void Testing(int cycles) {
	CycleDisplayLayout(cycles);
	for(int x = 1; x<=cycles; x++) {
		Cycle( 90,  50, 3000,x,cycles);
		Cycle(140, 100, 3000,x,cycles);
//		Cycle(  0,   0, 1000,x,cycles); // stopping the motor seems to make for worse results for the other samples.
	}  
}

// -------------------------------------
// Make sure the servo is safe.
void ServoSafe() {
	myservo.write(0);
}

// -------------------------------------
// Lay down a header to the file
void FileHeader() {
	tft.println("header");
	File dataOut = SD.open("output.txt", FILE_WRITE);
	if(dataOut) {
		dataOut.print("speed");   
		dataOut.print(",");
		dataOut.print("avgread"); 
		dataOut.print(",");
		dataOut.print("amp");     
		dataOut.print(",");
		dataOut.print("watt");
		dataOut.print(",");
		dataOut.print("gpw");
		dataOut.print(",");
		dataOut.println("vcc");
		dataOut.flush();
		dataOut.close();
	}
	else { 
		tft.print("error opening output.txt"); 
	}
}

// -------------------------------------
// Perform a particular speed and time section of the cycle
void Cycle(int speed, int pct, long time, int cycle, int cycles) {
	myservo.write(speed);
	delay(500);

	unsigned long curtime = millis();
	
	// Run the cycle for as long as provided
	while(millis() < curtime+time) {
		// read thrust at 100%
		float avgread = scale.get_units(3); 
		// read vcc - A0 is a 90v divider, A2 is ~17.79V
		float rawvcc = (analogRead(A2));
		float vcc = (rawvcc*17.79)/1000;
		// read amperage
		float rawamp = (analogRead(A1));
		float amp = (rawamp*94.0819)/1000; // technically it's 90/1024 (90A sensor, 10 bits of precision), but this matches the powermeter better.
		// calculate watts and grams per watts
		float watt = vcc*amp;
		float gpw = avgread/watt;

		if(millis() > curtime + 500) { // don't log the first half second's data
			File dataOut = SD.open("output.txt", FILE_WRITE);
			if(dataOut) {
				dataOut.print(pct);  
				dataOut.print(",");
				dataOut.print(avgread); 
				dataOut.print(",");
				dataOut.print(amp);    
				dataOut.print(",");
				dataOut.print(watt);    
				dataOut.print(",");
				dataOut.print(gpw);    
				dataOut.print(",");
				dataOut.println(vcc);
				dataOut.flush();
			}
			else { 
				tft.print("error opening output.txt"); 
			}
			dataOut.flush();
			dataOut.close();
		}
		// ------------------------------------------------------------
		// LCD display bits below.

		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);

		// Servo Speed
		tft.setCursor(0,0); tft.setTextSize(2);
		tft.print(dtostrf(pct,3,0,tmp)); tft.println("%");

		// cycle count
		tft.setCursor(60,8); tft.setTextSize(1);
		tft.print(dtostrf(cycle,2,0,tmp));
  
		// Thrust
		tft.setCursor( ( (128/2) - ( (5*10) / 2 ) ),33);
		tft.setTextSize(2);
		tft.print(dtostrf(avgread,-5,1,tmp));

		// Amps
		tft.setCursor(((128/2)-((5*10)/2)),66);
		tft.print(dtostrf(amp,-5,2,tmp));
		/* -- only display raw values in testing only, comment in production
		tft.setCursor(0,66);
		tft.setTextSize(1);
		tft.print(rawamp);
		tft.setTextSize(2);
		*/
		
		// Watts
		tft.setCursor(((128/2)-((5*10)/2)),99);
		tft.print(dtostrf(watt,-5,2,tmp));
		/* -- only display raw values in testing only, comment in production
		tft.setCursor(0,99);
		tft.setTextSize(1);
		tft.print(rawvcc);
		tft.setTextSize(2);
		*/
		
		// g/W
		tft.setCursor(((128/2)-((5*10)/2)),132);
		tft.print(dtostrf(gpw,-5,2,tmp));

		//time
		tft.setCursor(0,148);
		tft.setTextSize(1);
		tft.print(millis());

		// vcc in the lower corner
		tft.setCursor(98,148);
		if(vcc < 10.6) {
			// why blue?  because red is blue.  I suspect I have a RGB/BGR
			tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
		}
		else {
			tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		}
		tft.print(vcc);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  
	}
}

// -------------------------------------
// Set up initial layout for the display
void CycleDisplayLayout(int cycles) {
	// cycle count
	tft.setCursor(60,0);
	tft.setTextSize(1);
	tft.print("Cycle");
  
	tft.setCursor(78,8);
	tft.print("of");

	tft.setCursor (96,8);
	tft.print(cycles);
  
	// Thrust
	tft.setCursor(((128/2)-((7*5)/2)),24); // screen_width/2 - charactercount*charwidth/2 = center-ish.
	tft.print("Thrust:");

	// Amps
	tft.setCursor(((128/2)-((5*5)/2)),57);
	tft.print("Amps:");

	// Volts
	tft.setCursor(((128/2)-((6*5)/2)),90);
	tft.print("Watts:");

	// Volts
	tft.setCursor(((128/2)-((4*5)/2)),123);
	tft.print("g/W:");
}  

