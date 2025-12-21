package com.relojtoques

import android.util.Log

object CardDecoder {
    private const val TAG = "CardDecoder"

    // Caracteres Unicode invisibles
    private const val ZWSP = "\u200B"  // Zero Width Space
    private const val ZWNJ = "\u200C"  // Zero Width Non-Joiner
    private const val ZWJ = "\u200D"   // Zero Width Joiner
    private const val LRM = "\u200E"   // Left-to-Right Mark

    /**
     * Decodifica un SSID de WiFi beacon a una carta
     *
     * @param ssid SSID del beacon WiFi
     * @return Card si se decodifica correctamente, null si no es válido
     */
    fun decodeCard(ssid: String): Card? {
        try {
            // Verificar prefijo
            if (!ssid.startsWith("CARD_")) {
                return null
            }

            // Extraer parte codificada
            val encoded = ssid.substring(5)

            if (encoded.length < 6) {
                Log.w(TAG, "SSID demasiado corto: $encoded")
                return null
            }

            // Decodificar palo (primer carácter)
            val suitChar = encoded.substring(0, 1)
            val suit = when (suitChar) {
                ZWSP -> "♥"  // Corazones
                ZWNJ -> "♠"  // Picas
                ZWJ -> "♣"   // Tréboles
                LRM -> "♦"   // Diamantes
                else -> {
                    Log.w(TAG, "Carácter de palo desconocido: ${suitChar.toByteArray().joinToString()}")
                    return null
                }
            }

            // Saltar separador (índice 1)
            // Decodificar número (índices 2-5, 4 caracteres binarios)
            if (encoded.length < 6) {
                Log.w(TAG, "No hay suficientes caracteres para el número")
                return null
            }

            val numberBits = encoded.substring(2, 6)
            var number = 0

            for (i in 0 until 4) {
                number = number shl 1
                val bit = numberBits.substring(i, i + 1)
                if (bit == ZWNJ) {
                    number = number or 1
                }
            }

            // Validar rango
            if (number < 1 || number > 13) {
                Log.w(TAG, "Número fuera de rango: $number")
                return null
            }

            val card = Card(suit, number)
            Log.i(TAG, "Carta decodificada: ${card.getDisplayName()}")
            return card

        } catch (e: Exception) {
            Log.e(TAG, "Error decodificando carta: ${e.message}", e)
            return null
        }
    }

    /**
     * Convierte bytes a representación hexadecimal para debug
     */
    private fun ByteArray.toHexString(): String {
        return joinToString(" ") { "%02X".format(it) }
    }
}
