
const int rpmPin = 3; // yellow wire

unsigned long prevrpmMicros = 0;        
unsigned long prevspeedMicros = 0;

int long rpmInterval;           // interval at which to blink
unsigned long currentMicros = 0;
unsigned long rpmOnMicros = 0;
bool rpmOn = false;


float hourFrequencies[] = {2.0,16.5,32.0,49.5,65.0,82.5,100.0,118.5,136.0,154.0,172.0,189.0,218.0};
float hourFreq = 1.0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("hello, world!");
  pinMode(rpmPin, OUTPUT);

}

  void loop() {
    while (Serial.available() > 0) {
    hourFreq = Serial.parseInt();
    rpmInterval = 1000000.0/hourFreq;
    Serial.print(hourFreq);
    Serial.print("  ");  
    Serial.println(rpmInterval);
    }

  currentMicros = micros();
  if (!rpmOn && currentMicros - prevrpmMicros >= long(rpmInterval))
    {
    // save the last time you blinked the LED
    prevrpmMicros = currentMicros;
    rpmOnMicros = currentMicros;
    digitalWrite(rpmPin, HIGH);   // turn the LED on (HIGH is the voltage level)
    rpmOn = true;
    }
  if (rpmOn && currentMicros - rpmOnMicros >= 150)
    {
//    rpmOnMicros = currentMicros;
    digitalWrite(rpmPin, LOW);   // turn the LED off (HIGH is the voltage level)
    rpmOn = false; 
    }
}
