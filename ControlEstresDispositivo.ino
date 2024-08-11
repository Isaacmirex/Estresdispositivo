#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PulseSensorPlayground.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Dirección I2C del módulo LCD
#define I2C_ADDR 0x27  // Cambia esta dirección si es diferente

// Tamaño del LCD (en columnas y filas)
#define LCD_COLS 16
#define LCD_ROWS 2

// se define las librerias de temperatura:
OneWire ourWire(26);                  // Se establece el pin 26 como bus OneWire
DallasTemperature sensors(&ourWire);  // Se declara una variable u objeto para nuestro sensor

// constantes de la respiracion
const int sensorPin = 13;                          // Pin digital al que está conectado el sensor MQ3
const long rango = 10;                             // Umbral más sensible para detectar cambios en la señal
long intervaloBPM = 60000;                         // 1 minuto en milisegundos
long tiempoEstabilizacion = 20000;                 // 20 segs de estabilización

int lecturaSensor = 0;
int pulsos = 0;
int pulsosPorMinuto = 0;
int conteoCaidas = 0;
bool respiracionDetectada = false;

unsigned long tiempoAnteriorBPM = 0;
unsigned long tiempoAnteriorLectura = 0;
unsigned long inicioEstabilizacion = 0;
const unsigned long intervaloLectura = 80;  // Intervalo de tiempo entre lecturas en milisegundos
unsigned long tiempoRestante = intervaloBPM;

// Definición de los pines para los pulsadores
const int PIN_PULSADOR_1 = 15;
const int PIN_PULSADOR_2 = 2;
const int PIN_PULSADOR_3 = 4;
const int PIN_PULSADOR_4 = 5;
const int PIN_PULSADOR_5 = 18;
const int PIN_PULSADOR_6 = 23;

bool flag_pulso = false;
bool flag_temp = false;
bool flag_fr = false;

// pulso
const int PulseWire = 27;
int Threshold = 550;
PulseSensorPlayground pulseSensor;
int myBPM = 0;

// Inicializar el objeto LiquidCrystal_I2C
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

void setup() {
  sensors.begin();
  // Serial.begin(9600);
  // Configure the PulseSensor object, by assigning our variables to it.
  pulseSensor.analogInput(PulseWire);
  // pulseSensor.blinkOnPulse(LED);       //auto-magically blink Arduino's LED with heartbeat.
  pulseSensor.setThreshold(Threshold);

  // Double-check the "pulseSensor" object was created and "began" seeing a signal.
  if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object !");
  }  // This prints one time at Arduino power-up, or on Arduino reset.

  // Inicializar el LCD
  lcd.init();

  // Encender la retroiluminación del LCD (opcional)
  lcd.backlight();

  // Establecer el cursor en la primera columna (0) y primera fila (0)
  lcd.setCursor(0, 0);

  // Mostrar el mensaje "Control Ansiedad"
  lcd.print("Control Ansiedad");

  lcd.setCursor(0, 1);
  lcd.print("Por Isaac Romero.");

  // Configurar pines de los pulsadores como entradas
  pinMode(PIN_PULSADOR_1, INPUT);  // B2
  pinMode(PIN_PULSADOR_2, INPUT);  // B1
  pinMode(PIN_PULSADOR_3, INPUT);  // B0
  pinMode(PIN_PULSADOR_4, INPUT);  // A2
  pinMode(PIN_PULSADOR_5, INPUT);  // A1
  pinMode(PIN_PULSADOR_6, INPUT);  // A0
  delay(1000);
  MenuHome();
}

void loop() {
  if (flag_pulso) {
    if (pulseSensor.sawStartOfBeat()) {         // Constantly test to see if "a beat happened".
      myBPM = pulseSensor.getBeatsPerMinute();  // Calls function on our pulseSensor object that returns BPM as an "int".
                                                // "myBPM" hold this BPM value now.
    }

    delay(20);  // Considered best practice in a simple sketch.

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pulso: " + String(myBPM+5) + " LPM");
    lcd.setCursor(0, 1);
    lcd.print("5=salir ");
    if (digitalRead(PIN_PULSADOR_5) == HIGH) {
      flag_pulso = false;
      MenuHome();
    }
  }

  if (flag_temp) {
    sensors.requestTemperatures();            // Se envía el comando para leer la temperatura
    float temp = sensors.getTempCByIndex(0);  // Se obtiene la temperatura en ºC
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp:" + String(temp+2) + " C");
    lcd.setCursor(0, 1);
    lcd.print("5=salir");
    if (digitalRead(PIN_PULSADOR_5) == HIGH) {
      flag_temp = false;
      MenuHome();
    }
  }

  if (flag_fr) {
    unsigned long tiempoActual = millis();
    // Esperar el período de estabilización antes de comenzar las lecturas
    if (tiempoActual - inicioEstabilizacion < tiempoEstabilizacion) {
      if ((tiempoActual - inicioEstabilizacion) % 1000 == 0) {
        int tiempoRestanteEstabilizacion = (tiempoEstabilizacion - (tiempoActual - inicioEstabilizacion)) / 1000;
        Serial.print("Calentando sensor... ");
        Serial.print("Tiempo restante: ");
        Serial.print(tiempoRestanteEstabilizacion);
        Serial.println(" segundos");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Iniciando sensor");
        lcd.setCursor(0, 1);
        lcd.print("Tiempo: ");
        lcd.print(tiempoRestanteEstabilizacion);
        lcd.print(" seg");
      }
       if (digitalRead(PIN_PULSADOR_5) == HIGH) {
      flag_fr = false;
      MenuHome();
    }
      return;
    }

    if (tiempoActual - tiempoAnteriorLectura >= intervaloLectura) {
      lecturaSensor = analogRead(sensorPin);

      if (lecturaSensor < 100) {
        conteoCaidas++;
      } else {
        conteoCaidas = 0;
        respiracionDetectada = false;
      }

      if (conteoCaidas >= 7 && !respiracionDetectada) {
        pulsos++;
        Serial.println("Respiración detectada");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Respiracion");
        respiracionDetectada = true;
        conteoCaidas = 0;
      }

      tiempoAnteriorLectura = tiempoActual;
    }

    if (tiempoActual - tiempoAnteriorBPM >= 1000) {  // Actualizar la cuenta regresiva cada segundo
      tiempoRestante -= 1000;
      Serial.print("Tiempo restante: ");
      Serial.print(tiempoRestante / 1000);
      Serial.println(" segundos");
      tiempoAnteriorBPM = tiempoActual;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tiempo:");
      lcd.print(tiempoRestante / 1000);
      lcd.print("Seg");
    }

    if (tiempoRestante == 0) {
      pulsosPorMinuto = pulsos;
      Serial.print("Frecuencia respiratoria por minuto: ");
      Serial.println(pulsosPorMinuto);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("FR: ");
      lcd.print(pulsosPorMinuto);
      lcd.print(" min");
      lcd.setCursor(0, 1);
      lcd.print("5=salir");
      delay(10000);
      pulsos = 0;
      tiempoRestante = intervaloBPM;    // Reiniciar la cuenta regresiva
      inicioEstabilizacion = millis();  // Registrar el tiempo de inicio para la estabilización
    }
     if (digitalRead(PIN_PULSADOR_5) == HIGH) {
      flag_fr = false;
      MenuHome();
    }
   
  }

}

void MenuHome() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1=FRESP 2=TEMP");
  lcd.setCursor(0, 1);
  lcd.print("3=PULSO");
  char key = '0';
  while (true) {
    if (digitalRead(PIN_PULSADOR_1) == HIGH) {
      FR();
      break;
    } else if (digitalRead(PIN_PULSADOR_2) == HIGH) {
      TEP();
      break;
    } else if (digitalRead(PIN_PULSADOR_3) == HIGH) {
      PULSE();
      break;
    }
  }
}

void TEP() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INICIO TEMP");
  lcd.setCursor(0, 1);
  lcd.print("5=Salir 6=Inicio");
  char key = '0';
  while (true) {
    if (digitalRead(PIN_PULSADOR_5) == HIGH) {
      MenuHome();
      break;
    } else if (digitalRead(PIN_PULSADOR_6) == HIGH) {
      flag_temp = true;
      loop();
      break;
    }
  }
}

void PULSE() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INICIO PULSO");
  lcd.setCursor(0, 1);
  lcd.print("5=Salir 6=Iniciar");
  while (true) {
    if (digitalRead(PIN_PULSADOR_5) == HIGH) {
      MenuHome();
      break;
    } else if (digitalRead(PIN_PULSADOR_6) == HIGH) {
      INICIOPULSO();
      break;
    }
  }
}

void INICIOPULSO() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INICIANDO PULSO");
  lcd.setCursor(0, 1);
  lcd.print("Colocar sensor");
  delay(100);
  flag_pulso = true;
  loop();
}

void FR() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INICIANDO FR");
  lcd.setCursor(0, 1);
  lcd.print("5=Salir 6=Iniciar ");
  char key = '0';
  while (true) {
    if (digitalRead(PIN_PULSADOR_5) == HIGH) {
      MenuHome();
      break;
    } else if (digitalRead(PIN_PULSADOR_6) == HIGH) {
      INICIOFR();
      break;
    }
  }
}

void INICIOFR() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INICIANDO PULSO");
  lcd.setCursor(0, 1);
  lcd.print("Colocar sensor");
  delay(100);
  flag_fr = true;

  // Reiniciar variables
  inicioEstabilizacion = millis();
  tiempoRestante = intervaloBPM;
  pulsos = 0;
  conteoCaidas = 0;
  respiracionDetectada = false;
  loop();
}