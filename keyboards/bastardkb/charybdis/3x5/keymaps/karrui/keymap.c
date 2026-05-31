/**
 * Copyright 2021 Charly Delay <charly@codesink.dev> (@0xcharly)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

#ifdef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
#    include "timer.h"
#endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE

enum charybdis_keymap_layers {
    LAYER_BASE = 0,
    LAYER_FUNCTION,
    LAYER_NAVIGATION,
    LAYER_MEDIA,
    LAYER_POINTER,
    LAYER_NUMERAL,
    LAYER_SYMBOLS,
    // Raised automatically by trackball movement.  Kept last so adding it
    // doesn't shift the indices of the existing (VIA-visible) layers.
    LAYER_AUTO_POINTER,
};

// Automatically enable sniping-mode on the pointer layer.
#define CHARYBDIS_AUTO_SNIPING_ON_LAYER LAYER_POINTER

// Custom keymap keycodes.  Placed in the `QK_USER` range so they don't collide
// with the keyboard's own custom keycodes (DRGSCRL, DPI_MOD, ...) and can be
// bound from VIA via the "Any" key (QK_USER_0 == 0x7E40).
enum keymap_keycodes {
    // Toggle the auto pointer-layer trigger on/off at runtime.
    AUTO_POINTER_LAYER_TOGGLE = QK_USER_0,
};
#define AML_TOG AUTO_POINTER_LAYER_TOGGLE

#ifdef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
static uint16_t auto_pointer_layer_timer = 0;
// Running sum of pointer movement since the last trigger.  Accumulating across
// polling cycles (instead of testing a single report) keeps the trigger
// consistent whether a half is used alone or paired -- when paired, the split
// transport slices the same swipe into a different number of cycles, so a
// per-report threshold mistunes between the two configurations.
static uint16_t auto_pointer_layer_cum = 0;
// Runtime on/off switch for the whole auto-trigger feature, toggled from VIA
// via AUTO_POINTER_LAYER_TOGGLE.  Defaults on.
static bool auto_pointer_layer_enabled = true;
#    ifdef RGB_MATRIX_ENABLE
// Whether the auto-trigger currently owns the RGB feedback (green).  Lets the
// layer-off handler restore the default effect without clobbering RGB set by
// other layers.
static bool auto_pointer_rgb_on = false;
#    endif // RGB_MATRIX_ENABLE

#    ifndef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS
#        define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS 1000
#    endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS

// Accumulated movement (sum of |x| + |y| over cycles) required to trigger.
// Higher = more deliberate movement needed.  Lower = more sensitive.
#    ifndef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD
#        define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD 280
#    endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD
#endif     // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE

#define ESC_MED LT(LAYER_MEDIA, KC_ESC)
#define SPC_NAV LT(LAYER_NAVIGATION, KC_SPC)
#define TAB_FUN LT(LAYER_FUNCTION, KC_TAB)
#define ENT_SYM LT(LAYER_SYMBOLS, KC_ENT)
#define BSP_NUM LT(LAYER_NUMERAL, KC_BSPC)
#define _L_PTR(KC) LT(LAYER_POINTER, KC)

#ifndef POINTING_DEVICE_ENABLE
#    define DRGSCRL KC_NO
#    define DPI_MOD KC_NO
#    define S_D_MOD KC_NO
#    define SNIPING KC_NO
#endif // !POINTING_DEVICE_ENABLE

// clang-format off
/** \brief ColemakDH layout (3 rows, 10 columns). */
#define LAYOUT_LAYER_BASE                                                                     \
       KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,    KC_J,    KC_L,    KC_U,    KC_Y,    KC_QUOT, \
       KC_A,    KC_R,    KC_S,    KC_T,    KC_G,    KC_M,    KC_N,    KC_E,    KC_I,    KC_O, \
       KC_Z,    KC_X,    KC_C,    KC_D,    KC_V,    KC_K,    KC_H, KC_COMM,    KC_DOT, KC_SLSH, \
                      ESC_MED, SPC_NAV, TAB_FUN, ENT_SYM, BSP_NUM

/** Convenience row shorthands. */
#define _______________DEAD_HALF_ROW_______________ XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX
#define ______________HOME_ROW_GACS_L______________ KC_LGUI, KC_LALT, KC_LCTL, KC_LSFT, XXXXXXX
#define ______________HOME_ROW_GACS_R______________ XXXXXXX, KC_LSFT, KC_LCTL, KC_LALT, KC_LGUI

/*
 * Layers used on the Charybdis Nano.
 *
 * These layers started off heavily inspired by the Miryoku layout, but trimmed
 * down and tailored for a stock experience that is meant to be fundation for
 * further personalization.
 *
 * See https://github.com/manna-harbour/miryoku for the original layout.
 */

/**
 * \brief Function layer.
 *
 * Secondary right-hand layer has function keys mirroring the numerals on the
 * primary layer with extras on the pinkie column, plus system keys on the inner
 * column. App is on the tertiary thumb key and other thumb keys are duplicated
 * from the base layer to enable auto-repeat.
 */
#define LAYOUT_LAYER_FUNCTION                                                                 \
    _______________DEAD_HALF_ROW_______________, KC_PSCR,   KC_F7,   KC_F8,   KC_F9,  KC_F12, \
    ______________HOME_ROW_GACS_L______________, KC_SCRL,   KC_F4,   KC_F5,   KC_F6,  KC_F11, \
    _______________DEAD_HALF_ROW_______________, KC_PAUS,   KC_F1,   KC_F2,   KC_F3,  KC_F10, \
                      XXXXXXX, XXXXXXX, _______, XXXXXXX, XXXXXXX

/**
 * \brief Media layer.
 *
 * Tertiary left- and right-hand layer is media and RGB control.  This layer is
 * symmetrical to accomodate the left- and right-hand trackball.
 */
#define LAYOUT_LAYER_MEDIA                                                                    \
    XXXXXXX,RGB_RMOD, RGB_TOG, RGB_MOD, XXXXXXX, XXXXXXX,RGB_RMOD, RGB_TOG, RGB_MOD, XXXXXXX, \
    KC_MPRV, KC_VOLD, KC_MUTE, KC_VOLU, KC_MNXT, KC_MPRV, KC_VOLD, KC_MUTE, KC_VOLU, KC_MNXT, \
    XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX,XXXXXXX,XXXXXXX,XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
                      _______, KC_MPLY, KC_MSTP, KC_MSTP, KC_MPLY

/** \brief Mouse emulation and pointer functions. */
#define LAYOUT_LAYER_POINTER                                                                  \
    DT_PRNT,  DT_UP,  DT_DOWN, DPI_MOD, S_D_MOD, S_D_MOD, DPI_MOD, XXXXXXX, XXXXXXX, XXXXXXX, \
    ______________HOME_ROW_GACS_L______________, ______________HOME_ROW_GACS_R______________, \
    _______, DRGSCRL, C(KC_C), C(KC_V), C(KC_A), XXXXXXX, XXXXXXX, SNIPING, AML_TOG, _______, \
                      KC_BTN2, KC_BTN1, KC_BTN3, KC_BTN3, KC_BTN1

/**
 * \brief Navigation layer.
 *
 * Primary right-hand layer (left home thumb) is navigation and editing. Cursor
 * keys are on the home position, line and page movement below, clipboard above,
 * caps lock and insert on the inner column. Thumb keys are duplicated from the
 * base layer to avoid having to layer change mid edit and to enable auto-repeat.
 */
#define LAYOUT_LAYER_NAVIGATION                                                               \
    _______________DEAD_HALF_ROW_______________, XXXXXXX, XXXXXXX, KC_UP, XXXXXXX, XXXXXXX,    \
    ______________HOME_ROW_GACS_L______________, KC_CAPS, KC_LEFT, KC_DOWN,   KC_UP, KC_RGHT, \
    _______________DEAD_HALF_ROW_______________,  KC_INS, KC_HOME, KC_PGDN, KC_PGUP,  KC_END, \
                      XXXXXXX, _______, XXXXXXX,  KC_ENT, KC_BSPC

/**
 * \brief Numeral layout.
 *
 * Primary left-hand layer (right home thumb) is numerals and symbols. Numerals
 * are in the standard numpad locations with symbols in the remaining positions.
 * `KC_DOT` is duplicated from the base layer.
 */
#define LAYOUT_LAYER_NUMERAL                                                                  \
    KC_GRV,  KC_SCLN, KC_MINS, KC_EQL, KC_BSLS, _______________DEAD_HALF_ROW_______________, \
    KC_1,    KC_2,    KC_3,    KC_4,   KC_5,    ______________HOME_ROW_GACS_R______________, \
    KC_6,    KC_7,    KC_8,    KC_9,   KC_0,    _______________DEAD_HALF_ROW_______________, \
                       KC_LBRC, KC_RBRC, KC_DOT, XXXXXXX, _______

/**
 * \brief Symbols layer.
 *
 * Secondary left-hand layer has shifted symbols in the same locations to reduce
 * chording when using mods with shifted symbols. `KC_LPRN` is duplicated next to
 * `KC_RPRN`.
 */
#define LAYOUT_LAYER_SYMBOLS                                                                  \
    KC_TILD,  KC_SCLN, KC_UNDS, KC_PLUS, KC_PIPE, _______________DEAD_HALF_ROW_______________, \
    KC_EXLM,  KC_AT, KC_HASH, KC_DLR, KC_PERC, ______________HOME_ROW_GACS_R______________, \
    KC_CIRC, KC_AMPR,   KC_ASTR, KC_LPRN, KC_RPRN, _______________DEAD_HALF_ROW_______________, \
                      KC_LCBR, KC_RCBR, KC_GT, _______, XXXXXXX

/**
 * \brief Auto mouse layer.
 *
 * Raised automatically by trackball movement (see the auto-trigger below).
 * Transparent everywhere except mouse controls, so any other key falls through
 * to the base layer and types immediately (which also drops this layer).  The
 * right-hand bottom row gives one-handed clicks: H = left, "," = right,
 * "." = drag-scroll, "/" = middle; thumbs mirror the manual pointer layer.
 */
#define LAYOUT_LAYER_AUTO_POINTER                                                              \
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
    _______, _______, _______, _______, _______, _______, KC_BTN1, KC_BTN2, DRGSCRL, KC_BTN3, \
                      KC_BTN2, KC_BTN1, KC_BTN3, KC_BTN3, KC_BTN1

/**
 * \brief Add Home Row mod to a layout.
 *
 * Expects a 10-key per row layout.  Adds support for GACS (Gui, Alt, Ctl, Shift)
 * home row.  The layout passed in parameter must contain at least 20 keycodes.
 *
 * This is meant to be used with `LAYER_ALPHAS_QWERTY` defined above, eg.:
 *
 *     HOME_ROW_MOD_GACS(LAYER_ALPHAS_QWERTY)
 */
#define _HOME_ROW_MOD_GACS(                                            \
    L00, L01, L02, L03, L04, R05, R06, R07, R08, R09,                  \
    L10, L11, L12, L13, L14, R15, R16, R17, R18, R19,                  \
    ...)                                                               \
             L00,         L01,         L02,         L03,         L04,  \
             R05,         R06,         R07,         R08,         R09,  \
      LGUI_T(L10), LALT_T(L11), LCTL_T(L12), LSFT_T(L13),        L14,  \
             R15,  RSFT_T(R16), RCTL_T(R17), LALT_T(R18), RGUI_T(R19), \
      __VA_ARGS__
#define HOME_ROW_MOD_GACS(...) _HOME_ROW_MOD_GACS(__VA_ARGS__)

/**
 * \brief Add pointer layer keys to a layout.
 *
 * Expects a 10-key per row layout.  The layout passed in parameter must contain
 * at least 30 keycodes.
 *
 * This is meant to be used with `LAYER_ALPHAS_QWERTY` defined above, eg.:
 *
 *     POINTER_MOD(LAYER_ALPHAS_QWERTY)
 */
#define _POINTER_MOD(                                                  \
    L00, L01, L02, L03, L04, R05, R06, R07, R08, R09,                  \
    L10, L11, L12, L13, L14, R15, R16, R17, R18, R19,                  \
    L20, L21, L22, L23, L24, R25, R26, R27, R28, R29,                  \
    ...)                                                               \
             L00,         L01,         L02,         L03,         L04,  \
             R05,         R06,         R07,         R08,         R09,  \
             L10,         L11,         L12,         L13,         L14,  \
             R15,         R16,         R17,         R18,         R19,  \
      _L_PTR(L20),        L21,         L22,         L23,         L24,  \
             R25,         R26,         R27,         R28,  _L_PTR(R29), \
      __VA_ARGS__
#define POINTER_MOD(...) _POINTER_MOD(__VA_ARGS__)

#define LAYOUT_wrapper(...) LAYOUT(__VA_ARGS__)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [LAYER_BASE] = LAYOUT_wrapper(
    POINTER_MOD(HOME_ROW_MOD_GACS(LAYOUT_LAYER_BASE))
  ),
  [LAYER_FUNCTION] = LAYOUT_wrapper(LAYOUT_LAYER_FUNCTION),
  [LAYER_NAVIGATION] = LAYOUT_wrapper(LAYOUT_LAYER_NAVIGATION),
  [LAYER_MEDIA] = LAYOUT_wrapper(LAYOUT_LAYER_MEDIA),
  [LAYER_NUMERAL] = LAYOUT_wrapper(LAYOUT_LAYER_NUMERAL),
  [LAYER_POINTER] = LAYOUT_wrapper(LAYOUT_LAYER_POINTER),
  [LAYER_SYMBOLS] = LAYOUT_wrapper(LAYOUT_LAYER_SYMBOLS),
  [LAYER_AUTO_POINTER] = LAYOUT_wrapper(LAYOUT_LAYER_AUTO_POINTER),
};

#ifdef CHORDAL_HOLD
// Per-key handedness for Chordal Hold's opposite-hands rule.  Fingers are L/R by
// side; thumbs and the pointer-layer keys (Z, /) are '*' (exempt) so they can
// settle as held in *same-hand* chords too -- e.g. Z->X to drop straight into
// drag-scroll, which the same-hand rule would otherwise force to a tap.  O is
// also '*' because it's commonly capitalized with the *right* Shift (N) -- a
// same-hand chord that would otherwise be forced to a tap (the "nover" bug).
const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS] PROGMEM = LAYOUT_wrapper(
    'L', 'L', 'L', 'L', 'L',   'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L',   'R', 'R', 'R', 'R', '*',
    '*', 'L', 'L', 'L', 'L',   'R', 'R', 'R', 'R', '*',
                   '*', '*', '*',   '*', '*'
);
#endif // CHORDAL_HOLD
// clang-format on

#ifdef COMBO_ENABLE
// Z + X together -> drag-scroll, instantly.  Independent of Z's tap-hold timing,
// so it gives snappy drag-scroll entry without affecting how Z types or enters
// the pointer layer.  "zx" isn't a real letter sequence, so it won't misfire
// while typing; a deliberate "hold Z, then press X later" still uses the layer.
const uint16_t PROGMEM combo_zx_dragscroll[] = {_L_PTR(KC_Z), KC_X, COMBO_END};
combo_t key_combos[] = {
    COMBO(combo_zx_dragscroll, DRGSCRL),
};
#endif // COMBO_ENABLE

#ifdef POINTING_DEVICE_ENABLE
#    ifdef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
// Keys that should NOT drop the auto mouse layer: mouse buttons, drag-scroll,
// and modifiers (for click-combos).  Everything else returns to base on press.
static bool is_auto_mouse_keep_alive(uint16_t keycode) {
    switch (keycode) {
        case KC_BTN1 ... KC_BTN5:
        case DRGSCRL:
            return true;
        default:
            return IS_MODIFIER_KEYCODE(keycode);
    }
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    // Feature switched off from VIA: don't run any auto-trigger logic.  Manual
    // `_L_PTR` holds are unaffected -- they're driven by the layer-tap, not here.
    if (!auto_pointer_layer_enabled) {
        return mouse_report;
    }
    // Don't auto-trigger while the rich pointer layer is held manually (`_L_PTR`).
    // That's a deliberate mode with its own sniping DPI and button set.
    if (layer_state_is(LAYER_POINTER)) {
        auto_pointer_layer_cum = 0;
        return mouse_report;
    }
    auto_pointer_layer_cum += abs(mouse_report.x) + abs(mouse_report.y);
    if (auto_pointer_layer_cum > CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD) {
        auto_pointer_layer_cum = 0;
        const bool was_off = auto_pointer_layer_timer == 0;
        auto_pointer_layer_timer = timer_read();
        if (was_off) {
            layer_on(LAYER_AUTO_POINTER);
#        ifdef RGB_MATRIX_ENABLE
            rgb_matrix_mode_noeeprom(RGB_MATRIX_NONE);
            // Green (HSV_GREEN is hue 85, sat 255) but at the user's current
            // brightness -- HSV_GREEN's value of 255 would force full brightness
            // and ignore the configured level.
            rgb_matrix_sethsv_noeeprom(85, 255, rgb_matrix_get_val());
            auto_pointer_rgb_on = true;
#        endif // RGB_MATRIX_ENABLE
        }
    }
    return mouse_report;
}

void matrix_scan_user(void) {
    if (auto_pointer_layer_timer != 0 && TIMER_DIFF_16(timer_read(), auto_pointer_layer_timer) >= CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS) {
        auto_pointer_layer_timer = 0;
        auto_pointer_layer_cum   = 0;
        // RGB reset is handled by layer_state_set_user on the layer-off transition.
        layer_off(LAYER_AUTO_POINTER);
    }
}
#    endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE

#    ifdef CHARYBDIS_AUTO_SNIPING_ON_LAYER
layer_state_t layer_state_set_user(layer_state_t state) {
    // Sniping DPI follows the manual pointer layer only; the auto mouse layer
    // keeps the normal DPI.  Since they are now separate layers this is a simple
    // membership test.
    charybdis_set_pointer_sniping_enabled(layer_state_cmp(state, CHARYBDIS_AUTO_SNIPING_ON_LAYER));
#        if defined(CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE) && defined(RGB_MATRIX_ENABLE)
    // Restore the default RGB effect when the auto mouse layer turns off, however
    // it was raised (auto timeout, keypress, or handoff to a manual hold).
    // Guarded by `auto_pointer_rgb_on` so we don't clobber RGB owned by other
    // layers.
    if (!layer_state_cmp(state, LAYER_AUTO_POINTER) && auto_pointer_rgb_on) {
        rgb_matrix_mode_noeeprom(RGB_MATRIX_DEFAULT_MODE);
        auto_pointer_rgb_on = false;
    }
#        endif
    return state;
}
#    endif // CHARYBDIS_AUTO_SNIPING_ON_LAYER
#endif     // POINTING_DEVICE_ENABLE

#ifdef RGB_MATRIX_ENABLE
// Forward-declare this helper function since it is defined in
// rgb_matrix.c.
void rgb_matrix_update_pwm_buffers(void);
#endif

// Tapping term for the pointer-layer keys, shorter than the global 250 so the
// layer is reachable with a comfortable hold, but not so short that a normally-
// typed 'z' or '/' times out into a hold mid-word.  175ms matches the feel you're
// used to.  Everything else falls through to g_tapping_term so DYNAMIC_TAPPING_TERM
// (DT_ keys) keeps working.
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case _L_PTR(KC_Z):
        case _L_PTR(KC_SLSH):
            return 175;
    }
#ifdef DYNAMIC_TAPPING_TERM_ENABLE
    return g_tapping_term;
#else
    return TAPPING_TERM;
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
    // On the auto mouse layer, pressing any non-mouse key returns to the base
    // layer immediately so typing doesn't wait for the timeout.  Mouse buttons,
    // drag-scroll, and modifiers (see is_auto_mouse_keep_alive) keep it up.
    if (record->event.pressed && layer_state_is(LAYER_AUTO_POINTER) && !is_auto_mouse_keep_alive(keycode)) {
        auto_pointer_layer_timer = 0;
        auto_pointer_layer_cum   = 0;
        layer_off(LAYER_AUTO_POINTER); // RGB reset handled by layer_state_set_user
    }
#endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
    switch (keycode) {

#ifdef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
    case AUTO_POINTER_LAYER_TOGGLE:
        if (record->event.pressed) {
            auto_pointer_layer_enabled = !auto_pointer_layer_enabled;
            // If turning the feature off while the auto mouse layer is up, drop
            // it now instead of waiting out the timeout.
            if (!auto_pointer_layer_enabled && auto_pointer_layer_timer != 0) {
                auto_pointer_layer_timer = 0;
                auto_pointer_layer_cum   = 0;
                layer_off(LAYER_AUTO_POINTER);
            }
        }
        return false;
#endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE

    case RCTL_T(KC_N):
        /*
        This piece of code nullifies the effect of Right Shift when tapping
        the RCTL_T(KC_N) key.
        This helps rolling over RSFT_T(KC_E) and RCTL_T(KC_N)
        to obtain the intended "en" instead of "N".
        Consequently, capital N can only be obtained by tapping RCTL_T(KC_N)
        and holding LSFT_T(KC_S) (which is the left Shift mod tap).
        */

        /*
        Detect the tap.
        We're only interested in overriding the tap behavior
        in a certain cicumstance. The hold behavior can stay the same.
        */
        if (record->event.pressed && record->tap.count > 0) {
            // Detect right Shift
            if (get_mods() & MOD_BIT(KC_RSFT)) {
                // temporarily disable right Shift
                // so that we can send KC_E and KC_N
                // without Shift on.
                unregister_mods(MOD_BIT(KC_RSFT));
                tap_code(KC_E);
                tap_code(KC_N);
                // restore the mod state
                add_mods(MOD_BIT(KC_RSFT));
                // to prevent QMK from processing RCTL_T(KC_N) as usual in our special case
                return false;
            }
        }
         /*else process RCTL_T(KC_N) as usual.*/
        return true;

    case LCTL_T(KC_T):
        /*
        This piece of code nullifies the effect of Left Shift when
        tapping the LCTL_T(KC_T) key.
        This helps rolling over LSFT_T(KC_S) and LCTL_T(KC_T)
        to obtain the intended "st" instead of "T".
        Consequently, capital T can only be obtained by tapping LCTL_T(KC_T)
        and holding RSFT_T(KC_E) (which is the right Shift mod tap).
        */

        if (record->event.pressed && record->tap.count > 0) {
            if (get_mods() & MOD_BIT(KC_LSFT)) {
                unregister_mods(MOD_BIT(KC_LSFT));
                tap_code(KC_S);
                tap_code(KC_T);
                add_mods(MOD_BIT(KC_LSFT));
                return false;
            }
        }
         /*else process LCTL_T(KC_T) as usual.*/
        return true;
    }
    return true;
};