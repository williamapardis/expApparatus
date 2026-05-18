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
#define stock_pmp_addr 103
#define fresh_pmp_addr 99
#define empty_pmp_addr 101

// device objects
Ezo_board stock_pmp = Ezo_board(stock_pmp_addr, "stock");                   //create a pump circuit object, whose address is 103 and name is "PMP_UP". This pump dispenses pH up solution.
Ezo_board fresh_pmp = Ezo_board(fresh_pmp_addr, "fresh"); 
Ezo_board empty_pmp = Ezo_board(empty_pmp_addr, "empty"); 

//Pumps {thermal equilibration, dose 1, dose 2, dose n... empty, DI}
// int dt[] = {10000, 10000, 10000, 10000, 20000, 10000};
// double dv[] = {1, 1, 1, 10, 5};
int dt[] = {3600000, 2700000, 2700000, 2700000, 600000, 300000};
double dv[] = {71, 94, 132, 726, 429};
int now, then, step, interval, offset, fresh=0, empty=0, stock=0;
int totalSteps = sizeof(dv) / sizeof(dv[0]);
int titrationSteps = totalSteps-2;

//Temperature Setpoints
int Tset[] = {15, 25, 35};
int tempSteps = sizeof(Tset) / sizeof(Tset[0]);
int Tj=0;

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
		fresh_pmp.send_cmd("X");
		empty_pmp.send_cmd("X");
    step=totalSteps+1;
	} else if(command == "RESET"){
		step = 0;
		Tj = 0;
  } else{
    Serial.println("I do not understand you, type help for help.");
  }
}

void setup() {

  //Wire = Wire1;                           //if your using qt py
  //Wire.setPins(SDA1, SCL1);
  Wire.begin();                             //start the I2C
  Serial.begin(115200);                     //start the serial communication to the computer
	Serial1.begin(9600);

  step = 0;
  then = millis();
  interval=millis();

	//changing temperature setpoint
	Serial1.print("RS0"+String(Tset[Tj])+"0C\r");
	//printing action
	Serial.print(rtc.getTime());Serial.print(',');
	Serial.println("temperature set " + String(Tj) + " set to " + String(Tset[Tj]));
	//indexing
	Tj++;
}



void loop() {

  int now = millis();
  //bool ready = GSheet.ready();
	if(step<titrationSteps){
		if(now-then>dt[step]){

			//dosing
			stock_pmp.send_cmd_with_num("d,-", dv[step]);
			stock+=dv[step];
			
			//printing action
			Serial.print(rtc.getTime());Serial.print(',');
			Serial.println("stock step " + String(step) + " dispensing " + String(dv[step]));

			//indexing
			step++;
			then = millis();
			
		} 
	} else if(now-then>dt[step] && step == totalSteps-2){
		//set new temperature setpoint so we can begin equilibrating
		Serial1.print("RS0"+String(Tset[Tj])+"0C\r");
		
		//printing action
		Serial.print(rtc.getTime());Serial.print(',');
		Serial.println("temperature set " + String(Tj) + " set to " + String(Tset[Tj]));
		Tj++;

		//remove excess solution so I can prepare for dillution
		empty_pmp.send_cmd_with_num("d,-", dv[step]);
		empty+=dv[step];

		//printing action
		Serial.print(rtc.getTime());Serial.print(',');
		Serial.println("empty step " + String(step) + " dispensing " + String(dv[step]));

		//indexing
		step++;
		then = millis();

	} else if(now-then>dt[step] && step == totalSteps-1){
		//dillute
		fresh_pmp.send_cmd_with_num("d,-", dv[step]);
		fresh+=dv[step];

		//printing action
		Serial.print(rtc.getTime());Serial.print(',');
		Serial.println("fresh step " + String(step) + " dispensing " + String(dv[step]));

		//indexing
		step++;
		then = millis();

	} else if(now-then>dt[step] && step == totalSteps && Tj<tempSteps){
		step=0;
		then = millis();
	}

  if (Serial.available() > 0) {
    parseCmd();
  }

  //averaging
  if(now-interval>10000){
    Serial.print(rtc.getTime());Serial.print(',');
    Serial.print(dt[step]);Serial.print(',');
		Serial.print(now-then);Serial.print(',');
    Serial.print(step);Serial.print(',');
		Serial.print(stock);Serial.print(',');
		Serial.print(fresh);Serial.print(',');
		Serial.print(empty);Serial.print(',');
		Serial.println(Tset[Tj-1]);

    interval=millis();
  }


}

