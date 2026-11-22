/*
=================================================================================
  SISTEMA DE MEDIÇÃO DE EMPUXO - ESP32 + HX711 (MODO PC)
=================================================================================
  - Sensor: Célula de Carga com Módulo HX711
  - Pinos de Comunicação: DT (GPIO 4), SCK (GPIO 5)
  - Funcionalidade: Calibração assistida, tara, leitura serial e extração de CSV
=================================================================================
              ESQUEMA DE LIGAÇÃO: HX711 NO ESP32 (30 PINOS)
=================================================================================
  HX711   ->   ESP32 (30 Pinos)
---------------------------------------------------------------------------------
   VCC    ->   5V / 3V3
   GND    ->   GND
   DT     ->   GPIO D4
   SCK    ->   GPIO D5
=================================================================================
*/

#include <HX711_ADC.h>
#include <EEPROM.h>
#include <LittleFS.h>
x
#define EEPROM_SIZE 32

const int HX711_dout = 4; // GPIO 4 (D4)
const int HX711_sck  = 5; // GPIO 5 (D5)

HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
float calibrationValue = 1.0;
unsigned long tempoAnterior = 0;
bool leituraContinuaAtiva = false;

void setup() {
  Serial.begin(115200);
  delay(10);

  if (!EEPROM.begin(EEPROM_SIZE)) Serial.println("Erro na EEPROM!");
  if (!LittleFS.begin(true)) Serial.println("Erro no LittleFS!");

  Serial.println("\n=== MODALIDADE BANCADA / PC (ESP32 + HX711) ===");

  LoadCell.begin();
  LoadCell.start(2000, true);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("ERRO CRÍTICO: HX711 não detectado!");
    while (1);
  } else {
    EEPROM.get(calVal_eepromAdress, calibrationValue);
    if (isnan(calibrationValue) || calibrationValue <= 0.0 || calibrationValue == 1.0) {
      calibrationValue = 420.0;
      Serial.println("Calibração ausente. Usando padrão (420.0).");
    } else {
      Serial.print("Calibração carregada da EEPROM: ");
      Serial.println(calibrationValue, 6);
    }
    LoadCell.setCalFactor(calibrationValue);
  }

  while (!LoadCell.update()) delay(1);

  exibirMenu();
}

void loop() {
  static boolean novosDados = false;

  if (LoadCell.update()) novosDados = true;

  if (Serial.available() > 0) {
    char cmd = Serial.read();

    switch(cmd) {
      case 't': // Tare
        Serial.println("Zerando balança...");
        LoadCell.tareNoDelay();
        break;

      case 'c': // Calibrate
        modoCalibracao();
        break;

      case 's': // Save EEPROM
        EEPROM.put(calVal_eepromAdress, calibrationValue);
        if (EEPROM.commit()) Serial.println("Calibração salva permanentemente na EEPROM!");
        else Serial.println("Erro ao gravar EEPROM.");
        break;

      case 'p': // Single Read
        Serial.print("Peso Atual: ");
        Serial.print(LoadCell.getData(), 2);
        Serial.println(" kg");
        break;

      case 'd': // Toggle Continuous Read
        leituraContinuaAtiva = !leituraContinuaAtiva;
        Serial.println(leituraContinuaAtiva ? "Leitura contínua: ATIVADA" : "Leitura contínua: DESATIVADA");
        break;

      case 'i': // Tech Info
        exibirInfo();
        break;

      case 'l': // Read saved CSV test file from Flash
        lerArquivoTeste();
        break;

      case 'r': // Restart
        Serial.println("Reiniciando ESP32...");
        delay(100);
        ESP.restart();
        break;
    }
  }

  // Leitura contínua atualizada para 100ms (0,1s)
  if (leituraContinuaAtiva && novosDados) {
    if (millis() > tempoAnterior + 100) {
      Serial.print("Peso: ");
      Serial.print(LoadCell.getData(), 2);
      Serial.println(" kg");
      novosDados = false;
      tempoAnterior = millis();
    }
  }

  if (LoadCell.getTareStatus()) Serial.println(">> Balança Zerada! <<");
}

void exibirMenu() {
  Serial.println("\n=== MENU DE COMANDOS (MONITOR SERIAL @ 115200) ===");
  Serial.println(" 't' -> Executar Tara (Zerar)");
  Serial.println(" 'c' -> Iniciar Calibração Assistida");
  Serial.println(" 's' -> Salvar Fator de Calibração na EEPROM");
  Serial.println(" 'p' -> Leitura Única de Peso");
  Serial.println(" 'd' -> Ativar/Desativar Leitura Contínua na tela");
  Serial.println(" 'l' -> BAIXAR/LER Teste Gravado na Memória Flash (CSV)");
  Serial.println(" 'i' -> Mostrar Informações do Sistema");
  Serial.println(" 'r' -> Reiniciar ESP32");
  Serial.println("==================================================");
}

void modoCalibracao() {
  Serial.println("\n=== CALIBRAÇÃO ===");
  Serial.println("1. Remova todo o peso e envie 't'");
  boolean ok = false;
  unsigned long timeout = millis();

  while (!ok && (millis() - timeout < 30000)) {
    LoadCell.update();
    if (Serial.available() > 0 && Serial.read() == 't') LoadCell.tareNoDelay();
    if (LoadCell.getTareStatus()) ok = true;
    delay(10);
  }
  if (!ok) { Serial.println("Tempo esgotado!"); return; }

  Serial.println("2. Coloque um peso conhecido e digite o valor em kg (Ex: 20.0):");
  float massa = 0;
  ok = false;
  timeout = millis();
  while(Serial.available()) Serial.read();

  while (!ok && (millis() - timeout < 60000)) {
    LoadCell.update();
    if (Serial.available() > 0) {
      massa = Serial.readString().toFloat();
      if (massa > 0) ok = true;
    }
    delay(10);
  }
  if (!ok) { Serial.println("Tempo esgotado!"); return; }

  delay(3000);
  LoadCell.refreshDataSet();
  calibrationValue = LoadCell.getNewCalibration(massa);

  if (calibrationValue != 0) {
    LoadCell.setCalFactor(calibrationValue);
    Serial.print("Novo Fator Gerado: ");
    Serial.println(calibrationValue, 6);
    Serial.println("Envie o comando 's' para salvar na EEPROM!");
  }
}

void exibirInfo() {
  Serial.println("\n=== INFORMAÇÕES ===");
  Serial.print("Fator de Calibração Atual: ");
  Serial.println(calibrationValue, 6);
  Serial.print("Peso Lido Agora: ");
  Serial.print(LoadCell.getData(), 2);
  Serial.println(" kg");
}

void lerArquivoTeste() {
  File file = LittleFS.open("/teste_motor.csv", FILE_READ);
  if (file) {
    Serial.println("\n--- INÍCIO DO ARQUIVO CSV ---");
    while (file.available()) Serial.write(file.read());
    file.close();
    Serial.println("--- FIM DO ARQUIVO CSV ---");
  } else {
    Serial.println("\nNenhum teste gravado na memória Flash.");
  }
}