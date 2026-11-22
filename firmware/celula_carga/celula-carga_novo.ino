
//Conexão da Célula de Carga ao Módulo HX711
//Fio Vermelho: Conecte no pino E+ (Excitação Positiva / Alimentação do Sensor)
//Fio Preto: Conecte no pino E- (Excitação Negativa / GND do Sensor)
//Fio Verde: Conecte no pino A+ (Sinal Positivo / Leitura de Dados)
//Fio Branco: Conecte no pino A- (Sinal Negativo / Leitura de Dados)

// =========================================================================================
// PINAGEM DO HX711 COM O ESP32 (30 PINOS)
// =========================================================================================
// Conecte o pino VCC do HX711 -> pino 3V3 do ESP32 (seguro para o nível lógico de 3.3V)
// Conecte o pino GND do HX711 -> pino GND do ESP32


#include <HX711_ADC.h> // Inclui a biblioteca do HX711 otimizada para células de carga
#include <EEPROM.h>    // Inclui a biblioteca para salvar dados na memória permanente do Arduino

// =========================================================================================
// CONFIGURAÇÕES DE PINOS E VARIÁVEIS
// =========================================================================================
// Pinos do HX711 adaptados para o ESP32
const int HX711_dout = 4; // GPIO 4 (Pino D4 no ESP32)
const int HX711_sck = 5;  // GPIO 5 (Pino D5 no ESP32)

// Cria o objeto da célula de carga passando os pinos configurados
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0; // Endereço da memória EEPROM onde o fator será salvo
float calibrationValue = 1.0;      // Variável que armazenará o fator de calibração
unsigned long tempoAnterior = 0;   // Controla o intervalo de tempo para exibição contínua
bool leituraContinuaAtiva = false; // Status do modo de leitura contínua (ligado/desligado)

void setup() {
  Serial.begin(57600); // Inicializa a comunicação Serial. Configure seu monitor para 57600 Baud!
  delay(10);
  
  Serial.println();
  Serial.println("=== SISTEMA DE BALANÇA (CÉLULA 100KG) ===");
  Serial.println("Iniciando o sistema...");

  LoadCell.begin(); // Inicializa o hardware do HX711
  
  // Define o tempo que a balança leva para estabilizar e zerar ao ligar (2000 milissegundos = 2 segundos)
  unsigned long tempoEstabilizacao = 2000;
  boolean realizarTaraInicial = true; 
  
  // Inicializa a célula. Ela vai ler o peso atual da plataforma e definir como ZERO (tara inicial)
  LoadCell.start(tempoEstabilizacao, realizarTaraInicial);
  
  // Verifica se houve falha de comunicação física com o módulo HX711
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("ERRO CRÍTICO: Módulo HX711 não detectado.");
    Serial.println("Por favor, verifique se os pinos DT(4) e SCK(5) estão bem conectados.");
    while (1); // Trava o código aqui para proteção
  } else {
    // Tenta ler o fator de calibração salvo anteriormente na memória EEPROM
    EEPROM.get(calVal_eepromAdress, calibrationValue);
    
    // Se a memória estiver vazia ou com valores inválidos, aplica um valor padrão temporário
    if (isnan(calibrationValue) || calibrationValue == 0.0 || calibrationValue == 1.0) {
      calibrationValue = 420.0; // Valor padrão inicial. Será substituído quando você calibrar.
      Serial.println("Nenhuma calibração anterior encontrada. Usando valor padrão provisório.");
    } else {
      Serial.print("Calibração carregada com sucesso da EEPROM: ");
      Serial.println(calibrationValue);
    }
    
    // Aplica o fator de calibração na biblioteca
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Inicialização do hardware concluída.");
  }

  // Aguarda até que a célula faça a primeira leitura válida
  while (!LoadCell.update()) {
    delay(1);
  }
  
  // Exibe o menu de comandos no Monitor Serial
  Serial.println();
  Serial.println("=== SISTEMA PRONTO E ZERADO ===");
  Serial.println("Digite os comandos abaixo no Monitor Serial:");
  Serial.println(" 't' -> Executa a Tara (Zera a balança)");
  Serial.println(" 'c' -> Inicia o Modo de Calibração");
  Serial.println(" 's' -> Salva a calibração atual na EEPROM");
  Serial.println(" 'p' -> Mostra apenas o peso atual uma vez");
  Serial.println(" 'd' -> Ativa/Desativa a leitura contínua de peso");
  Serial.println(" 'i' -> Exibe informações técnicas do sistema");
  Serial.println("==============================================");
}

void loop() {
  static boolean novosDadosDisponiveis = false;
  const int intervaloLeituraContínua = 500; // Tempo em milissegundos (0.5s) entre cada atualização de peso na tela

  // O método update() deve ser chamado constantemente. Ele retorna 'true' se houver nova leitura pronta
  if (LoadCell.update()) {
    novosDadosDisponiveis = true;
  }

  // Verifica se o usuário enviou algum comando de texto pelo Monitor Serial
  if (Serial.available() > 0) {
    char caractereRecebido = Serial.read(); // Lê o caractere enviado
    
    switch(caractereRecebido) {
      case 't': // Comando para Tarar (Zerar)
        Serial.println("Executando tara... Remova os pesos da balança.");
        LoadCell.tareNoDelay(); // Zera a balança em segundo plano (sem travar o código)
        break;
        
      case 'c': // Comando para Calibrar
        modoCalibracao();
        break;
        
      case 's': // Comando para Gravar na EEPROM
        EEPROM.put(calVal_eepromAdress, calibrationValue);
        Serial.print("Sucesso! Valor de calibração (");
        Serial.print(calibrationValue);
        Serial.println(") gravado na memória permanente.");
        break;
        
      case 'p': // Comando para Leitura Única
        Serial.print("Peso medido atual: ");
        Serial.print(LoadCell.getData(), 2); // Exibe o peso com 2 casas decimais (kg)
        Serial.println(" kg");
        break;
        
      case 'd': // Comando para alternar Modo Contínuo
        leituraContinuaAtiva = !leituraContinuaAtiva;
        Serial.println(leituraContinuaAtiva ? "Modo leitura contínua: ATIVADO" : "Modo leitura contínua: DESATIVADO");
        break;
        
      case 'i': // Comando de Informações do Sistema
        exibirInfoSistema();
        break;
        
      case 'r': // Comando oculto para reiniciar o Arduino por software
        Serial.println("Reiniciando o sistema...");
        setup();
        break;
    }
  }

  // Se o modo contínuo estiver ativo e houver uma nova leitura do sensor disponível
  if (leituraContinuaAtiva && novosDadosDisponiveis) {
    // Controla o tempo para não inundar o Monitor Serial com dados rápidos demais
    if (millis() > tempoAnterior + intervaloLeituraContínua) {
      float pesoAtual = LoadCell.getData(); // Captura o peso processado
      
      Serial.print("Peso: ");
      Serial.print(pesoAtual, 2); // Exibe com 2 casas decimais (Ex: 10.55 kg)
      Serial.println(" kg");
      
      novosDadosDisponiveis = false; // Reseta a flag de novos dados
      tempoAnterior = millis();      // Atualiza o marcador de tempo
    }
  }

  // Monitora se o processo de tara em segundo plano foi finalizado
  if (LoadCell.getTareStatus() == true) {
    Serial.println(">> Balança Zerada com sucesso (Tara Concluída)! <<");
  }
}

// =========================================================================================
// FUNÇÃO DO MODO DE CALIBRAÇÃO ASSISTIDA
// =========================================================================================
void modoCalibracao() {
  Serial.println();
  Serial.println("=== MODO DE CALIBRAÇÃO INICIADO ===");
  Serial.println("1. Remova QUALQUEER peso ou objeto de cima da balança.");
  Serial.println("2. Envie a letra 't' aqui no monitor para zerar o peso inicial...");
  
  boolean passoConcluido = false;
  unsigned long timeout = millis();
  
  // Loop com tempo limite de 30 segundos para realizar a tara de calibração
  while (!passoConcluido && (millis() - timeout < 30000)) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char caractere = Serial.read();
      if (caractere == 't') {
        LoadCell.tareNoDelay();
        Serial.println("Processando tara de calibração...");
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Peso base zerado com sucesso!");
      passoConcluido = true;
    }
    delay(10);
  }

  if (!passoConcluido) {
    Serial.println("Tempo esgotado! Modo calibração cancelado.");
    return;
  }

  // Próximo passo da calibração
  Serial.println();
  Serial.println("3. Coloque um PESO CONHECIDO sobre a balança.");
  Serial.println("   * Dica para célula de 100kg: Use um peso entre 10kg e 50kg para melhor precisão.");
  Serial.println("4. Digite o valor exato deste peso em kg no monitor e envie (Ex: 20.0 ou 15.50):");

  float massaConhecida = 0;
  passoConcluido = false;
  timeout = millis();
  
  // Limpa quaisquer caracteres perdidos no buffer da serial antes de ler o valor
  while(Serial.available()) Serial.read();

  // Loop com tempo limite de 60 segundos para receber o valor do peso digitado pelo usuário
  while (!passoConcluido && (millis() - timeout < 60000)) {
    LoadCell.update();
    
    if (Serial.available() > 0) {
      String textoDigitado = Serial.readString();
      textoDigitado.trim(); // Remove espaços em branco ou quebras de linha nas pontas
      
      massaConhecida = textoDigitado.toFloat(); // Converte o texto digitado para número decimal
      if (massaConhecida > 0) {
        Serial.print("Peso conhecido registrado: ");
        Serial.print(massaConhecida, 2);
        Serial.println(" kg");
        passoConcluido = true;
      }
    }
    delay(10);
  }

  if (!passoConcluido) {
    Serial.println("Tempo esgotado! Nenhum peso válido foi digitado.");
    return;
  }

  Serial.println("Aguardando 3 segundos para estabilização mecânica do peso...");
  delay(3000); // Aguarda o balanço físico do peso sobre a balança parar
  
  // Executa os cálculos matemáticos para gerar o novo fator com base no peso real sobre a célula
  LoadCell.refreshDataSet();
  calibrationValue = LoadCell.getNewCalibration(massaConhecida);
  
  if (calibrationValue != 0) {
    LoadCell.setCalFactor(calibrationValue); // Aplica o novo fator imediatamente
    
    Serial.println();
    Serial.println("=== CALIBRAÇÃO REALIZADA COM SUCESSO! ===");
    Serial.print("Novo fator gerado: ");
    Serial.println(calibrationValue, 6);
    Serial.println("IMPORTANTE: Envie o comando 's' agora para salvar este ajuste na EEPROM!");
  } else {
    Serial.println("ERRO: Não foi possível calcular o fator. Verifique a fiação.");
  }
  Serial.println("==============================================");
}

// =========================================================================================
// FUNÇÃO PARA EXIBIR STATUS DO SISTEMA
// =========================================================================================
void exibirInfoSistema() {
  Serial.println();
  Serial.println("=== INFORMAÇÕES TÉCNICAS ===");
  Serial.print("Fator de calibração ativo: ");
  Serial.println(calibrationValue, 6);
  
  LoadCell.update();
  Serial.print("Última leitura bruta convertida: ");
  Serial.print(LoadCell.getData(), 2);
  Serial.println(" kg");
  Serial.println("============================");
}
