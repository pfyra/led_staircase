#include <EEPROM.h>
#include <Time.h>
#include "libs/NewRemoteSwitch/NewRemoteReceiver.h"

#if 1
#define SERIAL_BEGIN(x) Serial.begin(x)
#define PRINT(x) Serial.print(x)
#define PRINTLN(x) Serial.println(x)
#else
#define SERIAL_BEGIN(x)
#define PRINT(x)
#define PRINTLN(x)
#endif

/*
The available states of the leds are:
  * always off, leds are shut off
  * always on, leds are dimmed to the level referred to as 'dim level' below
  * motion sensor, leds are normally dimmed to 'minimum dim level' and increased to 'dim level' when motion is detected
*/

/*
Serial commands:
 'u' / 'd'  increase / decrease the dim level
 'p' / 'o'  increase / decrease the minimum dim level
 'l' / 'k'  increase / decrease time leds are lit when motion sensor is triggered
 'b' / 'n'  increase / decrease the sleep time in loop (higher = slower dim)
 's'        save the dim level, lit time, etc etc (else changes are lost upon reboot)
 'm'        cycle through the states
 'r'        toggle photoresistor readings
*/


enum {
  EEPROM_ADDR_DIM_LVL = 0,  /* dim level when motion sensor is used or static state                  */
  EEPROM_ADDR_STATE,        /* state to startup at                                                   */
  EEPROM_ADDR_LIT_TIME,     /* time that leds are on before dimming down                             */
  EEPROM_ADDR_MIN_DIM_LVL,  /* minimum dim level when motion sensor is used but no motion registered */
  EEPROM_ADDR_SLEEP_TIME    /* sleep time in the loop(), higher = slower dim                         */
};

enum AnalogPin {
  APIN_PHOTORESISTOR = 0,
  APIN_1,
  APIN_2,
  APIN_3,
  APIN_4,
  APIN_5
};


enum DigitalPin {
  PIN_0 = 0,
  PIN_1,
  PIN_RF_RECV,
  PIN_MOTION_SENSOR,
  PIN_4,
  PIN_PIR,
  PIN_6,
  PIN_7,
  PIN_8,
  PIN_9,
  PIN_LEDS,
  PIN_11,
  PIN_12,
  PIN_ACTIVITY_LED
};

enum InterruptPins {
  INT_RF_RECV = 0,
  INT_MOTION_SENSOR
};

enum state_e{
  STATE_STATIC_OFF = 0,
  STATE_STATIC_ON,
  STATE_MOTION_SENSOR,
  STATE_END_OF_LIST
};

// "e2c": enum -> characters
static const char* state_e2c(enum state_e s){
  switch(s){
  case STATE_STATIC_OFF:    return "off";
  case STATE_STATIC_ON:     return "on";
  case STATE_MOTION_SENSOR: return "sensor";
  default:                  return "err";
  }
}

uint8_t current_dim_level = 0;
uint8_t target_dim_level = 0;
uint8_t min_dim_level = 0;
//uint8_t motion_dim_level = 0;  // when motion sensor is used
uint16_t target_dim_level_calculation(void);

volatile time_t time_motion = 0;

state_e state = STATE_STATIC_OFF;

bool activity_led_lit = false;
time_t activity_led_lit_since = 0;

uint8_t lit_time = 0; //time leds are lit when motion sensor activates

uint8_t sleep_time = 10;

bool photoresistor_readings = false;
time_t photoresistor_read_time = 0;
void photoresistor(void);
uint16_t photoresistor_value = 0;


void serial();

void activity_led_on(){
  activity_led_lit = true;
  digitalWrite(PIN_ACTIVITY_LED, HIGH);
  activity_led_lit_since = millis();
}

void activity_led_off(){
  activity_led_lit = false;
  digitalWrite(PIN_ACTIVITY_LED, LOW);
  activity_led_lit_since = 0;
}
#if 0
void rf_callback(NewRemoteCode code){
/*
  PRINTLN(code.address);
  PRINTLN(code.groupBit);
  PRINTLN(code.switchType);
  PRINTLN(code.unit);
  PRINTLN(code.dimLevelPresent);
  PRINTLN(code.dimLevel);
*/

  // 32847562 is the dimmer
  // 25324145 is the "dimmer" for lit_time
  // 14621782 is the motion sensor on/off


  if (code.address == 14621782){
    switch(code.switchType){
    case NewRemoteCode::off:
      PRINTLN("MOTION OFF");
      state = STATE_STATIC_OFF;
      break;
    case NewRemoteCode::on:
      PRINTLN("MOTION ON");
      state = STATE_MOTION_SENSOR;
      break;
    default:
      break;
    }
    activity_led_on();

  }else if (code.address == 25324145){
    lit_time = code.dimLevel*4;
    PRINT("New dim time for motion sensor: ");
    PRINT(lit_time);
    EEPROM.write(EEPROM_ADDR_LIT_TIME, lit_time);
    activity_led_on();
  }else if (code.address == 32847562){
    motion_dim_level = code.dimLevel*16;
    PRINT("New dim level: ");
    PRINTLN(motion_dim_level);
    EEPROM.write(EEPROM_ADDR_DIM_LVL, motion_dim_level);

    if (STATE_MOTION_SENSOR != state){
      // activate leds unless motion sensor is used
      state = STATE_STATIC_ON;
      target_dim_level = code.dimLevel*16;
    }
    activity_led_on();
  }
  EEPROM.write(EEPROM_ADDR_STATE, state);
}
#endif

void motion_sensor_interrupt(){
  time_motion = now();
}

void setup(){

  SERIAL_BEGIN(115200);
  PRINTLN("booting...");
  pinMode(PIN_ACTIVITY_LED, OUTPUT);
  digitalWrite(PIN_ACTIVITY_LED, HIGH);

  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_LEDS, OUTPUT);
/*
  motion_dim_level = EEPROM.read(EEPROM_ADDR_DIM_LVL);
  PRINT("max dim level: ");
  PRINTLN(motion_dim_level);
*/
 // NewRemoteReceiver::init(INT_RF_RECV, 2, &rf_callback);

  attachInterrupt(INT_MOTION_SENSOR, motion_sensor_interrupt, HIGH);

  state = (state_e)EEPROM.read(EEPROM_ADDR_STATE);
  PRINT("state: ");
  PRINTLN(state);

  lit_time = EEPROM.read(EEPROM_ADDR_LIT_TIME);
  PRINT("lit time:");
  PRINTLN(lit_time);

  if (STATE_STATIC_ON == state) {
    target_dim_level = target_dim_level_calculation();
  } else if (STATE_MOTION_SENSOR == state) {
    target_dim_level = min_dim_level;
  }

  sleep_time = EEPROM.read(EEPROM_ADDR_SLEEP_TIME);
  PRINT("sleep time:");
  PRINTLN(sleep_time);


  PRINTLN("done.");
  digitalWrite(PIN_ACTIVITY_LED, LOW);
}

void loop(){
  delay(sleep_time);

  if (current_dim_level == min_dim_level){
    digitalWrite(PIN_ACTIVITY_LED, LOW);
  }else{
    digitalWrite(PIN_ACTIVITY_LED, HIGH);
  }

  photoresistor();

  if (target_dim_level < current_dim_level){
    /* dim down */
    current_dim_level -= 2;
    analogWrite(PIN_LEDS, current_dim_level);
    PRINTLN(current_dim_level);
  }else if (target_dim_level > current_dim_level){
    /* dim up */
    current_dim_level += 2;
    analogWrite(PIN_LEDS, current_dim_level);
    PRINTLN(current_dim_level);
  } else {
    /* target dim level reached */
  }

  serial();

  switch(state){
  case STATE_STATIC_ON:

    break;
  case STATE_STATIC_OFF:
    target_dim_level = 0;
    break;
  case STATE_MOTION_SENSOR:
    target_dim_level = target_dim_level_calculation();
    break;
  default:
    PRINT("bad state: ");
    PRINTLN(state);
    break;
  }
}

void photoresistor(){
  if (current_dim_level == min_dim_level){
    photoresistor_value = analogRead(APIN_PHOTORESISTOR);
  }

  if (photoresistor_readings){
    if (photoresistor_read_time + 1 < now()){
      PRINTLN(photoresistor_value);
      photoresistor_read_time = now();
    }
  }
}

uint16_t target_dim_level_calculation(void){

  if (now() >= time_motion + lit_time){
    /* dim down */
    /* this is the regular, idle, state of the stair case */
    return  min_dim_level;
  }

  /* 
   * Motion has been detected in the stair case, 
   * set appropriate dim level:
   */

  if (photoresistor_value > 90){
    /* the stair case is pretty lit up already, dont dim up */
    return min_dim_level;
  }


  /* calculate a nice dim target based on the available light */
  uint16_t target = 2 * photoresistor_value;

  /* limit the dim level. Too bright is not nice.. */
  if (target > 90){
    target = 90;
  } 

  if (photoresistor_value < 5){
    /* it is too dark, give the leds an extra boost */
    target = min_dim_level + 10;
  }

  target &= 0xfffe; // make even

  return target;
}

void serial(){
  while (Serial.available()){
    switch(Serial.read()){
    case 'u': /* 'u'p the dim level */
      if (target_dim_level < 254){
        target_dim_level += 2;
//        motion_dim_level += 2;
      }
      break;
    case 'p': /* u'p' the min dim level */
      if (min_dim_level < 254 /*&& min_dim_level < motion_dim_level*/){
        min_dim_level += 2;
        PRINT("min dim: ");
        PRINTLN(min_dim_level);
      }
      break;
    case 'd': /* 'd'own the dim level */
      if (target_dim_level > 1){
        target_dim_level -= 2;
//        motion_dim_level -= 2;
      }
      break;
    case 'o': /* d'o'wn the min dim level */
      if (min_dim_level > 1){
        min_dim_level -= 2;
        PRINT("min dim: ");
        PRINTLN(min_dim_level);
      }
      break;
    case 's': /* 's'ave the dim level */
      PRINTLN("dim level, lit time, state, sleep time saved");
//      EEPROM.write(EEPROM_ADDR_DIM_LVL, motion_dim_level);
      EEPROM.write(EEPROM_ADDR_LIT_TIME, lit_time);
      EEPROM.write(EEPROM_ADDR_STATE, state);
      EEPROM.write(EEPROM_ADDR_MIN_DIM_LVL, min_dim_level);
      EEPROM.write(EEPROM_ADDR_SLEEP_TIME, sleep_time);
      PRINTLN(EEPROM.read(EEPROM_ADDR_DIM_LVL));
      PRINTLN(EEPROM.read(EEPROM_ADDR_LIT_TIME));
      PRINTLN(EEPROM.read(EEPROM_ADDR_STATE));
      PRINTLN(EEPROM.read(EEPROM_ADDR_MIN_DIM_LVL));
      PRINTLN(EEPROM.read(EEPROM_ADDR_SLEEP_TIME));
      break;
    case 'l': /* 'l'onger lit time when motion sensor used */
      if (lit_time < 255){
        ++lit_time;
      }
      PRINT("Lit time: ");
      PRINTLN(lit_time);
      break;
    case 'k': /* 'k'ortare (shorter in swedish) lit time when motion sensor used */
      if (lit_time > 0){
        --lit_time;
      }
      PRINT("Lit time: ");
      PRINTLN(lit_time);
      break;
    case 'b': /* increase sleep time */
      if (sleep_time < 255){
        ++sleep_time;
      }
      PRINT("sleep time: ");
      PRINTLN(sleep_time);
      break;
    case 'n': /* shorten sleep time */
      if (sleep_time > 0){
        --sleep_time;
      }
      PRINT("sleep time: ");
      PRINTLN(sleep_time);
      break;
    case 'm': /* 'm'otion sensor on/off  (cycle the states)*/
      /* state = state + 1 % STATE_END_OF_LIST; doesn't work, hence: */
      switch (state){
      case STATE_STATIC_OFF:    state = STATE_STATIC_ON;     break;
      case STATE_STATIC_ON:     state = STATE_MOTION_SENSOR; break;
      case STATE_MOTION_SENSOR: state = STATE_STATIC_OFF;    break;
      default:                  state = STATE_STATIC_OFF;    break;
      }
      PRINT("state: ");
      PRINTLN(state_e2c(state));
      break;
    case 'r': /* toggle photo Resistor readings on/off */
      photoresistor_readings = !photoresistor_readings;
      PRINT("photoresistor readings: ");
      PRINTLN(photoresistor_readings);
      break;
    default:
      break;
    }
  }
}
