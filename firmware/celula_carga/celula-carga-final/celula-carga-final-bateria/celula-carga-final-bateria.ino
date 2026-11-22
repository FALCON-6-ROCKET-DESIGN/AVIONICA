// =========================================================================================
// CÓDIGO 2: MODO BATERIA - DATALOGGER AUTOMÁTICO DE CAMPO
// =========================================================================================
#include <HX711_ADC.h>
#include <EEPROM.h>
#include <LittleFS.h>

#define EEPROM_SIZE 32

const int HX711_dout = 4;
const int HX711_sck  = 5;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

struct Amostra {
  unsigned long tempo;
  float peso;
};

const int TAMANHO_BUFFER = 10; 
Amostra bufferRAM[TAMANHO_BUFFER];
int indiceBuffer = 0;

unsigned long tempoInicio = 0;
const int calVal_eepromAdress = 0;
float calibrationValue = 1.0;

void salvarBufferNaFlash() {
  if (indiceBuffer == 0) return;

  File file = LittleFS.open("/teste_motor.csv", FILE_APPEND);
  if (file) {
    for (int i = 0; i < indiceBuffer; i++) {
      file.print(bufferRAM[i].tempo);
      file.print(",");
      file.println(bufferRAM[i].peso, 3);
    }
    file.close();
    indiceBuffer = 0; // Libera o buffer para as próximas 10 leituras
  }
}

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  LittleFS.begin(true);

  // Carrega a calibração que foi salva previamente na EEPROM
  EEPROM.get(calVal_eepromAdress, calibrationValue);

  LoadCell.begin();
  
  // Zera a balança na inicialização (3 segundos de estabilização mecânica)
  LoadCell.start(3000, true);

  if (!isnan(calibrationValue) && calibrationValue > 0) {
    LoadCell.setCalFactor(calibrationValue);
  }

  while (!LoadCell.update()) delay(1);

  // Sobrescreve e cria um novo arquivo limpo para este teste
  File file = LittleFS.open("/teste_motor.csv", FILE_WRITE);
  if (file) {
    file.println("Tempo_ms,Forca_kg"); // Cabeçalho do CSV
    file.close();
  }

  tempoInicio = millis(); // Marca o tempo zero do disparo
}

void loop() {
  // Leitura contínua na taxa máxima do HX711
  if (LoadCell.update()) {
    float peso = LoadCell.getData();

    // Adiciona a leitura atual ao Buffer na RAM
    bufferRAM[indiceBuffer].tempo = millis() - tempoInicio;
    bufferRAM[indiceBuffer].peso = peso;
    indiceBuffer++;

    // Descarrega na Flash a cada 10 leituras concluídas
    if (indiceBuffer >= TAMANHO_BUFFER) {
      salvarBufferNaFlash();
    }
  }
}