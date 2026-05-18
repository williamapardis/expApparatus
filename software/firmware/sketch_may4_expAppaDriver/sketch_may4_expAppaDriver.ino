//high level help
#include <TaskManagerIO.h>

//communictation
#include <OneWire.h>

//hardware 
#include <SparkFun_ADS1219.h>
#include <DallasTemperature.h>




// Potentiostat ADC 
SfeADS1219ArdI2C pStat;

ads1219_input_multiplexer_config_t muxInputs[] = {	// pStat channels
  ADS1219_CONFIG_MUX_SINGLE_0,
  ADS1219_CONFIG_MUX_SINGLE_1,
  ADS1219_CONFIG_MUX_SINGLE_2,
  ADS1219_CONFIG_MUX_SINGLE_3,
  ADS1219_CONFIG_MUX_SHORTED
};

float E0,E1,E2,E3;	// ADC Variables
int sampleCount;		// count for averaging


// Temperature sensor
OneWire oneWire(ONE_WIRE_BUS); 				// 1wire communication obj.
const int ONE_WIRE_BUS = 2;  					// data pin
DallasTemperature sensors(&oneWire);	// sensor obj.
sensors.setWaitForConversion(false); 	// report last value, do not wait for conversion



// This is the recovery of the Es from the ADC
void sample(){

	// there will be some kind of loop here
	E0 += ADCreading;// E1, E2, E3
	sampleCount++;

}

// I think I will collect temperature and report it in this funct. 
void report(){

	sensors.requestTemperatures();               // ask sensor(s) to measure
  float tempC = sensors.getTempCByIndex(0);    // first sensor on the bus
  Serial.print(tempC);Serial.print(',');
  
  // averaging
  E0 /= sampleCount;E1 /= sampleCount;
  E2 /= sampleCount;E3 /= sampleCount;
  
  Serial.print(E0);Serial.print(',');
  Serial.print(E1);Serial.print(',');
  Serial.print(E2);Serial.print(',');
  Serial.print(E3);Serial.print(',');
  
  E0 = 0;E1 = 0;E2 = 0;E3 = 0;
  sampleCount=0;

}

void setTemperature(){

// this is the RS232 communication with the circulator
Serial1.print("RS0200\r");

}

void titrate(){

	taskManager.scheduleOnce(30,stockAdd,TIME_MINUTES);

}

void stockAdd(){

	//pumpy business

	if(not at the last step){
		taskManager.scheduleOnce(30,stockAdd,TIME_MINUTES);
	}
	else{
		taskManager.scheduleOnce(30,empty,TIME_MINUTES);
		taskManager.scheduleOnce(30+timeItTakesToEmpty,dilute,TIME_MINUTES);
	}

}

void setup(){

	Serial.begin(115200);
	Serial1.begin(9600);
	
  sensors.begin();

	//scheduling temperature steps and setting the initial temperature
	taskManager.scheduleFixedRate(120,setTemperature,TIME_MINUTES);
	setTemperature();
	
	//scheduling titration with a delay for thermal equilibration
	taskManager.scheduleOnce(thermalDelay,[](){
		
		taskManager.scheduleFixedRate(120,titration,Time_MINUTES);
		
	},TIME_MIMUTES);
	
	//sampling and reporting sensor data
	taskManager.scheduleFixedRate(100,sample,TIME_MILLISECONDS);
	taskManager.scheduleFixedRate(1,report,TIME_SECONDS);

	E0 = 0;E1 = 0;E2 = 0;E3 = 0;
  sampleCount=0;

}

void main(){

	taskManager.runLoop();

}