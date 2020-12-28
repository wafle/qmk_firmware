/*
Some VIM features implemented for Windows/Linux.

Supported features:
 * Navigation: w, b, j, k, with repetitions
 * (y)anking, (d)eleting, (c)hanging, including navgiation motions
 * Visual mode toggles shift, which mostly works
 * <> indentation deletes spaces from the beginning of the line(s)
 * Pasting with repetitions
 * Jumping to insert mode with c, o, i, a (also with uppercase)

Possible improvements:
 * macOS does word navigation with ALT instead of CTRL, and CMD for Copy/Paste, these could be abstracted, but there is no host detection functionality in QMK currently.
 * (.) repeat is possible, but would need rethinking.
 * Keys are currently translate to tapping instead of pressing on keydown and releasing on keyup, so holding e.g. w will not result in in repeated jumps.
*/
#include "config.h"
#include "print.h"
#include "keycode.h"
#include "quantum.h"
#include "quantum_keycodes.h"
#include "string.h"

#define INDENT_SIZE 2

enum custom_keycodes {
    PLACEHOLDER = SAFE_RANGE,  // can always be here
    VIM_0,
    VIM_1,
    VIM_2,
    VIM_3,
    VIM_4,
    VIM_5,
    VIM_6,
    VIM_7,
    VIM_8,
    VIM_9,
    VIM_A,
    VIM_B,
    VIM_C,
    VIM_D,
    VIM_E,
    VIM_H,
    VIM_I,
    VIM_J,
    VIM_K,
    VIM_L,
    VIM_O,
    VIM_P,
    VIM_S,
    VIM_U,
    VIM_V,
    VIM_VI,
    VIM_W,
    VIM_X,
    VIM_Y,
    VIM_DOT,
    VIM_COMM,
    VIM_SHIFT,
};

#define PRESS(keycode) register_code16(keycode)
#define RELEASE(keycode) unregister_code16(keycode)
void TAP(uint16_t keycode) {
    PRESS(keycode);
    RELEASE(keycode);
}
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

int repeat_counter = 0;
bool visual_mode  = false;
bool should_delete  = false;
bool should_copy    = false;
bool should_change  = false;
bool should_insert  = false;
bool shifted = false;

#define WITH_REPEATER(motion)                                                             \
    for (repeat_counter = MAX(repeat_counter, 1); repeat_counter > 0; --repeat_counter) { \
        motion;                                                                           \
    }

void TAP_N_TIMES(uint16_t keycode) { WITH_REPEATER(TAP(keycode)); }

void (*command_buffer)(void) = NULL;

void insert_mode(void) {
    repeat_counter = 0;
    visual_mode  = false;
    should_delete  = false;
    should_copy    = false;
    should_insert  = false;
    command_buffer = NULL;
    shifted = false;
    RELEASE(KC_LSHIFT);
    RELEASE(KC_LCTRL);
    layer_move(_QWERTY);
}

void motion_finisher(void) {
    if (should_copy) {
        should_copy = false;
        TAP(LCTL(should_delete ? KC_X : KC_C));
        RELEASE(KC_LSHIFT);
    }
    should_delete = false;
    if (should_insert) {
        insert_mode();
        return;
    }
    repeat_counter = 0;
}

void select_n_lines(int n, bool down) {
    RELEASE(KC_LSHIFT);
    TAP(down ? KC_HOME : KC_END);
    PRESS(KC_LSHIFT);
    for (; n > 1; --n) {
        TAP(down ? KC_DOWN : KC_UP);
    }
    TAP(down ? KC_END : KC_HOME);
    // Home/End stop at EOL, but we want to include newline.
    // TODO: add our own newline, then copy it and delete the old one? this would solve deleting at the beginning/end of files.
    TAP(down ? KC_RIGHT : KC_LEFT);
    RELEASE(KC_LSHIFT);
}

void VI_VISUAL(void) {
    if (!visual_mode) {
        PRESS(KC_LSHIFT);
    } else {
        RELEASE(KC_LSHIFT);
    }
    visual_mode = !visual_mode;
}

void VI_DELETE(bool shifted) {
    if (visual_mode) {
        should_copy = true;
        should_delete = true;
        motion_finisher();
        return;
    }
    if (shifted) {
        should_copy   = true;
        should_delete = true;
        PRESS(KC_LSHIFT);
        PRESS(KC_END);
        motion_finisher();
        return;
    }
    if (!should_delete) {
        should_copy   = true;
        should_delete = true;
    } else {
        select_n_lines(repeat_counter, true);
        motion_finisher();
    }
}

void VIM_COPY(bool shifted) {
    if (visual_mode) {
        should_copy = true;
        motion_finisher();
        return;
    } else if (shifted) {
        select_n_lines(0, true);
        should_copy = true;
        motion_finisher();
    } else {
        if (should_copy) {
            select_n_lines(repeat_counter, true);
            motion_finisher();
        } else {
            should_copy = true;
        }
    }
}

void VIM_BACK(void) {
    if (should_copy) {
        PRESS(KC_LSHIFT);
    }
    PRESS(KC_LCTRL);
    TAP_N_TIMES(KC_LEFT);
    RELEASE(KC_LCTRL);
    motion_finisher();
}

void VIM_FORWARD(void) {
    if (should_copy) {
        PRESS(KC_LSHIFT);
    }
    PRESS(KC_LCTRL);
    TAP_N_TIMES(KC_RIGHT);
    RELEASE(KC_LCTRL);
    motion_finisher();
}

void VIM_UNDO(void) { TAP_N_TIMES(LCTL(KC_Z)); }

void VIM_NEWLINE(bool shifted) {
    if (shifted) {
        TAP(KC_HOME);
        TAP(KC_ENTER);
        TAP(KC_UP);
    } else {
        TAP(KC_END);
        TAP(KC_ENTER);
    }
    insert_mode();
}

void VIM_INSERT(bool shifted) {
    if (shifted) {
        TAP(KC_HOME);
    }
    insert_mode();
}

void VIM_APPEND(bool shifted) {
    if (shifted) {
        TAP(KC_END);
    } else {
        TAP(KC_RIGHT);
    }
    insert_mode();
}

void VIM_CHANGE(bool shifted) {
    should_copy   = true;
    should_delete = true;
    should_insert = true;
    if (visual_mode) {
        motion_finisher();
    }
}

void VIM_PASTE(bool shifted) {
    if (shifted) {
        TAP(KC_LEFT);
    }
    PRESS(KC_LCTRL);
    TAP_N_TIMES(KC_V);
    RELEASE(KC_LCTRL);
}

void VIM_DOWN(bool shifted) {
    if (shifted) {
        WITH_REPEATER(TAP(KC_END); TAP(KC_DELETE));} else {
        if (command_buffer) {
            WITH_REPEATER(command_buffer(); TAP(KC_DOWN));
            command_buffer();
            command_buffer = NULL;
        } else {
            TAP_N_TIMES(KC_DOWN);
            motion_finisher();
        }
    }
}

void VIM_UP(void) {
    if (command_buffer) {
        WITH_REPEATER(command_buffer(); TAP(KC_UP));
        command_buffer();
        command_buffer = NULL;
    } else {
        TAP_N_TIMES(KC_UP);
        motion_finisher();
    }
}

void shift_left_sequence(void) {
    TAP(KC_HOME);
    for (int i = 0; i < INDENT_SIZE; ++i) {
        TAP(KC_DEL);
    }
}

void VIM_SHIFT_LEFT(void) {
    if (command_buffer == shift_left_sequence) {
        if (repeat_counter > 0) {
            VIM_DOWN(false);
        } else {
            command_buffer();
            command_buffer = NULL;
        }
    } else {
        command_buffer = shift_left_sequence;
    }
}

void shift_right_sequence(void) {
    TAP(KC_HOME);
    for (int i = 0; i < INDENT_SIZE; ++i) {
        TAP(KC_SPACE);
    }
}

void VIM_SHIFT_RIGHT(void) {
    if (command_buffer == shift_right_sequence) {
        if (repeat_counter > 0) {
            VIM_DOWN(false);
        } else {
            command_buffer();
            command_buffer = NULL;
        }
    } else {
        command_buffer = shift_right_sequence;
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case VIM_SHIFT:
            shifted = record->event.pressed;
            return false;
        case VIM_0 ... VIM_9:
            if (record->event.pressed) {
                repeat_counter = repeat_counter * 10 + keycode - VIM_0;
            }
            return false;
        case VIM_B:
            if (record->event.pressed) {
                VIM_BACK();
            }
            return false;
        case VIM_W:
            if (record->event.pressed) {
                VIM_FORWARD();
            }
            return false;
        case VIM_V:
            if (record->event.pressed) {
                VI_VISUAL();
            }

            return false;
        case VIM_D:
            if (record->event.pressed) {
                VI_DELETE(shifted);
            }
            return false;
        case VIM_U:
            if (record->event.pressed) {
                VIM_UNDO();
            }
            return false;
        case VIM_O:
            if (record->event.pressed) {
                VIM_NEWLINE(shifted);
            }
            return false;
        case VIM_I:
            if (record->event.pressed) {
                VIM_INSERT(shifted);
            }
            return false;
        case VIM_A:
            if (record->event.pressed) {
                VIM_APPEND(shifted);
            }
            return false;
        case VIM_Y:
            if (record->event.pressed) {
                VIM_COPY(shifted);
            }
            return false;
        case VIM_P:
            if (record->event.pressed) {
                VIM_PASTE(shifted);
            }
            return false;
        case VIM_J:
            if (record->event.pressed) {
                VIM_DOWN(shifted);
            }
            return false;
        case VIM_K:
            if (record->event.pressed) {
                VIM_UP();
            }
            return false;
        case VIM_COMM:
            if (record->event.pressed) {
                if (shifted) {
                    VIM_SHIFT_LEFT();
                }
            }
            return false;
        case VIM_DOT:
            if (record->event.pressed) {
                if (shifted) {
                    VIM_SHIFT_RIGHT();
                }
            }
            return false;
    }
    return true;
}
