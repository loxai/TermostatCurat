#include <M5StickC.h>
#define USE_MAX31865

#ifdef USE_DHT11
#include "DHT.h"
#define PIN_TEMP 26
DHT dht(PIN_TEMP, DHT11);
#endif

#ifdef USE_MAX31865
#include <Adafruit_MAX31865.h>
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(32, 0, 36, 26);
// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  104.0

#endif

//estat en que cal posar el pin per activar sortida (relé) (false pot ser on, o viceversa, segons circuit, per a PIN 10, LED, CALENTADOR_ON és false)
#define CALENTADOR_ON false

//PIN 10 és el led, pin 33 (?) per enviar senyal a relé
#define PIN_LED 10
#define PIN_CALENTADOR 33


float tempActual = -1;

int totalFases = 6;
//minuts per cada fase: rampa, meseta, rampa, meseta, etc...
int minsFase[] = {1, 25, 10, 10,15,30};
//temps per cada rampa, meseta (normalment, meseta manté temperatura d'anterior rampa)
//important: hi ha d'haver tants grau fase com mins fase = totalFases
int grausFase[] = {28,28,50,50,80,80};
int indexFase = 0;
long tempsIniciFase;
long tempsRestant ;
int grausObjectiu;
float grausIntermig;
bool warnFaseFreda;
bool warnError;
String msg = "ok";

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(TFT_GREEN);

  pinMode(PIN_LED, OUTPUT);//al M5, això ja es configura per defecte
  pinMode(PIN_CALENTADOR, OUTPUT);

#ifdef USE_DHT11
  dht.begin();
#endif
#ifdef USE_MAX31865
  thermo.begin(MAX31865_3WIRE);  // canviar a 2WIRE o 4WIRE segons calgui
#endif

  engegaCalentador(false);

  sleep(3);
}


void loop(){
  tempActual = preguntaTemp();

  if (indexFase < 0){
    if (M5.BtnB.isPressed() || M5.BtnA.isPressed()){
      indexFase = 0;
      tempsIniciFase = millis();
    }
  }
  else{ 
    grausObjectiu = grausFase[indexFase];
    
    long tempsFase = minsFase[indexFase] * 60 * 1000;//durada en millis de fase actual
    tempsRestant = (tempsIniciFase + tempsFase) - millis();

    if (tempActual != -666)
    if (tempsRestant < 0){
      if (tempActual >= grausObjectiu){
        indexFase++;
        if (indexFase >= totalFases){
          engegaCalentador(false);
          msg = "COMPLETAT";
        } else {
          tempsIniciFase = millis();
          msg = "NOVA FASE";
        }
        warnFaseFreda = false;
      } else{
        msg = "(!) ESCALFA EXTRA (!)";
        engegaCalentador(true);
        warnFaseFreda = true;
      }
    } else {

      int grausBase = tempActual;
      if (indexFase > 0)
        grausBase = grausFase[indexFase -1];

      //formula experimental. la idea és calcular la temperatura intermitja q hauríem de tenir en aquest moment de fase, per assegurar suau manteniment de la línia de temp
      grausIntermig = grausBase + (grausObjectiu - tempActual) * (1-tempsRestant/(float)tempsFase);
      if (tempActual < grausIntermig)
      {
        msg = "ESCALFANT";
        engegaCalentador(true);
      }
      else
      {
        msg = "APAGAT";
        engegaCalentador(false);
      }
    }
  }
  mostraDades();
  delay(1000);
}


void mostraDades(){
  //warnFaseFreda vol dir que la fase s'ha acabat
  //però no s'ha arribat a la temperatura objectiu
  //no es passarà de fase fins arribar-hi. mostrat amb fons blau
  M5.Lcd.fillScreen(warnFaseFreda? TFT_BLUE : TFT_BLACK);

  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.setTextColor(warnError? TFT_RED : TFT_WHITE);  
  M5.Lcd.setTextSize(1);

  M5.Lcd.print("Mins Total ");
  M5.Lcd.println((float)millis() / (60 * 1000));

  if (indexFase >= 0){
    M5.Lcd.print("Mins ");
    if (tempsRestant > 0){
      //M5.Lcd.print("Restant ");
      M5.Lcd.print((float)tempsRestant / (60 * 1000));
      M5.Lcd.print(" de ");
    }
    M5.Lcd.println(minsFase[indexFase]);
    
    M5.Lcd.print("Fase ");
    M5.Lcd.print(indexFase);
    M5.Lcd.print(" Temp ");
    M5.Lcd.println(tempActual);
    M5.Lcd.print("º");
    
    M5.Lcd.print("Objectiu ");
    M5.Lcd.print(grausIntermig);
    M5.Lcd.print("º a ");
    M5.Lcd.print(grausFase[indexFase]);
    M5.Lcd.println("º");
  }
  M5.Lcd.print(msg);
}
float preguntaTemp(){
#ifdef USE_DHT11
  return dht.readTemperature();
#endif
#ifdef USE_MAX31865
  msg = "ok";
  uint16_t rtd = thermo.readRTD();

  //Serial.print("RTD value: "); Serial.println(rtd);
  msg += rtd;
  msg += " ";
  float ratio = rtd;
  ratio /= 32768;
  //Serial.print("Ratio = "); Serial.println(ratio,8);
  //Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  //Serial.print("Temperature = "); Serial.println(thermo.temperature(RNOMINAL, RREF));
  //msg += RREF*ratio;
  //msg += " ";
  msg += thermo.temperature(RNOMINAL, RREF);
  //msg += " ";

  // Check and print any faults
  uint8_t fault = thermo.readFault();
  if (fault) {
    msg = "E ";
    //msg += rtd;
    //Serial.print("Fault 0x"); Serial.println(fault, HEX);
    //msg += "FAULT ";
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      msg += "RTD High Threshold";
      //Serial.println("RTD High Threshold"); 
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      msg += "RTD Low Threshold";
      }
    if (fault & MAX31865_FAULT_REFINLOW) {
      msg += "REFIN- > 0.85 x Bias"; 
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      msg += "REFIN- < 0.85 x Bias - FORCE- open";
      //Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      msg += "RTDIN- < 0.85 x Bias - FORCE- open";
      //Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_OVUV) {
      msg += "Under/Over voltage";
      //Serial.println("Under/Over voltage"); 
    }
    thermo.clearFault();
    warnError = true;
    return -666;
  }
  return thermo.temperature(RNOMINAL, RREF);
#endif
}
void engegaCalentador(bool engega){
  if (engega){
    digitalWrite(PIN_CALENTADOR, CALENTADOR_ON);
#ifdef PIN_LED
    digitalWrite(PIN_LED, false);
#endif
  }else{
#ifdef PIN_LED
    digitalWrite(PIN_LED, true);
#endif
    digitalWrite(PIN_CALENTADOR, !CALENTADOR_ON);
  }
}
