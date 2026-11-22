/*
=================================================================================
  CÓDIGO DE TESTE DE BANCADA - TRANSMISSOR LORA EBYTE E22-900T30D (ESP32)
=================================================================================
  Objetivo: Validar funcionamento, comunicação UART (Serial1), pino AUX e
            envio de pacotes de teste em novos módulos LoRa E22-900T30D.

  [!] AVISO CRÍTICO DE HARDWARE (ANTENA):
      O E22-900T30D é um módulo de alta potência (30 dBm = 1 Watt).
      NUNCA ligue ou transmita sem uma antena 915 MHz devidamente rosqueada
      no conector SMA, pois a potência refletida pode queimar o amplificador RF!

---------------------------------------------------------------------------------
  ESQUEMA DE LIGAÇÃO: LORA E22-900T30D -> ESP32 (30 PINOS) - MODO UART
---------------------------------------------------------------------------------
  LoRa E22   ->   ESP32
  VCC        ->   5V (VIN)   <-- Recomendado 5V (o T30D consome até ~650mA no pico TX;
                                 evite alimentar pelo pino 3V3 do ESP32)
  GND        ->   GND
  TXD        ->   GPIO 33 (RX1)  <-- Ligação cruzada (TXD do LoRa no RX do ESP32)
  RXD        ->   GPIO 32 (TX1)  <-- Ligação cruzada (RXD do LoRa no TX do ESP32)
  AUX        ->   GPIO 25        <-- Indica se o rádio está livre (HIGH) ou ocupado (LOW)
  M0         ->   GND (Fixo)     <-- Coloca o módulo em Modo 0 (Normal)
  M1         ->   GND (Fixo)     <-- Coloca o módulo em Modo 0 (Normal)

---------------------------------------------------------------------------------
  DETALHES DE OPERAÇÃO: PINOS M0, M1, AUX E COMUNICAÇÃO UART
---------------------------------------------------------------------------------
  1. Modos de Operação (Pinos M0 e M1):
     - M0 = GND e M1 = GND -> Modo 0 (Normal): Transmissão e recepção ativas.
     - M0 = 3V3 e M1 = GND -> Modo 1 (WOR): Wake-on-Radio (economia de energia).
     - M0 = GND e M1 = 3V3 -> Modo 2 (Configuração): Permite alterar canal/endereço.
     - M0 = 3V3 e M1 = 3V3 -> Modo 3 (Deep Sleep): Rádio desligado/em repouso.
     * Neste teste usamos M0 e M1 fixos no GND (Modo 0) para comunicação direta.

  2. Pino AUX (Diagnóstico de Hardware):
     - Nível ALTO (HIGH / 3.3V): Módulo energizado, inicializado e pronto.
     - Nível BAIXO (LOW / 0V): Módulo ocupado (transmitindo, recebendo ou
       inicializando). Se ficar travado em LOW, há falha de alimentação ou fiação.

  3. Velocidade Serial (Baud Rate):
     - UART com o LoRa E22: 9600 bps (padrão de fábrica do módulo).
     - Monitor Serial do PC : 115200 bps.

---------------------------------------------------------------------------------
  BIBLIOTECAS NECESSÁRIAS
---------------------------------------------------------------------------------
  - HardwareSerial / Serial1 (Nativa):
    Já vem integrada ao pacote do ESP32 na Arduino IDE. Não precisa instalar
    nada; ela gerencia a porta serial nos pinos 32 e 33.

  - LoRa_E22.h (Instalar):
    No Gerenciador de Bibliotecas da Arduino IDE (Ctrl + Shift + I), pesquise
    por "EByte LoRa E22 library" (de Renzo Mischianti) e instale.

---------------------------------------------------------------------------------
  COMO VALIDAR RAPIDAMENTE O TRANSMISSOR NA BANCADA
---------------------------------------------------------------------------------
  1. Rosqueie a antena no módulo LoRa antes de ligar o cabo USB no ESP32.
  2. Abra o Monitor Serial em 115200 bps.
  3. Verifique se aparece "[OK] Pino AUX em HIGH". Se falhar, confira VCC (5V)
     e GND.
  4. A cada 1 segundo o código enviará um pacote de teste. Se o retorno for
     "Status: Success", o ESP32 conseguiu entregar os dados ao rádio e o módulo
     concluiu a transmissão RF com sucesso.
=================================================================================
*/

#include <Arduino.h>
#include <LoRa_E22.h>

#ifndef UNCONFIGURED
  #define UNCONFIGURED 255
#endif

// Mapeamento de pinos no ESP32 (30 pinos)
#define ESP32_RX_PIN 33   // Conectado ao TXD do LoRa E22
#define ESP32_TX_PIN 32   // Conectado ao RXD do LoRa E22
#define PIN_E22_AUX  25   // Conectado ao AUX do LoRa E22

#define INTERVALO_ENVIO_MS 1000 // Envia 1 pacote por segundo

HardwareSerial loraSerial(1);
// M0 e M1 estão ligados fisicamente no GND, por isso passam como UNCONFIGURED
LoRa_E22 e22(&loraSerial, PIN_E22_AUX, UNCONFIGURED, UNCONFIGURED);

// Estrutura do pacote de teste (deve ser idêntica no Receptor)
struct __attribute__((packed)) PacoteTeste {
  uint32_t pacoteID;
  uint32_t tempoMs;
  float    valorSimulado;
  char     mensagem[16];
};
static_assert(sizeof(PacoteTeste) == 28, "Tamanho do PacoteTeste alterado!");

uint32_t contadorPacotes = 0;
unsigned long ultimoEnvio = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== INICIANDO TESTE DE BANCADA - TRANSMISSOR LORA E22 ===");

  pinMode(PIN_E22_AUX, INPUT_PULLDOWN);
  loraSerial.begin(9600, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);
  e22.begin();

  delay(300); // Aguarda o módulo estabilizar após energização

  // Validação rápida do pino AUX
  if (digitalRead(PIN_E22_AUX) == HIGH) {
    Serial.println("[OK] Modulo LoRa E22 detectado (Pino AUX em HIGH).");
  } else {
    Serial.println("[AVISO] Pino AUX nao respondeu HIGH!");
    Serial.println("Verifique: alimentacao 5V (VIN), GND e se M0/M1 estao no GND.");
  }

  Serial.println("Iniciando transmissao de pacotes de teste...\n");
}

void loop() {
  unsigned long agora = millis();

  if (agora - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    ultimoEnvio = agora;
    contadorPacotes++;

    PacoteTeste pkt;
    pkt.pacoteID = contadorPacotes;
    pkt.tempoMs = agora;
    pkt.valorSimulado = random(100, 999) / 10.0;
    snprintf(pkt.mensagem, sizeof(pkt.mensagem), "PING_LORA_OK");

    // Envia o pacote via LoRa
    ResponseStatus rs = e22.sendMessage(&pkt, sizeof(PacoteTeste));

    Serial.printf("[TX] Pacote #%lu | Tempo: %lu ms | Valor: %.1f | Msg: %s | Status: %s\n",
                  pkt.pacoteID,
                  pkt.tempoMs,
                  pkt.valorSimulado,
                  pkt.mensagem,
                  rs.getResponseDescription().c_str());
  }
}