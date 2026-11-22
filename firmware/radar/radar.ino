#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// Coordenadas de Destino
const double DESTINO_LAT = -21.897079;
const double DESTINO_LNG = -49.016867;

// Pinos dos LEDs
const int LED_ESQUERDA = 27;
const int LED_FRENTE   = 26;
const int LED_DIREITA  = 25;

// Pinos da Serial 2 para o GPS
const int RX_PIN = 16;
const int TX_PIN = 17;

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// Variáveis para reter a última localização e rumo válidos
double ultimaLat = 0.0;
double ultimaLng = 0.0;
double ultimoRumo = 0.0;
bool temLocalizacaoValida = false;
bool temRumoValido = false;

// Variável para a trava de chegada (Histerese)
bool chegouNoDestino = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  gpsSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(LED_ESQUERDA, OUTPUT);
  pinMode(LED_FRENTE, OUTPUT);
  pinMode(LED_DIREITA, OUTPUT);

  Serial.println("Iniciando Rastreador GPS...");
  Serial.println("Aguardando fixacao inicial...");
}

void loop() {
  // Lê continuamente os dados do GPS
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Atualiza cache com novas leituras válidas
  if (gps.location.isValid() && gps.location.isUpdated()) {
    ultimaLat = gps.location.lat();
    ultimaLng = gps.location.lng();
    temLocalizacaoValida = true;
  }

  // Atualiza o rumo apenas se estiver se movendo
  if (gps.course.isValid() && gps.speed.kmph() > 2.0) {
    ultimoRumo = gps.course.deg();
    temRumoValido = true;
  }

  // Processa navegação baseada no último estado conhecido
  if (temLocalizacaoValida) {
    double distancia = TinyGPSPlus::distanceBetween(
      ultimaLat, ultimaLng,
      DESTINO_LAT, DESTINO_LNG
    );

    // Lógica de Trava (Histerese) para evitar oscilação dos LEDs
    if (distancia < 5.0) {
      chegouNoDestino = true;  // Trava o status como chegou
    } else if (distancia > 15.0) {
      chegouNoDestino = false; // Destrava se afastar mais de 15 metros
    }

    // Se estiver no destino, pisca os LEDs e reinicia o loop
    if (chegouNoDestino) {
      Serial.println("DESTINO ALCANCADO!");
      piscarTodosLeds();
      return; 
    }

    // Calcula o rumo até o alvo a partir da última posição conhecida
    double azimuteDestino = TinyGPSPlus::courseTo(
      ultimaLat, ultimaLng,
      DESTINO_LAT, DESTINO_LNG
    );

    // Atualiza os LEDs direcionais se tiver um rumo válido
    if (temRumoValido) {
      double diferenca = azimuteDestino - ultimoRumo;

      // Normaliza entre -180 e +180 graus
      if (diferenca > 180.0) diferenca -= 360.0;
      if (diferenca < -180.0) diferenca += 360.0;

      atualizarLeds(diferenca);

      // Aviso no monitor serial
      if (gps.location.age() > 2000) {
        Serial.print("[SEM FIX ATUAL - USANDO CACHE] ");
      }
      Serial.print("Distancia: "); Serial.print(distancia);
      Serial.print("m | Correcao: "); Serial.print(diferenca); Serial.println(" deg");
    } else {
      apagarLeds();
      Serial.println("Movimente-se para calibrar o rumo...");
    }
  }
}

void atualizarLeds(double angulo) {
  apagarLeds();
  if (angulo < -15.0) {
    digitalWrite(LED_ESQUERDA, HIGH);
  } else if (angulo > 15.0) {
    digitalWrite(LED_DIREITA, HIGH);
  } else {
    digitalWrite(LED_FRENTE, HIGH);
  }
}

void apagarLeds() {
  digitalWrite(LED_ESQUERDA, LOW);
  digitalWrite(LED_FRENTE, LOW);
  digitalWrite(LED_DIREITA, LOW);
}

void piscarTodosLeds() {
  digitalWrite(LED_ESQUERDA, HIGH);
  digitalWrite(LED_FRENTE, HIGH);
  digitalWrite(LED_DIREITA, HIGH);
  delay(30);
  apagarLeds();
  delay(30);
}