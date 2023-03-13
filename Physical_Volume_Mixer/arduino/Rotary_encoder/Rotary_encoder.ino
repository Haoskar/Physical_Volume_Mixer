#include <LiquidCrystal.h> 

#define ARRAY_SIZE 10

int pinA_1 = 30; // Connected to CLK Grön
int pinB_1 = 31; // Connected to DT  Gul
//int encoderPosCount = 0;
int pinALast_1;
//int aVal;


int pinA_2 = 32; // Connected to CLK Grön
int pinB_2 = 33; // Connected to DT  Gul
int pinALast_2;

int pinA_3 = 34; // Connected to CLK Grön
int pinB_3 = 35; // Connected to DT  Gul
int pinALast_3;


//This is to move the array index it sends to the right on the number line ex: array_offset + i
int button_right = 22;
int button_right_val = 0;
int button_right_prev_val = -1;

//This is to move the array index it sends to the left on the number line ex: array_offset + i
int button_left = 24;
int button_left_val = 0;
int button_left_prev_val = -1;

unsigned long button_left_lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long button_right_lastDebounceTime = 0;  // the last time the output pin was toggled

const unsigned long debounceDelay = 800;    // the debounce time; increase if the output flickers

int array_offset = 0;

const int RS = 4, EN = 5, D4 = 10, D5 = 11, D6 = 12, D7 = 13;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);


String programs[100]; 
int programsSize = 0;

int displayIndex = 0; //This is what array_offset is displayed right now
int oldProgramSize = 0;

void setup() {
  pinMode (pinA_1,INPUT);
  pinMode (pinB_1,INPUT);

  pinMode (pinA_2,INPUT);
  pinMode (pinB_2,INPUT);

  pinMode (pinA_3,INPUT);
  pinMode (pinB_3,INPUT);

  pinMode(button_right, INPUT);
  pinMode(button_left, INPUT);

  /* Read Pin A
  Whatever state it's in will reflect the last position
  */
  pinALast_1 = digitalRead(pinA_1);
  pinALast_2 = digitalRead(pinA_2);
  pinALast_2 = digitalRead(pinA_3);
  Serial.begin (9600);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Hello World!");
  lcd.setCursor(0, 1); 
  lcd.print("Oskar");

  Serial.println("STARTING");
}

void rotary_encoder_read(int pinA, int pinB, int *pinALast, int index, int offset){
  boolean bCW;
  int aVal = digitalRead(pinA);
  if(aVal != *pinALast){
    if(digitalRead(pinB) != aVal){
      //Clockwise
      bCW = true;
    }
    else{
      //Counter Clockwise
      bCW = false;      
    }

    Serial.print("Index: ");
    Serial.print(index + offset);
    Serial.print(" Clockwise: ");
    Serial.println(bCW ? "True" : "False");

    delay(6);
  }
  *pinALast = aVal;
}

void displayTextOnLCD(){
  //do not update unless something has changed!
  if(displayIndex == array_offset && programsSize == oldProgramSize)
    return;
  //something changed updating
  lcd.clear();
  for(int i = 0; i < 2; i++){
    lcd.setCursor(0, i);
    displayIndex = array_offset;
    oldProgramSize = programsSize;
    if(i + array_offset >= programsSize || i + array_offset < 0){
      lcd.print("NOTHING");
    }
    else{
      lcd.print(programs[i + array_offset]);      
    }
  }
}

void loop(){

  if(Serial.available() > 0){
    String a = Serial.readString();

    programsSize = 0;
    int ssIndex = -1;
    while((ssIndex = a.indexOf(',')) != -1){
      programs[programsSize] = a.substring(0,ssIndex);
      programsSize++;
      a.remove(0,ssIndex+1);  
      Serial.println(a);    
    }

    a.replace("\n","\0");
    programs[programsSize] = a;
    programsSize++;
    //lcd.setCursor(0, 0);
    //lcd.clear();
    //lcd.print(programs[0]);
  }

  displayTextOnLCD();

  button_left_val = digitalRead(button_left);
  if(button_left_val != button_left_prev_val && (button_left_lastDebounceTime - millis()) > debounceDelay)
  {
    button_left_prev_val = button_left_val;
    if(array_offset <= 0){
      Serial.println("Can't switch left!");
    }else{
      Serial.print("Switching left: ");
      array_offset--;
      Serial.println(array_offset);
    }
    
  }
  
  button_right_val = digitalRead(button_right);
  if(button_right_val != button_right_prev_val && (button_right_lastDebounceTime - millis()) > debounceDelay)
  {
    button_right_prev_val = button_right_val;
    if(array_offset > ARRAY_SIZE){
      Serial.println("Can't switch right!");
    }else{
      Serial.print("Switching right: ");
      array_offset++;
      Serial.println(array_offset);
      
    }
    
  }


  rotary_encoder_read(pinA_1, pinB_1, &pinALast_1, 0, 0);
  rotary_encoder_read(pinA_2, pinB_2, &pinALast_2, 1, array_offset);
  rotary_encoder_read(pinA_3, pinB_3, &pinALast_3, 2, array_offset);
}

/*
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

    Serial.print("20,");
    Serial.println(bCW);
      //Serial.print("Encoder Position: ");
      //Serial.println(encoderPosCount);
  }
  pinALast = aVal;
}*/