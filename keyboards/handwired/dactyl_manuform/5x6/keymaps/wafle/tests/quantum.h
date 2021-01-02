/* Mock of quantum.h for testing */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <vector>
#include <utility>

typedef struct {
    uint8_t col;
    uint8_t row;
} keypos_t;

typedef struct {
    keypos_t key;
    bool     pressed;
    uint16_t time;
} keyevent_t;


typedef struct {
    bool    interrupted :1;
    bool    reserved2   :1;
    bool    reserved1   :1;
    bool    reserved0   :1;
    uint8_t count       :4;
} tap_t;

typedef struct {
    keyevent_t event;
    tap_t tap;
} keyrecord_t;

keyrecord_t pressed = {{{0,0},true,0}, {0,0,0,0,0}};
keyrecord_t depressed = {{{0,0},false,0}, {0,0,0,0,0}};


// egrep -o "KC_([A-Z0-9]+)" vim.h | sort | uniq >> tests/quantum.h
enum Keycodes {
    KC_A,
    KC_B,
    KC_C,
    KC_COMM,
    KC_D,
    KC_DEL,
    KC_DELETE,
    KC_DOT,
    KC_DOWN,
    KC_END,
    KC_ENTER,
    KC_H,
    KC_HOME,
    KC_I,
    KC_J,
    KC_K,
    KC_L,
    KC_LCTRL,
    KC_LEFT,
    KC_LSHIFT,
    KC_NO,
    KC_O,
    KC_P,
    KC_RIGHT,
    KC_S,
    KC_SPACE,
    KC_U,
    KC_UP,
    KC_V,
    KC_W,
    KC_X,
    KC_Y,
    KC_Z,
    KC_1,
    KC_2,
    KC_3,
    KC_4,
    KC_5,
    KC_6,
    KC_7,
    KC_8,
    KC_9,
    KC_0,
    SAFE_RANGE,
    QK_LCTL = 0x0100,
    QK_LSFT                 = 0x0200,
};

using keypress = std::pair<uint16_t, keyrecord_t*>;
std::vector<keypress> keycodes;

void register_code16(uint16_t keycode) {
    keycodes.push_back({keycode, &pressed});
};

void unregister_code16(uint16_t keycode) {
    keycodes.push_back({keycode, &depressed});
};

void layer_move(int16_t __attribute__((unused))x) {};

#define LCTL(kc) (QK_LCTL | (kc))
#define LSFT(kc) (QK_LSFT | (kc))
