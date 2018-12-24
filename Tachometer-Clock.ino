// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

DateTime now;

// constants won't change. Used here to set a pin number:
const int speedPin = 6; //blue wire 
const int rpmPin = 3; // yellow wire
const int fuelPinLow = 4; // Fuel gauge wire with 100Ohm resistor
const int fuelPinHigh = 5; // Same Fuel gauge wire but with no resistor
const int buttonPin = 7; // wired to NO button, goes to ground when closed
const int lightPin = 9;  // wired to high beam indicator light, but could be any indicator

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long prevrpmMicros = 0;        // will store last time tach pin was made high
unsigned long prevspeedMicros = 0;      // will store last time speed sensor pin was made high
unsigned long prevChangeMillis = 0;     // for keeping track of when updates happen (every second)
unsigned long buttonPressMillis = 0;    // will store when the button was pressed
unsigned long buttonTime = 0;           // how long the button was held down
bool rpmOn = false;                     // whether or not the tach pin is HIGH
bool speedOn = false;                   // whether or not the tach pin is HIGH

// other variables:
int long speedInterval = 172;           // interval at which to toggle the speed sensor high
int long rpmInterval;                   // interval at which to toggle the tach sensor
unsigned long changeInterval = 1000;    // check the time every second
unsigned long currentMicros = 0;        // for doing the above without using evil delays

// One would think that you can just "do math" to compute the tach signal frequencies
// and use that to display the hours.  Unfortunately, these cheap tachometers don't really 
// seem to work correctly, so I had to manually find these values for 0 thru 12K rpms
// Also, 218.0 was the max that the tach would recognize, so it doesn't REALLY go to 12!
float hourFrequencies[] = {2.0,16.5,32.0,49.5,65.0,82.5,100.0,118.5,136.0,154.0,172.0,189.0,218.0};
float hourFreq = 2.0;                   // doesn't really need to be pre-set, but whatever

int g = 1;        // just being lazy with Gear Pin variable names, so sue me.
int activeFuelPin = 1;    
int fuelPins[] = {fuelPinLow,fuelPinHigh};    // So you can toggle between easily
int gearpins[] = {A0,A2,A3,A1};               // The pins that are connected to the gear indicators
                                              // you could use all 6, but I was sick of soldering
bool hourSetting = false;                     // whether or not the clock (hours) is being set
bool minuteSetting = false;                   // whether or not the clock (minutes) is being set
bool pressed = false;                         // has the button been pressed?
int this_hour = 1;                            // the following 4 variables are used when setting the time
int new_hour = 1;
int this_minute = 0;
int new_minute = 0;

void setup () {
  pinMode(buttonPin, INPUT_PULLUP);   // switch is wired to ground
  pinMode(lightPin, OUTPUT);
  blinky();                           // this just blinks the light when "booting up" for flair.
  pinMode(speedPin, OUTPUT);
  pinMode(rpmPin, OUTPUT);

  delay(500);                         // no need really, just more flair
  Serial.begin(9600);
  Serial.println("hello, world!");    // heh.
 
  rtc.begin();        

  if (! rtc.isrunning()) 
    {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(2018,1,1,6,40,0));  // just in case something went wrong like a dead battery 
    }
  now = rtc.now();
  Serial.println(now.hour());
  if (now.hour() > 24) // It shows as 153 if the clock is not set. Weird.
    {
    Serial.println("setting");
    rtc.adjust(DateTime(2018,1,1,6,40,0));  
    }
  
}

void loop () 
  {
  unsigned long currentMillis = millis();  // blatently stolen from "blink without delay"
  int button = getButton();  // this just returns how long the button has been held down.
  if (button > 0) { Serial.println(button);}  //mostly for debugging, could be deleted.
  if (button > 1000)    // if button was held down for over a second, go into setting mode(s)
    {
    if (hourSetting == false && minuteSetting == false)
      {
      hourSetting  = true;
      blinky();
      now = rtc.now;
      this_hour = now.hour();  
      }
    else if (hourSetting)   // if I WAS setting the hour, now set the minutes
      {
      hourSetting = false;
      minuteSetting = true;
      blinky();
      now = rtc.now;
      this_minute = now.minute();
      }
    else if (minuteSetting)  // write the updated time to the RTC module
      {
      minuteSetting = false;
      blinky();
      rtc.adjust(DateTime(2018,1,1,this_hour,this_minute,0));  
      }
    } 
 

  if (hourSetting)
    {
    if (button > 2 && button < 1000)  // sort of a poor man's debounce
      {
      this_hour++;
      if (this_hour > 12) {this_hour = 0;}
      rpmInterval = 1000000.0/hourFrequencies[this_hour];
      //rpmInterval = hourIntervals[this_hour];
      }
    }
  if (minuteSetting)
    {
    if (button > 2 && button < 500)
      {
      this_minute++;
      if (this_minute > 59) {this_minute = 0;}
    if (this_minute > 0)
      {
      speedInterval = 1000000.0/(float(this_minute)*(1.01655));
      }
    else
      {
      speedInterval = 500000;   // basically, avoiding a divide by zero situation
      } 
      }
    }

  // every second, see if any of the intervals (see below) need to be updated
  // realistically, this could be every 15 seconds, but there's no reason not to do it frequently  
  if (currentMillis - prevChangeMillis >= changeInterval && !hourSetting && !minuteSetting)
    {
    prevChangeMillis = currentMillis;
    now = rtc.now();
    int hours = now.hour();
    int minutes = now.minute();
    int seconds = now.second();
    // First, update/toggle which fuel gauge pin is connected to the fuel gauge sender wire.  
    // The fuel gauge reacts VERY slowly, so I just set it to an empty tank for the last 15
    // seconds of every minute, and a full tank the rest of the time.
    activeFuelPin = int(seconds/45);  
    for (int x=0; x <2;x++)
      {
      pinMode(fuelPins[x], INPUT);  // make the unused pin(s) float
      }
    pinMode(fuelPins[activeFuelPin], OUTPUT);
    digitalWrite(fuelPins[activeFuelPin], LOW);      
    g = int(seconds/15); // returns a number between 0 and 3, for my 4 gears
    for (int x=0; x <4;x++)
      {
      pinMode(gearpins[x], INPUT);  // make all of the gear indicator pins float  
      }
    pinMode(gearpins[g], OUTPUT);
    digitalWrite(gearpins[g], LOW);  // drive the selected gear pin to ground to "activate" it.
    if (hours >= 12) {hours = hours-12;}  // convert to a 12 hour clock.  

    // find out the fractional hour based on where we are in the hour
    float this_hours = float(hours)+float(minutes)/60.0;

    // debugging
    Serial.print(hours);
    Serial.print("  ");
    Serial.print(minutes);
    Serial.print("  ");
    Serial.print(seconds);
    Serial.print("   ");
    Serial.println(this_hours);

    // This is the formula for converting frequency into the interval between pulses in microseconds
    // based on the "table" of manually discovered frequencies, and interpolating between them
    rpmInterval = 1000000.0/((this_hours - hours)*(hourFrequencies[hours+1] - hourFrequencies[hours]) + hourFrequencies[hours]);

    if (minutes > 0)
      {
      speedInterval = 1000000.0/(float(minutes)*(1.01655));  //the formula works out OK
      }
    else
      {
      speedInterval = 500000;   // Though shall not divide by zero
      }  
   // debugging again
    Serial.print(hours);
    Serial.print("  ");
    Serial.print(this_hours);
    Serial.print("  ");
    Serial.print(speedInterval);
    Serial.print("  ");
    Serial.println(rpmInterval);    
    }

  // OK, now that we have the intervals, this is the part that must run constantly and toggles
  // the tachometer and speedometer pins.  It's basically just "blink without delay" but w/ micros()
  currentMicros = micros();
  if (!rpmOn && currentMicros - prevrpmMicros >= rpmInterval)
    {
    // save the last time you got high
    prevrpmMicros = currentMicros;
    digitalWrite(rpmPin, HIGH);   // turn the rpm/tach pin on (HIGH is the voltage level)
    rpmOn = true;
    }
  if (rpmOn && currentMicros - prevrpmMicros >= 150)  // only pulse for 150 microseconds
    {
    digitalWrite(rpmPin, LOW);
    rpmOn = false;     
    }
  currentMicros = micros();
  if (!speedOn && currentMicros - prevspeedMicros >= speedInterval)
    {
    // save the last time you blinked the LED
    prevspeedMicros = currentMicros;
    digitalWrite(speedPin, HIGH);   // turn the speedometer pin on (HIGH is the voltage level)
    speedOn = true;
    }  
   if (speedOn && currentMicros - prevspeedMicros >= 150) // only pulse for 150 microseconds
    {
    digitalWrite(speedPin, LOW);    // turn the speedo pin off by making the voltage LOW
    speedOn = false;
    }  
}

int getButton()   // this just returns how long the button was down, AFTER is goes high
  {               // you could probably do better with a interrupt, but <shrug> 
  unsigned long currentMillis = millis();
  if (digitalRead(buttonPin) == LOW)
    {
    if (!pressed)
      {buttonPressMillis = millis();}  
    pressed = true;
    digitalWrite(lightPin, HIGH);
    return -1;
    }
  else  
    {
    digitalWrite(lightPin, LOW); 
    if (pressed == true)
      {
      pressed = false;
      return int(currentMillis-buttonPressMillis);  
      }
    else
      {
      return 0;  
      }
    }
  }

void blinky()   //c'mon, this is obvious.  yeah, I should have used a loop, but copy/paste was quicker
  {
  digitalWrite(lightPin, HIGH);
  delay(250);
  digitalWrite(lightPin, LOW);
  delay(150);
  digitalWrite(lightPin, HIGH);
  delay(250);
  digitalWrite(lightPin, LOW);
  delay(150);
  digitalWrite(lightPin, HIGH);
  delay(250);
  digitalWrite(lightPin, LOW);  
  }

