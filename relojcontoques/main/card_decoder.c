#include "card_decoder.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "CARD_DECODER";

// Pause detection (between suit and number)
#define PAUSE_DURATION_MS   2500  // 2.5 seconds pause to separate suit from number

typedef enum {
    STATE_IDLE,
    STATE_COUNTING_SUIT,
    STATE_WAITING_FOR_NUMBER,
    STATE_COUNTING_NUMBER,
} decoder_state_t;

static decoder_state_t s_state = STATE_IDLE;
static uint8_t s_tap_count = 0;
static uint64_t s_last_tap_time = 0;
static card_t s_current_card = {0};
static card_decoded_callback_t s_callback = NULL;

static const char *suit_names[] = {
    "",
    "Corazones ♥",
    "Picas ♠",
    "Tréboles ♣",
    "Diamantes ♦"
};

static const char *number_names[] = {
    "",
    "As", "2", "3", "4", "5", "6", "7", "8", "9", "10",
    "J", "Q", "K"
};

void card_decoder_init(void) {
    s_state = STATE_IDLE;
    s_tap_count = 0;
    s_last_tap_time = 0;
    memset(&s_current_card, 0, sizeof(card_t));
}

void card_decoder_register_callback(card_decoded_callback_t callback) {
    s_callback = callback;
}

void card_decoder_feed_tap(bool short_tap) {
    uint64_t now = esp_timer_get_time() / 1000ULL; // Convert to ms
    uint64_t time_since_last = now - s_last_tap_time;

    ESP_LOGI(TAG, "Tap received: %s, state=%d, count=%d, time_since_last=%llu ms",
             short_tap ? "SHORT" : "LONG", s_state, s_tap_count, time_since_last);

    switch (s_state) {
        case STATE_IDLE:
            // Start counting suit taps
            if (short_tap) {
                s_state = STATE_COUNTING_SUIT;
                s_tap_count = 1;
                s_last_tap_time = now;
                ESP_LOGI(TAG, "Started counting suit taps");
            }
            break;

        case STATE_COUNTING_SUIT:
            if (short_tap) {
                // Check if this is a pause (time gap indicates end of suit)
                if (time_since_last > PAUSE_DURATION_MS) {
                    // Pause detected - suit is complete, this is first number tap
                    if (s_tap_count >= 1 && s_tap_count <= 4) {
                        s_current_card.suit = (card_suit_t)s_tap_count;
                        ESP_LOGI(TAG, "Suit decoded: %s (%d taps)",
                                 suit_names[s_current_card.suit], s_tap_count);

                        // This tap is the first number tap
                        s_state = STATE_COUNTING_NUMBER;
                        s_tap_count = 1;
                        s_last_tap_time = now;
                    } else {
                        ESP_LOGW(TAG, "Invalid suit tap count: %d, resetting", s_tap_count);
                        card_decoder_reset();
                    }
                } else {
                    // Continue counting suit taps
                    s_tap_count++;
                    s_last_tap_time = now;
                    ESP_LOGI(TAG, "Suit tap count: %d", s_tap_count);

                    // Limit suit to 4 (Diamantes)
                    if (s_tap_count > 4) {
                        ESP_LOGW(TAG, "Too many suit taps, resetting");
                        card_decoder_reset();
                    }
                }
            }
            break;

        case STATE_COUNTING_NUMBER:
            if (short_tap) {
                // Check if pause indicates end of card
                if (time_since_last > PAUSE_DURATION_MS) {
                    // Card is complete
                    if (s_tap_count >= 1 && s_tap_count <= 13) {
                        s_current_card.number = s_tap_count;
                        ESP_LOGI(TAG, "Card decoded: %s %s",
                                 number_names[s_current_card.number],
                                 suit_names[s_current_card.suit]);

                        if (s_callback) {
                            s_callback(s_current_card);
                        }

                        card_decoder_reset();
                    } else {
                        ESP_LOGW(TAG, "Invalid number tap count: %d, resetting", s_tap_count);
                        card_decoder_reset();
                    }
                } else {
                    // Continue counting number taps
                    s_tap_count++;
                    s_last_tap_time = now;
                    ESP_LOGI(TAG, "Number tap count: %d", s_tap_count);

                    // Limit to 13 (King)
                    if (s_tap_count > 13) {
                        ESP_LOGW(TAG, "Too many number taps, resetting");
                        card_decoder_reset();
                    }
                }
            }
            break;

        default:
            card_decoder_reset();
            break;
    }
}

void card_decoder_reset(void) {
    ESP_LOGI(TAG, "Decoder reset");
    s_state = STATE_IDLE;
    s_tap_count = 0;
    s_last_tap_time = 0;
    memset(&s_current_card, 0, sizeof(card_t));
}

void card_to_string(card_t card, char *buffer, size_t buffer_size) {
    if (card.suit < 1 || card.suit > 4 || card.number < 1 || card.number > 13) {
        snprintf(buffer, buffer_size, "Invalid Card");
        return;
    }

    snprintf(buffer, buffer_size, "%s de %s",
             number_names[card.number],
             suit_names[card.suit]);
}
