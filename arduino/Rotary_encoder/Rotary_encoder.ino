#include <LiquidCrystal.h> 

#define ARRAY_SIZE 10

typedef struct __a{
  int pinA;
  int pinB;
  int pinALast;
  unsigned long lastTime;
}RotaryEncoder;

//Constants
const int PIN_A_1 = 30;
const int PIN_B_1 = 31;

const int PIN_A_2 = 38;
const int PIN_B_2 = 39;

const int PIN_A_3 = 50;
const int PIN_B_3 = 51;
RotaryEncoder rots[3];

const unsigned long ROTARY_ENCODER_DEBOUNCE_DELAY = 6;

//This is to move the array index it sends to the right on the number line ex: array_offset + i
int button_right = 35;
int button_right_val = 0;
int button_right_prev_val = -1;

//This is to move the array index it sends to the left on the number line ex: array_offset + i
int button_left = 47;
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

RotaryEncoder rotary_init(int pinA, int pinB){
  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);

  RotaryEncoder r;
  r.pinA = pinA;
  r.pinB = pinB;
  r.lastTime = 0;
  r.pinALast = digitalRead(pinA);

  return r;
}

void setup() {

  //Rotary Encoders
  rots[0] = rotary_init(PIN_A_1, PIN_B_1);
  rots[1] = rotary_init(PIN_A_2, PIN_B_2);
  rots[2] = rotary_init(PIN_A_3, PIN_B_3);


  pinMode(button_right, INPUT);
  pinMode(button_left, INPUT);

  /* Read Pin A
  Whatever state it's in will reflect the last position
  */
  Serial.begin (9600);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Hello World!");
  lcd.setCursor(0, 1); 
  lcd.print("Oskar");

}

void rotary_encoder_read(RotaryEncoder *r, int index, int offset){
  if( (r->lastTime - millis()) <= ROTARY_ENCODER_DEBOUNCE_DELAY )
    return;


  boolean bCW;
  int aVal = digitalRead(r->pinA);
  if(aVal != r->pinALast){
    if(digitalRead(r->pinB) != aVal){
      //Clockwise
      bCW = true;
    }
    else{
      //Counter Clockwise
      bCW = false;      
    }

    Serial.print(index + offset);
    Serial.print(",");
    Serial.println(bCW);
    r->lastTime = millis();
  }
  r->pinALast = aVal;
  
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
  
  
  while(Serial.available() > 0){
    String a = Serial.readStringUntil('\n');

    if(a.compareTo(String("s")) == 0){
      programsSize = 0;
      continue;
    }
    else if(a.compareTo(String("e")) == 0){
      break;
    }    
    programs[programsSize] = a;
    programsSize++;
  }
  

  displayTextOnLCD();

  button_left_val = digitalRead(button_left);
  if(button_left_val != button_left_prev_val && (button_left_lastDebounceTime - millis()) > debounceDelay)
  {
    button_left_prev_val = button_left_val;
    if(array_offset <= 0){
    }else{
      array_offset--;
    }
    
  }
  
  button_right_val = digitalRead(button_right);
  if(button_right_val != button_right_prev_val && (button_right_lastDebounceTime - millis()) > debounceDelay)
  {
    button_right_prev_val = button_right_val;
    if(array_offset > ARRAY_SIZE){
    }else{
      array_offset++;
      
    }
    
  }


  rotary_encoder_read(&rots[0], 0, 0);
  rotary_encoder_read(&rots[1], 1, array_offset);
  rotary_encoder_read(&rots[2], 2, array_offset);
}
