#include <Arduino.h>

#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = A3;
const int LOADCELL_SCK_PIN = A2;

int32_t zero_factor = 0;

int32_t calibration_factor = 60000;

HX711 scale;

typedef enum event{
  NO_EVENT = -1,
  ZERO_FACTOR,
  AUTO_CAL_FACTOR,
  CONFIG_ZERO_FACTOR,
  CONFIG_CAL_FACTOR,
  READ_VAL
} input_event;

input_event event = NO_EVENT;

void hardwareConfig(){
  Serial.begin(9600);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void checkLoadCellReady(){
  while(true){
    if(scale.is_ready()) Serial.println("HX711 ready!"); return;
    Serial.println("HX711 not found ..");
    delay(1000);
  }
}

float getUnitKg(){
  return(scale.get_units() * 0.453592);
}

void setup() {
  hardwareConfig();
  checkLoadCellReady();
  Serial.println("Setup completed");
}

input_event handleInput(){
  Serial.println("======================================================");
  Serial.println("Enter 1 to find and auto set zero factor (Please remove item from scale first)");
  Serial.println("Enter 2 to auto calibration factor mode");
  Serial.println("Enter 3 to manually config zero factor");
  Serial.println("Enter 4 to manually config calibration factor");
  Serial.println("Enter 5 to show weight on scales");
  Serial.println("======================================================");

  while(true){
    if(Serial.available()){
      char input_val = Serial.read();
      switch(input_val){
        case '1': return ZERO_FACTOR;
        case '2': return AUTO_CAL_FACTOR;
        case '3': return CONFIG_ZERO_FACTOR;
        case '4': return CONFIG_CAL_FACTOR;
        case '5': return READ_VAL;
        default: Serial.println("Error no command found! Please enter command again"); break;
      }
    }
  }
}

void zeroFactorEvent(){
  Serial.println("Find zero Factor .......");
  scale.set_scale();
  scale.tare();
  zero_factor = scale.read_average(20);
  Serial.println("=== Find zero factor finished ===");
  Serial.print("Zero factor now = ");
  Serial.println(zero_factor);
}

float _getInput(){
  while(true){
    if(Serial.available()){
      String string_weight = Serial.readString();
      return string_weight.toFloat();
    }
  }
}

void calFactorEvent(){
  Serial.println("Please enter real weight to auto calibration");
  Serial.println("Besure to put weight on scale first before enter value");
  float target_weight = _getInput();
  Serial.print("Start calibrating at target weight: ");
  Serial.println(target_weight);

  // To convert floating point to decimal point for easy calculation
  uint8_t dec_point = 1;
  const int DEC_POINT = 2;
  for( uint8_t i=1; i<DEC_POINT+1; i++ ){
    dec_point = dec_point*10;
  }
  while(true){
    scale.set_scale(calibration_factor);
    float read_weight = getUnitKg();
    String data = String(read_weight, DEC_POINT);
    Serial.print(data);
    Serial.print(" kg"); 
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();
    long real_weight_dec_point = (target_weight*dec_point);
    long read_weight_dec_point = read_weight*dec_point;
    Serial.print(real_weight_dec_point);
    Serial.print(" , ");
    Serial.println(read_weight_dec_point);

    if (read_weight_dec_point == 0){
      Serial.println("Infinity calibration detect! will return to menu (ABORT)");
      return;
    }

    long different_error;

    if (real_weight_dec_point == read_weight_dec_point){

      Serial.println("=== Calibration finished ===");
      Serial.print("Calibrate factor now = ");
      Serial.println(calibration_factor);
      return;

    } else if (real_weight_dec_point > read_weight_dec_point) {

      different_error = real_weight_dec_point - read_weight_dec_point;
      if (different_error > 100) calibration_factor -= 1000;
      else if (different_error > 10) calibration_factor -= 10;
      else calibration_factor -= 1;

    } else if (real_weight_dec_point < read_weight_dec_point) {
      different_error = read_weight_dec_point - real_weight_dec_point;
      if (different_error > 100) calibration_factor += 1000;
      else if (different_error > 10) calibration_factor += 10;
      else calibration_factor += 1;
    }
}
}

void configZeroEvent(){
  Serial.println("Please enter zero factor (float)");
  Serial.print("Current value is: ");
  Serial.println(zero_factor);
  float zero_config = _getInput();
  Serial.println("Zero factor will change from ");
  Serial.print(zero_factor);
  Serial.print(" to => ");
  Serial.println(zero_config);
  zero_factor = zero_config;
  return;
}

void configCalibrateEvent(){
  Serial.println("Please enter Calibrate factor (float)");
  Serial.print("Current value is: ");
  Serial.println(calibration_factor);
  float calibration_config = _getInput();
  Serial.println("Calibrate factor will change from ");
  Serial.print(calibration_factor);
  Serial.print(" to => ");
  Serial.println(calibration_config);
  calibration_factor = calibration_config;
  return;
}

bool _isExitLoopEvent(){
  if(Serial.available()){
    char cmd = Serial.read();
    if (cmd == 'x' || cmd == 'X') return true;
  }
  return false;
}

void readEvent(){
  Serial.println("Start read loop enter 'x' to return to menu");
  scale.set_scale(calibration_factor);
  while(true){
    const uint32_t interval_timer = 500;
    static uint32_t last_time = millis();
    if (_isExitLoopEvent()) return;
    if(millis() - last_time >= interval_timer){
      Serial.print("Weight: ");
      Serial.print(getUnitKg());
      Serial.println(" kg");
      last_time = millis();
    }
  }
}

void handleEvent(input_event event_val){
  switch(event_val){
    case ZERO_FACTOR: zeroFactorEvent(); break;
    case AUTO_CAL_FACTOR: calFactorEvent(); break;
    case CONFIG_ZERO_FACTOR: configZeroEvent(); break;
    case CONFIG_CAL_FACTOR: configCalibrateEvent(); break;
    case READ_VAL: readEvent(); break;
    default: Serial.println("Error please try again"); break;
  }
}


void loop() {
  input_event event = handleInput();
  handleEvent(event);
}
