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
*/
#include "quantum.h"

#ifndef INDENT_SIZE
#    define INDENT_SIZE 2
#endif
#ifndef INSERT_MODE_LAYER
#    define INSERT_MODE_LAYER 0
#endif

enum custom_keycodes {
    PLACEHOLDER = SAFE_RANGE,  // can always be here
    VIM_GO
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
bool in_normal_mode     = false;

#define WITH_REPEATER(motion, repeat)               \
    for (uint16_t i = MAX(1, repeat); i > 0; --i) { \
        motion;                                     \
    }

void TAP_N_TIMES(uint16_t keycode, uint16_t repeat) { WITH_REPEATER(TAP(keycode), repeat); }

void normal_mode(void) {
    in_normal_mode = true;
    current        = DefaultCommand;
}

void insert_mode(void) {
    in_normal_mode = false;
    visual_mode    = false;
    shifted        = false;
    current        = DefaultCommand;
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

uint16_t translate_motion(uint16_t vim_key) {
    switch (vim_key) {
        case KC_W:
        case KC_B:
            return LCTL(current.motion == KC_W ? KC_RIGHT : KC_LEFT);
        case KC_H:
            return KC_LEFT;
        case KC_J:
            return KC_DOWN;
        case KC_K:
            return KC_UP;
        case KC_L:
            return KC_RIGHT;
        default:
            return KC_NO;
    }
}

void trigger_motion(uint16_t vim_key) {
    uint16_t navigation = translate_motion(vim_key);
    TAP_N_TIMES(navigation, current.repeat);
}

void trigger_and_hold_motion(uint16_t vim_key) {
    uint16_t navigation = translate_motion(vim_key);
    PRESS(navigation);
    if (current.repeat > 1) {
        WITH_REPEATER(RELEASE(navigation); PRESS(navigation), current.repeat - 1);
    }
}

void release_motion(uint16_t vim_key) {
    uint16_t navigation = translate_motion(vim_key);
    RELEASE(navigation);
}

void execute_current(void) {
    bool should_delete = current.action == KC_C || current.action == KC_D;
    bool should_copy   = should_delete || current.action == KC_Y;
    if (should_copy && !visual_mode) {
        if (current.shifted && should_delete) {
            PRESS(KC_LSHIFT);
            PRESS(KC_END);
            RELEASE(KC_LSHIFT);
        } else if (current.motion == KC_J || current.motion == KC_K) {
            select_n_lines(current.repeat, current.motion == KC_J);
            paste_line_end_fix = true;
        } else {
            PRESS(KC_LSHIFT);
            trigger_motion(current.motion);
            RELEASE(KC_LSHIFT);
            paste_line_end_fix = false;
        }
        TAP(LCTL(should_delete ? KC_X : KC_C));
        // Delete remaining newline
        if (should_delete && paste_line_end_fix && !current.shifted) TAP(KC_DEL);
        // Jump to left of selection
        if (!should_delete) TAP(KC_LEFT);
    } else if (should_copy && visual_mode) {
        RELEASE(KC_LSHIFT);
        TAP(LCTL(should_delete ? KC_X : KC_C));
        // Jump to left of selection
        if (!should_delete) TAP(KC_LEFT);
        paste_line_end_fix = false;
    }
    if (current.action == KC_C) {
        insert_mode();
    }
    if (current.action == KC_J) {
        WITH_REPEATER(TAP(KC_END); TAP(KC_DELETE); TAP(KC_SPACE), current.repeat);
    }
    if (current.action == KC_P) {
        if (shifted) {
            TAP(KC_LEFT);
        }
        if (paste_line_end_fix) {
            TAP(KC_END);
        }
        WITH_REPEATER(
            if (paste_line_end_fix) { TAP(KC_ENTER); }; TAP(LCTL(KC_V)), current.repeat);
        if (paste_line_end_fix) TAP(KC_HOME);
    }
    if (current.action == KC_U) {
        TAP_N_TIMES(LCTL(KC_Z), current.repeat);
    }
    if (current.action == KC_DOT || current.action == KC_COMM) {
        uint16_t k = current.action == KC_DOT ? KC_SPACE : KC_DEL;
        go_to_line_start();
        for (int i = 0; i < INDENT_SIZE; ++i) TAP(k);
        if (current.repeat > 0 && (current.motion == KC_J || current.motion == KC_K)) {
            for (int i = 0; i < current.repeat; ++i) {
                TAP(current.motion == KC_J ? KC_DOWN : KC_UP);
                if (current.action == KC_DOT) TAP(KC_HOME);
                for (int i = 0; i < INDENT_SIZE; ++i) TAP(k);
            }
            // Go back to starting position
            for (int i = 0; i < current.repeat; ++i) {
                TAP(current.motion == KC_J ? KC_UP : KC_DOWN);
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
        current.motion  = KC_J;
        execute_current();
        return;
    }
    if (current.action == keycode) {
        current.motion = KC_J;
        execute_current();
    } else {
        current.action = keycode;
    }
}

void maybe_motion(uint16_t keycode) {
    if (visual_mode || current.action == 0) {
        current.motion = keycode;
        trigger_and_hold_motion(current.motion);
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
    TAP_N_TIMES(KC_DEL, current.repeat);
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
    if (keycode == VIM_GO) {
        normal_mode();
        return true;
    }
    if (!in_normal_mode) {
        return true;
    }
    switch (keycode) {
        case KC_LSHIFT:
            shifted = record->event.pressed;
            return false;
        case KC_1 ... KC_0:
            if (record->event.pressed) {
                uint16_t digit = (keycode - KC_1 + 1) % 10;
                current.repeat = current.repeat * 10 + digit;
            }
            return false;
        // navigation block
        case KC_J:
            if (record->event.pressed && shifted) {
                current.action = KC_J;
                execute_current();
                return false;
            }
        case KC_H:
        case KC_K:
        case KC_L:
        case KC_W:
        case KC_B:
            if (record->event.pressed) {
                maybe_motion(keycode);
            } else {
                release_motion(keycode);
            }
            return false;

        case KC_V:
            if (record->event.pressed) {
                VI_VISUAL();
            }
            return false;

        // maybe actions
        case KC_DOT:
            if (record->event.pressed && !shifted) {
                repeat_last_action();
                return false;
            }
        case KC_COMM:
            if (record->event.pressed && shifted) {
                maybe_action(keycode, false);
                return false;
            }
        case KC_C:
        case KC_Y:
        case KC_D:
            if (record->event.pressed) {
                maybe_action(keycode, shifted);
            }
            return false;

        // immediate action
        case KC_U:
        case KC_P:
            if (record->event.pressed) {
                current.action  = keycode;
                current.shifted = shifted;
                execute_current();
            }
            return false;

        // immediate non-repeatable insert mode jumpers
        case KC_O:
            if (record->event.pressed) {
                VIM_NEWLINE(shifted);
            }
            return false;
        case KC_I:
            if (record->event.pressed) {
                VIM_INSERT(shifted);
            }
            return false;
        case KC_A:
            if (record->event.pressed) {
                VIM_APPEND(shifted);
            }
            return false;
        case KC_S:
            if (record->event.pressed) {
                VIM_SUB();
            }
            return false;
        default:
            return true;
    }
}
