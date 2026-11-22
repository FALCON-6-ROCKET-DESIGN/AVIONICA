/*
=================================================================================
  RECEPTOR LORA E22 — ESTAÇÃO DE SOLO (USB BINÁRIO PARA DASHBOARD PYTHON)
=================================================================================
  Objetivo: Receber via LoRa E22-900T30D os pacotes resumidos de telemetria
            enviados pelo foguete/aviônica (55 bytes) e repassá-los via porta
            USB Serial em formato binário estruturado para o Dashboard Python.

  [!] AVISO CRÍTICO DE HARDWARE (ANTENA):
      O E22-900T30D é um módulo de alta potência (30 dBm = 1W). Mantenha sempre
      uma antena 915 MHz rosqueada no conector SMA antes de energizar o ESP32.

---------------------------------------------------------------------------------
  ESQUEMA DE LIGAÇÃO: LORA E22-900T30D -> ESP32 (30 PINOS) - MODO UART1
---------------------------------------------------------------------------------
  LoRa E22   ->   ESP32 (30 Pinos)
  VCC        ->   5V (VIN)       <-- Recomendado 5V para estabilidade do módulo
  GND        ->   GND
  TXD        ->   GPIO 33 (RX1)  <-- Ligação cruzada (TXD do LoRa no RX do ESP32)
  RXD        ->   GPIO 32 (TX1)  <-- Ligação cruzada (RXD do LoRa no TX do ESP32)
  AUX        ->   GPIO 25        <-- Indica atividade/disponibilidade do rádio
  M0         ->   GND (Fixo)     <-- Modo 0 (Operação Normal TX/RX)
  M1         ->   GND (Fixo)     <-- Modo 0 (Operação Normal TX/RX)

---------------------------------------------------------------------------------
  PROTOCOLO DE COMUNICAÇÃO (LORA -> ESP32 -> DASHBOARD PYTHON)
---------------------------------------------------------------------------------
  1. Sincronismo da Struct (55 Bytes):
     - A struct "TelemetriaResumida" deve ser IDÊNTICA (mesma ordem e mesmos
       tipos de variáveis) no Transmissor, no Receptor e no Dashboard Python.
     - A diretiva "static_assert" impede a compilação caso o tamanho mude.

  2. Enquadramento Binário no USB (58 Bytes por frame):
     - Para evitar que o Python perca o alinhamento caso algum byte se perca
       na USB, o ESP32 envia cada pacote encapsulado no seguinte formato:
       [ 0xAA ] [ 0x55 ] [ 55 bytes de Payload ] [ 1 byte Checksum XOR ]

  3. Velocidades Seriais (Baud Rate):
     - UART1 (ESP32 <-> LoRa E22) : 9600 bps (padrão de fábrica do E22).
     - USB   (ESP32 <-> PC/Python): 115200 bps.

---------------------------------------------------------------------------------
  BIBLIOTECAS NECESSÁRIAS
---------------------------------------------------------------------------------
  - Arduino.h e HardwareSerial (Nativas):
    Já vêm integradas ao pacote do ESP32 na Arduino IDE. Não precisa instalar
    nada; gerenciam a porta serial física UART1 nos pinos 32 e 33.

  - LoRa_E22.h (Instalar):
    No Gerenciador de Bibliotecas da Arduino IDE (Ctrl + Shift + I), pesquise
    por "EByte LoRa E22 library" (de Renzo Mischianti) e instale.

---------------------------------------------------------------------------------
  COMO VALIDAR E OPERAR A ESTAÇÃO DE SOLO
---------------------------------------------------------------------------------
  1. Teste Rápido no Monitor Serial:
     Como este código envia dados BINÁRIOS brutos (Serial.write), ao abrir o
     Monitor Serial da Arduino IDE em 115200 bps com o transmissor ligado, você
     verá caracteres estranhos/símbolos aparecendo periodicamente — isso confirma
     que os frames binários estão chegando corretamente.

  2. Uso com o Dashboard Python (IMPORTANTE):
     - FECHE o Monitor Serial da Arduino IDE antes de iniciar o Dashboard Python!
       O Windows/Linux permite apenas um programa conectado à porta COM por vez.
     - Mantenha M0 e M1 sempre aterrados no GND durante a operação.
=================================================================================
*/

#include <Arduino.h>
#include <LoRa_E22.h>

#ifndef UNCONFIGURED
  #define UNCONFIGURED 255
#endif

#define ESP32_RX_PIN 33
#define ESP32_TX_PIN 32
#define PIN_E22_AUX  25

HardwareSerial loraSerial(1);
LoRa_E22 e22(&loraSerial, PIN_E22_AUX, UNCONFIGURED, UNCONFIGURED);

// --- PACOTE RESUMIDO (tem que bater com o transmissor) ---
struct __attribute__((packed)) TelemetriaResumida {
  uint32_t pacoteID;
  uint32_t tempoMs;
  float    latitude;
  float    longitude;
  float    altBMP;
  float    altMax;
  float    velocidadeGPS_mps;
  uint8_t  satelites;
  uint8_t  statusSquib;
  uint8_t  validadeLocalizacao;
  float    roll, pitch, yaw;        // graus (Euler) - orientação do BNO055
  float    accL_x, accL_y, accL_z;  // aceleração linear (sem gravidade)
};
static_assert(sizeof(TelemetriaResumida) == 55,
              "Tamanho do TelemetriaResumida mudou! Atualize transmissor.ino e o dashboard Python.");

const uint8_t SYNC_BYTE_1 = 0xAA;
const uint8_t SYNC_BYTE_2 = 0x55;

uint8_t calcularChecksum(const uint8_t* data, size_t len) {
  uint8_t chk = 0;
  for (size_t i = 0; i < len; i++) chk ^= data[i];
  return chk;
}

void setup() {
  // Comunicação de alta velocidade com o Python/Streamlit via USB
  Serial.begin(115200);
  delay(1000);

  // Inicializa comunicação UART1 com o rádio LoRa E22 a 9600 baud
  loraSerial.begin(9600, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);
  e22.begin();
}

void loop() {
  if (e22.available() > 0) {
    ResponseStructContainer rsc = e22.receiveMessage(sizeof(TelemetriaResumida));

    // status.code == 1 significa SUCESSO (recebeu exatamente os bytes esperados)
    if (rsc.status.code == 1) {
      TelemetriaResumida dados = *(TelemetriaResumida*) rsc.data;

      uint8_t frame[2 + sizeof(TelemetriaResumida) + 1];
      frame[0] = SYNC_BYTE_1;
      frame[1] = SYNC_BYTE_2;
      memcpy(&frame[2], &dados, sizeof(TelemetriaResumida));
      frame[2 + sizeof(TelemetriaResumida)] = calcularChecksum((uint8_t*)&dados, sizeof(TelemetriaResumida));

      Serial.write(frame, sizeof(frame));
    }

    rsc.close(); // Libera a memória RAM
  }
  delay(5);
}