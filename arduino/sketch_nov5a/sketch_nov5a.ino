#define POT_PORTS 3
#define NUM_POT_PREV_VAL 13
//This is to move the array index it sends to the right on the number line ex: pot_arr_offset + i
int button_right = 12;
int button_right_val = 0;
int button_right_prev_val = -1;

//This is to move the array index it sends to the left on the number line ex: pot_arr_offset + i
int button_left = 11;
int button_left_val = 0;
int button_left_prev_val = -1;


int pot_arr_offset = 0;

int pot_ports[] = {A0, A1, A2};
int pot_val = -1;
int pot_prev_val[NUM_POT_PREV_VAL] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

unsigned long button_left_lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long button_right_lastDebounceTime = 0;  // the last time the output pin was toggled

unsigned long debounceDelay = 500;    // the debounce time; increase if the output flickers

void setup() {
  Serial.begin(9600);
  pinMode(button_right, INPUT);
  pinMode(button_left, INPUT);
}

void loop() {
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


  for(int i = 0; i < POT_PORTS; i++)
  {
    pot_val = analogRead(pot_ports[i]);
    pot_val = map(pot_val, 0, 1023, 0, 100);
    if(i == 0)
    {
      if(  (   (pot_val > pot_prev_val[i] || pot_val < pot_prev_val[i] || pot_prev_val[i] == -1) &&  pot_val % 2 == 0  ) )
      {
        pot_prev_val[i] = pot_val;

          char s[100]; 
          sprintf(s,"%d,%03d", i, pot_val);
          Serial.println(s);
      }
    }
    else if(   (pot_val > pot_prev_val[i + pot_arr_offset] || pot_val < pot_prev_val[i + pot_arr_offset] || pot_prev_val[i + pot_arr_offset] == -1) 
               &&  pot_val % 2 == 0)
      {
        pot_prev_val[i + pot_arr_offset] = pot_val;

        char s[100]; 
        sprintf(s,"%d,%03d", i + pot_arr_offset, pot_val);
        Serial.println(s);
      }
  }
}
