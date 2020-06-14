
#include <Adafruit_NeoPixel.h>
int pin         =  3; // LED strip data pin
int numPixels   = 115; // LEDs are wired left to right in a matrix of 23x5, and are powered from a seperate power supply
int pixelFormat = NEO_GRB + NEO_KHZ800;
Adafruit_NeoPixel *pixels;
Adafruit_NeoPixel strip(numPixels, pin, NEO_GRB + NEO_KHZ800);

// mapping for PET IO to arduino pins
const int petCA1 = 13;
const int petPB0 = 12;
const int petPB1 = 11;
const int petPB2 = 10;
const int petPB3 = 9;
const int petPB4 = 8;
const int petPB5 = 7;
const int petPB6 = 6;
const int petPB7 = 5;  
const int petCB2 = 4;
//PET ground is wired to arduino ground
//Pin 3 of PET Cassette port 2 is wired to Vin of the arduino (PET controllable 6volt motor power)
//pin 6 of PET cassette port 2 is wired to PET ground (brings motor power high on pet power up??)
//pin 4 of pet cassette port 2 is wired to nothing for now (read line)
//pin 5 of pet cassette port 2 is wired to nothing for now (write line)
// pin A0 of arduino is used as power for LM386 audio amp
const int ampPwr =A0;

//more info on pet IO below

const byte dipPins[] = {
  petPB0, petPB1, petPB2, petPB3, petPB4, petPB5, petPB6, petPB7};  //An array to receive a binary 8-bit number on not necessarily consecutive pins, ordered by LSb to MSb
const byte dipSize = sizeof(8);  // Size of the dipPins[] array in number of bytes. (Remember, this is one of the few built in functions that doesn't use camel case...)





void setup(){
  //start serial connection. NOTE: pet controlled power cycling of the arduino isn't possible with usb connected. this breaks some stuff. or it can be seen as a feature to enable the led strip to run while the pet is powered off
  Serial.begin(9600);
  pixels = new Adafruit_NeoPixel(numPixels, pin, pixelFormat);
  pixels->begin(); 
  pixels->clear(); // Set all pixel colors to 'off'

  //set up pins. petPB0-7 are for data, CA1 and CB2 are handshake lines
  // pin A0 functions as power source for LM386 amp module
  //CB2 also functions as the PET's shift register, which is also how the PET generates sound. 
  pinMode(petCA1, OUTPUT );
  pinMode(petPB0, INPUT_PULLUP);
  pinMode(petPB1, INPUT_PULLUP);
  pinMode(petPB2, INPUT_PULLUP);
  pinMode(petPB3, INPUT_PULLUP);
  pinMode(petPB4, INPUT_PULLUP);
  pinMode(petPB5, INPUT_PULLUP);
  pinMode(petPB6, INPUT_PULLUP);
  pinMode(petPB7, INPUT_PULLUP);
  pinMode(petCB2, INPUT_PULLUP);
  pinMode(ampPwr, OUTPUT); 
  digitalWrite(petCA1, 0); //send ready signal
  digitalWrite(ampPwr, 1); //turn on amplifier

  //blink green to indicate a reset happened
  solidColor(0,1,0);
  delay(100);
  solidColor(0,0,0);


}


/* ************   MAIN LOOP   **************************************************** */



void loop(){


  Serial.println("waiting for mode");
  // expect a numeric value 1 to 254 to indicate subroutine to run
  // most subroutines run until a value off 255 is received from the PET
  // maybe explore the use of interrupts in the future

  switch (getPetByte()) {

  case 1:
    {
      Serial.println("Solid Color Mode");
      Serial.println("Input RGB");
      int rIn=getPetByte();
      int gIn=getPetByte();
      int bIn=getPetByte();
      solidColor(rIn,gIn,bIn);
    }
    break;

  case 2: 
    {   
      Serial.println("Rainbow Mode");
      while (parallelToByte()!=255) {
        rainbow(1);
      } 
    } 
    delay(25);
    digitalWrite(petCA1, HIGH);
    digitalWrite(petCA1, LOW);
    delay(25);
    solidColor(0,0,0);
    break;

  case 3: 
    {   
      Serial.println("Blinken Lights Mode");
      Serial.println("Enter rgb");
      int rIn=getPetByte();
      int gIn=getPetByte();
      int bIn=getPetByte();
      Serial.println("Running. To exit loop send 255");
      while (parallelToByte()!=255) {
        blinkenLights(rIn,gIn,bIn);
      }
    }
    delay(25);
    digitalWrite(petCA1, HIGH);
    digitalWrite(petCA1, LOW);
    delay(25);
    solidColor(0,0,0);
    break;

  case 4:  
    {
      Serial.println("Matrix Mode"); 
      Serial.println("Running. To exit loop send 255");
      while (parallelToByte()!=255) { 
        matrix();
      }
      delay(25);
      digitalWrite(petCA1, HIGH);
      digitalWrite(petCA1, LOW);
      delay(25);
    }
    solidColor(0,0,0);
    break;

  case 5:
    {
      Serial.println("Shift Register Mode");
      Serial.println("enter rgb bias and delay");
      int rb=getPetByte();
      int gb=getPetByte();
      int bb=getPetByte();
      int dd=getPetByte();
      digitalWrite(ampPwr,getPetByte()); // control power to lm386 amp
      Serial.println("Running. Arduino must be reset to enter other modes");
      //this just reads bits from the shift register and assigns those value to pixels
      while (1){
        for (int i=1;i<numPixels;i++) {
          pixels->setPixelColor(i, pixels->Color(digitalRead(petCB2)*bb+digitalRead(petCB2)*bb,gb*digitalRead(petCB2)+digitalRead(petCB2)*gb,rb*digitalRead(petCB2)+digitalRead(petCB2)*rb));
        }
        pixels->show();  
        delay(dd); 
      }
    }
    break;

  case 255:
    {
      solidColor(0,0,0);
    }
    break;

  default:
    Serial.println("invalid mode");
    prn2Term();
    break;



  } 


}





/* *******    LED   ROUTINES   **************************************************************************************************** */




void solidColor(int r, int g, int b) {
  for (int i=1;i<numPixels;i++) {
    pixels->setPixelColor(i, pixels->Color(r,g,b));  
  }
  pixels->show();
}






void blinkenLights(int r, int g, int b) {
  for (int i=1;i<numPixels;i++) {
    int blinKen=random(0,2);
    pixels->setPixelColor(i, pixels->Color(r*blinKen,g*blinKen, b*blinKen));
  }  
  pixels->show();  
  delay(750);
}






void matrix() {

  int numDrops=random(1,4);
  for(int j=0;j<numDrops;j=j+1) { 
    for(int i=random(1,24); i<numPixels; i=i+23) { 
      pixels->setPixelColor(i, pixels->Color(1, 16, 0));
      pixels->show();  
      delay(150); 
    }
  }

  for (int i=1;i<numPixels;i++) {
    pixels->setPixelColor(i, pixels->Color(0,0,0));
    pixels->show();   
  }
}




void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue,255,48)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}



/* ********   COMMUNICATION SUBROUTINES   ********************/

int getPetByte(){
  //wait for cb2 strobe from PET
  while (digitalRead(petCB2)==0) {
  };
  delay(25);
  digitalWrite(petCA1, HIGH); //Strobe PET's CA1 pin
  digitalWrite(petCA1, LOW);
  delay(25);
  int ret=parallelToByte();
  return ret;
}

int parallelToByte()
{
  int ret = 0;  // Initialize the variable to return
  int bitValue[] = {
    1,2,4,8,16,32,64,128    };  // bit position decimal equivalents
  for(int i = 0; i < 8; i++)  // cycle through all the pins
  {
    if(digitalRead(dipPins[i]) == HIGH)  // because all the bits are pre-loaded with zeros, only need to do something when we need to flip a bit high
    {
      ret = ret + bitValue[i];  // adding the bit position decimal equivalent flips that bit position
    }
  }
  return ret;
}

void prn2Term(){
  int binaryNumber = parallelToByte();
  Serial.print(digitalRead(petPB0));
  Serial.print(digitalRead(petPB1));
  Serial.print(digitalRead(petPB2));
  Serial.print(digitalRead(petPB3));
  Serial.print(digitalRead(petPB4));
  Serial.print(digitalRead(petPB5));
  Serial.print(digitalRead(petPB6));
  Serial.print(digitalRead(petPB7));
  Serial.print(" ");
  Serial.print(digitalRead(petCA1)); 
  Serial.print(" ");
  Serial.print(digitalRead(petCB2)); 
  Serial.print(" ");
  Serial.println(binaryNumber);
}





/* PET User and cassette port wiring

Cassette port
-------------
	   1   2   3   4   5   6
	  --- --- --- --- --- ---
	==========================
	  --- --- --- --- --- ---
	   A   B   C   D   E   F


Pin# Name  Description                                              
-------------------------------------------------------------------------------
A-1  GND   Digital Ground
B-2  +5V   +5 Volts to operate cassette circuitry only               maybe use to power fan
C-3  Motor Computer controlled +6V for cassette motor                wire to arduino unregulated power in 
D-4  Read  Read line from cassette                                   unused for now
E-5  Write Write line to cassette                                    unused for now
F-6  Sense Monitors closure of any locking type cassette switch      wire to ground


User Port Pinouts
-----------------
rear view for User port:

	   1   2   3   4   5   6   7   8   9   10  11  12
	  --- --- --- --- --- --- --- --- --- --- --- ---
	===================================================
	  --- --- --- --- --- --- --- --- --- --- --- ---
	   A   B   C   D   E   F   H   J   K   L   M   N

Pin# Function Description                                           wire to Arduino pin
-------------------------------------------------------------------------------------------
 1   Ground   System Ground
 2   TV Video Video Out for external displays
 3   SRQ      Connected to IEEE-488 SRQ (PIA2 CB1 in only)
 4   EOI      Connected to IEEE-488 EOI (PIA1 PA6 in, PIA1 CA2 out)
 5   Diagnostic Sense (PIA1 PA7)
	      Held low causes power up to diagnostic routines or monitor
 6   Read 1   Connected to cassette #1 read line (PIA1 CA1)
 7   Read 2   Connected to cassette #2 read line (VIA CB1)
 8   Write    Diagnostic cassette write verify (VIA PA3)
 9   Vert     TV Vertical for external displays
10   Horiz    TV Horizontal for external displays
11   GND
12   GND
 A   GND
 B   CA1      Edge sensitive input for 6522 VIA                            13
 C   PB0      PB0-7 are independently programmable                         12
 D   PB1      for Input or Output                                          11
 E   PB2                                                                   10
 F   PB3                                                                    9 
 H   PB4                                                                    8
 J   PB5                                                                    7
 K   PB6                                                                    6
 L   PB7                                                                    5
 M   CB2      Special IO pin of VIA, connected to shift register            4
 N   GND      Digital ground                                               ground
*/


