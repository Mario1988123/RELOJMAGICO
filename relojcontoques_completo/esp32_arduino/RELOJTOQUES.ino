/*
 * RELOJ TOQUES - Sistema de transmisión de cartas por WiFi
 *
 * Hardware: ESP32-S3 con acelerómetro QMI8658
 *
 * Uso:
 * 1. Toca para seleccionar PALO (1-4 toques):
 *    1 toque  = Corazones ♥
 *    2 toques = Picas ♠
 *    3 toques = Tréboles ♣
 *    4 toques = Diamantes ♦
 *
 * 2. Espera 2-3 segundos
 *
 * 3. Toca para seleccionar NÚMERO (1-13 toques):
 *    1 toque  = As
 *    2-10     = 2-10
 *    11       = J
 *    12       = Q
 *    13       = K
 *
 * La carta se enviará automáticamente por WiFi beacon
 */

#include <Wire.h>
#include <WiFi.h>
#include "esp_wifi.h"

// ============================================================================
// CONFIGURACIÓN DE PINES
// ============================================================================
#define I2C_SDA 18
#define I2C_SCL 8

// ============================================================================
// CONFIGURACIÓN QMI8658
// ============================================================================
#define QMI8658_ADDR        0x6B
#define QMI8658_WHO_AM_I    0x00
#define QMI8658_CTRL1       0x02
#define QMI8658_CTRL2       0x03
#define QMI8658_CTRL7       0x08
#define QMI8658_ACCEL_X_L   0x35

// ============================================================================
// CONFIGURACIÓN DE DETECCIÓN
// ============================================================================
#define TAP_THRESHOLD       1500.0    // mg - umbral para detectar toque
#define TAP_COOLDOWN_MS     200       // ms entre toques
#define PAUSE_DURATION_MS   2500      // ms de pausa entre palo y número

// ============================================================================
// CARACTERES INVISIBLES PARA WIFI BEACON
// ============================================================================
#define ZWSP  "\xE2\x80\x8B"  // U+200B Zero Width Space
#define ZWNJ  "\xE2\x80\x8C"  // U+200C Zero Width Non-Joiner
#define ZWJ   "\xE2\x80\x8D"  // U+200D Zero Width Joiner
#define LRM   "\xE2\x80\x8E"  // U+200E Left-to-Right Mark

// ============================================================================
// ESTRUCTURAS Y VARIABLES GLOBALES
// ============================================================================

enum CardSuit {
  SUIT_HEARTS = 1,   // ♥
  SUIT_SPADES = 2,   // ♠
  SUIT_CLUBS = 3,    // ♣
  SUIT_DIAMONDS = 4  // ♦
};

struct Card {
  CardSuit suit;
  uint8_t number;  // 1-13
};

enum DecoderState {
  STATE_IDLE,
  STATE_COUNTING_SUIT,
  STATE_COUNTING_NUMBER
};

// Variables del decodificador
DecoderState state = STATE_IDLE;
uint8_t tapCount = 0;
unsigned long lastTapTime = 0;
Card currentCard = {SUIT_HEARTS, 0};

// Variables del detector de toques
unsigned long lastTapDetected = 0;
bool tapInProgress = false;
unsigned long tapStartTime = 0;

// ============================================================================
// FUNCIONES DEL ACELERÓMETRO QMI8658
// ============================================================================

void qmi8658WriteReg(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t qmi8658ReadReg(uint8_t reg) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(QMI8658_ADDR, 1);
  return Wire.read();
}

bool qmi8658Init() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);  // 400kHz

  delay(10);

  // Verificar WHO_AM_I
  uint8_t whoami = qmi8658ReadReg(QMI8658_WHO_AM_I);
  Serial.print("QMI8658 WHO_AM_I: 0x");
  Serial.println(whoami, HEX);

  // Configurar acelerómetro: 4g, 125Hz
  qmi8658WriteReg(QMI8658_CTRL1, 0x00);  // Disable all
  delay(10);
  qmi8658WriteReg(QMI8658_CTRL2, 0x95);  // Accel: 4g, 125Hz
  qmi8658WriteReg(QMI8658_CTRL7, 0x03);  // Enable accel

  Serial.println("QMI8658 inicializado");
  return true;
}

void qmi8658ReadAccel(float* ax, float* ay, float* az) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(QMI8658_ACCEL_X_L);
  Wire.endTransmission(false);
  Wire.requestFrom(QMI8658_ADDR, 6);

  uint8_t data[6];
  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }

  int16_t raw_x = (int16_t)((data[1] << 8) | data[0]);
  int16_t raw_y = (int16_t)((data[3] << 8) | data[2]);
  int16_t raw_z = (int16_t)((data[5] << 8) | data[4]);

  // Convertir a mg (4g range)
  *ax = (float)raw_x * 4000.0 / 32768.0;
  *ay = (float)raw_y * 4000.0 / 32768.0;
  *az = (float)raw_z * 4000.0 / 32768.0;
}

// ============================================================================
// DECODIFICADOR DE CARTAS
// ============================================================================

const char* getSuitName(CardSuit suit) {
  switch(suit) {
    case SUIT_HEARTS: return "Corazones ♥";
    case SUIT_SPADES: return "Picas ♠";
    case SUIT_CLUBS: return "Tréboles ♣";
    case SUIT_DIAMONDS: return "Diamantes ♦";
    default: return "???";
  }
}

const char* getNumberName(uint8_t num) {
  static char buf[4];
  switch(num) {
    case 1: return "As";
    case 11: return "J";
    case 12: return "Q";
    case 13: return "K";
    default:
      sprintf(buf, "%d", num);
      return buf;
  }
}

void resetDecoder() {
  state = STATE_IDLE;
  tapCount = 0;
  lastTapTime = 0;
  Serial.println("Decodificador reseteado");
}

void feedTap(bool shortTap) {
  unsigned long now = millis();
  unsigned long timeSinceLast = now - lastTapTime;

  Serial.print("TAP detectado: ");
  Serial.print(shortTap ? "CORTO" : "LARGO");
  Serial.print(" | Estado: ");
  Serial.print(state);
  Serial.print(" | Cuenta: ");
  Serial.print(tapCount);
  Serial.print(" | Tiempo desde último: ");
  Serial.print(timeSinceLast);
  Serial.println(" ms");

  switch(state) {
    case STATE_IDLE:
      if (shortTap) {
        state = STATE_COUNTING_SUIT;
        tapCount = 1;
        lastTapTime = now;
        Serial.println("→ Iniciando cuenta de PALO");
      }
      break;

    case STATE_COUNTING_SUIT:
      if (shortTap) {
        if (timeSinceLast > PAUSE_DURATION_MS) {
          // Pausa detectada - palo completo
          if (tapCount >= 1 && tapCount <= 4) {
            currentCard.suit = (CardSuit)tapCount;
            Serial.print("→ PALO decodificado: ");
            Serial.print(getSuitName(currentCard.suit));
            Serial.print(" (");
            Serial.print(tapCount);
            Serial.println(" toques)");

            // Este toque es el primero del número
            state = STATE_COUNTING_NUMBER;
            tapCount = 1;
            lastTapTime = now;
          } else {
            Serial.println("✗ Cantidad de toques inválida para palo");
            resetDecoder();
          }
        } else {
          // Continuar contando palo
          tapCount++;
          lastTapTime = now;
          Serial.print("→ Cuenta de PALO: ");
          Serial.println(tapCount);

          if (tapCount > 4) {
            Serial.println("✗ Demasiados toques para palo");
            resetDecoder();
          }
        }
      }
      break;

    case STATE_COUNTING_NUMBER:
      if (shortTap) {
        if (timeSinceLast > PAUSE_DURATION_MS) {
          // Carta completa
          if (tapCount >= 1 && tapCount <= 13) {
            currentCard.number = tapCount;

            Serial.println("");
            Serial.println("╔════════════════════════════════════════╗");
            Serial.print("║  CARTA DETECTADA: ");
            Serial.print(getNumberName(currentCard.number));
            Serial.print(" de ");
            Serial.print(getSuitName(currentCard.suit));
            for(int i = 0; i < 10; i++) Serial.print(" ");
            Serial.println("║");
            Serial.println("╚════════════════════════════════════════╝");
            Serial.println("");

            // Enviar por WiFi
            sendCardBeacon(currentCard);

            resetDecoder();
          } else {
            Serial.println("✗ Cantidad de toques inválida para número");
            resetDecoder();
          }
        } else {
          // Continuar contando número
          tapCount++;
          lastTapTime = now;
          Serial.print("→ Cuenta de NÚMERO: ");
          Serial.println(tapCount);

          if (tapCount > 13) {
            Serial.println("✗ Demasiados toques para número");
            resetDecoder();
          }
        }
      }
      break;
  }
}

// ============================================================================
// WIFI BEACON
// ============================================================================

void buildCardSSID(Card card, char* ssid, size_t maxLen) {
  strcpy(ssid, "CARD_");
  size_t offset = strlen(ssid);

  // Codificar palo
  const char* suitChar;
  switch(card.suit) {
    case SUIT_HEARTS: suitChar = ZWSP; break;
    case SUIT_SPADES: suitChar = ZWNJ; break;
    case SUIT_CLUBS: suitChar = ZWJ; break;
    case SUIT_DIAMONDS: suitChar = LRM; break;
    default: suitChar = ZWSP; break;
  }
  strncat(ssid, suitChar, maxLen - offset - 1);
  offset = strlen(ssid);

  // Separador
  strncat(ssid, ZWSP, maxLen - offset - 1);
  offset = strlen(ssid);

  // Codificar número en binario (4 bits)
  for (int i = 3; i >= 0; i--) {
    if ((card.number >> i) & 1) {
      strncat(ssid, ZWNJ, maxLen - offset - 1);
    } else {
      strncat(ssid, ZWSP, maxLen - offset - 1);
    }
    offset = strlen(ssid);
  }

  Serial.print("SSID construido: \"");
  Serial.print(ssid);
  Serial.print("\" (");
  Serial.print(strlen(ssid));
  Serial.println(" bytes)");
}

void sendBeaconFrame(const char* ssid) {
  uint8_t beacon[128];
  memset(beacon, 0, sizeof(beacon));

  // Frame Control
  beacon[0] = 0x80; beacon[1] = 0x00;

  // Duration
  beacon[2] = 0x00; beacon[3] = 0x00;

  // Destination (broadcast)
  memset(&beacon[4], 0xFF, 6);

  // Source & BSSID (MAC del ESP32)
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_AP, mac);
  memcpy(&beacon[10], mac, 6);
  memcpy(&beacon[16], mac, 6);

  // Sequence Control
  beacon[22] = 0x00; beacon[23] = 0x00;

  // Timestamp
  memset(&beacon[24], 0, 8);

  // Beacon Interval
  beacon[32] = 0x64; beacon[33] = 0x00;

  // Capability
  beacon[34] = 0x01; beacon[35] = 0x04;

  // SSID
  size_t ssidLen = strlen(ssid);
  beacon[36] = 0x00;        // Element ID
  beacon[37] = ssidLen;     // Length
  memcpy(&beacon[38], ssid, ssidLen);

  size_t frameLen = 38 + ssidLen;

  // Enviar
  esp_wifi_80211_tx(WIFI_IF_AP, beacon, frameLen, false);
}

void sendCardBeacon(Card card) {
  char ssid[64];
  buildCardSSID(card, ssid, sizeof(ssid));

  Serial.println("Enviando beacons WiFi...");

  // Enviar 50 beacons (~5 segundos)
  for (int i = 0; i < 50; i++) {
    sendBeaconFrame(ssid);
    delay(100);

    if (i % 10 == 0) {
      Serial.print(".");
    }
  }

  Serial.println("");
  Serial.println("✓ Transmisión completada");
}

// ============================================================================
// DETECTOR DE TOQUES
// ============================================================================

void detectTaps() {
  float ax, ay, az;
  qmi8658ReadAccel(&ax, &ay, &az);

  float mag = sqrt(ax*ax + ay*ay + az*az);
  float magNoGravity = fabs(mag - 1000.0);

  unsigned long now = millis();

  // Detectar inicio de toque
  if (magNoGravity > TAP_THRESHOLD && !tapInProgress) {
    unsigned long dt = now - lastTapDetected;
    if (dt > TAP_COOLDOWN_MS) {
      tapInProgress = true;
      tapStartTime = now;
    }
  }

  // Detectar fin de toque
  if (tapInProgress && magNoGravity < (TAP_THRESHOLD * 0.5)) {
    unsigned long duration = now - tapStartTime;
    bool isShort = duration < 2000;

    feedTap(isShort);

    tapInProgress = false;
    lastTapDetected = now;
  }
}

// ============================================================================
// SETUP Y LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║     RELOJ TOQUES - WiFi Beacons        ║");
  Serial.println("║                                        ║");
  Serial.println("║  Instrucciones:                        ║");
  Serial.println("║  1. Toca para PALO (1-4):              ║");
  Serial.println("║     • 1 = Corazones ♥                  ║");
  Serial.println("║     • 2 = Picas ♠                      ║");
  Serial.println("║     • 3 = Tréboles ♣                   ║");
  Serial.println("║     • 4 = Diamantes ♦                  ║");
  Serial.println("║                                        ║");
  Serial.println("║  2. Espera 2-3 segundos                ║");
  Serial.println("║                                        ║");
  Serial.println("║  3. Toca para NÚMERO (1-13):           ║");
  Serial.println("║     • 1 = As, 11 = J, 12 = Q, 13 = K  ║");
  Serial.println("║                                        ║");
  Serial.println("║  Ejemplo: 3 de Corazones               ║");
  Serial.println("║  → 1 toque (♥)                         ║");
  Serial.println("║  → Espera 2-3s                         ║");
  Serial.println("║  → 3 toques (3)                        ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("");

  // Inicializar acelerómetro
  Serial.println("Inicializando QMI8658...");
  if (!qmi8658Init()) {
    Serial.println("✗ Error al inicializar QMI8658");
    while(1) delay(1000);
  }

  // Inicializar WiFi
  Serial.println("Inicializando WiFi...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32_BEACON", "", 1, 0, 0);

  Serial.println("");
  Serial.println("✓ Sistema listo - Esperando toques...");
  Serial.println("");
}

void loop() {
  detectTaps();
  delay(20);  // 50 Hz
}
