Projecte Tutorial Arduino per a AirSolé

Exemple de configuració temperatures/minuts per fase
int totalFases = 6;
//minuts per cada fase: rampa, meseta, rampa, meseta, etc...
int minsFase[] = {1, 25, 10, 10,15,30};
//temps per cada rampa, meseta (normalment, meseta manté temperatura d'anterior rampa)
//important: hi ha d'haver tants grau fase com mins fase = totalFases
int grausFase[] = {28,28,50,50,80,80};


// PINS SPI: CS, DI, DO, CLK. 32, 0, 36, 26

Cal calibrar segons sensor
// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  104.0

thermo.begin(MAX31865_3WIRE);  // canviar a 2WIRE o 4WIRE segons calgui