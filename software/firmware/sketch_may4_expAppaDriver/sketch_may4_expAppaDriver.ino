#include <Arduino.h>
#include <Wire.h>    //include arduinos i2c library
#include <ESP32Time.h>
#include <Ezo_i2c.h> //include the EZO I2C library from https://github.com/Atlas-Scientific/Ezo_I2c_lib
#include <Ezo_i2c_util.h> //brings in common print statements


//esp internal clock
ESP32Time rtc;

//I2C port objects
#define DEFAULT_I2C_PORT Wire
#define SECONDARY_I2C_PORT Wire1

// device addresses
#define stock_pmp_addr 101

// device objects
Ezo_board stock_pmp = Ezo_board(stock_pmp_addr, "stock");                   //create a pump circuit object, whose address is 103 and name is "PMP_UP". This pump dispenses pH up solution.


//Pumps
int dt[] = {900000, 900000, 900000, 900000, 900000, 900000, 900000};
double dv[] = {28, 31, 35, 84, 53, 61};
int now, then, step, interval, offset, fresh=0, empty=0, stock=0;
int totalSteps = sizeof(dt) / sizeof(dt[0]);

//ADC
int bits;
float E,Vo;
//avg
int i=0;

// Example of a while loop with timeout
bool readWithTimeout(int timeout_ms) {
  unsigned long startTime = millis();
  
  // Loop until timeout is reached
  while ((millis() - startTime) < timeout_ms) {
    // Do your operation here
    if (Serial.available() > 0) {
      // Process data
      return true;  // Success
    }
    
    // Optional: add a small delay to prevent CPU hogging
    delay(1);
  }
  
  // If we get here, we timed out
  Serial.println("timeout");
  return false;  // Timeout occurred
}

void parseCmd(){
  String command = Serial.readStringUntil('\n');  // Read until newline character
  command.trim();  // Remove any whitespace
    
  // Process command
  if (command == "help") {
    Serial.println("PMP - forwards pump commands to the atlas scientific dosing pump");  
    Serial.println("  avaiable pump commands:");
    Serial.println("      D,[ml] dispense [this specific volume]");
    Serial.println("      D,* dispense until the stop command is given");
  } else if(command == "stock_pmp"){
  Serial.print("What is your stock pump command? ");
    if(readWithTimeout(10000)){
      command = Serial.readStringUntil('\n');
      command.trim();  // Remove any whitespace
      Serial.println(command);
      stock_pmp.send_cmd(command.c_str());
    }  

  } else if(command == "SKIP"){
    dt[step]=0;
  } else if(command == "OFF"){
    stock_pmp.send_cmd("X");
    step=totalSteps;
  } else{
    Serial.println("I do not understand you, type help for help.");
  }
}

void setup() {

  //Wire = Wire1;                           //if your using qt py
  //Wire.setPins(SDA1, SCL1);
  Wire.begin();                             //start the I2C
  Serial.begin(115200);                     //start the serial communication to the computer

  step = 0;
  then = millis();
  interval=millis();

}



void loop() {

  int now = millis();
  //bool ready = GSheet.ready();

  if(now-then>dt[step] && step<totalSteps){
    Serial.println("dosing step " + String(step) + " dispensing " + String(dv[step]));
    stock_pmp.send_cmd_with_num("d,-", dv[step]);
    stock+=dv[step];
    step++;
    then = millis();
  } 


  if (Serial.available() > 0) {
    parseCmd();
  }

  //averaging
  if(now-interval>1000){
    Serial.print(rtc.getTime());Serial.print(',');
    Serial.print(interval);Serial.print(',');
    Serial.print(step);Serial.print(',');
    Serial.println(stock);

    interval=millis();
  }


}

