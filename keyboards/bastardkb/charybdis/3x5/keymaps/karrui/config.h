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
#pragma once

#ifdef VIA_ENABLE
/* VIA configuration. */
// 8 layers: the 7 base layers + the auto mouse layer (LAYER_AUTO_POINTER).  The
// auto layer must be within this count or VIA won't initialize its keycodes.
#    define DYNAMIC_KEYMAP_LAYER_COUNT 8
#endif // VIA_ENABLE

#ifndef __arm__
/* Disable unused features. */
#    define NO_ACTION_ONESHOT
#endif // __arm__

/* Charybdis-specific features. */
#ifdef POINTING_DEVICE_ENABLE
// Automatically enable the pointer layer when moving the trackball.  See also:
// - `CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS`
// - `CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD`
#    define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
#endif // POINTING_DEVICE_ENABLE

#define TAPPING_TERM 175

// Home row mod tuning.  Chordal Hold settles same-hand chords as taps (so
// same-hand rolls like "as"/"en" don't fire mods), while HOLD_ON_OTHER_KEY_PRESS
// settles opposite-hand chords as holds immediately (so cross-hand mods like
// Shift+letter engage even on fast rolls, instead of dropping the mod and
// emitting the tap -- the "The -> nthe" bug).
#define CHORDAL_HOLD
#define HOLD_ON_OTHER_KEY_PRESS
// Flow Tap: during fast typing (a tap-hold key pressed within this many ms of
// the previous typing key), force the tap.  Catches fast cross-hand rolls of
// home-row pairs (to/is/it/an/or) that HOLD_ON_OTHER_KEY_PRESS would otherwise
// misfire as mods.  Default key set includes Space; see is_flow_tap_key() to
// customize (e.g. drop KC_SPC if capitals after a space get eaten).
#define FLOW_TAP_TERM 150