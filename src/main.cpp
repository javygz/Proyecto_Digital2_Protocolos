// Proyecto 2 - ESP32 esclavo SPI + I2C (version simple)
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <ESP32SPISlave.h>

// ---- Pines SPI (conectados a la Nucleo) ----
#define SPIRELOJ 27
#define SPIMISO 23
#define SPIMOSI 35
#define SPICS 16

// ---- Pines I2C ----
#define I2CSDA 21
#define I2CSCL 22
#define DIRECCIONI2C 0x28

// ---- Otros pines ----
#define potenciometro 34
#define ledr 13                   // LED 1
#define leda 12                // LED 2
#define ledv 14                   // LED 3


#define d4  17
#define d5  19
#define d6  18
#define d7  5
#define en  4
#define rs  2

LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // RS, EN, D4, D5, D6, D7
ESP32SPISlave spiSlave;

// Trama SPI de 4 bytes: [0xAA][numero de led][ms alto][ms bajo]
uint8_t datosRecibidos[4];
uint8_t datosEnviar[4];

volatile uint8_t valorPotenciometro = 0;   // lectura de 0 a 255
char letraUltimoLed = '-';                 // R, A o V (se muestra en la LCD)
int numeroLedEncendido = 0;                // 0 = ninguno
unsigned long momentoApagado = 0;          // millis() en que se apaga el LED

// ---- Potenciometro ----
void leerPotenciometro() {
  valorPotenciometro = map(analogRead(potenciometro), 0, 4095, 0, 255);
}

// ---- LEDs (sin delay) ----
void encenderLed(int numeroLed, unsigned int tiempoMs) {
  numeroLedEncendido = numeroLed;
  momentoApagado = millis() + tiempoMs;
  switch (numeroLed) {
    case 1: letraUltimoLed = 'R'; break;
    case 2: letraUltimoLed = 'A'; break;
    case 3: letraUltimoLed = 'V'; break;
  }
}

void actualizarLeds() {
  digitalWrite(ledr, LOW);
  digitalWrite(leda, LOW);
  digitalWrite(ledv, LOW);
  if (millis() >= momentoApagado) return;  // tiempo cumplido: todo apagado
  switch (numeroLedEncendido) {
    case 1: digitalWrite(ledr, HIGH); break;
    case 2: digitalWrite(leda, HIGH); break;
    case 3: digitalWrite(ledv, HIGH); break;
  }
}

// ---- LCD ----
void actualizarLcd() {
  char texto[17];
  float voltios = valorPotenciometro * 3.3 / 255.0;
  snprintf(texto, sizeof(texto), "%4.2fV %-3u   %c   ", voltios, valorPotenciometro, letraUltimoLed);
  lcd.setCursor(0, 1);
  lcd.print(texto);
}

// ---- I2C ----
void i2cAlRecibir(int cantidad) {
  while (Wire.available()) Wire.read();    // la Nucleo solo pide una medicion
}

void i2cAlPedirDato() {
  Wire.write((uint8_t)valorPotenciometro);
}

// ---- SPI ----
void interpretarComandoSPI() {
  if (datosRecibidos[0] != 0xAA) return;   // trama invalida
  int numeroLed = datosRecibidos[1];
  unsigned int tiempoMs = (datosRecibidos[2] << 8) | datosRecibidos[3];
  if (numeroLed >= 1 && numeroLed <= 3 && tiempoMs > 0) {
    encenderLed(numeroLed, tiempoMs);
  }
}

void revisarSPI() {
  if (spiSlave.hasTransactionsCompletedAndAllResultsHandled()) {
    spiSlave.queue(datosEnviar, datosRecibidos, 4);
    spiSlave.trigger();
  }
  if (spiSlave.hasTransactionsCompletedAndAllResultsReady(1)) {
    spiSlave.numBytesReceivedAll();        // consume el resultado
    interpretarComandoSPI();
  }
}

void setup() {
  pinMode(ledr, OUTPUT);
  pinMode(leda, OUTPUT);
  pinMode(ledv, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Pot1: Pot1: LED:");

  Wire.onReceive(i2cAlRecibir);
  Wire.onRequest(i2cAlPedirDato);
  Serial.begin(115200);
Serial.println(Wire.begin(DIRECCIONI2C, I2CSDA, I2CSCL, 100000) ? "I2C OK" : "I2C FALLO");

  spiSlave.setDataMode(SPI_MODE0);
  spiSlave.setQueueSize(1);
  spiSlave.begin(HSPI, SPIRELOJ, SPIMISO, SPIMOSI, SPICS);
}

void loop() {
  static unsigned long ultimaLecturaPot = 0;
  static unsigned long ultimaActualizacionLcd = 0;
  unsigned long ahora = millis();

  revisarSPI();
  actualizarLeds();
  if (ahora - ultimaLecturaPot >= 20) {
    ultimaLecturaPot = ahora;
    leerPotenciometro();
  }
  if (ahora - ultimaActualizacionLcd >= 200) {
    ultimaActualizacionLcd = ahora;
    actualizarLcd();
  }
}