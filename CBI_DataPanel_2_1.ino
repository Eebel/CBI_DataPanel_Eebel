#include <LedControl.h> 

//////////////////////////////////////////////////////////
// 2 Aug 2024 - Added the ability to disable and clear the lighting sequences when each door is closed -  Eebel
//            - Renamed to CBI_DataPanel 2.1 - Eebel
//            - I know tis code is not optimized.  However, it works.
//            - IMPORTANT!!!! Both signal pins need to be powered and sending the correct PWM signal (above or below 1500 micorseconds) for the code to work
//              - If only one pin is powered the sequence lags due to the pulseIn() command attempting to read an invalid data
//              - Both pins can safely be depowered when not in use (i.e. the doors are closed)
// 5/31/2014 - Renamed to CBI_DataPanel_2_0 - VAShadow 
// 5/30/2014 - Added CBI Support - VAShadow 
//
// 
// 12/05/2013 - CuriousMarc DataPort sketch v1.0
// Alternate sketch for Michael's Erwin DataPort 
// with a more organic feel
//
///////////////////////////////////////////////////////////

// change this to match which Arduino pins you connect your panel to,
// which can be any 3 digital pins you have available. 
// This is the pinout for the DroidConUino, same as the R series
#define DATAIN_PIN 13 //2 
#define CLOCK_PIN  12 //4
#define LOAD_PIN   11 //8
#define DATAPORT_INPUT_PIN 10
#define CBI_INPUT_PIN 9


//Set this to which Analog Pin you use for the voltage in.
#define analoginput 0

#define greenVCC 12.0    // Green LED on if above this voltage
#define yellowVCC 11.0   // Yellow LED on if above this voltage
#define redVCC 10.5      // Red LED on if above this voltage
#define voltScale 5.04   // A handy way to tune your results.  Higher values is a higher voltage poduced from the monitoring
                        //  Tune this value until the vin shows the correct battery voltage in the serial monitor.
                        
// For 15volts: R1=47k, R2=24k
// For 30volts: R1=47k, R2=9.4k
#define R1 47000.0     // >> resistance of R1 in ohms << the more accurate these values are
#define R2 24000.0     // >> resistance of R2 in ohms << the more accurate the measurement will be


// change this to match your hardware, I have dataport as the device 0 (first in chain)
// and I don't have a CBI, but it would be second in the chain
// Invert the 1 and the 0 if you have them in the other order
#define DATAPORT 0 // the dataport one is first in the chain (device 0)
#define CBI      1 // I don't have a CBI, so I put it in second position (device 1)

// Number of Maxim chips that are connected
#define NUMDEV 2   // One for the dataport, one for the battery indicator

// the dataport is quite dim, so I put it to the maximum
#define DATAPORTINTENSITY 15  // 15 is max
#define CBIINTENSITY 12  // 15 is max

// uncomment this to test the LEDS one after the other at startup
//#define TEST
// This will revert to the old style block animation for comparison
//#define LEGACY
// If you are using the voltage monitor uncomment this
#define monitorVCC
// Uncomment #define tuneVCCValue to tune the values. Once they are correct comment it out for better performance
//#define tuneVCCValue

// the timing values below control the various effects. Tweak to your liking.
// values are in ms. The lower the faster.
#define TOPBLOCKSPEED   70
#define BOTTOMLEDSPEED  200
#define REDLEDSPEED     500
#define BLUELEDSPEED    500
#define BARGRAPHSPEED   200
#define CBISPEED        25

// Uncomment this if you want an alternate effect for the blue LEDs, where it moves
// in sync with the bar graph
//#define BLUELEDTRACKGRAPH

//============================================================================

float vout = 0.0;       // for voltage out measured analog input
int value = 0;          // used to hold the analog value coming out of the voltage divider
float vin = 0.0;        // voltage calulcated... since the divider allows for 15 volts

bool dataPortState = false; //lights are off
unsigned long dataPanelDuration;  //Start with the datapanel door closed

bool cbiState = false;  //lights are off
unsigned long cbiDuration; // start with the cbi door closed

int lightsState = 0;  //0 = both off, 1= CBI on and DP Off, 2 = CBI off and DP on, 3 = both on

// Instantiate LedControl driver
LedControl lc=LedControl(DATAIN_PIN,CLOCK_PIN,LOAD_PIN,NUMDEV);   // RSeries FX i2c v5 Module Logics Connector 

void setup() 
{
  Serial.begin(57600);                    

  // initialize Maxim driver chips
  lc.shutdown(DATAPORT,false);                  // take out of shutdown
  lc.clearDisplay(DATAPORT);                    // clear
  lc.setIntensity(DATAPORT,DATAPORTINTENSITY);  // set intensity
  
  lc.shutdown(CBI,false);                       // take out of shutdown
  lc.clearDisplay(CBI);                         // clear
  lc.setIntensity(CBI,CBIINTENSITY);            // set intensity
  
#ifdef  TEST// test LEDs
  singleTest(); 
  delay(2000);
#endif


#ifndef monitorVCC
  pinMode(analoginput, INPUT);
#endif

pinMode(DATAPORT_INPUT_PIN, INPUT_PULLUP);
pinMode(CBI_INPUT_PIN, INPUT_PULLUP);
}


void loop() 
{ 
    // START ENCODING
    // Grab the state of the input pins for the dataport and cbi and encode it as an integer (0-3)
    // I am sending a PWM signal from a Pololu Maestro.
    // To enable lighting set anu value above 1500 microseconds
    // To disable lighting set the pin value to less than 1500 microseconds
    dataPanelDuration = pulseIn(DATAPORT_INPUT_PIN, HIGH); // get current PWM of DATAPORT input pin.  Is the pin HIGH OR LOW
    cbiDuration = pulseIn(CBI_INPUT_PIN, HIGH);           // get current PWM of CBI input pin.  Is the pin HIGH OR LOW

    //convert the dataPort state to an boolean type
    if (dataPanelDuration > 1500){
      dataPortState = 1; 
    }
    else{
      dataPortState = 0;
    }

    //convert the cbi state to a boolean type
    if (cbiDuration > 1500){
        cbiState = 1;
      }
      else {
        cbiState = 0;
      }

    //Make an int based on the two boolean values
    lightsState = MakeInt(dataPortState, cbiState);
    //END OF ENCODING

  // this is the legacy algorythm. Super simple, but very blocky.
  //
#ifdef LEGACY
  switch (lightsState){
      case 0: //both off
        lc.clearDisplay(CBI);
        lc.clearDisplay(DATAPORT);
        break;
      case 1: //cbi ON only
        #ifdef monitorVCC
          for (int row=0; row<4; row++) lc.setRow(CBI,row,random(0,256));
          getVCC();
        #else
          for (int row=0; row<7; row++) lc.setRow(CBI,row,random(0,256));
        #endif 
        lc.clearDisplay(DATAPORT);
        break;
      case 2: //dataPort ON only
        for (int row=0; row<6; row++) lc.setRow(DATAPORT,row,random(0,256));
        lc.clearDisplay(CBI);
        break;
      case 3: //both ON
        for (int row=0; row<6; row++) lc.setRow(DATAPORT,row,random(0,256));
        #ifdef monitorVCC
          for (int row=0; row<4; row++) lc.setRow(CBI,row,random(0,256));
          getVCC();
        #else
          for (int row=0; row<7; row++) lc.setRow(CBI,row,random(0,256));
        #endif
        break;        
  }
    delay(1000);
  #else
    //START OF MY DOOR CODE
    switch (lightsState){
      case 0: //both off
        lc.clearDisplay(CBI);
        lc.clearDisplay(DATAPORT);
        break;
      case 1: //cbi ON only
        updateCBILEDs();
          #ifdef monitorVCC
        getVCC();
  #endif
        lc.clearDisplay(DATAPORT);
        break;
      case 2: //dataPort ON only
        updateTopBlocks();
        bargraphDisplay(0);
        updatebottomLEDs();
        updateRedLEDs();
        updateBlueLEDs();
        lc.clearDisplay(CBI);
        break;
      case 3: //both ON
        updateTopBlocks();
        bargraphDisplay(0);
        updatebottomLEDs();
        updateRedLEDs();
        updateBlueLEDs();
        updateCBILEDs();
        #ifdef monitorVCC
          getVCC();
        #endif
        break;
    } //END OF MY DOOR CODE 

  // #ifdef monitorVCC
  //   getVCC();
  // #endif
  //  #ifndef BLUELEDTRACKGRAPH
  //    updateBlueLEDs();
  //  #endif

  #endif

}
// END OF MAIN LOOP
//*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*
//*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*
//*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*



///////////////////////////////////////////////////
// Test LEDs, each Maxim driver row in turn
// Each LED blinks according to the col number
// Col 0 is just on
// Col 1 blinks twice
// col 2 blinks 3 times, etc...
//

#define TESTDELAY 30
void singleTest() 
{
  for(int row=0;row<6;row++) 
  {
    for(int col=0;col<7;col++) 
    {
      delay(TESTDELAY);
      lc.setLed(DATAPORT,row,col,true);
      delay(TESTDELAY);
      for(int i=0;i<col;i++) 
      {
        lc.setLed(DATAPORT,row,col,false);
        delay(TESTDELAY);
        lc.setLed(DATAPORT,row,col,true);
        delay(TESTDELAY);
      }
    }
  }
 
 
 for(int row=0;row<4;row++) 
  {
    for(int col=0;col<5;col++) 
    {
      delay(TESTDELAY);
      lc.setLed(CBI,row,col,true);
      delay(TESTDELAY);
      for(int i=0;i<col;i++) 
      {
        lc.setLed(CBI,row,col,false);
        delay(TESTDELAY);
        lc.setLed(CBI,row,col,true);
        delay(TESTDELAY);
      }
    }
  }
 
 lc.setLed(CBI,4,5,true);
 delay(TESTDELAY);
 lc.setLed(CBI,5,5,true);
 delay(TESTDELAY);
 lc.setLed(CBI,6,5,true);
 delay(TESTDELAY);
}

///////////////////////////////////
// animates the two top left blocks
// (green and yellow blocks)
void updateTopBlocks()
{
  static unsigned long timeLast=0;
  unsigned long elapsed;
  elapsed=millis();
  if ((elapsed - timeLast) < TOPBLOCKSPEED) return;
  timeLast = elapsed; 

  lc.setRow(DATAPORT,4,randomRow(4)); // top yellow blocks
  lc.setRow(DATAPORT,5,randomRow(4)); // top green blocks

}

///////////////////////////////////
// animates the CBI
//
void updateCBILEDs()
{
  static unsigned long timeLast=0;
  unsigned long elapsed;
  elapsed=millis();
  if ((elapsed - timeLast) < CBISPEED) return;
  timeLast = elapsed; 

  #ifdef monitorVCC
    lc.setRow(CBI,random(4),randomRow(random(4)));
  #else
    lc.setRow(CBI,random(7),randomRow(random(4)));
  #endif
}

////////////////////////////////////
// Utility to generate random LED patterns
// Mode goes from 0 to 6. The lower the mode
// the less the LED density that's on.
// Modes 4 and 5 give the most organic feel
byte randomRow(byte randomMode)
{
  switch(randomMode)
  {
    case 0:  // stage -3
      return (random(256)&random(256)&random(256)&random(256));
      break;
    case 1:  // stage -2
      return (random(256)&random(256)&random(256));
      break;
    case 2:  // stage -1
      return (random(256)&random(256));
      break;
    case 3: // legacy "blocky" mode
      return random(256);
      break;
    case 4:  // stage 1
      return (random(256)|random(256));
      break;
    case 5:  // stage 2
      return (random(256)|random(256)|random(256));
      break;
    case 6:  // stage 3
      return (random(256)|random(256)|random(256)|random(256));
      break;
    default:
      return random(256);
      break;
  }
}

//////////////////////
// bargraph for the right column
// disp 0: Row 2 Col 5 to 0 (left bar) - 6 to 0 if including lower red LED, 
// disp 1: Row 3 Col 5 to 0 (right bar)

#define MAXGRAPH 2

void bargraphDisplay(byte disp)
{ 
  static byte bargraphdata[MAXGRAPH]; // status of bars
  
  if(disp>=MAXGRAPH) return;
  
  // speed control
  static unsigned long previousDisplayUpdate[MAXGRAPH]={0,0};

  unsigned long currentMillis = millis();
  if(currentMillis - previousDisplayUpdate[disp] < BARGRAPHSPEED) return;
  previousDisplayUpdate[disp] = currentMillis;
  
  // adjust to max numbers of LED available per bargraph
  byte maxcol;
  if(disp==0 || disp==1) maxcol=6;
  else maxcol=3;  // for smaller graph bars, not defined yet
  
  // use utility to update the value of the bargraph  from it's previous value
  byte value = updatebar(disp, &bargraphdata[disp], maxcol);
  byte data=0;
  // transform value into byte representing of illuminated LEDs
  // start at 1 so it can go all the way to no illuminated LED
  for(int i=1; i<=value; i++) 
  {
    data |= 0x01<<i-1;
  }
  // transfer the byte column wise to the video grid
  fillBar(disp, data, value, maxcol);   
}

/////////////////////////////////
// helper for updating bargraph values, to imitate bargraph movement
byte updatebar(byte disp, byte* bargraphdata, byte maxcol)
{
  // bargraph values go up or down one pixel at a time
  int variation = random(0,3);            // 0= move down, 1= stay, 2= move up
  int value=(int)(*bargraphdata);         // get the previous value
  //if (value==maxcol) value=maxcol-2; else      // special case, staying stuck at maximum does not look realistic, knock it down
  value += (variation-1);                 // grow or shring it by one step
#ifndef BLUELEDTRACKGRAPH
  if (value<=0) value=0;                  // can't be lower than 0
#else
  if (value<=1) value=1;                  // if blue LED tracks, OK to keep lower LED always on
#endif
  if (value>maxcol) value=maxcol;         // can't be higher than max
  (*bargraphdata)=(byte)value;            // store new value, use byte type to save RAM
  return (byte)value;                     // return new value
}

/////////////////////////////////////////
// helper for lighting up a bar of LEDs based on a value
void fillBar(byte disp, byte data, byte value, byte maxcol)
{
  byte row;
  
  // find the row of the bargraph
  switch(disp)
  {
    case 0:
      row = 2;
      break;
    case 1:
      row = 3;
      break;
    default:
      return;
      break;
  }
  
  for(byte col=0; col<maxcol; col++)
  {
    // test state of LED
    byte LEDon=(data & 1<<col);
    if(LEDon)
    {
      //lc.setLed(DATAPORT,row,maxcol-col-1,true);  // set column bit
      lc.setLed(DATAPORT,2,maxcol-col-1,true);      // set column bit
      lc.setLed(DATAPORT,3,maxcol-col-1,true);      // set column bit
      //lc.setLed(DATAPORT,0,maxcol-col-1,true);      // set blue column bit
    }
    else
    {
      //lc.setLed(DATAPORT,row,maxcol-col-1,false); // reset column bit
      lc.setLed(DATAPORT,2,maxcol-col-1,false);     // reset column bit
      lc.setLed(DATAPORT,3,maxcol-col-1,false);     // reset column bit
      //lc.setLed(DATAPORT,0,maxcol-col-1,false);     // set blue column bit
    }
  }
#ifdef BLUELEDTRACKGRAPH
  // do blue tracking LED
  byte blueLEDrow=B00000010;
  blueLEDrow=blueLEDrow<<value;
  lc.setRow(DATAPORT,0,blueLEDrow);
#endif
}

/////////////////////////////////
// This animates the bottom white LEDs
void updatebottomLEDs()
{
  static unsigned long timeLast=0;
  unsigned long elapsed=millis();
  if ((elapsed - timeLast) < BOTTOMLEDSPEED) return;
  timeLast = elapsed;  
  
  // bottom LEDs are row 1, 
  lc.setRow(DATAPORT,1,randomRow(4));
}

////////////////////////////////
// This is for the two red LEDs
void updateRedLEDs()
{
  static unsigned long timeLast=0;
  unsigned long elapsed=millis();
  if ((elapsed - timeLast) < REDLEDSPEED) return;
  timeLast = elapsed;  
  
  // red LEDs are row 2 and 3, col 6, 
  lc.setLed(DATAPORT,2,6,random(0,2));
  lc.setLed(DATAPORT,3,6,random(0,2));
}

//////////////////////////////////
// This animates the blue LEDs
// Uses a random delay, which never exceeds BLUELEDSPEED 
void updateBlueLEDs()
{
  static unsigned long timeLast=0;
  static unsigned long variabledelay=BLUELEDSPEED;
  unsigned long elapsed=millis();
  if ((elapsed - timeLast) < variabledelay) return;
  timeLast = elapsed;  
  variabledelay=random(10, BLUELEDSPEED);
  
  /*********experimental, moving dots animation
  static byte stage=0;
  stage++;
  if (stage>7) stage=0;
  byte LEDstate=B00000011;
  // blue LEDs are row 0 col 0-5 
  lc.setRow(DATAPORT,0,LEDstate<<stage);
  *********************/
  
  // random
  lc.setRow(DATAPORT,0,randomRow(4));   
}


void getVCC()
{
  value = analogRead(analoginput); // this must be between 0.0 and 5.0 - otherwise you'll let the blue smoke out of your ar
  vout= (value * voltScale)/1024.0;  //voltage coming out of the voltage divider
  vin = vout / (R2/(R1+R2)); //voltage to display

  lc.setLed(CBI,6,5,(vin >= greenVCC));
  lc.setLed(CBI,5,5,(vin >= yellowVCC));
  lc.setLed(CBI,4,5,(vin >= redVCC));

  #ifdef tuneVCCValue
    Serial.print("Volt Out = ");                                  // DEBUG CODE
    Serial.print(vout, 1);   //Print float "vin" with 1 decimal   // DEBUG CODE
    Serial.print("\tVolts Calc = ");                             // DEBUG CODE
    Serial.println(vin, 1);   //Print float "vin" with 1 decimal   // DEBUG CODE
  #endif
}

int MakeInt(bool b1, bool b2)
//Creates an integer from the two boolena values (0-3)
{
   return b1 | (b2<<1);
}
