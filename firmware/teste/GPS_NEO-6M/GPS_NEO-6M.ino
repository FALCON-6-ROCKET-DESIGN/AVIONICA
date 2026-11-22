/*
=================================================================================
  CÓDIGO DE TESTE DE BANCADA - MÓDULO GPS NEO-6M (ESP32)
=================================================================================
  Objetivo: Validar funcionamento, comunicação UART (Serial2), recepção de
            dados NMEA e fixação de satélites de novos módulos GPS NEO-6M.

---------------------------------------------------------------------------------
  ESQUEMA DE LIGAÇÃO: MÓDULO GPS NEO-6M -> ESP32 (30 PINOS) - MODO UART
---------------------------------------------------------------------------------
  GPS NEO-6M   ->   ESP32
  VCC          ->   3V3 ou 5V (Módulos GY-GPS6MV2 possuem regulador 3.3V interno)
  GND          ->   GND
  TX (do GPS)  ->   GPIO 16 (RX2)  <-- ATENÇÃO: Ligação cruzada (TX no RX)!
  RX (do GPS)  ->   GPIO 17 (TX2)  <-- ATENÇÃO: Ligação cruzada (RX no TX)!
  PPS          ->   Solto (Pino de pulso por segundo; não usado neste teste)

---------------------------------------------------------------------------------
  DETALHES DE COMUNICAÇÃO UART E FUNCIONAMENTO DO MÓDULO
---------------------------------------------------------------------------------
  1. Baud Rate (Velocidade Serial):
     - O padrão de fábrica do chip u-blox NEO-6M é 9600 bps (8N1).
     - O Monitor Serial do computador deve ser aberto em 115200 bps.

  2. Ligação Cruzada (TX <-> RX):
     - O pino TX (transmissão) do GPS envia dados e deve entrar no pino RX
       (recepção - GPIO 16) do ESP32. Se ligar TX com TX e RX com RX, nenhum
       dado será recebido.

  3. LED Indicador e Partida a Frio (Cold Start):
     - Módulos novos ou desligados há muito tempo precisam baixar o almanaque
       dos satélites (Cold Start), o que leva de 1 a 5 minutos em céu aberto.
     - LED apagado/fixo: Buscando satélites (sem sinal válido ainda).
     - LED piscando (1x por segundo): Sinal fixado (3D Fix obtido!).
     - Em ambientes fechados (longe de janelas), o GPS comunica normalmente com
       o ESP32, mas não conseguirá fixar satélites (Latitude/Longitude = 0.00).

---------------------------------------------------------------------------------
  BIBLIOTECAS NECESSÁRIAS
---------------------------------------------------------------------------------
  - HardwareSerial / Serial2 (Nativa):
    Já vem integrada ao pacote do ESP32 na Arduino IDE. Não precisa instalar
    nada; ela gerencia a segunda porta serial física nos pinos 16 e 17.

  - TinyGPSPlus.h (Instalar):
    No Gerenciador de Bibliotecas da Arduino IDE (Ctrl + Shift + I), pesquise
    por "TinyGPSPlus" (de Mikal Hart) e instale.

---------------------------------------------------------------------------------
  COMO VALIDAR RAPIDAMENTE O MÓDULO NOVO NA BANCADA
---------------------------------------------------------------------------------
  1. Monitor Serial:
     Ajuste a velocidade (baud rate) do Monitor Serial para 115200.

  2. Teste de Comunicação (Pode ser feito dentro de casa):
     Ao ligar, observe o campo "Bytes NMEA recebidos". Se esse número subir
     continuamente a cada segundo, a comunicação entre GPS e ESP32 está perfeita.
     Se ficar em 0, verifique a alimentação ou inverta os fios TX e RX.

  3. Teste de Satélites (Perto de janela ou área externa):
     Conecte a antena cerâmica com o lado metálico (quadrado bege/prateado)
     voltado para cima. Assim que o LED da placa começar a piscar, o status
     mudará para "VALIDO (FIX OK)" e as coordenadas aparecerão (prontas para
     copiar e colar direto no Google Maps).
=================================================================================
*/

#include <TinyGPSPlus.h>

TinyGPSPlus gps;
#define gpsSerial Serial2

// Pinos da Serial2 do ESP32
#define RXD2 16  // Conectar ao TX do GPS
#define TXD2 17  // Conectar ao RX do GPS

unsigned long ultimoPrint = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  // Inicializa a Serial2 a 9600 bps (padrão de fábrica do NEO-6M)
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("\n=== INICIANDO TESTE DE BANCADA DO GPS NEO-6M ===");
  Serial.println("Aguardando dados NMEA do modulo...\n");
}

void loop() {
  // Lê continuamente os bytes do GPS sem usar delay() para não estourar o buffer
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Atualiza o Monitor Serial a cada 1 segundo usando millis()
  if (millis() - ultimoPrint >= 1000) {
    ultimoPrint = millis();

    // Diagnóstico caso nenhum dado chegue do módulo
    if (gps.charsProcessed() < 10) {
      Serial.println("[ERRO] Nenhum dado recebido do GPS! Verifique VCC, GND e se TX/RX estao cruzados.");
      return;
    }

    exibirTelemetriaGPS();
  }
}

void exibirTelemetriaGPS() {
  Serial.println("--------------------------------------------------");
  
  // Status da comunicação e fixação
  Serial.print("Comunicacao UART : OK (Bytes NMEA recebidos: ");
  Serial.print(gps.charsProcessed());
  Serial.println(")");

  Serial.print("Status do Sinal  : ");
  if (gps.location.isValid()) {
    Serial.println("VALIDO (FIX OK!)");
  } else {
    Serial.println("BUSCANDO SATELITES (Va para area aberta/janela)");
  }

  Serial.print("Satelites Ativos : ");
  Serial.println(gps.satellites.value());

  // Coordenadas em formato decimal com sinal (pronto para colar no Google Maps)
  Serial.print("Latitude         : ");
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.println(gps.location.rawLat().negative ? " (S)" : " (N)");
  } else {
    Serial.println("Aguardando...");
  }

  Serial.print("Longitude        : ");
  if (gps.location.isValid()) {
    Serial.print(gps.location.lng(), 6);
    Serial.println(gps.location.rawLng().negative ? " (O/W)" : " (L/E)");
  } else {
    Serial.println("Aguardando...");
  }

  // Altitude e Velocidade
  Serial.print("Altitude         : ");
  if (gps.altitude.isValid()) {
    Serial.print(gps.altitude.meters(), 1);
    Serial.println(" m");
  } else {
    Serial.println("Aguardando...");
  }

  Serial.print("Velocidade       : ");
  if (gps.speed.isValid()) {
    Serial.print(gps.speed.kmph(), 1);
    Serial.println(" km/h");
  } else {
    Serial.println("Aguardando...");
  }

  // Data e Hora UTC
  Serial.print("Data (UTC)       : ");
  if (gps.date.isValid() && gps.date.year() > 2020) {
    Serial.printf("%02d/%02d/%04d\n", gps.date.day(), gps.date.month(), gps.date.year());
  } else {
    Serial.println("Aguardando...");
  }

  Serial.print("Hora (UTC)       : ");
  if (gps.time.isValid()) {
    Serial.printf("%02d:%02d:%02d\n", gps.time.hour(), gps.time.minute(), gps.time.second());
  } else {
    Serial.println("Aguardando...");
  }
}