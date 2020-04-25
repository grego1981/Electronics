// Bench PSU Arduino TFT
// forked from Iqbal Ibrahim
// by Gregoris Tsintaris
// July 2018 - April 2019
/*-----( Import needed libraries )-----*/
#include "DEV_Config.h"
#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include <Wire.h>
#include <max6675.h>
//#include <Adafruit_MAX31855.h>
#include <Adafruit_NeoPixel.h>
#include "ad8495.h"
#ifdef __AVR__
  #include <avr/power.h>
#endif
/*-----( Declare Constants and Pin Numbers )-----*/
#define HG_relay  5 //HG Heating Element power Status and relay control Pin
#define TSTR_pin  3 //tester power Status Pin
#define LED_PIN 39
#define Vpower 5.04
#define PWROK_PIN 2
#define PWRBTN 18 //watch for interrupt pins! Not all are interrupt pins!
#define PWRTRANS 32
#define PWRSI 36
#define PWRHG 21
#define HGReed 34
#define SPKR 44
#define NUMPIXELS 8
#define PIN_TEMP_SENSOR A10 //for ad8495
/*-----( Declare objects )-----*/
//Current sensing variables
int mVperAmp = 100;
int ACSoffset = 2500;
const int ac3v3 = A11;//Current sensing pins-->
const int ac5v = A7;
const int ac12v = A15;
const int ac012v = A13;
const int ac42v = A9;//Current sensing pins<--
double threevoltincur = 0;//Current sensing init values-->
double fivevoltincur = 0;
double twelvevoltincur = 0;
double vroneincur = 0;
double vrtwoincur = 0;//Current sensing init values<--
// Variables will change:
int PWRSTATE = HIGH;         // the current state of the output pin
int buttonState; // the current reading from the input pin
int beep_state, tstr_on_status, pwr_status, si_status, hg_status, FAN_State;
int lastButtonState = LOW;   // the previous reading from the input pin
static char outstr[15];
// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

float threevolt = 100.000;//Initial compare values to control A & V redraw times-->
float dif3v = 0.000;
float threeamp = 100.000;
float dif3a = 0.000;

float fivevolt = 100.000;
float dif5v = 0.000;
float fiveamp = 100.000;
float dif5a = 0.000;

float twelvevolt = 100.000;
float dif12v = 0.000;
float twelveamp = 100.000;
float dif12a = 0.000;

float vronevolt = 100.000;
float dif012v = 0.000;
float vroneamp = 100.000;
float dif012a = 0.000;

float vrtwovolt = 100.000;
float dif42v = 0.000;
float vrtwoamp = 100.000;
float dif42a = 0.000;
float diftemp = 0.000;//Initial compare values to control A & V redraw times<--
int Cool_DN = 0;
const unsigned long measurementPeriod = 500;//MAX chip poll interval 
unsigned long timer;//MAX chip timer counter as global
float curTemperatureSI = 0.000;//Initial compare values to control MAX C redraw times
float curTemperatureHG = 0.000;//Initial compare values to control MAX C redraw times
float threevoltin, fivevoltin, twelvevoltin, vronein, vrtwoin, cur_set_tempSI, cur_tempSI, difsettempSI, difcurtempSI, cur_set_tempHG, cur_tempHG, difsettempHG, difcurtempHG; 
float threevolts, fivevolts, twelvevolts, vronevolts, vrtwovolts;
/*-----( Declare Variables )-----*/
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

byte neopix_gamma[] = {  // Used to do the breathing effect
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


void readVoltages(){
  
  threevoltin =  (analogRead(A1)* Vpower)/1023.0;
  threevoltincur = (((analogRead(ac3v3) / 1023.0) * 5000) - ACSoffset) / mVperAmp;
  if (threevoltincur < 0.00) { threevoltincur = 0.000;}
  fivevoltin =   (analogRead(A2)* Vpower)/1023.0;
  fivevoltincur = (((analogRead(ac5v) / 1023.0) * 5000) - ACSoffset) / mVperAmp;
  if (fivevoltincur < 0.00) { fivevoltincur = 0.000;}
  twelvevoltin = (analogRead(A0)* Vpower)/1023.0;
  twelvevoltincur = (((analogRead(ac12v) / 1023.0) * 5000) - ACSoffset) / mVperAmp;
  if (twelvevoltincur < 0.00) { twelvevoltincur = 0.000;}
  vronein =      (analogRead(A4)* Vpower)/1023.0;
  vroneincur = (((analogRead(ac012v) / 1023.0) * 5000) - ACSoffset) / mVperAmp;
  if (vroneincur < 0.00) { vroneincur = 0.000;}
  vrtwoin =      (analogRead(A6)* Vpower)/1023.0;
  vrtwoincur = (((analogRead(ac42v) / 1023.0) * 5000) - ACSoffset) / mVperAmp;
  if (vrtwoincur < 0.00) { vrtwoincur = 0.000;}
} 
/* Added temperature controlled (sort of) for soldering iron
 *  We first read the temperature pot and the temp of the SI
 *  We then decide if it will be on or off. 0 if also Off.
 */
void readTemp(){
  
  int setTemperatureSIRaw = analogRead(A3);
  int setTemperatureHGRaw = analogRead(A5);
  float setTemperatureSI = setTemperatureSIRaw * (466.0 / 1023.0);
  float setTemperatureHG = setTemperatureHGRaw * (451.0 / 1023.0);
  int ktcSO = 31;
  //int ktcCSSI = 29;
  int ktcCSHG = 30;
  int ktcCLK = 28;
  //Adafruit_MAX31855 thermocouple(ktcCLK, ktcCS, ktcSO);
  //MAX6675 thermocoupleSI(ktcCLK, ktcCSSI, ktcSO);
  MAX6675 thermocoupleHG(ktcCLK, ktcCSHG, ktcSO);

  if (millis() - timer >= measurementPeriod) { //run check every 250ms adjust timer up top
    timer += measurementPeriod;
    
    for (char i = 0; i < 5; i++) {
    curTemperatureSI += ad8495_getTemperature(ad8495_getVoltage(analogRead(PIN_TEMP_SENSOR)));
    delay(5);
    }
    curTemperatureSI /= 5;
    //curTemperatureSI = thermocoupleSI.readCelsius();
    curTemperatureHG = thermocoupleHG.readCelsius();
  }
  difsettempSI = cur_set_tempSI - setTemperatureSI;
  if ((difsettempSI < -1.000) || (difsettempSI > 1.000)){
  GUI_DrawRectangle(253, 70, 313, 94,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(255, 70, dtostrf(setTemperatureSI,3,0,outstr), &Font24, LCD_BACKGROUND, GREEN);
  cur_set_tempSI = setTemperatureSI;
  }
  difsettempHG = cur_set_tempHG - setTemperatureHG;//change draw area
  if ((difsettempHG < -1.000) || (difsettempHG > 1.000)){
  GUI_DrawRectangle(253, 70, 313, 94,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(255, 70, dtostrf(setTemperatureHG,3,0,outstr), &Font24, LCD_BACKGROUND, GREEN);
  cur_set_tempHG = setTemperatureHG;
  }
  
  difcurtempSI = cur_tempSI - curTemperatureSI;
  if ((difcurtempSI < -1.000) || (difcurtempSI > 1.000)){
  GUI_DrawRectangle(362, 70, 422, 94,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(364, 70, dtostrf(curTemperatureSI,3,0,outstr), &Font24, LCD_BACKGROUND, RED);
  cur_tempSI = curTemperatureSI;
  }
  difcurtempHG = cur_tempHG - curTemperatureHG;
  if ((difcurtempHG < -1.000) || (difcurtempHG > 1.000)){
  GUI_DrawRectangle(362, 70, 422, 94,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(364, 70, dtostrf(curTemperatureHG,3,0,outstr), &Font24, LCD_BACKGROUND, RED);
  cur_tempHG = curTemperatureHG;
  }
  //Soldering Iron on/off
  if (digitalRead(PWROK_PIN) == HIGH && setTemperatureSI != 0){ //check if PSU is ON
   if (curTemperatureSI >= setTemperatureSI){
     digitalWrite(PWRSI, LOW);
     pixels.setPixelColor(5, pixels.Color(0,0,55));
     pixels.show();
     if (si_status == 1){
     GUI_DrawRectangle(243, 100, 457, 137,  BLUE, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DrawRectangle(180, 115, 218, 136, BLUE, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DisString_EN(190, 119, "ON", &Font16, LCD_BACKGROUND, WHITE);
     si_status = 0;
     }
   } else {
     digitalWrite(PWRSI, HIGH);
     pixels.setPixelColor(5, pixels.Color(125,64,0));
     pixels.show();
     //pulseRed(10);
     if (si_status == 0){
     GUI_DrawRectangle(243, 100, 457, 137,  YELLOW, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DrawRectangle(180, 115, 218, 136, YELLOW, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DisString_EN(190, 119, "ON", &Font16, LCD_BACKGROUND, WHITE);
     si_status = 1;
     }
     }
  } else {
     digitalWrite(PWRSI, LOW);
     pixels.setPixelColor(5, pixels.Color(0,55,0));
     pixels.show();
     if (si_status == 1){
     GUI_DrawRectangle(243, 100, 457, 137,  RED, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DrawRectangle(180, 115, 218, 136, RED, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DisString_EN(184, 119, "OFF", &Font16, LCD_BACKGROUND, WHITE);
     si_status = 0;
     }
  }
  //////////////////////////////////////Heat Gun//////////////////////////////////////
   if (digitalRead(PWROK_PIN) == HIGH && setTemperatureHG != 0 && digitalRead(HGReed) == LOW){ //check if PSU is ON AND Heatgun off the hook AND Heatgun FAN is ON
   if (curTemperatureHG >= setTemperatureHG){
     digitalWrite(HG_relay, LOW);
     digitalWrite(PWRHG, HIGH); //we need FAN to work if we use it!
     pixels.setPixelColor(6, pixels.Color(0,0,55));
     pixels.show();
     if (hg_status == 1){
     //GUI_DrawRectangle(243, 100, 457, 137,  BLUE, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DrawRectangle(180, 73, 218, 94, BLUE, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DisString_EN(190, 79, "ON", &Font16, LCD_BACKGROUND, WHITE);
     hg_status = 0;
     FAN_State = 1;
     }
   } else {
     digitalWrite(HG_relay, HIGH);
     digitalWrite(PWRHG, HIGH);
     pixels.setPixelColor(6, pixels.Color(125,64,0));
     pixels.show();
     if (hg_status == 0){
     //GUI_DrawRectangle(243, 100, 457, 137,  YELLOW, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DrawRectangle(180, 73, 218, 94, YELLOW, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DisString_EN(190, 79, "ON", &Font16, LCD_BACKGROUND, WHITE);
     hg_status = 1;
     FAN_State = 1;
     }
     }
  } else {
     if ((Cool_DN >= 100)&&(FAN_State == 1)) { //set timer for fan
     digitalWrite(PWRHG, LOW);
     Cool_DN = 0;
     FAN_State = 0;
     }
     digitalWrite(HG_relay, LOW); 
     pixels.setPixelColor(6, pixels.Color(0,55,0));
     pixels.show();
     if (hg_status == 1){
     //GUI_DrawRectangle(243, 100, 457, 137,  RED, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DrawRectangle(180, 73, 218, 94, RED, DRAW_FULL, DOT_PIXEL_DFT);
     GUI_DisString_EN(184, 79, "OFF", &Font16, LCD_BACKGROUND, WHITE);
     hg_status = 0;
     Cool_DN = Cool_DN + 1 ;
     }
  }
  //////////////////////////////////////Heat Gun//////////////////////////////////////
}
void calculatePrintVoltages(){
  dif3v = threevolt - threevoltin;
  threevolts = threevoltin;
  if ((dif3v > 0.02)|| (dif3v < -0.02)){ 
  GUI_DrawRectangle(226, 186, 313, 206,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(228, 186, dtostrf(threevoltin,5,3,outstr), &Font20, LCD_BACKGROUND, YELLOW);
  threevolt = threevoltin;
  }
  if (threevolts != 0){
    pixels.setPixelColor(0, pixels.Color(55,0,0));
    pixels.show();
    } else {
    pixels.setPixelColor(0, pixels.Color(0,55,0));
    pixels.show();
    }
  dif3a = threeamp - threevoltincur;
  if ((dif3a > 0.03)|| (dif3a < -0.03)){ 
    GUI_DrawRectangle(356, 186, 457, 206,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
    GUI_DisString_EN(358, 186, dtostrf(threevoltincur,5,3,outstr), &Font20, LCD_BACKGROUND, CYAN);
    threeamp = threevoltincur;
  }

  
  dif5v = fivevolt - fivevoltin;
  fivevolts = (fivevoltin)/ (1770.0/(996.0+1770.0));
  if ((dif5v > 0.02)|| (dif5v < -0.02)){ 
  GUI_DrawRectangle(226, 213, 313, 233,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(228, 213, dtostrf(fivevolts,5,3,outstr), &Font20, LCD_BACKGROUND, YELLOW);
  fivevolt = fivevoltin;
  }
    if (fivevolts != 0){
    pixels.setPixelColor(1, pixels.Color(55,0,0));
    pixels.show();
    } else {
    pixels.setPixelColor(1, pixels.Color(0,55,0));
    pixels.show();
    }
  dif5a = fiveamp - fivevoltincur;
  if ((dif5a > 0.03)|| (dif5a < -0.03)){
    GUI_DrawRectangle(356, 213, 457, 233,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
    GUI_DisString_EN(358, 213, dtostrf(fivevoltincur,5,3,outstr), &Font20, LCD_BACKGROUND, CYAN);
    fiveamp = fivevoltincur;
  }

  dif12v = twelvevolt - twelvevoltin;
  twelvevolts = (twelvevoltin ) / (1780.0/(4670.0+1780.0));
  if ((dif12v > 0.02)|| (dif12v < -0.02)){ 
  GUI_DrawRectangle(226, 240, 313, 260,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(228, 240, dtostrf(twelvevolts,5,3,outstr), &Font20, LCD_BACKGROUND, YELLOW);
  twelvevolt = twelvevoltin;
  }
    if (twelvevolts != 0){
    pixels.setPixelColor(2, pixels.Color(55,0,0));
    pixels.show();
    } else {
    pixels.setPixelColor(2, pixels.Color(0,55,0));
    pixels.show();
    }
  dif12a = twelveamp - twelvevoltincur;
  if ((dif12a > 0.03)|| (dif12a < -0.03)){
    GUI_DrawRectangle(356, 240, 457, 260,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
    GUI_DisString_EN(358, 240, dtostrf(twelvevoltincur,5,3,outstr), &Font20, LCD_BACKGROUND, CYAN);
    twelveamp = twelvevoltincur;
  }

  dif012v = vronevolt - vronein;
  vronevolts = (vronein ) / (1760.0/(4680.0+1760.0));
  if ((dif012v > 0.02)|| (dif012v < -0.02)){ 
  GUI_DrawRectangle(226, 267, 313, 287,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(228, 267, dtostrf(vronevolts,5,3,outstr), &Font20, LCD_BACKGROUND, YELLOW);
  vronevolt = vronein;
  }
    if (vronevolts != 0){
    pixels.setPixelColor(3, pixels.Color(55,0,0));
    pixels.show();
    } else {
    pixels.setPixelColor(3, pixels.Color(0,55,0));
    pixels.show();
    }
  dif012a = vroneamp - vroneincur;
  if ((dif012a > 0.03)|| (dif012a < -0.03)){
  GUI_DrawRectangle(356, 267, 457, 287,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(358, 267, dtostrf(vroneincur,5,3,outstr), &Font20, LCD_BACKGROUND, CYAN);
  vroneamp = vroneincur;
  }

  dif42v = vrtwovolt - vrtwoin;
  vrtwovolts = (vrtwoin ) / (1770.0/(26500.0+1770.0));
  if ((dif42v > 0.02) || (dif42v < -0.02)){
  GUI_DrawRectangle(226, 294, 313, 314,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(228, 294, dtostrf(vrtwovolts,5,3,outstr), &Font20, LCD_BACKGROUND, YELLOW);
  vrtwovolt = vrtwoin;
  }
    if (vrtwovolts != 0){
    pixels.setPixelColor(4, pixels.Color(55,0,0));
    pixels.show();
    } else {
    pixels.setPixelColor(4, pixels.Color(0,55,0));
    pixels.show();
    }
  dif42a = vrtwoamp - vrtwoincur;
  if ((dif42a > 0.03)|| (dif42a < -0.03)){
  GUI_DrawRectangle(356, 294, 457, 314,  BLACK, DRAW_FULL, DOT_PIXEL_DFT);
  GUI_DisString_EN(358, 294, dtostrf(vrtwoincur,5,3,outstr), &Font20, LCD_BACKGROUND, CYAN);
  vrtwoamp = vrtwoincur;
  }


  
}

void checkPowerOK(){

  if (digitalRead(PWROK_PIN) == HIGH) {
    if (beep_state == 1){
    GUI_DrawRectangle(180, 52, 218, 73, GREEN, DRAW_FULL, DOT_PIXEL_DFT);
    GUI_DisString_EN(190, 59, "ON", &Font16, LCD_BACKGROUND, WHITE);
    tone(SPKR, 1000); // Send 1KHz sound signal...
    delay(300);        // ...for 1 sec
    noTone(SPKR);     // Stop sound...
    beep_state=0;
    pwr_status=0;
      if (digitalRead(TSTR_pin) == HIGH) {  /*Check tester power Status*/
        if (tstr_on_status == 0){
        GUI_DrawRectangle(180, 94, 218, 115, GREEN, DRAW_FULL, DOT_PIXEL_DFT);
        GUI_DisString_EN(190, 99, "ON", &Font16, LCD_BACKGROUND, WHITE);
        pixels.setPixelColor(7, pixels.Color(0,55,0));
        pixels.show();
        tstr_on_status = 1;
        }
      } else {
        if (tstr_on_status == 1){
        GUI_DrawRectangle(180, 94, 218, 115, RED, DRAW_FULL, DOT_PIXEL_DFT);
        GUI_DisString_EN(184, 99, "OFF", &Font16, LCD_BACKGROUND, WHITE);
        tstr_on_status = 0;
        }
      }
      //delay(300);
    }else{
      if (pwr_status == 1){
      GUI_DrawRectangle(180, 52, 218, 73, GREEN, DRAW_FULL, DOT_PIXEL_DFT);
      GUI_DisString_EN(190, 59, "ON", &Font16, LCD_BACKGROUND, WHITE);
      pwr_status=0;
      }
      
      if (digitalRead(TSTR_pin) == HIGH) {  /*Check tester power Status*/
        if (tstr_on_status == 0){
        GUI_DrawRectangle(180, 94, 218, 115, GREEN, DRAW_FULL, DOT_PIXEL_DFT);
        GUI_DisString_EN(190, 99, "ON", &Font16, LCD_BACKGROUND, WHITE);
        pixels.setPixelColor(7, pixels.Color(0,55,0));
        pixels.show();
        tstr_on_status = 1;
        }
      } else {
        if (tstr_on_status == 1){
        GUI_DrawRectangle(180, 94, 218, 115, RED, DRAW_FULL, DOT_PIXEL_DFT);
        GUI_DisString_EN(184, 99, "OFF", &Font16, LCD_BACKGROUND, WHITE);
        pixels.setPixelColor(7, pixels.Color(55,0,0));
        pixels.show();
        tstr_on_status = 0;
        }
      }
      }
  }
  else if (digitalRead(PWROK_PIN) == LOW) {
    if (pwr_status == 0) {
    GUI_DrawRectangle(180, 52, 218, 73, RED, DRAW_FULL, DOT_PIXEL_DFT);
    GUI_DisString_EN(184, 59, "OFF", &Font16, LCD_BACKGROUND, WHITE);
    GUI_DrawRectangle(180, 94, 218, 115, RED, DRAW_FULL, DOT_PIXEL_DFT);
    GUI_DisString_EN(184, 99, "OFF", &Font16, LCD_BACKGROUND, WHITE);  /*If everything is down tester is down also*/
    pixels.setPixelColor(7, pixels.Color(0,0,0));
    pixels.show();
    pwr_status=1;
    si_status=0;
    FAN_State=1;
    }
    beep_state=1;
  }

  
}

void togglePower(){
 static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 200ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 300) 
 {
   // set the power:
  digitalWrite(PWRTRANS, PWRSTATE);
  Serial.println("checked");
 }
 PWRSTATE = !PWRSTATE;
 last_interrupt_time = interrupt_time;
// threevolt = 100.000;  
}

void pulseRed(uint8_t wait) {  //This is the part that does the breathing effect
  for(int j = 50; j < 256 ; j++){
          pixels.setPixelColor(5, pixels.Color(j,80,0, neopix_gamma[j] ) );
          delay(wait);
          pixels.show();
      }
  for(int j = 255; j >= 50 ; j--){
          pixels.setPixelColor(5, pixels.Color(j,80,0, neopix_gamma[j] ) );
          delay(wait);
          pixels.show();
        }
      }

void setup()   /****** SETUP: RUNS ONCE ******/
{
 pixels.begin(); // This initializes the NeoPixel library.
 pixels.setBrightness(75);
 pinMode(PWRBTN, INPUT);
 pinMode(PWROK_PIN,INPUT);
 pinMode(PWRTRANS, OUTPUT);
 pinMode(PWRSI, OUTPUT); //Soldering Iron Power Control
 pinMode(SPKR, OUTPUT); //set the speaker as output
 pinMode(HG_relay, OUTPUT);//HG Heating element Sense & SET
 pinMode(TSTR_pin, INPUT);//Tester Sense
 pinMode(PWRHG, OUTPUT);//HG FAN control
 pinMode(HGReed, INPUT);//HG Reed Switch
 attachInterrupt(digitalPinToInterrupt(PWRBTN), togglePower, RISING);
 digitalWrite(PWRTRANS, PWRSTATE);
 Serial.begin(115200);
 beep_state=1;
 System_Init();
 LCD_SCAN_DIR Lcd_ScanDir = SCAN_DIR_DFT;  
 LCD_Init( Lcd_ScanDir, 100);
 LCD_Clear(LCD_BACKGROUND);
 GUI_Show();
 tstr_on_status = 1;
 pwr_status = 1;
 FAN_State = 1;
 timer  = millis();
}
//--(end setup )---

void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{ 
  checkPowerOK();
  readVoltages();
  calculatePrintVoltages();
  readTemp();
}

//--(end main loop )---






//*********( THE END )***********
