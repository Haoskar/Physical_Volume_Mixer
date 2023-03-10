int pinA_1 = 13; // Connected to CLK Grön
int pinB_1 = 12; // Connected to DT  Gul
//int encoderPosCount = 0;
int pinALast_1;
//int aVal;


int pinA_2 = 10; // Connected to CLK Grön
int pinB_2 = 9; // Connected to DT  Gul
int pinALast_2;

int pinA_3 = 7; // Connected to CLK Grön
int pinB_3 = 6; // Connected to DT  Gul
int pinALast_3;


//This is to move the array index it sends to the right on the number line ex: pot_arr_offset + i
int button_right = 12;
int button_right_val = 0;
int button_right_prev_val = -1;

//This is to move the array index it sends to the left on the number line ex: pot_arr_offset + i
int button_left = 10;
int button_left_val = 0;
int button_left_prev_val = -1;

unsigned long button_left_lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long button_right_lastDebounceTime = 0;  // the last time the output pin was toggled

unsigned long debounceDelay = 500;    // the debounce time; increase if the output flickers

int pot_arr_offset = 0;


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
}

void rotary_encoder_read(int pinA, int pinB, int *pinALast, int index){
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
    Serial.print(index);
    Serial.print(" Clockwise: ");
    Serial.println(bCW ? "True" : "False");

    delay(6);
  }
  *pinALast = aVal;
}

void loop(){

    button_left_val = digitalRead(button_left);
  if(button_left_val != button_left_prev_val && (button_left_lastDebounceTime - millis()) > debounceDelay)
  {
    button_left_prev_val = button_left_val;
    if(pot_arr_offset <= 0){
      //Serial.println("Can't switch left!");
    }else{
      //Serial.println("Switching left");
      pot_arr_offset--;
      //Serial.println(pot_arr_offset);
    }
    
  }
  
  button_right_val = digitalRead(button_right);
  if(button_right_val != button_right_prev_val && (button_right_lastDebounceTime - millis()) > debounceDelay)
  {
    button_right_prev_val = button_right_val;
    if(pot_arr_offset > NUM_POT_PREV_VAL - POT_PORTS - 1){
      //Serial.println("Can't switch right!");
    }else{
      //Serial.print("Switching right: ");
      pot_arr_offset++;
      //Serial.println(pot_arr_offset + 2);
      
    }
    
  }


  rotary_encoder_read(pinA_1, pinB_1, &pinALast_1, 0);
  rotary_encoder_read(pinA_2, pinB_2, &pinALast_2, 1);
  rotary_encoder_read(pinA_3, pinB_3, &pinALast_3, 2);
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