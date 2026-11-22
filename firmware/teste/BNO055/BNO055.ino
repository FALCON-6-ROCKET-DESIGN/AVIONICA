/*
=================================================================================
  CÓDIGO DE TESTE DE BANCADA - SENSOR INERCIAL 9-DOF BNO055 (ESP32)
=================================================================================
  Objetivo: Validar funcionamento, comunicação I2C, calibração interna e leituras
            de Orientação (Euler/Quatérnions), Acelerômetro, Giroscópio,
            Magnetômetro e Temperatura de novos módulos BNO055.

---------------------------------------------------------------------------------
  ESQUEMA DE LIGAÇÃO: MÓDULO BNO055 -> ESP32 (30 PINOS) - MODO I2C
---------------------------------------------------------------------------------
  BNO055   ->   ESP32
  VCC/VIN  ->   3V3 (ou 5V se o módulo possuir regulador onboard)
  GND      ->   GND
  SCL      ->   GPIO 22 (D22)
  SDA      ->   GPIO 21 (D21)
  ADR/COM3 ->   GND, 3V3 ou Solto (Define o endereço I2C - veja detalhes abaixo)
  PS0/PS1  ->   Soltos ou GND (Mantém o sensor em modo I2C padrão)
  RST/INT  ->   Soltos (Não utilizados neste teste de bancada)

---------------------------------------------------------------------------------
  ENDEREÇOS I2C E CONFIGURAÇÃO DOS PINOS ADR (COM3), PS0 E PS1
---------------------------------------------------------------------------------
  1. Pino ADR (identificado como COM3 em algumas placas, como a GY-BNO055):
     - Ligado no GND (Nível Baixo)  -> Endereço 0x28
     - Ligado no 3V3 (Nível Alto)   -> Endereço 0x29
     - Solto (Desconectado)         -> Assume o resistor padrão da placa:
                                       * Placa azul Adafruit: pull-down -> 0x28
                                       * Placa roxa GY-BNO055: pull-up -> 0x29
                                       (Este código detecta 0x28 ou 0x29
                                       automaticamente na inicialização).

  2. Pinos PS0 e PS1 (Seleção de Protocolo - presentes na placa GY-BNO055):
     - Ambos soltos ou no GND       -> Ativa o modo I2C padrão.
     - Ligados em 3V3               -> Alteram para modo HID-I2C ou UART
                                       (desativando a comunicação I2C comum).

---------------------------------------------------------------------------------
  BIBLIOTECAS NECESSÁRIAS
---------------------------------------------------------------------------------
  - Wire.h (Nativa):
    Já vem integrada ao pacote do ESP32 e à Arduino IDE. Não precisa instalar
    nada; ela gerencia a comunicação I2C nos pinos SDA (21) e SCL (22).

  - Adafruit_BNO055.h (Instalar):
    No Gerenciador de Bibliotecas da Arduino IDE (Ctrl + Shift + I), pesquise
    por "Adafruit BNO055" (de Adafruit) e instale.
    (O arquivo utility/imumaths.h já vem incluso dentro desta biblioteca).

  - Adafruit_Sensor.h (Instalar - Dependência):
    Pesquise por "Adafruit Unified Sensor" (de Adafruit) e instale.
    Dica: Ao instalar a "Adafruit BNO055", a IDE perguntará se deseja instalar
    as dependências — basta clicar em "Install All".

---------------------------------------------------------------------------------
  COMO VALIDAR RAPIDAMENTE O SENSOR NOVO NA BANCADA
---------------------------------------------------------------------------------
  1. Monitor Serial:
     Ajuste a velocidade (baud rate) do Monitor Serial para 115200 e confirme
     se o endereço (0x28 ou 0x29) foi detectado com sucesso.

  2. Teste em Repouso (Gravidade e Giroscópio):
     Com a placa parada e plana sobre a mesa, o "Vetor de Gravidade" deve marcar
     cerca de 9.81 m/s² em um dos eixos (geralmente Z) e ~0.00 nos outros dois.
     O "Giroscópio" deve ficar próximo de 0.00 rad/s nos 3 eixos.

  3. Teste de Movimento (Euler e Quatérnions):
     Incline e gire o sensor com a mão; os ângulos de Heading, Roll e Pitch
     devem variar suavemente em tempo real sem travar.

  4. Teste de Calibração (Valores de 0 = não calibrado até 3 = calibrado):
     - GYRO : Deixe o sensor parado sobre a mesa por ~2 segundos (vai para 3).
     - ACCEL: Incline o sensor lentamente em 6 posições distintas (vai para 3).
     - MAG  : Faça movimentos suaves em formato de "8" no ar (vai para 3).
=================================================================================
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

#define SDA_PIN        21
#define SCL_PIN        22
#define SAMPLE_RATE_MS 1000  // Intervalo de atualização no Monitor Serial (ms)

// Instancia os dois possíveis endereços I2C do BNO055 para detecção automática
Adafruit_BNO055 bno28 = Adafruit_BNO055(55, 0x28, &Wire);
Adafruit_BNO055 bno29 = Adafruit_BNO055(55, 0x29, &Wire);
Adafruit_BNO055* bno = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== INICIANDO TESTE DE BANCADA DO BNO055 ===");

  Wire.begin(SDA_PIN, SCL_PIN);

  // Tenta conectar primeiro no endereço 0x28; se falhar, tenta o 0x29
  if (bno28.begin()) {
    bno = &bno28;
    Serial.println("Sensor BNO055 encontrado no endereco I2C: 0x28");
  } else if (bno29.begin()) {
    bno = &bno29;
    Serial.println("Sensor BNO055 encontrado no endereco I2C: 0x29");
  } else {
    Serial.println("ERRO: BNO055 nao encontrado em 0x28 nem em 0x29!");
    Serial.println("Verifique: alimentacao (3V3/5V), fios SDA/SCL e pinos PS0/PS1.");
    while (1) delay(100);
  }

  delay(500);
  bno->setExtCrystalUse(true); // Utiliza o cristal externo da placa para maior precisão

  Serial.println("Sensor inicializado! Mova o sensor para testar e calibrar.\n");
}

void loop() {
  sensors_event_t orientData, accelData, gravData, gyroData, magData;

  // Coleta todos os vetores do sensor
  bno->getEvent(&orientData, Adafruit_BNO055::VECTOR_EULER);
  bno->getEvent(&accelData,  Adafruit_BNO055::VECTOR_LINEARACCEL);
  bno->getEvent(&gravData,   Adafruit_BNO055::VECTOR_GRAVITY);
  bno->getEvent(&gyroData,   Adafruit_BNO055::VECTOR_GYROSCOPE);
  bno->getEvent(&magData,    Adafruit_BNO055::VECTOR_MAGNETOMETER);

  imu::Quaternion quat = bno->getQuat();
  int8_t temperatura = bno->getTemp();

  uint8_t sys = 0, gyro = 0, accel = 0, mag = 0;
  bno->getCalibration(&sys, &gyro, &accel, &mag);

  Serial.println("--------------------------------------------------");
  Serial.printf("Calibracao (0-3) -> SYS: %d | GYRO: %d | ACCEL: %d | MAG: %d\n",
                sys, gyro, accel, mag);

  Serial.printf("Orientacao Euler -> Heading: %7.2f | Roll: %6.2f | Pitch: %6.2f deg\n",
                orientData.orientation.x, orientData.orientation.y, orientData.orientation.z);

  Serial.printf("Quaternion       -> W: %6.3f | X: %6.3f | Y: %6.3f | Z: %6.3f\n",
                quat.w(), quat.x(), quat.y(), quat.z());

  Serial.printf("Acel. Linear     -> X: %6.2f | Y: %6.2f | Z: %6.2f m/s2\n",
                accelData.acceleration.x, accelData.acceleration.y, accelData.acceleration.z);

  Serial.printf("Vetor Gravidade  -> X: %6.2f | Y: %6.2f | Z: %6.2f m/s2\n",
                gravData.acceleration.x, gravData.acceleration.y, gravData.acceleration.z);

  Serial.printf("Giroscopio       -> X: %6.2f | Y: %6.2f | Z: %6.2f rad/s\n",
                gyroData.gyro.x, gyroData.gyro.y, gyroData.gyro.z);

  Serial.printf("Magnetometro     -> X: %6.2f | Y: %6.2f | Z: %6.2f uT\n",
                magData.magnetic.x, magData.magnetic.y, magData.magnetic.z);

  Serial.printf("Temperatura Chip -> %d °C\n", temperatura);

  delay(SAMPLE_RATE_MS);
}