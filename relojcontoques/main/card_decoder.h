#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Card suits
 */
typedef enum {
    CARD_SUIT_HEARTS = 1,   // ♥ Corazones
    CARD_SUIT_SPADES = 2,   // ♠ Picas
    CARD_SUIT_CLUBS = 3,    // ♣ Tréboles
    CARD_SUIT_DIAMONDS = 4, // ♦ Diamantes
} card_suit_t;

/**
 * @brief Card structure
 */
typedef struct {
    card_suit_t suit;
    uint8_t number; // 1-13 (Ace to King)
} card_t;

/**
 * @brief Callback when card is fully decoded
 *
 * @param card The decoded card
 */
typedef void (*card_decoded_callback_t)(card_t card);

/**
 * @brief Initialize card decoder
 */
void card_decoder_init(void);

/**
 * @brief Register callback for decoded cards
 *
 * @param callback Function to call when card is decoded
 */
void card_decoder_register_callback(card_decoded_callback_t callback);

/**
 * @brief Feed a tap to the decoder
 *
 * @param short_tap true for short tap, false for long tap
 */
void card_decoder_feed_tap(bool short_tap);

/**
 * @brief Reset decoder state
 */
void card_decoder_reset(void);

/**
 * @brief Get card name as string
 *
 * @param card The card to convert
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 */
void card_to_string(card_t card, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif
