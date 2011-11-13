/*
XBeeRadio.h - Library for communicating with heterogenous 802.15.4 networks.
	Created by Vasileios Georgitzikis, November 23, 2010.
*/

#include "WProgram.h"
#include "HomeAuto.h"


	HomeAuto::HomeAuto()
{
	init("unknown");
}

HomeAuto::HomeAuto(char* name)
{
	init(name);
}

void HomeAuto::init(char name[])
{
	this->name = (char*) malloc(sizeof(char) * (strlen(name)+1));
	strcpy(this->name, name);	
	sensorsOffset=0;
}


void HomeAuto::setup(void)
{
	setup(DEFAULT_BAUDRATE);
}

void HomeAuto::setup(long baudrate)
{
	//Start the serial and inform everyone we are turned on
	Serial.begin(baudrate);
	Serial.print(STARTING, BYTE);
	
	for(int i = 0; i < sensorsOffset; i++)
	{
		sensors[i]->setup();
	}
}
//let the world know our current state
void HomeAuto::broadcastState(void)
{
	// int status_report = checking_externally ? external_light_status : lightState;
	int status_report = 0;
	if(status_report)
		Serial.print(STATE_ON, BYTE);
	else
		Serial.print(STATE_OFF, BYTE);
}

//this is our listener function
void HomeAuto::checkForMessages(void)
{
	if (int avail_bytes = Serial.available())
	{
		for(int i=0; i< avail_bytes; i++)
		{
		//if we have input, and that input is either OPEN or CLOSE,
		//then do so accordingly
			char bla = (char) Serial.read();
			if(bla == V2_HEADER)
			{
				Serial.println("Got Header");
				char bla = (char) Serial.read();
				if(bla > 47 && bla < 53)
				{
					Serial.print("Got Number: ");
					Serial.println(bla-48, DEC);
					int target = bla - 48;
					char message[10];
					for(int i = 0; i < 10 ; i++)
					{
						message[i] = (char) Serial.read();
						if(message[i] == V2_FOOTER)
						{
							Serial.println("Got Footer");
							message[i]= '\0';
							break;
						}
					}
					Serial.print(message);
					sensors[target]->setState(message);
				}
			}
		}
	}
}

void HomeAuto::addSensor(Sensor* newSensor)
{
	if(sensorsOffset == 5)
		return;

	uint8_t oldUsedPins[25];
	uint8_t oupOffset = 0;

	for(int i = 0; i < sensorsOffset; i++)
	{
		Sensor* tmpSensor = sensors[i];
		pins tmpUsedPins = tmpSensor->usedPins();
		for(int j = 0; j < tmpUsedPins.numOfPins ; j++)
		{
			oldUsedPins[oupOffset++] = tmpUsedPins.pinsUsed[j];
		}
	}
	// Serial.print("Total old used pins: ");
	// Serial.println(oupOffset, DEC);

	pins usedPins = newSensor->usedPins();
	// Serial.println(usedPins.numOfPins, DEC);
	for(int i = 0; i < usedPins.numOfPins; i++)
	{
		// Serial.println(usedPins.pinsUsed[i], DEC);
		for(int j = 0; j < oupOffset; j++)
			if(oldUsedPins[j] == usedPins.pinsUsed[i])
			{
				// Serial.print("aborting, found used pin:");
				// Serial.println(usedPins.pinsUsed[i], DEC);
				break;
			}
	}
	// Serial.print("New total pins: ");
	// Serial.println(oupOffset+usedPins.numOfPins, DEC);

	sensors[sensorsOffset++] = newSensor;
}

void HomeAuto::check(void)
{
	for(int i = 0; i < sensorsOffset; i++)
	{
		sensors[i]->check();
		if(sensors[i]->mustBroadcast)
		{
			// Serial.print("Sensor: ");
			// Serial.println(i);
			sensors[i]->mustBroadcast=false;
			// BROADCAST STATE HERE **********$*#######***********************################
			Serial.print("R");
			Serial.print(i, DEC);
			Serial.print("F");
		}
	}
	checkForMessages();	
}

Sensor::Sensor()
{
	init("unknown");
}

Sensor::Sensor(char* name)
{
	init(name);
}

void Sensor::init(char name[])
{
	this->name = (char*) malloc(sizeof(char) * (strlen(name)+1));
	strcpy(this->name, name);
	mustBroadcast = false;	
	pullups = false;
}

pins Sensor::usedPins(void)
{
	pins retVal;
	retVal.numOfPins=0;
	return retVal;
}

void Sensor::setup()
{
	// Serial.println("LOLsetup");
}
void Sensor::check(void)
{
	// Serial.println("LOLcheck");
}
void Sensor::setState(char* newState)
{
	// Serial.println("LOLsetstate");
}
void Sensor::enablePullups(void)
{
	pullups = true;
}


Light::Light(uint8_t pin)
{
	init("unknown", pin, NO_SWITCH);
}

Light::Light(uint8_t pin, uint8_t pin2)
{
	init("unknown", pin, pin2);
}

Light::Light(char* name, uint8_t pin)
{
	init(name, pin, NO_SWITCH);
}

Light::Light(char* name, uint8_t pin, uint8_t pin2)
{
	init(name, pin, pin2);
}

void Light::init(char name[], uint8_t pin, uint8_t pin2)
{
	Sensor::init(name);
	lightPin = pin;
	light_switch = pin2;
	//the starting states are LOW
	lastSwitchState = LOW;
	lightState = LOW;
}

pins Light::usedPins(void)
{
	pins retVal;
	retVal.numOfPins=1;
	retVal.pinsUsed[0]=lightPin;
	if(light_switch > 1)
	{
		retVal.pinsUsed[1]=light_switch;
		retVal.numOfPins++;		
	}
	return retVal;
}

void Light::setup()
{
	//set thepins as input and output
	pinMode(lightPin, OUTPUT);
	if(light_switch > 1)
	{
		pinMode(light_switch, INPUT);
		if(pullups)
			digitalWrite(light_switch, HIGH);
		//check the light switch, and set it as the starting state
		//We do not care what state the light switch is in. We simply want
		//to check for changes in it state, so as to change the state of the
		//light accordingly. AKA if the light is on, and the lightSwitch is off
		//and changes to on, we turn the light off. This is done because the
		//two states are irrelevant, as the light can also be turned on and off
		//by the XBee remote.
		lastSwitchState = digitalRead(light_switch);
	}
}

void Light::check(void)
{
	do_check();
}

void Light::do_check()
{
//initialize timestamp variables
	static unsigned long timestamp = 0;
	if(light_switch > 1 && millis() - timestamp > period)
	{
		//if it's time to check the button, do so
		sampleButton();
		timestamp=millis();
	}
}

//Set the light to the value we are given
void Light::setLight(bool value)
{
//update the state, turn the pin on or off accordingly,
//and let everyone in the network know we have changed state
	digitalWrite(lightPin, value);
	lightState = value;
	// Serial.print("Light state is ");
	// Serial.println(lightState, DEC);
	// Serial.print("Light pin is ");
	// Serial.println(lightPin, DEC);
	mustBroadcast = true;
}

void Light::sampleButton(void)
{
//check the current state
	int currentSwitchState = digitalRead(light_switch);
	if(currentSwitchState != lastSwitchState)
	{
	//if the state has changed, reverse the light's state
		lastSwitchState = currentSwitchState;
		setLight(!lightState);
	}
}

void Light::setState(char* newState)
{
	if(newState[0] == V2_OPEN)
		setLight(1);
    else if(newState[0] == V2_CLOSE)
		setLight(0);	
}

// Shutters::Shutters(int light_pin1, int light_pin2, int light_switch1, int light_switch2)
// {
// 	init(light_pin1, light_pin2, light_switch1, light_switch2);
// }
// 
// 
// void Shutters::setup(void)
// {
// 	do_setup(DEFAULT_BAUDRATE);
// }
// void Shutters::setup(long baudrate)
// {
// 	do_setup(baudrate);
// }
// void Shutters::check(void)
// {
// 	  checkForMessages();
// 
// 	  int buttonState = checkForButtons();
// 
// 	//  if(buttonState != 0)
// 	//    Serial.print("Button State: ");
// 	//    Serial.println(buttonState, DEC);
// 
// 	  handleFSM(buttonState);
// 
// 	  delay(50);
// 
// 
// 	  static unsigned long broadcastTimestamp = 0;
// 	  if(millis() - broadcastTimestamp > broadcastPeriod)
// 	  {
// 	    //if it's time to broadcast our state, do so
// 	    broadcastTimestamp = millis();
// 	    broadcastState();
// 	  }
// }
// 
// void Shutters::init(int light_pin1, int light_pin2, int light_switch1, int light_switch2)
// {
// 	ledPin = light_pin1;
// 	ledPin2 = light_pin2;
// 	buttonPin = light_switch1;
// 	buttonPin2 = light_switch2;
// }
// 
// void Shutters::handleFSM(int currentButtonState)
// {
//     static int FSM = 0;
// //  static int oldFSM = 0;
// 
//   if(FSM == 0)
//   {
//     digitalWrite(ledPin, LOW);
//     digitalWrite(ledPin2, LOW);
//     FSM = currentButtonState;
//   }
//   else if(FSM == 1)
//   {
//     digitalWrite(ledPin, HIGH);
//     digitalWrite(ledPin2, LOW);
//     ourState = STATE_UNDEF;
//     FSM = currentButtonState;
//   }
//   else if(FSM == 2)
//   {
//     digitalWrite(ledPin, LOW);
//     digitalWrite(ledPin2, HIGH);
//     ourState = STATE_UNDEF;
//     FSM = currentButtonState;    
//   }
//   else if(FSM == 3)
//   {    
//     digitalWrite(ledPin, HIGH);
//     digitalWrite(ledPin2, LOW);
//     if(currentButtonState == 2 || currentButtonState == 4) FSM = 5;
//     
//     static unsigned long startingTimestamp = 0;
//     if(startingTimestamp == 0) startingTimestamp = millis();
//     if(millis() - startingTimestamp > time_to_light)
//     {
//       startingTimestamp = 0;
//       ourState = STATE_ON;
//       broadcastState();
//       FSM = 5;
//     }
// 
//   }
//   else if(FSM == 4)
//   {
//     digitalWrite(ledPin, LOW);
//     digitalWrite(ledPin2, HIGH);
//     if(currentButtonState == 1 || currentButtonState == 3) FSM = 5;
//     
//     static unsigned long startingTimestamp = 0;
//     if(startingTimestamp == 0) startingTimestamp = millis();
//     if(millis() - startingTimestamp > time_to_light)
//     {
//       startingTimestamp = 0;
//       ourState = STATE_OFF;
//       broadcastState();
//       FSM = 5;
//     }
// 
//   }
//   else if(FSM == 5)
//   {
//     digitalWrite(ledPin, LOW);
//     digitalWrite(ledPin2, LOW);
//     delay(150);    
//     FSM = 0;
//   } 
// //  if(oldFSM != FSM)
// //  {
// //    Serial.print("FSM: ");
// //    Serial.println(FSM, DEC);
// //  }
// //  oldFSM = FSM;
// }
// 
// void Shutters::setPullUpButtons(bool set)
// {
// 	digitalWrite(buttonPin, set);     
// 	digitalWrite(buttonPin2, set);   
// 	flipButton = set;  
// }
// 
// 
// int Shutters::checkForButtons(void)
// {
//   //The following variables will be used to check wether the intervals have passed
//   static unsigned long timestamp = 0, holdTimestamp =0;
//   const unsigned long buttonCheckInterval = 100;  // interval at which to check for button press (milliseconds)
//   const unsigned long decisionInterval = 500;
//   static unsigned long buttonDecideInterval = decisionInterval; // interval at which to decide if we are holding the button (milliseconds)  
//   
//   static int returnValue = 0;
//       
//   if(millis() - timestamp > buttonCheckInterval)
//   {
// 
//     static int buttonState = 0, oldButtonState = 0, holdButtonState = 0; // variable for reading the pushbutton status
//     static int buttonState2 = 0, oldButtonState2 = 0, holdButtonState2 = 0; // variable for reading the 2nd pushbutton status
//     
//     // read the state of the pushbutton values:
//     buttonState = digitalRead(buttonPin) ^ flipButton;
//     buttonState2 = digitalRead(buttonPin2) ^ flipButton;
//     
//     if(buttonState == HIGH) holdButtonState++;
//     if(buttonState2 == HIGH) holdButtonState2++;
//     
//     
//     if(buttonState == LOW)
//     {
//       if(oldButtonState == HIGH && holdButtonState < 2)
//         returnValue = 3;
//       else if(returnValue == 3)
//         returnValue = 0;
//       holdButtonState = 0;
//     }
//     
//     if(buttonState2 == LOW)
//     {
//       if(oldButtonState2 == HIGH && holdButtonState2 < 2) 
//         returnValue = 4;
//       else if(returnValue == 4)
//         returnValue = 0;
//       holdButtonState2 = 0;
//     }
//     
//     if(millis() - holdTimestamp > buttonDecideInterval)
//     {
//       if(holdButtonState > 1)
//       {
//         returnValue = 1;
//       }
//       else if(holdButtonState2 > 1)
//       {
//         returnValue = 2;
//       }
//       else if(returnValue == 1 || returnValue == 2)
//         returnValue = 0;
//       
//       holdTimestamp = millis();
//     }
//     
//     oldButtonState = buttonState;
//     oldButtonState2 = buttonState2;
//     
//     timestamp = millis();
//   }
// //  if(returnValue != 0) delay(50);
//   return returnValue;
// }
// 
// void Shutters::checkForMessages(void)
// {
//   if (Serial.available())
//   {
//     char bla = (char) Serial.read();
//     if(bla == OPEN) FSM_State = 2;
//     else if(bla == CLOSE) FSM_State = 4;
//     else if(bla == HALT) FSM_State = 0;
//   }
// }
// 
// //let the world know our current state
// void Shutters::broadcastState(void)
// {
//   Serial.print(ourState, BYTE);
// }
// 
// void Shutters::do_setup(long baudrate)
// {
// 	
// 	FSM_State = 0;
// 	ourState = STATE_UNDEF;
// 	flipButton = false;
// 	// initialize the LED pins as an outputs:
//   pinMode(ledPin, OUTPUT);      
//   pinMode(ledPin2, OUTPUT);      
//   // initialize the pushbutton pins as an inputs:
//   pinMode(buttonPin, INPUT);     
//   pinMode(buttonPin2, INPUT);   
// 
//   //XBee shit from now on
//   Serial.begin(baudrate);
//   Serial.print(STARTING, BYTE);
// }
