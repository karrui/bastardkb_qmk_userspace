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

#define TAPPING_TERM 250

// Home row mod tuning.  Chordal Hold settles same-hand chords as taps (so
// same-hand rolls like "as"/"en" don't fire mods).  Permissive Hold settles an
// opposite-hand chord as a hold only when the other key is pressed *and released*
// while the mod-tap is still down (a nested press) -- so cross-hand rolls, where
// you release the first key first ("er", "to"), stay taps, while holding Shift
// through a letter (nested press) still capitalizes.  Resolving holds vs. rolls
// by press/release ordering avoids the Flow-Tap-after-space problem that was
// eating capitals (the "The -> nthe" symptom).
#define CHORDAL_HOLD
#define PERMISSIVE_HOLD

// Enable per-key tapping term so get_tapping_term() is consulted.  This takes
// precedence over DYNAMIC_TAPPING_TERM_ENABLE's g_tapping_term, so the override
// returns g_tapping_term in the default case to keep the DT_ keys working.  Used
// to give the pointer-layer key (Z) a much shorter hold -- see get_tapping_term().
#define TAPPING_TERM_PER_KEY