# Integración con NotesVoice

Este documento explica cómo integrar el sistema de toques del reloj con la aplicación NotesVoice para mostrar las cartas detectadas.

## 🎯 Objetivo

Cuando el reloj detecta una carta mediante toques, envía un WiFi beacon con caracteres invisibles. La app NotesVoice debe:

1. Escanear beacons WiFi
2. Detectar SSIDs que empiecen con "CARD_"
3. Decodificar los caracteres invisibles
4. Mostrar la carta correspondiente en pantalla

## 📡 Formato del Beacon

### Estructura del SSID

```
"CARD_" + [PALO] + [SEPARADOR] + [NÚMERO_BINARIO]
```

- **Prefijo**: `"CARD_"` (visible)
- **PALO**: 1 carácter invisible
- **SEPARADOR**: `U+200B` (ZWSP)
- **NÚMERO**: 4 caracteres invisibles (binario)

### Tabla de Caracteres Invisibles

| Carácter | Unicode | Código UTF-8 | Significado |
|----------|---------|--------------|-------------|
| ZWSP | U+200B | E2 80 8B | Zero Width Space |
| ZWNJ | U+200C | E2 80 8C | Zero Width Non-Joiner |
| ZWJ | U+200D | E2 80 8D | Zero Width Joiner |
| LRM | U+200E | E2 80 8E | Left-to-Right Mark |

### Codificación de Palos

| Palo | Carácter | Unicode |
|------|----------|---------|
| Corazones ♥ | ZWSP | U+200B |
| Picas ♠ | ZWNJ | U+200C |
| Tréboles ♣ | ZWJ | U+200D |
| Diamantes ♦ | LRM | U+200E |

### Codificación de Números

El número se codifica en binario de 4 bits usando:
- `ZWSP (U+200B)` = bit 0
- `ZWNJ (U+200C)` = bit 1

**Ejemplos:**
- Número 1 (0001): ZWSP + ZWSP + ZWSP + ZWNJ
- Número 3 (0011): ZWSP + ZWSP + ZWNJ + ZWNJ
- Número 7 (0111): ZWSP + ZWNJ + ZWNJ + ZWNJ
- Número 13 (1101): ZWNJ + ZWNJ + ZWSP + ZWNJ

## 💻 Código de Decodificación

### Android (Kotlin)

```kotlin
data class Card(val suit: String, val number: Int)

class CardBeaconDecoder {
    companion object {
        const val ZWSP = "\u200B"  // U+200B
        const val ZWNJ = "\u200C"  // U+200C
        const val ZWJ = "\u200D"   // U+200D
        const val LRM = "\u200E"   // U+200E

        fun decodeCard(ssid: String): Card? {
            // Verificar prefijo
            if (!ssid.startsWith("CARD_")) return null

            // Extraer parte codificada (después del prefijo)
            val encoded = ssid.substring(5)
            if (encoded.length < 6) return null  // Mínimo: 1 palo + 1 sep + 4 número

            // Decodificar palo (primer carácter)
            val suit = when (encoded.substring(0, 1)) {
                ZWSP -> "♥"
                ZWNJ -> "♠"
                ZWJ -> "♣"
                LRM -> "♦"
                else -> return null
            }

            // Saltar separador (carácter en índice 1)
            // Decodificar número (caracteres 2-5)
            val numberBits = encoded.substring(2, 6)
            var number = 0

            for (i in 0 until 4) {
                number = number shl 1
                if (numberBits.substring(i, i+1) == ZWNJ) {
                    number = number or 1
                }
            }

            // Validar rango (1-13)
            if (number < 1 || number > 13) return null

            return Card(suit, number)
        }

        fun cardToString(card: Card): String {
            val numberName = when (card.number) {
                1 -> "As"
                11 -> "J"
                12 -> "Q"
                13 -> "K"
                else -> card.number.toString()
            }
            return "$numberName de ${getSuitName(card.suit)}"
        }

        private fun getSuitName(suit: String): String {
            return when (suit) {
                "♥" -> "Corazones"
                "♠" -> "Picas"
                "♣" -> "Tréboles"
                "♦" -> "Diamantes"
                else -> "Desconocido"
            }
        }
    }
}
```

### Uso en Android (WiFi Scanning)

```kotlin
class CardScannerService : Service() {
    private val wifiManager by lazy {
        applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
    }

    private val wifiScanReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val success = intent.getBooleanExtra(WifiManager.EXTRA_RESULTS_UPDATED, false)
            if (success) {
                scanSuccess()
            }
        }
    }

    private fun scanSuccess() {
        val results = wifiManager.scanResults

        results.forEach { scanResult ->
            val ssid = scanResult.SSID

            // Intentar decodificar si es un beacon de carta
            CardBeaconDecoder.decodeCard(ssid)?.let { card ->
                Log.d("CardScanner", "Carta detectada: ${CardBeaconDecoder.cardToString(card)}")

                // Mostrar carta en UI
                showCard(card)
            }
        }
    }

    private fun showCard(card: Card) {
        // TODO: Actualizar UI con la carta
        // Ejemplo: mostrar imagen de la carta en pantalla

        val cardName = CardBeaconDecoder.cardToString(card)

        // Opción 1: Mostrar notificación
        showNotification(cardName)

        // Opción 2: Actualizar Activity/Fragment
        sendBroadcast(Intent("CARD_DETECTED").apply {
            putExtra("suit", card.suit)
            putExtra("number", card.number)
            putExtra("name", cardName)
        })
    }

    fun startScanning() {
        // Registrar receiver
        val intentFilter = IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)
        registerReceiver(wifiScanReceiver, intentFilter)

        // Iniciar escaneo periódico
        scanPeriodically()
    }

    private fun scanPeriodically() {
        // Escanear cada 500ms para capturar beacons
        val handler = Handler(Looper.getMainLooper())
        handler.postDelayed(object : Runnable {
            override fun run() {
                wifiManager.startScan()
                handler.postDelayed(this, 500)
            }
        }, 0)
    }
}
```

### iOS (Swift)

```swift
import NetworkExtension
import SystemConfiguration.CaptiveNetwork

struct Card {
    let suit: String
    let number: Int
}

class CardBeaconDecoder {
    static let ZWSP = "\u{200B}"
    static let ZWNJ = "\u{200C}"
    static let ZWJ = "\u{200D}"
    static let LRM = "\u{200E}"

    static func decodeCard(ssid: String) -> Card? {
        guard ssid.hasPrefix("CARD_") else { return nil }

        let encoded = String(ssid.dropFirst(5))
        guard encoded.count >= 6 else { return nil }

        // Decodificar palo
        let suitChar = String(encoded.prefix(1))
        guard let suit = decodeSuit(suitChar) else { return nil }

        // Decodificar número (caracteres 2-5)
        let startIndex = encoded.index(encoded.startIndex, offsetBy: 2)
        let endIndex = encoded.index(encoded.startIndex, offsetBy: 6)
        let numberBits = String(encoded[startIndex..<endIndex])

        var number = 0
        for char in numberBits {
            number <<= 1
            if String(char) == ZWNJ {
                number |= 1
            }
        }

        guard number >= 1 && number <= 13 else { return nil }

        return Card(suit: suit, number: number)
    }

    private static func decodeSuit(_ char: String) -> String? {
        switch char {
        case ZWSP: return "♥"
        case ZWNJ: return "♠"
        case ZWJ: return "♣"
        case LRM: return "♦"
        default: return nil
        }
    }

    static func cardToString(_ card: Card) -> String {
        let numberName: String
        switch card.number {
        case 1: numberName = "As"
        case 11: numberName = "J"
        case 12: numberName = "Q"
        case 13: numberName = "K"
        default: numberName = "\(card.number)"
        }

        let suitName = getSuitName(card.suit)
        return "\(numberName) de \(suitName)"
    }

    private static func getSuitName(_ suit: String) -> String {
        switch suit {
        case "♥": return "Corazones"
        case "♠": return "Picas"
        case "♣": return "Tréboles"
        case "♦": return "Diamantes"
        default: return "Desconocido"
        }
    }
}
```

## 🔍 Testing

### Ejemplo de SSID Codificado

Para la carta **3 de Corazones**:

```
SSID visible: "CARD_"
Caracteres invisibles:
  - ZWSP (♥ Corazones)
  - ZWSP (separador)
  - ZWSP (bit 0 del número 3)
  - ZWSP (bit 0)
  - ZWNJ (bit 1)
  - ZWNJ (bit 1)  <- número 3 = 0011 binario

SSID completo (en hex UTF-8):
"CARD_" + E2 80 8B + E2 80 8B + E2 80 8B + E2 80 8B + E2 80 8C + E2 80 8C
```

### Herramientas de Debug

1. **WiFi Analyzer** (Android): Para ver SSIDs y verificar que los beacons se envían
2. **Hex Editor**: Para inspeccionar los bytes UTF-8 del SSID
3. **Console Log**: Para verificar la decodificación

## 📱 UI en NotesVoice

Sugerencias para mostrar la carta:

1. **Pantalla completa**: Mostrar imagen grande de la carta
2. **Notificación**: Alert con nombre de la carta
3. **Animación**: Efecto de "carta revelada"
4. **Sonido**: Feedback auditivo cuando se detecta
5. **Historial**: Lista de cartas detectadas recientemente

## ⚠️ Consideraciones

### Permisos Necesarios (Android)

```xml
<uses-permission android:name="android.permission.ACCESS_WIFI_STATE"/>
<uses-permission android:name="android.permission.CHANGE_WIFI_STATE"/>
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>
```

### Limitaciones iOS

- iOS tiene restricciones para escaneo WiFi en background
- Considerar usar Bluetooth Low Energy (BLE) como alternativa para iOS
- Requiere app en foreground para escaneo WiFi

### Optimizaciones

1. **Caché de SSIDs**: Evitar decodificar el mismo SSID múltiples veces
2. **Filtrado**: Solo procesar SSIDs que empiecen con "CARD_"
3. **Debouncing**: Evitar múltiples detecciones de la misma carta
4. **TTL**: Ignorar beacons antiguos (basado en timestamp)

## 🚀 Próximos Pasos

- [ ] Implementar decodificador en NotesVoice
- [ ] Añadir UI para mostrar cartas
- [ ] Implementar caché de cartas detectadas
- [ ] Añadir animaciones
- [ ] Testing con dispositivo real
- [ ] Documentar protocolo completo
- [ ] Considerar BLE como alternativa para iOS

## 📞 Soporte

Si tienes problemas con la integración, verifica:

1. Los beacons se están enviando (WiFi Analyzer)
2. El SSID tiene el formato correcto
3. Los caracteres invisibles se están codificando correctamente
4. La app tiene permisos de ubicación (Android) o WiFi (iOS)
