#include <HX711_ADC.h>
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// =====================================================
// PINOS DO HX711
// =====================================================

const int HX711_DOUT = 4;
const int HX711_SCK  = 5;

HX711_ADC LoadCell(HX711_DOUT, HX711_SCK);

// =====================================================
// PINOS DO CARTÃO SD
// =====================================================

const int SD_CS   = 13;
const int SD_SCK  = 18;
const int SD_MISO = 19;
const int SD_MOSI = 23;

// Frequência do barramento SPI
const uint32_t SD_SPI_FREQUENCY = 10000000; // 10 MHz

// =====================================================
// EEPROM E CALIBRAÇÃO
// =====================================================

const int EEPROM_SIZE = 64;
const int CALIBRATION_EEPROM_ADDRESS = 0;

float calibrationValue = 696.0;
bool eepromReady = false;

// =====================================================
// CARTÃO SD
// =====================================================

File dataFile;
String dataFileName;

volatile bool sdReady = false;
volatile bool sdWriteError = false;
volatile bool forceFlushRequested = false;

// Contadores
volatile uint32_t samplesAcquired = 0;
volatile uint32_t samplesSaved = 0;
volatile uint32_t samplesDropped = 0;

// =====================================================
// FILA DE DADOS
// =====================================================

struct WeightSample {
  uint64_t timeUs;
  float weightKg;
};

// 512 amostras representam aproximadamente 6,4 s em 80 Hz
const int SD_QUEUE_LENGTH = 512;

QueueHandle_t sdQueue = NULL;
TaskHandle_t sdTaskHandle = NULL;

// =====================================================
// BUFFER DE GRAVAÇÃO
// =====================================================

const size_t SD_BUFFER_SIZE = 4096;

char sdWriteBuffer[SD_BUFFER_SIZE];
size_t sdWriteBufferLength = 0;
uint32_t sdBufferedSamples = 0;

// =====================================================
// CONTROLE DA SERIAL
// =====================================================

bool continuousReading = false;

// =====================================================
// PROTÓTIPOS
// =====================================================

void calibrate();
void showSystemInfo();
void updateHx711For(unsigned long durationMs);

bool initializeSD();
String createDataFileName();

void sdWriterTask(void *parameter);
void writeSdBuffer(bool flushFile);

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(200);

  delay(500);

  Serial.println();
  Serial.println(F("===================================="));
  Serial.println(F(" HX711 80 Hz + GRAVACAO AUTOMATICA"));
  Serial.println(F("===================================="));

  // ---------------------------------------------------
  // EEPROM
  // ---------------------------------------------------

  eepromReady = EEPROM.begin(EEPROM_SIZE);

  if (!eepromReady) {
    Serial.println(F("AVISO: Falha ao inicializar EEPROM"));
    calibrationValue = 696.0;

  } else {
    EEPROM.get(
      CALIBRATION_EEPROM_ADDRESS,
      calibrationValue
    );

    if (isnan(calibrationValue) ||
        calibrationValue == 0.0 ||
        calibrationValue == 1.0) {

      calibrationValue = 696.0;

      Serial.println(F("EEPROM sem calibracao valida"));
      Serial.print(F("Usando fator padrao: "));
      Serial.println(calibrationValue, 6);

    } else {
      Serial.print(F("Calibracao carregada: "));
      Serial.println(calibrationValue, 6);
    }
  }

  // ---------------------------------------------------
  // HX711
  // ---------------------------------------------------

  Serial.println(F("Inicializando HX711..."));

  LoadCell.begin();

  /*
   * Média móvel de 16 amostras.
   * Em 80 Hz, corresponde a aproximadamente 0,2 s.
   */
  LoadCell.setSamplesInUse(16);

  unsigned long stabilizingTime = 2000;
  bool executeInitialTare = true;

  LoadCell.start(
    stabilizingTime,
    executeInitialTare
  );

  if (LoadCell.getTareTimeoutFlag() ||
      LoadCell.getSignalTimeoutFlag()) {

    Serial.println(F("ERRO: HX711 nao respondeu"));
    Serial.println(F("Verifique DT, SCK, alimentacao e GND"));

    while (true) {
      delay(1000);
    }
  }

  LoadCell.setCalFactor(calibrationValue);

  // Aguarda a primeira amostra
  while (!LoadCell.update()) {
    delay(1);
  }

  Serial.println(F("HX711 inicializado"));

  // ---------------------------------------------------
  // CARTÃO SD
  // ---------------------------------------------------

  Serial.println(F("Inicializando cartao SD..."));

  if (initializeSD()) {
    Serial.println(F("Cartao SD inicializado"));

    Serial.print(F("Arquivo criado: "));
    Serial.println(dataFileName);

    Serial.println(F("Gravacao automatica ATIVADA"));

  } else {
    Serial.println(F("ERRO: Cartao SD nao inicializado"));
    Serial.println(F("O sistema continuara sem gravar"));
  }

  // ---------------------------------------------------
  // MENU
  // ---------------------------------------------------

  Serial.println();
  Serial.println(F("=== SISTEMA PRONTO ==="));
  Serial.println(F("'t' - Executar tara"));
  Serial.println(F("'c' - Calibrar"));
  Serial.println(F("'s' - Salvar calibracao na EEPROM"));
  Serial.println(F("'p' - Mostrar peso atual"));
  Serial.println(F("'d' - Ativar/desativar exibicao em 80 Hz"));
  Serial.println(F("'f' - Forcar salvamento no cartao"));
  Serial.println(F("'i' - Informacoes do sistema"));
  Serial.println(F("'r' - Reiniciar o ESP32"));
  Serial.println(F("=============================="));
}

// =====================================================
// LOOP PRINCIPAL
// =====================================================

void loop() {
  /*
   * update() deve ser chamado continuamente.
   * Retorna true quando uma nova amostra está pronta.
   */
  bool newSample = LoadCell.update();

  // ---------------------------------------------------
  // NOVA AMOSTRA
  // ---------------------------------------------------

  if (newSample) {
    float weight = LoadCell.getData();

    samplesAcquired++;

    /*
     * Toda nova amostra é enviada automaticamente
     * para a fila de gravação.
     */
    if (sdReady &&
        !sdWriteError &&
        sdQueue != NULL) {

      WeightSample sample;

      sample.timeUs =
        (uint64_t)esp_timer_get_time();

      sample.weightKg = weight;

      /*
       * Não bloqueia caso o cartão fique momentaneamente
       * ocupado. A fila armazena temporariamente os dados.
       */
      if (xQueueSend(
            sdQueue,
            &sample,
            0
          ) != pdPASS) {

        samplesDropped++;
      }
    }

    // Exibição opcional no Monitor Serial
    if (continuousReading) {
      Serial.print(F("Peso: "));
      Serial.print(weight, 3);
      Serial.println(F(" kg"));
    }
  }

  // ---------------------------------------------------
  // COMANDOS DA SERIAL
  // ---------------------------------------------------

  if (Serial.available() > 0) {
    char command = Serial.read();

    switch (command) {
      case 't':
      case 'T':
        Serial.println(F("Executando tara..."));
        LoadCell.tareNoDelay();
        break;

      case 'c':
      case 'C':
        calibrate();
        break;

      case 's':
      case 'S':
        if (eepromReady) {
          EEPROM.put(
            CALIBRATION_EEPROM_ADDRESS,
            calibrationValue
          );

          if (EEPROM.commit()) {
            Serial.print(F("Calibracao salva: "));
            Serial.println(calibrationValue, 6);
          } else {
            Serial.println(F("Erro ao salvar na EEPROM"));
          }

        } else {
          Serial.println(F("EEPROM nao inicializada"));
        }
        break;

      case 'p':
      case 'P':
        Serial.print(F("Peso atual: "));
        Serial.print(LoadCell.getData(), 3);
        Serial.println(F(" kg"));
        break;

      case 'd':
      case 'D':
        continuousReading = !continuousReading;

        if (continuousReading) {
          Serial.println(F("Exibicao em 80 Hz ATIVADA"));
        } else {
          Serial.println(F("Exibicao em 80 Hz DESATIVADA"));
        }
        break;

      case 'f':
      case 'F':
        if (sdReady && !sdWriteError) {
          forceFlushRequested = true;
          Serial.println(F("Salvamento solicitado"));
        } else {
          Serial.println(F("Cartao SD indisponivel"));
        }
        break;

      case 'i':
      case 'I':
        showSystemInfo();
        break;

      case 'r':
      case 'R':
        Serial.println(F("Salvando e reiniciando..."));

        forceFlushRequested = true;

        delay(1000);
        ESP.restart();
        break;
    }
  }

  // ---------------------------------------------------
  // TARA FINALIZADA
  // ---------------------------------------------------

  if (LoadCell.getTareStatus()) {
    Serial.println(F("Tara completa!"));
  }

  // ---------------------------------------------------
  // AVISO DE ERRO NO CARTÃO
  // ---------------------------------------------------

  static bool sdErrorReported = false;

  if (sdWriteError && !sdErrorReported) {
    Serial.println();
    Serial.println(F("ERRO durante a gravacao no cartao SD"));
    Serial.println(F("Verifique o cartao e a alimentacao"));

    sdErrorReported = true;
  }
}

// =====================================================
// INICIALIZAÇÃO DO CARTÃO SD
// =====================================================

bool initializeSD() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(
    SD_SCK,
    SD_MISO,
    SD_MOSI,
    SD_CS
  );

  if (!SD.begin(
        SD_CS,
        SPI,
        SD_SPI_FREQUENCY
      )) {

    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    return false;
  }

  dataFileName = createDataFileName();

  if (dataFileName.length() == 0) {
    Serial.println(F("Limite de arquivos atingido"));
    return false;
  }

  dataFile = SD.open(
    dataFileName.c_str(),
    FILE_WRITE
  );

  if (!dataFile) {
    return false;
  }

  // Cabeçalho do arquivo CSV
  dataFile.println(F("tempo_us,peso_kg"));
  dataFile.flush();

  // Cria a fila de armazenamento temporário
  sdQueue = xQueueCreate(
    SD_QUEUE_LENGTH,
    sizeof(WeightSample)
  );

  if (sdQueue == NULL) {
    dataFile.close();
    return false;
  }

  sdReady = true;

  /*
   * Cria uma tarefa separada para gravar o cartão.
   * A leitura do HX711 permanece no núcleo principal.
   */
  BaseType_t taskResult = xTaskCreatePinnedToCore(
    sdWriterTask,
    "SD_Writer",
    6144,
    NULL,
    1,
    &sdTaskHandle,
    0
  );

  if (taskResult != pdPASS) {
    sdReady = false;

    dataFile.close();

    vQueueDelete(sdQueue);
    sdQueue = NULL;

    return false;
  }

  return true;
}

// =====================================================
// CRIAÇÃO AUTOMÁTICA DO NOME DO ARQUIVO
// =====================================================

String createDataFileName() {
  char candidate[24];

  /*
   * Procura automaticamente um nome disponível:
   * dados_001.csv, dados_002.csv etc.
   */
  for (int number = 1; number <= 999; number++) {
    snprintf(
      candidate,
      sizeof(candidate),
      "/dados_%03d.csv",
      number
    );

    if (!SD.exists(candidate)) {
      return String(candidate);
    }
  }

  return String("");
}

// =====================================================
// TAREFA DE GRAVAÇÃO
// =====================================================

void sdWriterTask(void *parameter) {
  WeightSample sample;

  unsigned long lastWriteTime = millis();
  unsigned long lastFlushTime = millis();

  while (true) {
    /*
     * Aguarda uma nova amostra por até 20 ms.
     */
    bool received = xQueueReceive(
      sdQueue,
      &sample,
      pdMS_TO_TICKS(20)
    ) == pdTRUE;

    if (received && !sdWriteError) {
      char line[64];

      int lineLength = snprintf(
        line,
        sizeof(line),
        "%llu,%.3f\n",
        (unsigned long long)sample.timeUs,
        sample.weightKg
      );

      if (lineLength > 0) {
        /*
         * Se não houver espaço, grava o buffer
         * antes de adicionar a nova linha.
         */
        if (sdWriteBufferLength + lineLength >=
            SD_BUFFER_SIZE) {

          writeSdBuffer(false);
          lastWriteTime = millis();
        }

        if (!sdWriteError &&
            lineLength < SD_BUFFER_SIZE) {

          memcpy(
            sdWriteBuffer + sdWriteBufferLength,
            line,
            lineLength
          );

          sdWriteBufferLength += lineLength;
          sdBufferedSamples++;
        }
      }
    }

    unsigned long currentTime = millis();

    /*
     * Grava automaticamente:
     * - quando o buffer chega a 3 KB;
     * - ou a cada 500 ms.
     */
    if (!sdWriteError &&
        sdWriteBufferLength > 0 &&
        (sdWriteBufferLength >= 3072 ||
         currentTime - lastWriteTime >= 500)) {

      writeSdBuffer(false);
      lastWriteTime = currentTime;
    }

    /*
     * Sincroniza fisicamente o arquivo
     * automaticamente a cada 2 segundos.
     */
    if (!sdWriteError &&
        currentTime - lastFlushTime >= 2000) {

      writeSdBuffer(true);
      lastFlushTime = currentTime;
    }

    // Salvamento manual opcional pelo comando "f"
    if (forceFlushRequested && !sdWriteError) {
      writeSdBuffer(true);

      forceFlushRequested = false;

      lastWriteTime = millis();
      lastFlushTime = millis();
    }

    if (sdWriteError) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

// =====================================================
// GRAVAÇÃO DO BUFFER NO CARTÃO
// =====================================================

void writeSdBuffer(bool flushFile) {
  if (!sdReady || !dataFile) {
    sdWriteError = true;
    return;
  }

  if (sdWriteBufferLength > 0) {
    size_t expectedBytes = sdWriteBufferLength;

    size_t writtenBytes = dataFile.write(
      (const uint8_t *)sdWriteBuffer,
      expectedBytes
    );

    if (writtenBytes != expectedBytes) {
      sdWriteError = true;
    } else {
      samplesSaved += sdBufferedSamples;
    }

    sdWriteBufferLength = 0;
    sdBufferedSamples = 0;
  }

  if (flushFile && !sdWriteError) {
    dataFile.flush();
  }
}

// =====================================================
// CALIBRAÇÃO
// =====================================================

void calibrate() {
  Serial.println();
  Serial.println(F("=== MODO CALIBRACAO ==="));
  Serial.println(F("Remova qualquer peso da celula"));
  Serial.println(F("Envie 't' para executar a tara"));

  bool resumeCalibration = false;
  unsigned long timeoutStart = millis();

  // Tara
  while (!resumeCalibration &&
         millis() - timeoutStart < 30000UL) {

    LoadCell.update();

    if (Serial.available() > 0) {
      char received = Serial.read();

      if (received == 't' ||
          received == 'T') {

        LoadCell.tareNoDelay();
        Serial.println(F("Tara iniciada..."));
      }
    }

    if (LoadCell.getTareStatus()) {
      Serial.println(F("Tara concluida"));
      resumeCalibration = true;
    }

    delay(1);
  }

  if (!resumeCalibration) {
    Serial.println(F("Timeout durante a tara"));
    return;
  }

  // Peso conhecido
  Serial.println();
  Serial.println(F("Coloque o peso conhecido"));
  Serial.println(F("Digite o peso em kg usando ponto"));
  Serial.println(F("Exemplo: 10.0"));

  float knownMass = 0.0;
  resumeCalibration = false;
  timeoutStart = millis();

  while (!resumeCalibration &&
         millis() - timeoutStart < 60000UL) {

    LoadCell.update();

    if (Serial.available() > 0) {
      knownMass = Serial.parseFloat();

      if (knownMass > 0.0) {
        Serial.print(F("Peso conhecido: "));
        Serial.print(knownMass, 3);
        Serial.println(F(" kg"));

        resumeCalibration = true;
      }
    }

    delay(1);
  }

  if (!resumeCalibration) {
    Serial.println(F("Nenhum peso valido informado"));
    return;
  }

  Serial.println(F("Aguardando estabilizacao..."));

  updateHx711For(2000);

  LoadCell.refreshDataSet();

  calibrationValue =
    LoadCell.getNewCalibration(knownMass);

  if (isnan(calibrationValue) ||
      calibrationValue == 0.0) {

    Serial.println(F("Erro durante a calibracao"));
    return;
  }

  LoadCell.setCalFactor(calibrationValue);

  Serial.println();
  Serial.println(F("=== CALIBRACAO CONCLUIDA ==="));

  Serial.print(F("Novo fator: "));
  Serial.println(calibrationValue, 6);

  Serial.println(F("Use 's' para salvar na EEPROM"));

  updateHx711For(1000);

  float measuredWeight = LoadCell.getData();
  float error = fabsf(measuredWeight - knownMass);

  Serial.print(F("Peso medido: "));
  Serial.print(measuredWeight, 3);
  Serial.println(F(" kg"));

  Serial.print(F("Erro: "));
  Serial.print(error, 3);
  Serial.println(F(" kg"));
}

// =====================================================
// INFORMAÇÕES DO SISTEMA
// =====================================================

void showSystemInfo() {
  Serial.println();
  Serial.println(F("=== INFORMACOES DO SISTEMA ==="));

  Serial.print(F("Fator de calibracao: "));
  Serial.println(calibrationValue, 6);

  Serial.print(F("Peso atual: "));
  Serial.print(LoadCell.getData(), 3);
  Serial.println(F(" kg"));

  Serial.print(F("Cartao SD: "));

  if (sdReady && !sdWriteError) {
    Serial.println(F("OK"));
  } else {
    Serial.println(F("ERRO"));
  }

  Serial.print(F("Arquivo: "));

  if (dataFileName.length() > 0) {
    Serial.println(dataFileName);
  } else {
    Serial.println(F("nenhum"));
  }

  Serial.print(F("Gravacao automatica: "));

  if (sdReady && !sdWriteError) {
    Serial.println(F("ATIVADA"));
  } else {
    Serial.println(F("INDISPONIVEL"));
  }

  Serial.print(F("Amostras adquiridas: "));
  Serial.println(samplesAcquired);

  Serial.print(F("Amostras salvas: "));
  Serial.println(samplesSaved);

  Serial.print(F("Amostras perdidas: "));
  Serial.println(samplesDropped);

  if (sdQueue != NULL) {
    Serial.print(F("Amostras aguardando gravacao: "));
    Serial.println(uxQueueMessagesWaiting(sdQueue));
  }

  Serial.println(F("=============================="));
}

// =====================================================
// ATUALIZA O HX711 DURANTE ESPERAS
// =====================================================

void updateHx711For(unsigned long durationMs) {
  unsigned long startTime = millis();

  while (millis() - startTime < durationMs) {
    LoadCell.update();
  }
}