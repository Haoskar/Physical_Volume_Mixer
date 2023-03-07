int pinA = 13; // Connected to CLK
int pinB = 12; // Connected to DT
int encoderPosCount = 0;
int pinALast;
int aVal;
boolean bCW;
void setup() {
pinMode (pinA,INPUT);
pinMode (pinB,INPUT);
/* Read Pin A
Whatever state it's in will reflect the last position
*/
pinALast = digitalRead(pinA);
Serial.begin (9600);
}
void loop() {
  aVal = digitalRead(pinA);
  if (aVal != pinALast){ // Means the knob is rotating
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    if (digitalRead(pinB) != aVal) { // Means pin A Changed first - We're Rotating Clockwise
      encoderPosCount ++;
      bCW = true;
    } else {// Otherwise B changed first and we're moving CCW
      bCW = false;
      encoderPosCount--;
      }
    delay(6);
    //Serial.print ("Rotated: ");
    /*if (bCW){
      Serial.println ("clockwise");
    }else{
      Serial.println("counterclockwise");
    }*/
    Serial.print("20,");
    Serial.println(bCW);
      //Serial.print("Encoder Position: ");
      //Serial.println(encoderPosCount);
  }
  pinALast = aVal;
}