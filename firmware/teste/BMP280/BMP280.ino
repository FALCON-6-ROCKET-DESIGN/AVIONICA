/*
=================================================================================
  CÓDIGO DE TESTE DE BANCADA - SENSOR BAROMÉTRICO BMP280 (ESP32)
=================================================================================
  Objetivo: Validar funcionamento, comunicação I2C e leituras de Temperatura,
            Pressão e Altitude Relativa de novos módulos BMP280.

---------------------------------------------------------------------------------
  ESQUEMA DE LIGAÇÃO: MÓDULO GY-BMP280 (6 PINOS) -> ESP32 (30 PINOS)
---------------------------------------------------------------------------------
  BMP280  ->   ESP32
  VCC     ->   3V3   (ATENÇÃO: O módulo roxo GY-BMP280 não tem regulador; use 3.3V!)
  GND     ->   GND
  SCL     ->   GPIO 22 (D22)
  SDA     ->   GPIO 21 (D21)
  CSB     ->   Solto ou 3V3 (Mantém o sensor em modo I2C)
  SDO     ->   GND, 3V3 ou Solto (Define o endereço I2C - veja tabela abaixo)

---------------------------------------------------------------------------------
  ENDEREÇOS I2C E CONFIGURAÇÃO DOS PINOS SDO E CSB
---------------------------------------------------------------------------------
  1. Pino SDO (Seleção de Endereço I2C):
     - Ligado no GND (Nível Baixo)  -> Endereço 0x76
     - Ligado no 3V3 (Nível Alto)   -> Endereço 0x77
     - Solto (Desconectado)         -> Assume 0x76 na maioria das placas GY-BMP280
                                       (pois possuem um resistor pull-down interno
                                       de 10kΩ para o GND). Porém, em alguns lotes
                                       sem esse resistor, o pino pode flutuar e
                                       falhar. Para teste rápido pode deixar solto;
                                       para projeto final, ligue no GND ou 3V3.

  2. Pino CSB (Seleção de Protocolo):
     - Solto ou ligado no 3V3       -> Ativa modo I2C (possui pull-up interno).
     - Ligado no GND                -> Ativa modo SPI (desativa o I2C).

---------------------------------------------------------------------------------
  BIBLIOTECAS NECESSÁRIAS
---------------------------------------------------------------------------------
  - Wire.h (Nativa):
    Já vem integrada ao pacote do ESP32 e à Arduino IDE. Não precisa instalar
    nada; ela gerencia a comunicação I2C nos pinos SDA (21) e SCL (22).

  - Adafruit_BMP280.h (Instalar):
    No Gerenciador de Bibliotecas da Arduino IDE (Ctrl + Shift + I), pesquise
    por "Adafruit BMP280 Library" (de Adafruit) e instale.

  - Adafruit_Sensor.h (Instalar - Dependência):
    Pesquise por "Adafruit Unified Sensor" (de Adafruit) e instale.
    Dica: Ao clicar para instalar a biblioteca do BMP280 na Arduino IDE,
    aparecerá um aviso perguntando se deseja instalar as dependências — basta
    clicar em "Install All" que ambas serão instaladas juntas.

---------------------------------------------------------------------------------
  COMO VALIDAR RAPIDAMENTE O SENSOR NOVO NA BANCADA
---------------------------------------------------------------------------------
  1. Monitor Serial:
     Ajuste a velocidade (baud rate) do Monitor Serial para 115200.

  2. Teste de Altitude:
     Após o código calibrar o ponto zero na mesa (~0.00 m), levante o sensor
     cerca de 1 metro com a mão; o valor de Altitude Relativa deve subir
     proporcionalmente (algo próximo de +0.80 m a +1.20 m).

  3. Teste de Temperatura:
     Encoste levemente o dedo sobre o pequeno encapsulamento metálico do chip;
     a temperatura deve subir imediatamente.
=================================================================================
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;
float pressaoBase;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== INICIANDO TESTE DO BMP280 ===");

  // Tenta conectar primeiro no endereço padrão (0x76). Se falhar, tenta o 0x77.
  if (bmp.begin(0x76)) {
    Serial.println("Sensor BMP280 encontrado no endereco I2C: 0x76");
  } else if (bmp.begin(0x77)) {
    Serial.println("Sensor BMP280 encontrado no endereco I2C: 0x77");
  } else {
    Serial.println("ERRO: BMP280 nao encontrado em 0x76 nem em 0x77!");
    Serial.println("Verifique: alimentacao 3V3, fios SDA/SCL e se CSB nao esta no GND.");
    while (1) delay(100);
  }

  // Configuração de amostragem para leituras estáveis e responsivas
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     // Modo de operação contínuo
                  Adafruit_BMP280::SAMPLING_X2,     // Oversampling de temperatura
                  Adafruit_BMP280::SAMPLING_X16,    // Oversampling de pressão
                  Adafruit_BMP280::FILTER_X16,      // Filtro IIR para reduzir ruído
                  Adafruit_BMP280::STANDBY_MS_63);  // Intervalo entre medições

  // Calibra a pressão inicial da bancada (define o "zero" da altitude relativa)
  float somaPressao = 0;
  for (int i = 0; i < 10; i++) {
    somaPressao += bmp.readPressure();
    delay(50);
  }
  pressaoBase = (somaPressao / 10.0) / 100.0;

  Serial.print("Pressao de referencia (Zero): ");
  Serial.print(pressaoBase, 2);
  Serial.println(" hPa\n");
}

void loop() {
  float temperatura = bmp.readTemperature();
  float pressao = bmp.readPressure() / 100.0;
  float altitudeRelativa = bmp.readAltitude(pressaoBase);

  Serial.print("Temp: ");
  Serial.print(temperatura, 2);
  Serial.print(" °C  |  Pressao: ");
  Serial.print(pressao, 2);
  Serial.print(" hPa  |  Altitude Relativa: ");
  Serial.print(altitudeRelativa, 2);
  Serial.println(" m");

  delay(500); // Atualiza a cada meio segundo para facilitar a leitura no Monitor Serial
}