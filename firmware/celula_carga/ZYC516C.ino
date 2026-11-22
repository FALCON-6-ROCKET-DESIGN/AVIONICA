#include <HX711_ADC.h>
#include <EEPROM.h>

// Pinos do HX711 (ajuste conforme sua ligação)
const int HX711_dout = 4; // Pino DT do HX711 -> Digital 4
const int HX711_sck = 5;  // Pino SCK do HX711 -> Digital 5

// Objeto HX711
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
float calibrationValue = 1.0;
unsigned long t = 0;
bool serialPrintIntervalPassed = false;

void setup() {
  Serial.begin(57600);
  delay(10);
  Serial.println();
  Serial.println("=== SISTEMA DE CELULA DE CARGA ===");
  Serial.println("Iniciando...");

  // Inicializar a célula de carga
  LoadCell.begin();
  
  // Tempo de estabilização e tara inicial (2 segundos)
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  
  LoadCell.start(stabilizingtime, _tare);
  
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("ERRO: Timeout - verifique a fiação e os pinos");
    Serial.println("Verifique se os pinos DT e SCK estao corretos");
    while (1);
  } else {
    // Carregar valor de calibração da EEPROM
    EEPROM.get(calVal_eepromAdress, calibrationValue);
    
    // Verificar se o valor da EEPROM é válido
    if (isnan(calibrationValue) || calibrationValue == 0.0 || calibrationValue == 1.0) {
      calibrationValue = 696.0; // Valor padrão inicial
      Serial.println("Usando valor de calibracao padrao");
    } else {
      Serial.print("Valor de calibracao carregado da EEPROM: ");
      Serial.println(calibrationValue);
    }
    
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Inicializacao completa");
  }

  // Aguardar primeira leitura
  while (!LoadCell.update()) {
    delay(1);
  }
  
  Serial.println();
  Serial.println("=== SISTEMA PRONTO ===");
  Serial.println("Comandos disponíveis:");
  Serial.println("'t' - Tara (zerar a balanca)");
  Serial.println("'c' - Calibrar com peso conhecido");
  Serial.println("'s' - Salvar calibracao na EEPROM");
  Serial.println("'p' - Mostrar peso atual");
  Serial.println("'d' - Modo leitura continua");
  Serial.println("'i' - Informacoes do sistema");
  Serial.println("==============================");
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 1000; // 1 segundo entre leituras

  // Verificar se há novos dados disponíveis
  if (LoadCell.update()) {
    newDataReady = true;
  }

  // Processar comandos da serial
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    
    switch(inByte) {
      case 't': // Tara
        Serial.println("Executando tara...");
        LoadCell.tareNoDelay();
        break;
        
      case 'c': // Calibrar
        calibrate();
        break;
        
      case 's': // Salvar na EEPROM
        EEPROM.put(calVal_eepromAdress, calibrationValue);
        Serial.print("Valor de calibracao (");
        Serial.print(calibrationValue);
        Serial.println(") salvo na EEPROM");
        break;
        
      case 'p': // Peso atual
        Serial.print("Peso atual: ");
        Serial.print(LoadCell.getData());
        Serial.println(" kg");
        break;
        
      case 'd': // Modo leitura contínua
        serialPrintIntervalPassed = !serialPrintIntervalPassed;
        Serial.println(serialPrintIntervalPassed ? "Leitura continua ATIVADA" : "Leitura continua DESATIVADA");
        break;
        
      case 'i': // Informações
        showSystemInfo();
        break;
        
      case 'r': // Reset
        Serial.println("Reiniciando sistema...");
        setup();
        break;
    }
  }

  // Mostrar leitura contínua se ativada
  if (serialPrintIntervalPassed && newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float weight = LoadCell.getData();
      Serial.print("Peso: ");
      Serial.print(weight, 3); // 3 casas decimais
      Serial.println(" kg");
      newDataReady = 0;
      t = millis();
    }
  }

  // Verificar se a tara foi completada
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tara completa!");
  }
}

void calibrate() {
  Serial.println();
  Serial.println("=== MODO CALIBRACAO ===");
  Serial.println("1. Remova qualquer peso da celula");
  Serial.println("2. Envie 't' para tarar (zerar)");
  
  boolean _resume = false;
  unsigned long timeout = millis();
  
  while (!_resume && (millis() - timeout < 30000)) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') {
        LoadCell.tareNoDelay();
        Serial.println("Tara executada... aguardando conclusao");
      }
    }
    
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tara concluida com sucesso!");
      _resume = true;
    }
    delay(100);
  }

  if (!_resume) {
    Serial.println("Timeout na calibracao");
    return;
  }

  Serial.println();
  Serial.println("3. Coloque um peso CONHECIDO sobre a celula");
  Serial.println("4. Digite o valor do peso (ex: 10.0 para 10kg) e pressione ENTER");

  float known_mass = 0;
  _resume = false;
  timeout = millis();
  
  while (!_resume && (millis() - timeout < 60000)) {
    LoadCell.update();
    
    if (Serial.available() > 0) {
      String input = Serial.readString();
      input.trim();
      
      known_mass = input.toFloat();
      if (known_mass > 0) {
        Serial.print("Peso conhecido: ");
        Serial.print(known_mass);
        Serial.println(" kg");
        _resume = true;
      }
    }
    delay(100);
  }

  if (!_resume) {
    Serial.println("Timeout - nenhum peso valido informado");
    return;
  }

  // Aguardar estabilização do peso
  Serial.println("Aguardando estabilizacao...");
  delay(2000);
  
  // Obter nova valor de calibração
  LoadCell.refreshDataSet();
  calibrationValue = LoadCell.getNewCalibration(known_mass);
  
  if (calibrationValue > 0) {
    LoadCell.setCalFactor(calibrationValue);
    
    Serial.println();
    Serial.println("=== CALIBRACAO CONCLUIDA ===");
    Serial.print("Novo valor de calibracao: ");
    Serial.println(calibrationValue, 6);
    Serial.println("Use 's' para salvar na EEPROM");
    Serial.println("Use 'p' para verificar o peso");
    
    // Testar a calibração
    delay(1000);
    LoadCell.update();
    float measuredWeight = LoadCell.getData();
    Serial.print("Peso medido: ");
    Serial.print(measuredWeight, 3);
    Serial.println(" kg");
    
    float error = abs(measuredWeight - known_mass);
    Serial.print("Erro: ");
    Serial.print(error, 3);
    Serial.println(" kg");
    
  } else {
    Serial.println("ERRO: Falha na calibracao");
  }
  
  Serial.println("==============================");
}

void showSystemInfo() {
  Serial.println();
  Serial.println("=== INFORMACOES DO SISTEMA ===");
  Serial.print("Valor de calibracao: ");
  Serial.println(calibrationValue, 6);
  Serial.print("Tara completa: ");
  Serial.println(LoadCell.getTareStatus() ? "SIM" : "NAO");
  
  LoadCell.update();
  Serial.print("Peso atual: ");
  Serial.print(LoadCell.getData(), 3);
  Serial.println(" kg");
  
  Serial.println("==============================");
}