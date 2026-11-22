/*
=================================================================================
  CÓDIGO DE TESTE DE BANCADA - MÓDULO CARTÃO MICRO SD (ESP32)
=================================================================================
  Objetivo: Validar funcionamento, comunicação SPI, reconhecimento do cartão,
            criação, escrita (append) e leitura de arquivos CSV em novos
            módulos Micro SD.

---------------------------------------------------------------------------------
  ESQUEMA DE LIGAÇÃO: MÓDULO MICRO SD -> ESP32 (30 PINOS) - MODO SPI (VSPI)
---------------------------------------------------------------------------------
  Módulo SD  ->   ESP32
  VCC        ->   5V (VIN)  <-- ATENÇÃO: Veja nota sobre alimentação abaixo!
  GND        ->   GND
  CS         ->   GPIO 5  (D5)   - Chip Select (Seleção do dispositivo)
  MOSI       ->   GPIO 23 (D23)  - Master Out Slave In (Dados ESP32 -> SD)
  SCK        ->   GPIO 18 (D18)  - Serial Clock (Relógio SPI)
  MISO       ->   GPIO 19 (D19)  - Master In Slave Out (Dados SD -> ESP32)

---------------------------------------------------------------------------------
  DETALHES DE COMUNICAÇÃO SPI, ALIMENTAÇÃO E FORMATAÇÃO DO CARTÃO
---------------------------------------------------------------------------------
  1. Protocolo SPI (Sem endereço I2C):
     - Diferente do I2C, o barramento SPI não usa endereços hexadecimais
       (como 0x76). A seleção do módulo é feita fisicamente pelo pino CS
       (GPIO 5). Se usar outros dispositivos SPI juntos, cada um deve ter seu
       próprio pino CS exclusivo.

  2. Alimentação (5V vs 3V3 - Causa nº 1 de falha!):
     - Módulos Micro SD comuns (6 pinos) possuem um regulador de tensão 3.3V
       e um conversor de nível lógico integrados. Por isso, DEVEM ser ligados
       no pino 5V (ou VIN) do ESP32. Se ligados no 3V3, o regulador derruba a
       tensão para ~2.2V e o cartão falha ao inicializar.
     - Exceção: Apenas módulos específicos "3.3V Only" (sem regulador na placa)
       devem ser ligados no pino 3V3.

  3. Formatação e Capacidade do Cartão:
     - O cartão DEVE estar formatado em FAT32 (cartões em exFAT ou NTFS não
       são reconhecidos pela biblioteca padrão).
     - Cartões até 32 GB (SDHC) já vêm em FAT32 de fábrica. Cartões de 64 GB ou
       mais (SDXC) vêm em exFAT e precisam ser convertidos para FAT32 via PC.
     - No ESP32, todo caminho de arquivo deve começar com barra: "/arquivo.csv".

---------------------------------------------------------------------------------
  BIBLIOTECAS NECESSÁRIAS
---------------------------------------------------------------------------------
  - SPI.h (Nativa):
    Já vem integrada ao pacote do ESP32 e à Arduino IDE. Não precisa instalar
    nada; ela gerencia o barramento SPI nos pinos 18, 19 e 23.

  - SD.h e FS.h (Nativas):
    Já vêm integradas ao pacote do ESP32 na Arduino IDE. Não precisa instalar
    nada; elas gerenciam o sistema de arquivos FAT32, leitura e gravação.

---------------------------------------------------------------------------------
  COMO VALIDAR RAPIDAMENTE O MÓDULO NOVO NA BANCADA
---------------------------------------------------------------------------------
  1. Monitor Serial:
     Insira um cartão Micro SD formatado em FAT32, ligue o ESP32 e abra o
     Monitor Serial na velocidade (baud rate) de 115200.

  2. Diagnóstico Inicial:
     O código exibirá o tipo do cartão (SDSC ou SDHC), a capacidade total em MB
     e fará um teste automático de criação e leitura de arquivo.

  3. Confirmação de Gravação Contínua:
     A cada 2 segundos uma nova linha será gravada no arquivo "/teste_sd.csv"
     e o tamanho total do arquivo (em bytes) será exibido. Se o número de bytes
     estiver subindo continuamente, a escrita física está funcionando.
     (Opcional: retire o cartão e abra o arquivo no Excel/Bloco de Notas do PC).
=================================================================================
*/

#include <FS.h>
#include <SD.h>
#include <SPI.h>

// Definição dos pinos do barramento SPI (Padrão VSPI do ESP32)
#define SD_CS_PIN   5
#define SPI_SCK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

const char* NOME_ARQUIVO = "/teste_sd.csv";
unsigned long contadorLeituras = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== INICIANDO TESTE DE BANCADA DO MODULO MICRO SD ===");

  // Inicializa o barramento SPI nos pinos definidos e tenta montar o cartão
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[ERRO] Falha ao inicializar o cartao SD!");
    Serial.println("Verifique:");
    Serial.println("  1. Se o cartao esta bem encaixado e formatado em FAT32;");
    Serial.println("  2. Se o VCC do modulo esta ligado no 5V (VIN);");
    Serial.println("  3. Se os fios CS(5), SCK(18), MISO(19) e MOSI(23) estao corretos.");
    while (1) delay(100);
  }

  // Exibe informações de diagnóstico do cartão reconhecido
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("[ERRO] Nenhum cartao Micro SD inserido no slot!");
    while (1) delay(100);
  }

  Serial.print("Tipo de Cartao : ");
  if (cardType == CARD_MMC)       Serial.println("MMC");
  else if (cardType == CARD_SDSC) Serial.println("SDSC (Ate 2GB)");
  else if (cardType == CARD_SDHC) Serial.println("SDHC (4GB a 32GB)");
  else                            Serial.println("Desconhecido");

  uint64_t cardSizeMB = SD.cardSize() / (1024 * 1024);
  Serial.printf("Capacidade     : %llu MB\n", cardSizeMB);

  // Verifica se o arquivo já existe; se não existir, cria com cabeçalho CSV
  if (!SD.exists(NOME_ARQUIVO)) {
    Serial.println("Criando arquivo '/teste_sd.csv' com cabecalho...");
    File dataFile = SD.open(NOME_ARQUIVO, FILE_WRITE);
    if (dataFile) {
      dataFile.println("ID_Leitura,Tempo_ms,Valor_Teste_1,Valor_Teste_2,Status");
      dataFile.close();
      Serial.println("Cabecalho gravado com sucesso!");
    } else {
      Serial.println("[ERRO] Falha ao criar o arquivo no cartao!");
      while (1) delay(100);
    }
  } else {
    Serial.println("Arquivo '/teste_sd.csv' ja existe. Novos dados serao anexados.");
  }

  // Teste rápido de leitura para validar se o pino MISO está funcionando
  File readTest = SD.open(NOME_ARQUIVO, FILE_READ);
  if (readTest) {
    Serial.print("Teste de Leitura (MISO): OK! Tamanho atual do arquivo: ");
    Serial.print(readTest.size());
    Serial.println(" bytes\n");
    readTest.close();
  } else {
    Serial.println("[ERRO] Falha no teste de leitura (verifique o pino MISO - GPIO 19).\n");
  }
}

void loop() {
  contadorLeituras++;
  unsigned long tempoAtual = millis();

  // Gera valores simulados para testar a formatação das colunas no CSV
  float valorTeste1 = random(200, 350) / 10.0;
  int valorTeste2 = random(40, 90);
  String statusTeste = "OK";

  // Abre o arquivo em modo FILE_APPEND para adicionar no final sem apagar o histórico
  File dataFile = SD.open(NOME_ARQUIVO, FILE_APPEND);

  if (dataFile) {
    dataFile.print(contadorLeituras);
    dataFile.print(",");
    dataFile.print(tempoAtual);
    dataFile.print(",");
    dataFile.print(valorTeste1, 1);
    dataFile.print(",");
    dataFile.print(valorTeste2);
    dataFile.print(",");
    dataFile.println(statusTeste);

    size_t tamanhoAtual = dataFile.size(); // Verifica o novo tamanho em bytes
    dataFile.close();                      // Fecha para consolidar a gravação física

    Serial.printf("[GRAVADO] Linha #%lu | Tempo: %lu ms | Dados: %.1f, %d, %s | Arquivo: %u bytes\n",
                  contadorLeituras, tempoAtual, valorTeste1, valorTeste2, statusTeste.c_str(), tamanhoAtual);
  } else {
    Serial.println("[ERRO] Nao foi possivel abrir o arquivo para gravacao!");
  }

  delay(2000); // Grava uma nova linha a cada 2 segundos
}