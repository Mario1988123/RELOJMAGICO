package com.relojtoques

data class Card(
    val suit: String,    // "♥", "♠", "♣", "♦"
    val number: Int      // 1-13
) {
    fun getDisplayName(): String {
        val numberName = when (number) {
            1 -> "As"
            11 -> "J"
            12 -> "Q"
            13 -> "K"
            else -> number.toString()
        }

        val suitName = when (suit) {
            "♥" -> "Corazones"
            "♠" -> "Picas"
            "♣" -> "Tréboles"
            "♦" -> "Diamantes"
            else -> "Desconocido"
        }

        return "$numberName de $suitName $suit"
    }

    fun getShortName(): String {
        val numberName = when (number) {
            1 -> "A"
            11 -> "J"
            12 -> "Q"
            13 -> "K"
            else -> number.toString()
        }
        return "$numberName$suit"
    }

    fun getColor(): String {
        return when (suit) {
            "♥", "♦" -> "#D32F2F"  // Rojo
            "♠", "♣" -> "#000000"  // Negro
            else -> "#666666"
        }
    }
}
