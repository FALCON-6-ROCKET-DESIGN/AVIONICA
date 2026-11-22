/*
=================================================================================
  CÓDIGO DE TESTE DE BANCADA - RECEPTOR LORA EBYTE E22-900T30D (ESP32)
=================================================================================
  Objetivo: Validar funcionamento, comunicação UART (Serial1), recepção RF,
            integridade dos bytes e contagem de perda de pacotes de novos
            módulos LoRa E22-900T30D diretamente no Monitor Serial.

  [!] AVISO CRÍTICO DE HARDWARE (ANTENA):
      Mantenha sempre uma antena 915 MHz rosqueada no conector SMA do módulo
      antes de energizar o ESP32.

---------------------------------------------------------------------------------
  ESQUEMA DE LIGAÇÃO: LORA E22-900T30D -> ESP32 (30 PINOS) - MODO UART
---------------------------------------------------------------------------------
  LoRa E22   ->   ESP32
  VCC        ->   5V (VIN)   <-- Recomendado 5V para estabilidade do módulo
  GND        ->   GND
  TXD        ->   GPIO 33 (RX1)  <-- Ligação cruzada (TXD do LoRa no RX do ESP32)
  RXD        ->   GPIO 32 (TX1)  <-- Ligação cruzada (RXD do LoRa no TX do ESP32)
  AUX        ->   GPIO 25        <-- Indica chegada de dados (vai para LOW ao receber)
  M0         ->   GND (Fixo)     <-- Coloca o módulo em Modo 0 (Normal)
  M1         ->   GND (Fixo)     <-- Coloca o módulo em Modo 0 (Normal)

---------------------------------------------------------------------------------
  DETALHES DE OPERAÇÃO E COMPATIBILIDADE ENTRE MÓDULOS NOVOS
---------------------------------------------------------------------------------
  1. Pareamento de Fábrica:
     - Módulos E22-900T30D novos vêm configurados de fábrica no mesmo canal,
       mesmo endereço (0x0000) e mesma taxa aérea (2.4 kbps). Portanto, dois
       módulos novos se comunicam imediatamente sem precisar configurar nada.
     - Se um módulo novo não receber nada de um módulo antigo, pode ser que o
       antigo tenha tido o canal/frequência alterado anteriormente.

  2. Diagnóstico de Fiação Cruzada (TXD / RXD):
     - O pino TXD do LoRa envia os dados recebidos pelo ar para o GPIO 33 (RX)
       do ESP32. Se TXD e RXD estiverem invertidos, o pino AUX piscará quando o
       transmissor enviar, mas nada aparecerá no Monitor Serial.

  3. Velocidade Serial (Baud Rate):
     - UART com o LoRa E22: 9600 bps (padrão de fábrica).
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
  COMO VALIDAR RAPIDAMENTE O RECEPTOR NA BANCADA
---------------------------------------------------------------------------------
  1. Rosqueie a antena no módulo receptor e ligue o ESP32 no computador.
  2. Abra o Monitor Serial em 115200 bps e confirme "[OK] Pino AUX em HIGH".
  3. Ligue o ESP32 Transmissor (com o código de teste TX).
  4. A cada 1 segundo, os pacotes devem aparecer no Monitor Serial com
     "Perdidos: 0". Se nada chegar em 10 segundos, inverta os fios dos pinos
     32 e 33 para testar se TXD/RXD não ficaram trocados.
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

HardwareSerial loraSerial(1);
// M0 e M1 estão ligados fisicamente no GND, por isso passam como UNCONFIGURED
LoRa_E22 e22(&loraSerial, PIN_E22_AUX, UNCONFIGURED, UNCONFIGURED);

// Estrutura do pacote de teste (deve ser idêntica no Transmissor)
struct __attribute__((packed)) PacoteTeste {
  uint32_t pacoteID;
  uint32_t tempoMs;
  float    valorSimulado;
  char     mensagem[16];
};
static_assert(sizeof(PacoteTeste) == 28, "Tamanho do PacoteTeste alterado!");

uint32_t ultimoPacoteID = 0;
uint32_t pacotesRecebidos = 0;
uint32_t pacotesPerdidos = 0;
unsigned long ultimoRecebimentoMs = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== INICIANDO TESTE DE BANCADA - RECEPTOR LORA E22 ===");

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

  Serial.println("Aguardando pacotes do transmissor...\n");
}

void loop() {
  // Verifica se chegaram dados na serial do LoRa
  if (e22.available() > 0) {
    ResponseStructContainer rsc = e22.receiveMessage(sizeof(PacoteTeste));

    if (rsc.status.code == 1) { // 1 = Sucesso na recepção
      PacoteTeste dados = *(PacoteTeste*) rsc.data;
      dados.mensagem[sizeof(dados.mensagem) - 1] = '\0'; // Garante fim de string seguro

      // Calcula se algum pacote foi perdido no caminho
      if (ultimoPacoteID > 0 && dados.pacoteID > (ultimoPacoteID + 1)) {
        pacotesPerdidos += (dados.pacoteID - ultimoPacoteID - 1);
      }
      ultimoPacoteID = dados.pacoteID;
      pacotesRecebidos++;
      ultimoRecebimentoMs = millis();

      Serial.printf("[RX OK] Pacote #%lu | Tempo TX: %lu ms | Valor: %.1f | Msg: %s | Recebidos: %lu | Perdidos: %lu\n",
                    dados.pacoteID,
                    dados.tempoMs,
                    dados.valorSimulado,
                    dados.mensagem,
                    pacotesRecebidos,
                    pacotesPerdidos);
    } else {
      Serial.printf("[RX ERRO] Falha ao ler pacote: %s\n", rsc.status.getResponseDescription().c_str());
    }

    rsc.close(); // Libera a memória alocada pela biblioteca
  }

  // Aviso amigável caso fique mais de 5 segundos sem receber nada
  if (millis() - ultimoRecebimentoMs > 5000) {
    ultimoRecebimentoMs = millis();
    Serial.println("[AGUARDANDO] Nenhum pacote recebido nos ultimos 5s (verifique se o TX esta ligado e TXD/RXD cruzados)...");
  }

  delay(5);
}