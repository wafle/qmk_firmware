/* A standard layout for the Dactyl Manuform 5x6 Keyboard */

#include QMK_KEYBOARD_H

#include "dactyl_manuform.h"
#include "quantum.h"
#include "vim.h"


#ifdef USE_I2C
#include <stddef.h>
#ifdef __AVR__
  #include <avr/io.h>
  #include <avr/interrupt.h>
#endif
#endif

#define LAYOUT_5x6_M(\
  L00, L01, L02, L03, L04, L05,                          R00, R01, R02, R03, R04, R05, \
  L10, L11, L12, L13, L14, L15,                          R10, R11, R12, R13, R14, R15, \
  L20, L21, L22, L23, L24, L25,                          R20, R21, R22, R23, R24, R25, \
  L30, L31, L32, L33, L34, L35,                          R30, R31, R32, R33, R34, R35, \
            L42, L43,                                             R42, R43,           \
                      L44, L45, L55,                     R50, R40, R41,                     \
                  L53, L54 ,                         R51, R52) \
  { \
    { L00,   L01,   L02, L03, L04, L05 }, \
    { L10,   L11,   L12, L13, L14, L15 }, \
    { L20,   L21,   L22, L23, L24, L25 }, \
    { L30,   L31,   L32, L33, L34, L35 }, \
    { KC_NO, KC_NO, L42, L43, L44, L45 }, \
    { KC_NO, KC_NO, KC_NO, L53, L54, L55 }, \
                                          \
    { R00, R01, R02, R03, R04,   R05   }, \
    { R10, R11, R12, R13, R14,   R15   }, \
    { R20, R21, R22, R23, R24,   R25   }, \
    { R30, R31, R32, R33, R34,   R35   }, \
    { R40, R41, R42, R43, KC_NO, KC_NO }, \
    { R50, R51, R52, KC_NO, KC_NO, KC_NO }  \
}


#define _QWERTY 0
#define _RAISE 1
#define _VIM 2

#define RAISE MO(_RAISE)
#define VIM TO(_VIM)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

  [_QWERTY] = LAYOUT_5x6_M(
     KC_GRAVE , KC_1  , KC_2  , KC_3  , KC_4  , KC_5,                         KC_6  , KC_7  , KC_8  , KC_9  , KC_0  ,KC_MINS,
     KC_TAB , KC_Q  , KC_W  , KC_E  , KC_R  , KC_T  ,                         KC_Y  , KC_U  , KC_I  , KC_O  , KC_P  ,KC_EQL,
     KC_LSFT, KC_A  , KC_S  , KC_D  , KC_F  , KC_G  ,                         KC_H  , KC_J  , KC_K  , KC_L  ,KC_SCLN,KC_QUOT,
     KC_LCTL, KC_Z  , KC_X  , KC_C  , KC_V  , KC_B  ,                         KC_N  , KC_M  ,KC_COMM,KC_DOT ,KC_SLSH,KC_BSLASH,
                      KC_LBRC,KC_RBRC,                                                     KC_PIPE, KC_NONUS_HASH,
                                      RAISE,KC_LALT, KC_LGUI,                   KC_APP, KC_DEL, KC_RSFT,
                                            KC_SPC, KC_ESC,                    KC_BSPC, KC_ENT
  ),

  [_RAISE] = LAYOUT_5x6_M(
     KC_F12 , KC_F1 , KC_F2 , KC_F3 , KC_F4 , KC_F5 ,                                              KC_F6  , KC_F7 , KC_F8 , KC_F9 ,KC_F10 ,KC_F11,
     KC_CAPSLOCK,KC_MEDIA_PREV_TRACK,KC_MEDIA_PLAY_PAUSE,KC_MEDIA_NEXT_TRACK,_______,_______,      KC_HOME, KC_PGDOWN, KC_PGUP, KC_END ,_______, _______,
     KC_LSFT,KC_HOME,KC_PGUP,KC_PGDN,KC_END ,KC_LPRN,                                              KC_LEFT, KC_DOWN, KC_UP,KC_RIGHT ,KC_MINS,_______,
     KC_LCTL,KC_MUTE,KC_VOLU,KC_VOLD,VIM,_______,                                              KC_PSCR , KC_MS_WH_DOWN , KC_MS_WH_UP ,_______ ,_______ ,RESET,
                    _______,_______,                                                                       _______, KC_P0,
                                    _______,_______,_______,                          _______,_______, _______,
                                           _______, _______,                               _______,_______

  ),

  [_VIM] = LAYOUT_5x6_M(
     KC_GRAVE , VIM_1, VIM_2, VIM_3, VIM_4, VIM_5,                          VIM_6, VIM_7, VIM_8, VIM_9, VIM_0, KC_MINS,
     KC_TAB, KC_Q, VIM_W,  KC_E  , KC_R  ,  _______,                     VIM_Y  , VIM_U  , VIM_I, VIM_O  , VIM_P  ,KC_PLUS,
     VIM_SHIFT, VIM_A  , VIM_S, VIM_D  , KC_F  , _______,                    VIM_H, VIM_J, VIM_K, VIM_L ,KC_MINS,_______,
     KC_LCTL, KC_Z  , KC_X  , VIM_C, VIM_V  ,  VIM_B,                       KC_N  , KC_M  ,VIM_COMM,VIM_DOT ,KC_SLSH,KC_BSLASH,
                     _______,_______,                                                       KC_PIPE, KC_EQL,
                                    _______,_______,_______,                          _______,_______, _______,
                                           _______, _______,                               _______,_______
  ),
};
