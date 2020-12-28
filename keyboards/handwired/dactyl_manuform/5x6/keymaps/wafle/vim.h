/*
Some VIM features implemented for Windows/Linux.

Supported features:
 * Navigation: w, b, h, j, k, l
 * Joining lines
 * (y)anking, (d)eleting, (c)hanging, with navgiation motions
 * Visual mode toggles shift, which mostly works
 * <> indentation deletes spaces from the beginning of the line(s)
 * Pasting
 * Jumping to insert mode with o, i, a, s (also with uppercase)
 * . Repeat last action
 * Everything except jumping to insert mode supports repetition, e.g. 3p pastes 3 times

Possible improvements:
 * macOS does word navigation with ALT instead of CTRL, and CMD for Copy/Paste, these could be abstracted, but there is no host detection functionality in QMK currently.
 * Keys currently translate to tapping instead of pressing on keydown and releasing on keyup, so holding e.g. w will not result in in repeated jumps.
*/
#include "config.h"
#include "print.h"
#include "keycode.h"
#include "quantum.h"
#include "quantum_keycodes.h"
#include "string.h"

#ifndef INDENT_SIZE
#    define INDENT_SIZE 2
#endif
#ifndef INSERT_MODE_LAYER
#    define INSERT_MODE_LAYER 0
#endif

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

typedef struct Command {
    uint16_t action;
    bool     shifted;
    uint16_t repeat;
    uint16_t motion;
} Command;

const Command DefaultCommand = {};
Command       current        = {};
Command       previous       = {};

bool visual_mode        = false;
bool paste_line_end_fix = false;
bool shifted            = false;

#define WITH_REPEATER(motion)                               \
    for (uint16_t i = MAX(1, current.repeat); i > 0; --i) { \
        motion;                                             \
    }

void TAP_N_TIMES(uint16_t keycode) { WITH_REPEATER(TAP(keycode)); }

void insert_mode(void) {
    visual_mode = false;
    shifted     = false;
    current = DefaultCommand;
    RELEASE(KC_LSHIFT);
    RELEASE(KC_LCTRL);
    layer_move(INSERT_MODE_LAYER);
}

void go_to_line_start(void) {
    // Some text editors jump after the indent on the first home.
    TAP(KC_END);
    TAP(KC_HOME);
    TAP(KC_HOME);
}

void select_n_lines(int n, bool down) {
    RELEASE(KC_LSHIFT);
    down ? go_to_line_start() : TAP(KC_END);
    PRESS(KC_LSHIFT);
    for (; n > 1; --n) {
        TAP(down ? KC_DOWN : KC_UP);
    }
    TAP(down ? KC_END : KC_HOME);
    RELEASE(KC_LSHIFT);
}

void trigger_motion(void) {
    switch (current.motion) {
        case VIM_W:
        case VIM_B:
            PRESS(KC_LCTRL);
            TAP_N_TIMES(current.motion == VIM_W ? KC_RIGHT : KC_LEFT);
            RELEASE(KC_LCTRL);
            break;
        case VIM_H:
            TAP_N_TIMES(KC_LEFT);
            break;
        case VIM_J:
            TAP_N_TIMES(KC_DOWN);
            break;
        case VIM_K:
            TAP_N_TIMES(KC_UP);
            break;
        case VIM_L:
            TAP_N_TIMES(KC_RIGHT);
            break;
    }
}

void execute_current(void) {
    bool should_delete = current.action == VIM_C || current.action == VIM_D;
    bool should_copy   = should_delete || current.action == VIM_Y;
    if (should_copy && !visual_mode) {
        if (current.shifted && should_delete) {
            PRESS(KC_LSHIFT);
            PRESS(KC_END);
            RELEASE(KC_LSHIFT);
        } else if (current.motion == VIM_J || current.motion == VIM_K) {
            select_n_lines(current.repeat, current.motion == VIM_J);
            paste_line_end_fix = true;
        } else {
            PRESS(KC_LSHIFT);
            trigger_motion();
            RELEASE(KC_LSHIFT);
            paste_line_end_fix = false;
        }
        TAP(LCTL(should_delete ? KC_X : KC_C));
        // Delete remaining newline
        if (should_delete && paste_line_end_fix) TAP(KC_DEL);
        // Jump to left of selection
        if (!should_delete) TAP(KC_LEFT);
    } else if (should_copy && visual_mode) {
        RELEASE(KC_LSHIFT);
        TAP(LCTL(should_delete ? KC_X : KC_C));
        // Jump to left of selection
        if (!should_delete) TAP(KC_LEFT);
        paste_line_end_fix = false;
    }
    if (current.action == VIM_C) {
        insert_mode();
    }
    if (current.action == VIM_J) {
        WITH_REPEATER(TAP(KC_END); TAP(KC_DELETE); TAP(KC_SPACE));
    }
    if (current.action == VIM_P) {
        if (shifted) {
            TAP(KC_LEFT);
        }
        if (paste_line_end_fix) {
            TAP(KC_END);
        }
        WITH_REPEATER(if (paste_line_end_fix) { TAP(KC_ENTER); }; TAP(LCTL(KC_V)));
        if (paste_line_end_fix) TAP(KC_HOME);
    }
    if (current.action == VIM_U) {
        TAP_N_TIMES(LCTL(KC_Z));
    }
    if (current.action == VIM_DOT || current.action == VIM_COMM) {
        uint16_t k = current.action == VIM_DOT ? KC_SPACE : KC_DEL;
        go_to_line_start();
        for (int i = 0; i < INDENT_SIZE; ++i) TAP(k);
        if (current.repeat > 0 && (current.motion == VIM_J || current.motion == VIM_K)) {
            for (int i = 0; i < current.repeat; ++i) {
                TAP(current.motion == VIM_J ? KC_DOWN : KC_UP);
                if (current.action == VIM_DOT) TAP(KC_HOME);
                for (int i = 0; i < INDENT_SIZE; ++i) TAP(k);
            }
            // Go back to starting position
            for (int i = 0; i < current.repeat; ++i) {
                TAP(current.motion == VIM_J ? KC_UP : KC_DOWN);
                TAP(KC_HOME);
            }
        }
    }
    previous = current;
    current  = DefaultCommand;
}

void VI_VISUAL(void) {
    if (!visual_mode) {
        PRESS(KC_LSHIFT);
    } else {
        RELEASE(KC_LSHIFT);
    }
    visual_mode = !visual_mode;
}

void maybe_action(uint16_t keycode, bool shifted) {
    if (visual_mode || shifted) {
        current.action  = keycode;
        current.shifted = shifted;
        current.motion  = VIM_J;
        execute_current();
        return;
    }
    if (current.action == keycode) {
        current.motion = VIM_J;
        execute_current();
    } else {
        current.action = keycode;
    }
}

void maybe_motion(uint16_t keycode) {
    if (visual_mode || current.action == 0) {
        current.motion = keycode;
        trigger_motion();
        current.repeat = 0;
    } else {
        current.motion = keycode;
        execute_current();
    }
}

void VIM_APPEND(bool shifted) {
    if (shifted) {
        TAP(KC_END);
    } else {
        TAP(KC_RIGHT);
    }
    insert_mode();
}

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

void VIM_SUB(void) {
    TAP_N_TIMES(KC_DEL);
    insert_mode();
}

void repeat_last_action(void) {
    uint16_t repeat = current.repeat;
    current         = previous;
    if (repeat > 0) {
        current.repeat *= repeat;
    }
    execute_current();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case VIM_SHIFT:
            shifted = record->event.pressed;
            return false;
        case VIM_0 ... VIM_9:
            if (record->event.pressed) {
                current.repeat = current.repeat * 10 + keycode - VIM_0;
            }
            return false;
        // navigation block
        case VIM_J:
            if (record->event.pressed && shifted) {
                current.action = VIM_J;
                execute_current();
                return false;
            }
        case VIM_H:
        case VIM_K:
        case VIM_L:
        case VIM_W:
        case VIM_B:
            if (record->event.pressed) {
                maybe_motion(keycode);
            }
            return false;

        case VIM_V:
            if (record->event.pressed) {
                VI_VISUAL();
            }
            return false;

        // maybe actions
        case VIM_DOT:
            if (record->event.pressed && !shifted) {
                repeat_last_action();
                return false;
            }
        case VIM_COMM:
            if (record->event.pressed && shifted) {
                maybe_action(keycode, false);
                return false;
            }
        case VIM_C:
        case VIM_Y:
        case VIM_D:
            if (record->event.pressed) {
                maybe_action(keycode, shifted);
            }
            return false;

        // immediate action
        case VIM_U:
        case VIM_P:
            if (record->event.pressed) {
                current.action  = keycode;
                current.shifted = shifted;
                execute_current();
            }
            return false;

        // immediate non-repeatable insert mode jumpers
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
        case VIM_S:
            if (record->event.pressed) {
                VIM_SUB();
            }
            return false;
    }
    return true;
}
