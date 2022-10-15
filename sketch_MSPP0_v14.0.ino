//Duration 5 minutes
//LiquidCrystal_I2C 1602 libraries
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h> // Using version 1.2.1 for LiquidCrystal_I2C 1602
/*(address,  En, Rw, Rs, d4, d5, d6, d7, backlighPin, backlighPol pol) 
initializing the LiquidCrystal_I2C 1602*/
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

//DHT11 (humidity and temperature sensor) libraries
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 2 // DHT11 sensor pin 2
DHT dht(DHTPIN, DHT11); //Initializing DHT11

//Gas detector MQ-5
#define AIn_gas A0 // Gas detector MQ-5 pin A0

//Dust sensor
int dustPin = 2; // Dust sensor - Arduino A2 pin
int ledPin = 9; // Led pin 9

// Variables for calculating the value of dust in air
float voltsMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

//Soil humidity
#define AIn_soil A1 //Soil humidity sensor pin A1

//Variables for button functions and calculations
volatile int countInterrupt = 0;
volatile bool flag = 1;
volatile int countButton=0;
volatile unsigned long timerPrev = 0;
volatile int numStorage=0;

/*Defining a variable for making a time management for the sensors
(used for calculation of an arithmetic mean)*/
int timer = 0;
/*Defining variables of arithmetic mean for each sensor*/
double arif_hum = 0, arif_gas = 0, arif_soil = 0, arif_dust = 0, arif_temp, arith = 0;
/*Defining a 2D array, which will contain arithmetic mean values of all sensors. 
(Rows: 0-humidity, 1-temperature, 2-toxic gases, 3-soil humidity, 4-dust density)*/
double arr_data[5][15];
unsigned long password=0;//Creating a variable for password
unsigned long login_password=0;
unsigned long memory=0;


/*Creating characters for drawing graphs
8, 7, 6, 5, 4, 3, 2, 1 lines*/
byte p00[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100};
byte p10[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111};
byte p20[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111};
byte p40[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111};
byte p50[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111};
byte p60[8] = {0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
byte p70[8] = {0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
byte p90[8] = {0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
byte p100[8] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};

void setup()
{
  //Initializing pin modes for each sensor
  pinMode(AIn_gas, INPUT);//Humidity sensor
  pinMode(ledPin, OUTPUT); //Dust sensor
  pinMode(AIn_soil, INPUT);//Soil humidity
  pinMode(13, OUTPUT);//built-in LED
  Serial.begin(9600);//Setting the data rate in bits per second

  pinMode(3, INPUT_PULLUP);//Button pin 3
  //Creating a priority function of interruption for the button (pin 3)
  attachInterrupt(1, buttonFunc, FALLING);

  dht.begin();//Launching the DHT11 sensor

  lcd.begin(16, 2); // sixteen characters across - 2 lines
  //initializing each character for making graphs
  lcd.createChar(0, p00);
  lcd.createChar(1, p10);
  lcd.createChar(2, p20);
  lcd.createChar(3, p40);
  lcd.createChar(4, p50);
  lcd.createChar(5, p60);
  lcd.createChar(6, p70);
  lcd.createChar(7, p90);
  lcd.createChar(8, p100);
  lcd.clear();
}

void loop()
{
  countButton=0;
  while(password<=9999999){
  for(int i=0;i<=9;i++){
      lcd.clear();//clearing the LCD display
      lcd.setCursor(0, 0);
      lcd.print("New password->");
      lcd.setCursor(15, 0); //setting cursor to a specific place on the LCD
      lcd.print(i);
      lcd.setCursor(0, 1);
      lcd.print(password);
      delay(700);
      if(password>=9999999){
        break;
      }
      cli();//stops the work of attachInterrupt
      //int countButton;//Defining a variable for counting button presses
      countButton = countInterrupt;
      if(numStorage!=countButton){
        numStorage=countInterrupt;
        password=(password*10)+i;
        Serial.println(i);
        Serial.println(password);
      }
      sei();//continues the work of attachInterrupt
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Created!");
  lcd.setCursor(0, 1);
  lcd.print(password);
  delay(3000);
  Serial.println("---------------");
  Serial.println(password);
  Serial.println(login_password);
  Serial.println("---------------");
  countInterrupt=0;
  countButton=0;
  
  //A nested loop for 2D array
  for (int r = 0; r < 15; r++) { //Outer loop
    for (int j = 0; j < 5; j++) { //Inner loop
      arr_data[j][r] = -1; //Filling the 2D array by "-1" as a default value
    }
  }
  //taking fifteen arithmetic mean values from each sensor (Outer loop)
  for (int i = 0; i < 15; i++) {
    //updating arithmetic mean values to 0.
    arif_hum = 0;
    arif_temp = 0;
    arif_gas = 0;
    arif_soil = 0;
    arif_dust = 0;
    //a loop for finding an arithmetic mean every 20 senonds (Inner loop)
    for (int j = 0; j < 20; j++) {
      delay(1000);//a delay for 1 second
      timer++;;//a timer counting every second
      digitalWrite(ledPin, LOW); // power on the LED
      //delayMicroseconds(280);//Delay in microseconds(280)

      voltsMeasured = analogRead(dustPin); // Reading the dust sensor value

      delayMicroseconds(40);
      digitalWrite(ledPin, HIGH); // turn the LED off
      delayMicroseconds(9680);

      //calculating dust density
      calcVoltage = voltsMeasured * (5.0 / 1024.0);
      dustDensity = abs(170 * calcVoltage - 0.1);//unit: ug/m3

      //taking the sum of values
      arif_hum += dht.readHumidity(); //in %
      arif_temp += dht.readTemperature(); //in *C
      arif_gas += analogRead(AIn_gas); //in ppm
      arif_soil += abs((1000 - int(analogRead(AIn_soil)))) / 10; //in %
      arif_dust += dustDensity; //in mg/m3

      if (timer % 20 == 0) { //making values a single number for graphs
        arif_hum = arif_hum / 20 / 10;
        arif_temp = arif_temp / 20 / 10;
        arif_gas = arif_gas / 20 / 100;
        arif_soil = arif_soil / 20 / 10;
        arif_dust = arif_dust / 20 / 80;
        arr_data[0][i] = arif_hum;
        arr_data[1][i] = arif_temp;
        arr_data[2][i] = arif_gas;
        arr_data[3][i] = arif_soil;
        arr_data[4][i] = arif_dust;
      }
      cli();//stops the work of attachInterrupt
      //int countButton;//Defining a variable for counting button presses(changing pages on display)
      if (countInterrupt <= 5) { //A loop of counting
        countButton = countInterrupt;
      }
      if (countInterrupt == 6) { //limitation of the maximum number of button presses
        countInterrupt = 0;
      }
      sei();//continues the work of attachInterrupt
      Serial.println(countButton);//printing the number of button presses in the Serial Monitor

      mainPage(countButton);
    }
  }
  passwordVerif(password);
  detachInterrupt(1);
  countButton=0;
  unsigned long prevTime1=millis();
  unsigned long prevTime2=millis();
  while(true){
    if (digitalRead(3)==LOW && (millis() - prevTime1) >= 200) { //Avoiding the bounce of contacts
      flag = 1;
      prevTime1 = millis(); //returns the number of milliseconds since the sketch started running
    }
    if (flag == 1) {
      countButton++;
      flag = 0;
    }
    if(countButton>5){
      countButton=0;
    }
    Serial.println(millis());//printing the number of button presses in the Serial Monitor
    if(millis()-prevTime2>500){
      mainPage(countButton);
      prevTime2=millis();
    }
  }
}

void passwordVerif(unsigned long password){
  enter:
  int countButton=0;
  int numStorage=0;
  while(login_password<=9999999){
  for(int i=0;i<=9;i++){
      lcd.clear();//clearing the LCD display
      lcd.setCursor(0, 0);
      lcd.print("Your password->");
      lcd.setCursor(15, 0); //setting cursor to a specific place on the LCD
      lcd.print(i);
      lcd.setCursor(0, 1);
      lcd.print(login_password);
      delay(700);
      if(login_password>=9999999){
        break;
      }
      cli();//stops the work of attachInterrupt
      int countButton;//Defining a variable for counting button presses
      countButton = countInterrupt;
      if(numStorage!=countButton){
        numStorage=countInterrupt;
        login_password=(login_password*10)+i;
        Serial.println(i);
        Serial.println(login_password);
      }
      sei();//continues the work of attachInterrupt
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Checking ...");
  lcd.setCursor(0, 1);
  lcd.print(login_password);
  delay(1000);
  if(login_password!=password){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("The password is");
    lcd.setCursor(1, 1);
    lcd.print("not correct!");
    delay(3000);
    login_password=0;
    goto enter;
  }
  else{
    Serial.println("---------------");
    Serial.println(password);
    Serial.println(login_password);
    Serial.println("---------------");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("The password is");
    lcd.setCursor(4, 1);
    lcd.print("correct!");
    delay(3000);
    login_password=0;
  }
}

void buttonFunc() { //a function for identifying button presses
  if ((millis() - timerPrev) >= 200) { //Avoiding the bounce of contacts
    flag = 1;
    timerPrev = millis(); //returns the number of milliseconds since the sketch started running
  }
  if (flag == 1) {
    countInterrupt++;
    flag = 0;
  }
}

void mainPage(int countButton){
  if (countButton == 0) { //choosing the pages according to the number of button presses
        lcd.clear();//clearing the LCD display
        lcd.setCursor(0, 0); //setting cursor to a specific place on the LCD
        lcd.print(int(dht.readHumidity())); lcd.print("%H");//Humidity (%)
        lcd.setCursor(0, 1);
        lcd.print(int(dht.readTemperature())); lcd.print("T");//Temperature (*C)
        lcd.setCursor(10, 1);
        lcd.print(int(analogRead(AIn_gas))); lcd.print("G");//Toxic gases (ppm)
        lcd.setCursor(5, 0);
        lcd.print(int(dustDensity)); lcd.print("D");//Dust density (ug/m3)
        lcd.setCursor(5, 1);
        lcd.print(abs((1000 - int(analogRead(AIn_soil)))) / 10); lcd.print("%S"); //Soil humidity (%)
      }
      else {
        drawGraph(countButton);//turning to another page and drawing graph for a specific sensor
      }
}

/*a function for drawing graphs and displaying an arithmetic mean*/
void drawGraph(int countButton) {
  lcd.clear();//clearing the LCD display
  lcd.setCursor(0, 0); //setting cursor to a specific place on the LCD
  lcd.print(countButton);//Displaying a page number
  lcd.setCursor(0, 1);
  switch (countButton) {
    /* Displaying an index for each type of value
    1-humidity, 2-temperature, 3-toxic gases, 4-soil humidity, 5-dust density*/
    case 1: lcd.print("H"); break;
    case 2: lcd.print("T"); break;
    case 3: lcd.print("G"); break;
    case 4: lcd.print("S"); break;
    case 5: lcd.print("D"); break;
  }
  lcd.setCursor(2, 0);
  lcd.print("AM=");//abbreviation of "Arithmetic mean"
  switch (countButton) { //Displaying a dynamic arithmetical mean data
    case 1: lcd.print(arithFunction(0, countButton - 1) * 10); //humidity
      lcd.print("%"); break;
    case 2: lcd.print(arithFunction(0, countButton - 1) * 10); //temperature
      lcd.print("*C"); break;
    case 3: lcd.print(arithFunction(0, countButton - 1) * 100); //toxic gases
      lcd.print("ppm"); break;
    case 4: lcd.print(arithFunction(0, countButton - 1) * 10); //soil humidity
      lcd.print("%"); break;
    case 5: lcd.print(arithFunction(0, countButton - 1) * 80); //dust density
      lcd.print("ug/m3"); break;
    default: break;
  }
  arith = 0; //updating an arithmetic mean value to 0.
  for (int t = 1; t < 16; t++) { //displaying graph for a specific sensor
    lcd.setCursor(t, 1);
    /*taking a round value from the 2D array for displaying 
    the necessary character (making a graph)*/
    switch (round(arr_data[countButton - 1][t - 1])) {
      case 0: lcd.write(byte(1)); break;
      case 1: lcd.write(byte(1)); break;
      case 2: lcd.write(byte(2)); break;
      case 3: lcd.write(byte(3)); break;
      case 4: lcd.write(byte(3)); break;
      case 5: lcd.write(byte(4)); break;
      case 6: lcd.write(byte(5)); break;
      case 7: lcd.write(byte(6)); break;
      case 8: lcd.write(byte(7)); break;
      case 9: lcd.write(byte(7)); break;
      case 10: lcd.write(byte(8)); break;
      default: break;
    }
  }
}
/*a recursive function for calculating a dynamic arithmetic mean data*/
double arithFunction(int startPoint, int rowNum) {
  if (startPoint > 14 || arr_data[rowNum][startPoint] == -1) {
    return arith / startPoint; //Returning an arithmetic value
  }
  arith += arr_data[rowNum][startPoint]; //Calculating the sum of values
  /*turning to the next index of the 2D array for a specific row,
   and calling this(arithFunction) function again*/
  arithFunction(startPoint + 1, rowNum);
}
