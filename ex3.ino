// LED matrix (MAX7219) tester
// By Amit Zohar, Dec. 19th, 2018 (v2 Dec. 22nd, 2019)
#include <math.h>
int DIN=3;
int CS=4;
int CLK=2;
int TEMPERATURE_SENSOR_PIN = A0;

typedef struct
{
   int ledIndex;
   bool isTurnedOnWithDigit;
} TempFractionsLeds;  

String DIGIT_NUMBERS[10][3] = {
  {"11111", "10001", "11111"}, // 0
  {"00000", "00010", "11111"}, // 1
  {"11101", "10101", "10111"}, // 2
  {"10101", "10101", "11111"}, // 3
  {"00111", "00100", "11111"}, // 4
  {"10111", "10101", "11101"}, // 5
  {"11111", "10101", "11101"}, // 6
  {"00001", "00001", "11111"}, // 7
  {"11111", "10101", "11111"}, // 8
  {"10111", "10101", "11111"}  // 9
};

int Vo;
float c1 = 0.001129148, c2 = 0.000234125, c3 = 0.0000000876741; //steinhart-hart coeficients for thermistor
TempFractionsLeds fractionsLeds[2]; // {ledIndex, isTurnedOnWithDigit}

// setup function
void setup() {
  Serial.begin(115200);
  pinMode(DIN,OUTPUT);
  pinMode(CS,OUTPUT);
  pinMode(CLK,OUTPUT);
  pinMode(TEMPERATURE_SENSOR_PIN,INPUT);

  initMatrixMode();
}

// loop function
void loop() {
  float temperature = readTemperatureFromSensor();
  int numberTemp = (int)temperature;
  int fraction = ((int)(temperature * 100.0) % 100) + 1;

  printTempToMonitor(numberTemp,fraction);

  resetMatrixLed();
  initFractionLeds(fraction);

  printTemperature(numberTemp);
  delay(200);
}

//The function print the temperature number and fraction on the matrix led
void printTemperature(int numberTemperature) {
  printNumberAndFraction(numberTemperature % 10, 1); //right digit
  printNumberAndFraction(numberTemperature / 10, 0); //left digit
  printOthersFractionLeds();
}

// The function read the temperature from the sensor and convert the value to celcius
float readTemperatureFromSensor(){
  float temperature;
  float R1 = 10000; // value of R1 on board
  float logR2, R2;
  
  Vo = analogRead(TEMPERATURE_SENSOR_PIN);
  R2 = R1 * (1023.0 / (float)Vo - 1.0); //calculate resistance on thermistor
  logR2 = log(R2);
  temperature = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2)); // temperatureerature in Kelvin
  temperature = temperature - 273.15; //convert Kelvin to Celcius

  return temperature;
}

// The function write bit to led and call clk 
void writeBit(bool b) // Write 1 bit to the buffer
{
  digitalWrite(DIN,b);
  digitalWrite(CLK,LOW);
  digitalWrite(CLK,HIGH);
}

// The function set buffer to column matrix led
void latchBuf() // Latch the entire buffer
{
  digitalWrite(CS,LOW);
  digitalWrite(CS,HIGH);
}

// The function init matrix modes (decode mode, scan limit, and shutdown)
void initMatrixMode(){
  //  Set registers: decode mode, scan limit, and shutdown (0x900, 0xB07, 0xC01)
  for (int i=0; i<4; i++) writeBit(LOW);
  writeBit(HIGH);
  for (int i=0; i<2; i++) writeBit(LOW);
  writeBit(HIGH);
  for (int i=0; i<8; i++) writeBit(LOW);
  latchBuf();  
  for (int i=0; i<4; i++) writeBit(LOW);
  writeBit(HIGH);
  writeBit(LOW);
  writeBit(HIGH);
  writeBit(HIGH);
  for (int i=0; i<5; i++) writeBit(LOW);
  for (int i=0; i<3; i++) writeBit(HIGH);
  latchBuf();  
  for (int i=0; i<4; i++) writeBit(LOW);
  for (int i=0; i<2; i++) writeBit(HIGH);
  for (int i=0; i<2; i++) writeBit(LOW);
  for (int i=0; i<7; i++) writeBit(LOW);
  writeBit(HIGH);
  latchBuf();  
}

// The function turn-off all the leds in the matrix leds
void resetMatrixLed(){
  for(int i = 0; i < 8; i ++){
    printColumnByIDAndNumber(i, "00000", false);
  }
}

// The function get a number and indication to unity indices and print on the matrix leds
void printNumberAndFraction(int number, int isUnity){
  int startColumn = 0;
  if(isUnity)
      startColumn = startColumn + 5;
      
  for(int i = 0; i < 3; i++)
    printColumnByIDAndNumber(i + startColumn, DIGIT_NUMBERS[number][i], true);
}

// The function print the temperature in celsius
void printTempToMonitor(int number, int fraction){
  Serial.print("Temperature: "); 
  Serial.print(number);
  Serial.print(".");
  Serial.print(fraction);
  Serial.println(" C"); 
  Serial.print("number: ");
  Serial.println(number);
  Serial.print("fraction: ");
  Serial.println(fraction);
}

// The function check if there are leds to turn-on after print columns
void printOthersFractionLeds(){
  for(int i = 0; i < 2; i++)
    if(!fractionsLeds[i].isTurnedOnWithDigit){
      printColumnByIDAndNumber(fractionsLeds[i].ledIndex, "00000", true); // if column  not updated by digit all column is dark so we just want to update the first led that inner functions check
    }
}

// The function init all the fraction leds indices and turn-off by the fracation number
void initFractionLeds(int fraction){
  int sideOfFractionLeds = fraction / 25; // 25 is the Quarter
  // 0  1  2  3 quarters
  // 01 23 45 56 indices quarters
  fractionsLeds[0].ledIndex = (sideOfFractionLeds * 2); 
  fractionsLeds[0].isTurnedOnWithDigit = false;
  fractionsLeds[1].ledIndex = (sideOfFractionLeds * 2) + 1;
  fractionsLeds[1].isTurnedOnWithDigit = false;
}

// The function update column matrix led by columnID and leds pattern. Update first led quarter temperature if in number column
void printColumnByIDAndNumber(int columnID, String leds, bool needToCheckFirstLed){
  setColumnMatrixLed(columnID);
  setColumnToBuffer(leds,needToCheckFirstLed && isFractionLedNeedToTurnOnWithColumn(columnID));
  latchBuf();
}

// The function check if need to turn-on the first led of this column
bool isFractionLedNeedToTurnOnWithColumn(int columnID){
  for(int i = 0; i < 2; i++){
    if(fractionsLeds[i].ledIndex == columnID){
      fractionsLeds[i].isTurnedOnWithDigit = true;
      return true;
    }
  }
  return false;
}

// The function build the bits to write in column, by the manual columns bit 
void setColumnMatrixLed(int columnID){  
  columnID += 1;

  for (int i = 0; i<4; i++) writeBit(LOW); // trash
  for(int i = 3; i >= 0; i--){
    writeBit(bitRead( columnID, i) ? HIGH : LOW);
  }
}

// The function write the bit binaries if isFractionLedNeedToTurnOnWithColumn is on, turn on first led, else print all three first leds
void setColumnToBuffer(String leds, bool isFractionLedNeedToTurnOnWithColumn){
  int i = 0;
  
  if(isFractionLedNeedToTurnOnWithColumn){
    i++;
    writeBit(HIGH);
  }
  
  for(; i < 3; i++) // start three or two bit always zero because we write in the upper screen
    writeBit(LOW);
    
  for(int i = 0; i < 5; i++)
    writeBit(leds[i] - '0');
}
