/*
   (c) Copyright 2001-2012  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org>,
              Ville Syrjälä <syrjala@sci.fi> and
              Claudio Ciccani <klan@users.sf.net>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <directfb.h>
#include <directfb_keynames.h>

#include <direct/debug.h>
#include <direct/list.h>
#include <direct/memcpy.h>
#include <direct/messages.h>


#include <fusion/conf.h>
#include <fusion/shmalloc.h>
#include <fusion/reactor.h>

#include <core/CoreInputDevice.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/core_parts.h>

#include <core/gfxcard.h>
#include <core/surface.h>
#include <core/surface_buffer.h>
#include <core/system.h>
#include <core/layer_context.h>
#include <core/layer_control.h>
#include <core/layer_region.h>
#include <core/layers.h>
#include <core/input.h>
#include <core/input_hub.h>
#include <core/windows.h>
#include <core/windows_internal.h>

#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/modules.h>
#include <direct/trace.h>

#include <fusion/build.h>

#include <misc/conf.h>
#include <misc/util.h>

#include <gfx/convert.h>


#define CHECK_INTERVAL 20000  // Microseconds
#define CHECK_NUMBER   200


D_DEBUG_DOMAIN( Core_Input,    "Core/Input",     "DirectFB Input Core" );
D_DEBUG_DOMAIN( Core_InputEvt, "Core/Input/Evt", "DirectFB Input Core Events & Dispatch" );


DEFINE_MODULE_DIRECTORY( dfb_input_modules, "inputdrivers", DFB_INPUT_DRIVER_ABI_VERSION );
//####modified by marvell-bg2
/*
 * One entry in the keymap of an input device.
 */
static const struct dfb_key_map_mrvl{
     int                         code;                  /* hardware
                                                           key code */
     int                         dfb_matching_code;     /* diki code*/
     DFBInputDeviceKeymapEntry   entryvar;              /* KeymapEntry struct var. code, locks, identifier, symbols[4]*/
}dfb_keyboard_key_map_mrvl[] = {
  { 0x00, DIKS_NULL, { 0x00, 0x00, DIKI_UNKNOWN, { DIKS_NULL, DIKS_NULL, DIKS_NULL, DIKS_NULL } } },
  { 0x01, DIKS_BACKSPACE, { 0x01, 0x00, DIKI_BACKSPACE, { DIKS_BACKSPACE, DIKS_BACKSPACE, DIKS_BACKSPACE, DIKS_BACKSPACE } } },
  { 0x02, DIKS_TAB, { 0x02, 0x01, DIKI_TAB, { DIKS_TAB, DIKS_TAB, DIKS_TAB, DIKS_TAB } } },
  { 0x03, DIKS_RETURN, { 0x03, 0x00, DIKI_ENTER, { DIKS_RETURN, DIKS_RETURN, DIKS_RETURN, DIKS_RETURN } } },
  { 0x04, DIKS_CANCEL, { 0x04, 0x00, DIKI_ESCAPE, { DIKS_ESCAPE, DIKS_ESCAPE, DIKS_ESCAPE, DIKS_ESCAPE } } },
  { 0x05, DIKS_ESCAPE, { 0x05, 0x00, DIKI_ESCAPE, { DIKS_ESCAPE, DIKS_ESCAPE, DIKS_ESCAPE, DIKS_ESCAPE } } },
  { 0x06, DIKS_SPACE, { 0x06, 0x00, DIKI_SPACE, { DIKS_SPACE, DIKS_SPACE, DIKS_SPACE, DIKS_SPACE } } },
  { 0x07, DIKS_COMMA, { 0x07, 0x01, DIKI_COMMA, { DIKS_COMMA, DIKS_LESS_THAN_SIGN, DIKS_COMMA, DIKS_COMMA } } },
  { 0x08, DIKS_MINUS_SIGN, { 0x08, 0x01, DIKI_MINUS_SIGN, { DIKS_MINUS_SIGN, DIKS_UNDERSCORE, DIKS_MINUS_SIGN, DIKS_MINUS_SIGN } } },
  { 0x09, DIKS_PERIOD, { 0x09, 0x01, DIKI_PERIOD, { DIKS_PERIOD, DIKS_GREATER_THAN_SIGN, DIKS_PERIOD, DIKS_PERIOD } } },
  { 0x0a, DIKS_SLASH, { 0x0a, 0x01, DIKI_SLASH, { DIKS_SLASH, DIKS_QUESTION_MARK, DIKS_SLASH, DIKS_SLASH } } },
  { 0x0b, DIKS_0, { 0x0b, 0x01, DIKI_0, { DIKS_0, DIKS_PARENTHESIS_RIGHT, DIKS_0, DIKS_0 } } },
  { 0x0c, DIKS_1, { 0x0c, 0x01, DIKI_1, { DIKS_1, DIKS_EXCLAMATION_MARK, DIKS_1, DIKS_1 } } },
  { 0x0d, DIKS_2, { 0x0d, 0x01, DIKI_2, { DIKS_2, DIKS_AT, DIKS_2, DIKS_2 } } },
  { 0x0e, DIKS_3, { 0x0e, 0x01, DIKI_3, { DIKS_3, DIKS_NUMBER_SIGN, DIKS_3, DIKS_3 } } },
  { 0x0f, DIKS_4, { 0x0f, 0x01, DIKI_4, { DIKS_4, DIKS_DOLLAR_SIGN, DIKS_4, DIKS_4 } } },
  { 0x10, DIKS_5, { 0x10, 0x01, DIKI_5, { DIKS_5, DIKS_PERCENT_SIGN, DIKS_5, DIKS_5 } } },
  { 0x11, DIKS_6, { 0x11, 0x01, DIKI_6, { DIKS_6, DIKS_CIRCUMFLEX_ACCENT, DIKS_6, DIKS_6 } } },
  { 0x12, DIKS_7, { 0x12, 0x01, DIKI_7, { DIKS_7, DIKS_AMPERSAND, DIKS_7, DIKS_7 } } },
  { 0x13, DIKS_8, { 0x13, 0x01, DIKI_8, { DIKS_8, DIKS_ASTERISK, DIKS_8, DIKS_8 } } },
  { 0x14, DIKS_9, { 0x14, 0x01, DIKI_9, { DIKS_9, DIKS_PARENTHESIS_LEFT, DIKS_9, DIKS_9 } } },
  { 0x15, DIKS_SEMICOLON, { 0x15, 0x01, DIKI_SEMICOLON, { DIKS_SEMICOLON, DIKS_COLON, DIKS_SEMICOLON, DIKS_SEMICOLON } } },
  { 0x16, DIKS_EQUALS_SIGN, { 0x16, 0x01, DIKI_EQUALS_SIGN, { DIKS_EQUALS_SIGN, DIKS_PLUS_SIGN, DIKS_EQUALS_SIGN, DIKS_EQUALS_SIGN } } },
  { 0x17, DIKS_SQUARE_BRACKET_LEFT, { 0x17, 0x01, DIKI_BRACKET_LEFT, { DIKS_SQUARE_BRACKET_LEFT, DIKS_CURLY_BRACKET_LEFT, DIKS_SQUARE_BRACKET_LEFT, DIKS_SQUARE_BRACKET_LEFT } } },
  { 0x18, DIKS_BACKSLASH, { 0x18, 0x01, DIKI_BACKSLASH, { DIKS_BACKSLASH, DIKS_VERTICAL_BAR, DIKS_BACKSLASH, DIKS_BACKSLASH } } },
  { 0x19, DIKS_SQUARE_BRACKET_RIGHT, { 0x19, 0x01, DIKI_BRACKET_RIGHT, { DIKS_SQUARE_BRACKET_RIGHT, DIKS_CURLY_BRACKET_RIGHT, DIKS_SQUARE_BRACKET_RIGHT, DIKS_SQUARE_BRACKET_RIGHT } } },
  { 0x1a, DIKS_GRAVE_ACCENT, { 0x1a, 0x00, DIKI_QUOTE_LEFT, { DIKS_GRAVE_ACCENT, DIKS_TILDE, DIKS_GRAVE_ACCENT, DIKS_GRAVE_ACCENT } } },
  { 0x1b, DIKS_SMALL_A, { 0x1b, 0x01, DIKI_A, { DIKS_SMALL_A, DIKS_CAPITAL_A, DIKS_SMALL_A, DIKS_SMALL_A } } },
  { 0x1c, DIKS_SMALL_B, { 0x1c, 0x01, DIKI_B, { DIKS_SMALL_B, DIKS_CAPITAL_B, DIKS_SMALL_B, DIKS_SMALL_B } } },
  { 0x1d, DIKS_SMALL_C, { 0x1d, 0x01, DIKI_C, { DIKS_SMALL_C, DIKS_CAPITAL_C, DIKS_SMALL_C, DIKS_SMALL_C } } },
  { 0x1e, DIKS_SMALL_D, { 0x1e, 0x01, DIKI_D, { DIKS_SMALL_D, DIKS_CAPITAL_D, DIKS_SMALL_D, DIKS_SMALL_D } } },
  { 0x1f, DIKS_SMALL_E, { 0x1f, 0x01, DIKI_E, { DIKS_SMALL_E, DIKS_CAPITAL_E, DIKS_SMALL_E, DIKS_SMALL_E } } },
  { 0x20, DIKS_SMALL_F, { 0x20, 0x01, DIKI_F, { DIKS_SMALL_F, DIKS_CAPITAL_F, DIKS_SMALL_F, DIKS_SMALL_F } } },
  { 0x21, DIKS_SMALL_G, { 0x21, 0x01, DIKI_G, { DIKS_SMALL_G, DIKS_CAPITAL_G, DIKS_SMALL_G, DIKS_SMALL_G } } },
  { 0x22, DIKS_SMALL_H, { 0x22, 0x01, DIKI_H, { DIKS_SMALL_H, DIKS_CAPITAL_H, DIKS_SMALL_H, DIKS_SMALL_H } } },
  { 0x23, DIKS_SMALL_I, { 0x23, 0x01, DIKI_I, { DIKS_SMALL_I, DIKS_CAPITAL_I, DIKS_SMALL_I, DIKS_SMALL_I } } },
  { 0x24, DIKS_SMALL_J, { 0x24, 0x01, DIKI_J, { DIKS_SMALL_J, DIKS_CAPITAL_J, DIKS_SMALL_J, DIKS_SMALL_J } } },
  { 0x25, DIKS_SMALL_K, { 0x25, 0x01, DIKI_K, { DIKS_SMALL_K, DIKS_CAPITAL_K, DIKS_SMALL_K, DIKS_SMALL_K } } },
  { 0x26, DIKS_SMALL_L, { 0x26, 0x01, DIKI_L, { DIKS_SMALL_L, DIKS_CAPITAL_L, DIKS_SMALL_L, DIKS_SMALL_L } } },
  { 0x27, DIKS_SMALL_M, { 0x27, 0x01, DIKI_M, { DIKS_SMALL_M, DIKS_CAPITAL_M, DIKS_SMALL_M, DIKS_SMALL_M } } },
  { 0x28, DIKS_SMALL_N, { 0x28, 0x01, DIKI_N, { DIKS_SMALL_N, DIKS_CAPITAL_N, DIKS_SMALL_N, DIKS_SMALL_N } } },
  { 0x29, DIKS_SMALL_O, { 0x29, 0x01, DIKI_O, { DIKS_SMALL_O, DIKS_CAPITAL_O, DIKS_SMALL_O, DIKS_SMALL_O } } },
  { 0x2a, DIKS_SMALL_P, { 0x2a, 0x01, DIKI_P, { DIKS_SMALL_P, DIKS_CAPITAL_P, DIKS_SMALL_P, DIKS_SMALL_P } } },
  { 0x2b, DIKS_SMALL_Q, { 0x2b, 0x01, DIKI_Q, { DIKS_SMALL_Q, DIKS_CAPITAL_Q, DIKS_SMALL_Q, DIKS_SMALL_Q } } },
  { 0x2c, DIKS_SMALL_R, { 0x2c, 0x01, DIKI_R, { DIKS_SMALL_R, DIKS_CAPITAL_R, DIKS_SMALL_R, DIKS_SMALL_R } } },
  { 0x2d, DIKS_SMALL_S, { 0x2d, 0x01, DIKI_S, { DIKS_SMALL_S, DIKS_CAPITAL_S, DIKS_SMALL_S, DIKS_SMALL_S } } },
  { 0x2e, DIKS_SMALL_T, { 0x2e, 0x01, DIKI_T, { DIKS_SMALL_T, DIKS_CAPITAL_T, DIKS_SMALL_T, DIKS_SMALL_T } } },
  { 0x2f, DIKS_SMALL_U, { 0x2f, 0x01, DIKI_U, { DIKS_SMALL_U, DIKS_CAPITAL_U, DIKS_SMALL_U, DIKS_SMALL_U } } },
  { 0x30, DIKS_SMALL_V, { 0x30, 0x01, DIKI_V, { DIKS_SMALL_V, DIKS_CAPITAL_V, DIKS_SMALL_V, DIKS_SMALL_V } } },
  { 0x31, DIKS_SMALL_W, { 0x31, 0x01, DIKI_W, { DIKS_SMALL_W, DIKS_CAPITAL_W, DIKS_SMALL_W, DIKS_SMALL_W } } },
  { 0x32, DIKS_SMALL_X, { 0x32, 0x01, DIKI_X, { DIKS_SMALL_X, DIKS_CAPITAL_X, DIKS_SMALL_X, DIKS_SMALL_X } } },
  { 0x33, DIKS_SMALL_Y, { 0x33, 0x01, DIKI_Y, { DIKS_SMALL_Y, DIKS_CAPITAL_Y, DIKS_SMALL_Y, DIKS_SMALL_Y } } },
  { 0x34, DIKS_SMALL_Z, { 0x34, 0x01, DIKI_Z, { DIKS_SMALL_Z, DIKS_CAPITAL_Z, DIKS_SMALL_Z, DIKS_SMALL_Z } } },
  { 0x35, DIKS_APOSTROPHE, { 0x35, 0x01, DIKI_QUOTE_RIGHT, { DIKS_APOSTROPHE, DIKS_QUOTATION, DIKS_APOSTROPHE, DIKS_APOSTROPHE } } },
  { 0x36, DIKS_DELETE, { 0x36, 0x00, DIKI_DELETE, { DIKS_DELETE, DIKS_DELETE, DIKS_DELETE, DIKS_DELETE } } },
  { 0x37, DIKS_ENTER, { 0x37, 0x00, DIKI_ENTER, { DIKS_RETURN, DIKS_RETURN, DIKS_RETURN, DIKS_RETURN } } },
  { 0x38, DIKS_CURSOR_LEFT, { 0x38, 0x00, DIKI_LEFT, { DIKS_CURSOR_LEFT, DIKS_CURSOR_LEFT, DIKS_CURSOR_LEFT, DIKS_CURSOR_LEFT } } },
  { 0x39, DIKS_CURSOR_RIGHT, { 0x39, 0x00, DIKI_RIGHT, { DIKS_CURSOR_RIGHT, DIKS_CURSOR_RIGHT, DIKS_CURSOR_RIGHT, DIKS_CURSOR_RIGHT } } },
  { 0x3a, DIKS_CURSOR_UP, { 0x3a, 0x00, DIKI_UP, { DIKS_CURSOR_UP, DIKS_CURSOR_UP, DIKS_CURSOR_UP, DIKS_CURSOR_UP } } },
  { 0x3b, DIKS_CURSOR_DOWN, { 0x3b, 0x00, DIKI_DOWN, { DIKS_CURSOR_DOWN, DIKS_CURSOR_DOWN, DIKS_CURSOR_DOWN, DIKS_CURSOR_DOWN } } },
  { 0x3c, DIKS_INSERT, { 0x3c, 0x00, DIKI_INSERT, { DIKS_INSERT, DIKS_INSERT, DIKS_INSERT, DIKS_INSERT } } },
  { 0x3d, DIKS_HOME, { 0x3d, 0x00, DIKI_HOME, { DIKS_HOME, DIKS_HOME, DIKS_HOME, DIKS_HOME } } },
  { 0x3e, DIKS_END, { 0x3e, 0x00, DIKI_END, { DIKS_END, DIKS_END, DIKS_END, DIKS_END } } },
  { 0x3f, DIKS_PAGE_UP, { 0x3f, 0x00, DIKI_PAGE_UP, { DIKS_PAGE_UP, DIKS_PAGE_UP, DIKS_PAGE_UP, DIKS_PAGE_UP } } },
  { 0x40, DIKS_PAGE_DOWN, { 0x40, 0x00, DIKI_PAGE_DOWN, { DIKS_PAGE_DOWN, DIKS_PAGE_DOWN, DIKS_PAGE_DOWN, DIKS_PAGE_DOWN } } },
  { 0x41, DIKS_PRINT, { 0x41, 0x00, DIKI_PRINT, { DIKS_PRINT, DIKS_PRINT, DIKS_PRINT, DIKS_PRINT } } },
  { 0x42, DIKS_PAUSE, { 0x42, 0x00, DIKI_PAUSE, { DIKI_PAUSE, DIKS_BREAK, DIKI_PAUSE, DIKS_BREAK } } },
  { 0x43, DIKS_F1, { 0x43, 0x00, DIKI_F1, { DIKS_F1, DIKS_F1, DIKS_F1, DIKS_F1 } } },
  { 0x44, DIKS_F2, { 0x44, 0x00, DIKI_F2, { DIKS_F2, DIKS_F2, DIKS_F2, DIKS_F2 } } },
  { 0x45, DIKS_F3, { 0x45, 0x00, DIKI_F3, { DIKS_F3, DIKS_F3, DIKS_F3, DIKS_F3 } } },
  { 0x46, DIKS_F4, { 0x46, 0x00, DIKI_F4, { DIKS_F4, DIKS_F4, DIKS_F4, DIKS_F4 } } },
  { 0x47, DIKS_F5, { 0x47, 0x00, DIKI_F5, { DIKS_F5, DIKS_F5, DIKS_F5, DIKS_F5 } } },
  { 0x48, DIKS_F6, { 0x48, 0x00, DIKI_F6, { DIKS_F6, DIKS_F6, DIKS_F6, DIKS_F6 } } },
  { 0x49, DIKS_F7, { 0x49, 0x00, DIKI_F7, { DIKS_F7, DIKS_F7, DIKS_F7, DIKS_F7 } } },
  { 0x4a, DIKS_F8, { 0x4a, 0x00, DIKI_F8, { DIKS_F8, DIKS_F8, DIKS_F8, DIKS_F8 } } },
  { 0x4b, DIKS_F9, { 0x4b, 0x00, DIKI_F9, { DIKS_F9, DIKS_F9, DIKS_F9, DIKS_F9 } } },
  { 0x4c, DIKS_F10, { 0x4c, 0x00, DIKI_F10, { DIKS_F10, DIKS_F10, DIKS_F10, DIKS_F10 } } },
  { 0x4d, DIKS_F11, { 0x4d, 0x00, DIKI_F11, { DIKS_F11, DIKS_F11, DIKS_F11, DIKS_F11 } } },
  { 0x4e, DIKS_F12, { 0x4e, 0x00, DIKI_F12, { DIKS_F12, DIKS_F12, DIKS_F12, DIKS_F12 } } },
  { 0x4f, DIKS_SHIFT, { 0x4f, 0x00, DIKI_SHIFT_L, { DIKS_SHIFT, DIKS_SHIFT, DIKS_SHIFT, DIKS_SHIFT } } },
  { 0x50, DIKS_SHIFT, { 0x50, 0x00, DIKI_SHIFT_R, { DIKS_SHIFT, DIKS_SHIFT, DIKS_SHIFT, DIKS_SHIFT } } },
  { 0x51, DIKS_CONTROL, { 0x51, 0x00, DIKI_CONTROL_L, { DIKS_CONTROL, DIKS_CONTROL, DIKS_CONTROL, DIKS_CONTROL } } },
  { 0x52, DIKS_CONTROL, { 0x52, 0x00, DIKI_CONTROL_R, { DIKS_CONTROL, DIKS_CONTROL, DIKS_CONTROL, DIKS_CONTROL } } },
  { 0x53, DIKS_ALT, { 0x53, 0x00, DIKI_ALT_L, { DIKS_ALT, DIKS_ALT, DIKS_ALT, DIKS_ALT } } },
  { 0x54, DIKS_ALT, { 0x54, 0x00, DIKI_ALT_R, { DIKS_ALT, DIKS_ALT, DIKS_ALT, DIKS_ALT } } },
  { 0x55, DIKS_META, { 0x55, 0x00, DIKI_META_L, { DIKS_META, DIKS_META, DIKS_META, DIKS_META } } },
  { 0x56, DIKS_META, { 0x56, 0x00, DIKI_META_R, { DIKS_META, DIKS_META, DIKS_META, DIKS_META } } },
  { 0x57, DIKS_SUPER, { 0x57, 0x00, DIKI_SUPER_L, { DIKS_SUPER, DIKS_SUPER, DIKS_SUPER, DIKS_SUPER } } },
  { 0x58, DIKS_SUPER, { 0x58, 0x00, DIKI_SUPER_R, { DIKS_SUPER, DIKS_SUPER, DIKS_SUPER, DIKS_SUPER } } },
  { 0x59, DIKS_HYPER, { 0x59, 0x00, DIKI_HYPER_L, { DIKS_HYPER, DIKS_HYPER, DIKS_HYPER, DIKS_HYPER } } },
  { 0x5a, DIKS_HYPER, { 0x5a, 0x00, DIKI_HYPER_R, { DIKS_HYPER, DIKS_HYPER, DIKS_HYPER, DIKS_HYPER } } },
  { 0x5b, DIKS_CAPS_LOCK, { 0x5b, 0x00, DIKI_CAPS_LOCK, { DIKS_CAPS_LOCK, DIKS_CAPS_LOCK, DIKS_CAPS_LOCK, DIKS_CAPS_LOCK } } },
  { 0x5c, DIKS_NUM_LOCK, { 0x5c, 0x00, DIKI_NUM_LOCK, { DIKS_NUM_LOCK, DIKS_NUM_LOCK, DIKS_NUM_LOCK, DIKS_NUM_LOCK } } },
  { 0x5d, DIKS_SCROLL_LOCK, { 0x5d, 0x00, DIKI_SCROLL_LOCK, { DIKS_SCROLL_LOCK, DIKS_SCROLL_LOCK, DIKS_SCROLL_LOCK, DIKS_SCROLL_LOCK } } },
  { 0x5e, DIKS_PLAY, { 0x5e, 0x00, DIKS_PLAY, { DIKS_PLAY, DIKS_PLAY, DIKS_PLAY, DIKS_PLAY } } }, //daljeetk@marvell.com
  { 0x5f, DIKS_SLOW, { 0x5f, 0x00, DIKS_SLOW, { DIKS_SLOW, DIKS_SLOW, DIKS_SLOW, DIKS_SLOW } } }, //daljeetk@marvell.com
  { 0x60, DIKS_STOP, { 0x60, 0x00, DIKS_STOP, { DIKS_STOP, DIKS_STOP, DIKS_STOP, DIKS_STOP } } }, //daljeetk@marvell.com
  { 0x61, DIKS_REWIND, { 0x61, 0x00, DIKS_REWIND, { DIKS_REWIND, DIKS_REWIND, DIKS_REWIND, DIKS_REWIND } } }, //daljeetk@marvell.com
  { 0x62, DIKS_FASTFORWARD, { 0x62, 0x00, DIKS_FASTFORWARD, { DIKS_FASTFORWARD, DIKS_FASTFORWARD, DIKS_FASTFORWARD, DIKS_FASTFORWARD } } }, //daljeetk@marvell.com
  { 0x63, DIKS_PREVIOUS, { 0x63, 0x00, DIKS_PREVIOUS, { DIKS_PREVIOUS, DIKS_PREVIOUS, DIKS_PREVIOUS, DIKS_PREVIOUS } } }, //daljeetk@marvell.com
  { 0x64, DIKS_SCREEN, { 0x64, 0x00, DIKI_PRINT, { DIKS_SCREEN, DIKS_SCREEN, DIKS_SCREEN, DIKS_SCREEN } } }, //daljeetk@marvell.com
  { 0x65, DIKS_EJECT, { 0x65, 0x00, DIKS_EJECT, { DIKS_EJECT, DIKS_EJECT, DIKS_EJECT, DIKS_EJECT } } }, //daljeetk@marvell.com
};

#define ARRAY_N_ELEMENTS(arr)               (sizeof (arr) / sizeof ((arr)[0]))

#define MRVL_NUM_KEY_MAP_KEYS (ARRAY_N_ELEMENTS (dfb_keyboard_key_map_mrvl))
//####end of marvell-bg2

/**********************************************************************************************************************/

typedef enum {
     CIDC_RELOAD_KEYMAP
} CoreInputDeviceCommand;

typedef struct {
     DirectLink               link;

     int                      magic;

     DirectModuleEntry       *module;

     const InputDriverFuncs  *funcs;

     InputDriverInfo          info;

     int                      nr_devices;
} InputDriver;

typedef struct {
     int                          min_keycode;
     int                          max_keycode;
     int                          num_entries;
     DFBInputDeviceKeymapEntry   *entries;
} InputDeviceKeymap;

//####modified by marvell-bg2
/* global keymap ptr to store the keymap exposed by gdk, to
 * be used by other devices of Keyboard type */
InputDeviceKeymap *external_keymap;
//####end of marvell-bg2

typedef struct {
     int                          magic;

     DFBInputDeviceID             id;            /* unique device id */

     int                          num;

     InputDeviceInfo              device_info;

     InputDeviceKeymap            keymap;
	 //####modified by marvell-bg2
     bool 			  external_keymap_loaded;    /* true if an external keymap is loaded for this device*/
	 //####end of marvell-bg2

     CoreInputDeviceState         state;

     DFBInputDeviceKeyIdentifier  last_key;      /* last key pressed */
     DFBInputDeviceKeySymbol      last_symbol;   /* last symbol pressed */
     bool                         first_press;   /* first press of key */

     FusionReactor               *reactor;       /* event dispatcher */
     FusionSkirmish               lock;

     unsigned int                 axis_num;
     DFBInputDeviceAxisInfo      *axis_info;
     FusionRef                    ref; /* Ref between shared device & local device */

     FusionCall                   call;
} InputDeviceShared;

struct __DFB_CoreInputDevice {
     DirectLink          link;

     int                 magic;

     InputDeviceShared  *shared;

     InputDriver        *driver;
     void               *driver_data;

     CoreDFB            *core;
};

/**********************************************************************************************************************/

DirectResult
CoreInputDevice_Call( CoreInputDevice     *device,
                      FusionCallExecFlags  flags,
                      int                  call_arg,
                      void                *ptr,
                      unsigned int         length,
                      void                *ret_ptr,
                      unsigned int         ret_size,
                      unsigned int        *ret_length )
{
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     return fusion_call_execute3( &device->shared->call, flags, call_arg, ptr, length, ret_ptr, ret_size, ret_length );
}

/**********************************************************************************************************************/

typedef struct {
     int                 magic;

     int                 num;
     InputDeviceShared  *devices[MAX_INPUTDEVICES];
     FusionReactor      *reactor; /* For input hot-plug event */
} DFBInputCoreShared;

struct __DFB_DFBInputCore {
     int                 magic;

     CoreDFB            *core;

     DFBInputCoreShared *shared;

     DirectLink         *drivers;
     DirectLink         *devices;

     CoreInputHub       *hub;
};


DFB_CORE_PART( input_core, InputCore );

/**********************************************************************************************************************/

typedef struct {
     DFBInputDeviceKeySymbol      target;
     DFBInputDeviceKeySymbol      result;
} DeadKeyCombo;

typedef struct {
     DFBInputDeviceKeySymbol      deadkey;
     const DeadKeyCombo          *combos;
} DeadKeyMap;

/* Data struct of input device hotplug event */
typedef struct {
     bool             is_plugin;   /* Hotplug in or not */
     int              dev_id;      /* Input device ID*/
     struct timeval   stamp;       /* Time stamp of event */
} InputDeviceHotplugEvent;

/**********************************************************************************************************************/

static const DeadKeyCombo combos_grave[] = {         
     { DIKS_SPACE,     (unsigned char) '`' },        
     { DIKS_SMALL_A,   (unsigned char) '�' },        
     { DIKS_SMALL_E,   (unsigned char) '�' },        
     { DIKS_SMALL_I,   (unsigned char) '�' },        
     { DIKS_SMALL_O,   (unsigned char) '�' },        
     { DIKS_SMALL_U,   (unsigned char) '�' },        
     { DIKS_CAPITAL_A, (unsigned char) '�' },        
     { DIKS_CAPITAL_E, (unsigned char) '�' },        
     { DIKS_CAPITAL_I, (unsigned char) '�' },        
     { DIKS_CAPITAL_O, (unsigned char) '�' },        
     { DIKS_CAPITAL_U, (unsigned char) '�' },        
     { 0, 0 }                                        
};                                                   
                                                     
static const DeadKeyCombo combos_acute[] = {         
     { DIKS_SPACE,     (unsigned char) '\'' },       
     { DIKS_SMALL_A,   (unsigned char) '�' },        
     { DIKS_SMALL_E,   (unsigned char) '�' },        
     { DIKS_SMALL_I,   (unsigned char) '�' },        
     { DIKS_SMALL_O,   (unsigned char) '�' },        
     { DIKS_SMALL_U,   (unsigned char) '�' },        
     { DIKS_SMALL_Y,   (unsigned char) '�' },        
     { DIKS_CAPITAL_A, (unsigned char) '�' },        
     { DIKS_CAPITAL_E, (unsigned char) '�' },        
     { DIKS_CAPITAL_I, (unsigned char) '�' },        
     { DIKS_CAPITAL_O, (unsigned char) '�' },        
     { DIKS_CAPITAL_U, (unsigned char) '�' },        
     { DIKS_CAPITAL_Y, (unsigned char) '�' },        
     { 0, 0 }                                        
};                                                   
                                                     
static const DeadKeyCombo combos_circumflex[] = {    
     { DIKS_SPACE,     (unsigned char) '^' },        
     { DIKS_SMALL_A,   (unsigned char) '�' },        
     { DIKS_SMALL_E,   (unsigned char) '�' },        
     { DIKS_SMALL_I,   (unsigned char) '�' },        
     { DIKS_SMALL_O,   (unsigned char) '�' },        
     { DIKS_SMALL_U,   (unsigned char) '�' },        
     { DIKS_CAPITAL_A, (unsigned char) '�' },        
     { DIKS_CAPITAL_E, (unsigned char) '�' },        
     { DIKS_CAPITAL_I, (unsigned char) '�' },        
     { DIKS_CAPITAL_O, (unsigned char) '�' },        
     { DIKS_CAPITAL_U, (unsigned char) '�' },        
     { 0, 0 }                                        
};                                                   
                                                     
static const DeadKeyCombo combos_diaeresis[] = {     
     { DIKS_SPACE,     (unsigned char) '�' },        
     { DIKS_SMALL_A,   (unsigned char) '�' },        
     { DIKS_SMALL_E,   (unsigned char) '�' },        
     { DIKS_SMALL_I,   (unsigned char) '�' },        
     { DIKS_SMALL_O,   (unsigned char) '�' },        
     { DIKS_SMALL_U,   (unsigned char) '�' },        
     { DIKS_CAPITAL_A, (unsigned char) '�' },        
     { DIKS_CAPITAL_E, (unsigned char) '�' },        
     { DIKS_CAPITAL_I, (unsigned char) '�' },        
     { DIKS_CAPITAL_O, (unsigned char) '�' },        
     { DIKS_CAPITAL_U, (unsigned char) '�' },        
     { 0, 0 }                                        
};                                                   
                                                     
static const DeadKeyCombo combos_tilde[] = {         
     { DIKS_SPACE,     (unsigned char) '~' },        
     { DIKS_SMALL_A,   (unsigned char) '�' },        
     { DIKS_SMALL_N,   (unsigned char) '�' },        
     { DIKS_SMALL_O,   (unsigned char) '�' },        
     { DIKS_CAPITAL_A, (unsigned char) '�' },        
     { DIKS_CAPITAL_N, (unsigned char) '�' },        
     { DIKS_CAPITAL_O, (unsigned char) '�' },        
     { 0, 0 }                                        
};

static const DeadKeyMap deadkey_maps[] = {           
     { DIKS_DEAD_GRAVE,      combos_grave },         
     { DIKS_DEAD_ACUTE,      combos_acute },         
     { DIKS_DEAD_CIRCUMFLEX, combos_circumflex },    
     { DIKS_DEAD_DIAERESIS,  combos_diaeresis },     
     { DIKS_DEAD_TILDE,      combos_tilde }          
};

/* define a lookup table to go from key IDs to names.
 * This is used to look up the names provided in a loaded key table */
/* this table is roughly 4Kb in size */
DirectFBKeySymbolNames(KeySymbolNames);
DirectFBKeyIdentifierNames(KeyIdentifierNames);

/**********************************************************************************************************************/

static void init_devices( CoreDFB *core );

static void allocate_device_keymap( CoreDFB *core, CoreInputDevice *device );

static DFBInputDeviceKeymapEntry *get_keymap_entry( CoreInputDevice *device,
                                                    int              code );

static DFBResult set_keymap_entry( CoreInputDevice                 *device,
                                   int                              code,
                                   const DFBInputDeviceKeymapEntry *entry );

static DFBResult load_keymap( CoreInputDevice           *device,
                              char                      *filename );

static DFBResult reload_keymap( CoreInputDevice *device );

static DFBInputDeviceKeySymbol     lookup_keysymbol( char *symbolname );
static DFBInputDeviceKeyIdentifier lookup_keyidentifier( char *identifiername );

/**********************************************************************************************************************/

static bool lookup_from_table( CoreInputDevice    *device,
                               DFBInputEvent      *event,
                               DFBInputEventFlags  lookup );

static void fixup_key_event  ( CoreInputDevice    *device,
                               DFBInputEvent      *event );

static void fixup_mouse_event( CoreInputDevice    *device,
                               DFBInputEvent      *event );

static void flush_keys       ( CoreInputDevice    *device );

static bool core_input_filter( CoreInputDevice    *device,
                               DFBInputEvent      *event );

/**********************************************************************************************************************/

static DFBInputDeviceKeyIdentifier symbol_to_id( DFBInputDeviceKeySymbol     symbol );

static DFBInputDeviceKeySymbol     id_to_symbol( DFBInputDeviceKeyIdentifier id,
                                                 DFBInputDeviceModifierMask  modifiers,
                                                 DFBInputDeviceLockState     locks );

/**********************************************************************************************************************/

static ReactionResult local_processing_hotplug( const void *msg_data, void *ctx );

/**********************************************************************************************************************/

static ReactionFunc dfb_input_globals[MAX_INPUT_GLOBALS+1] = {
/* 0 */   _dfb_windowstack_inputdevice_listener,
          NULL
};

DFBResult
dfb_input_add_global( ReactionFunc  func,
                      int          *ret_index )
{
     int i;

     D_DEBUG_AT( Core_Input, "%s( %p, %p )\n", __FUNCTION__, func, ret_index );

     D_ASSERT( func != NULL );
     D_ASSERT( ret_index != NULL );

     for (i=0; i<MAX_INPUT_GLOBALS; i++) {
          if (!dfb_input_globals[i]) {
               dfb_input_globals[i] = func;

               D_DEBUG_AT( Core_Input, "  -> index %d\n", i );

               *ret_index = i;

               return DFB_OK;
          }
     }

     return DFB_LIMITEXCEEDED;
}

DFBResult
dfb_input_set_global( ReactionFunc func,
                      int          index )
{
     D_DEBUG_AT( Core_Input, "%s( %p, %d )\n", __FUNCTION__, func, index );

     D_ASSERT( func != NULL );
     D_ASSERT( index >= 0 );
     D_ASSERT( index < MAX_INPUT_GLOBALS );

     D_ASSUME( dfb_input_globals[index] == NULL );

     dfb_input_globals[index] = func;

     return DFB_OK;
}

/**********************************************************************************************************************/

static DFBInputCore       *core_local; /* FIXME */
static DFBInputCoreShared *core_input; /* FIXME */

#if FUSION_BUILD_MULTI
static Reaction            local_processing_react; /* Local reaction to hot-plug event */
#endif


static DFBResult
dfb_input_core_initialize( CoreDFB            *core,
                           DFBInputCore       *data,
                           DFBInputCoreShared *shared )
{
#if FUSION_BUILD_MULTI
     DFBResult result = DFB_OK;
#endif

     D_DEBUG_AT( Core_Input, "dfb_input_core_initialize( %p, %p, %p )\n", core, data, shared );

     D_ASSERT( data != NULL );
     D_ASSERT( shared != NULL );

     core_local = data;   /* FIXME */
     core_input = shared; /* FIXME */

     data->core   = core;
     data->shared = shared;


     direct_modules_explore_directory( &dfb_input_modules );

#if FUSION_BUILD_MULTI
     /* Create the reactor that responds input device hot-plug events. */
     core_input->reactor = fusion_reactor_new(
                              sizeof( InputDeviceHotplugEvent ),
                              "Input Hotplug",
                              dfb_core_world(core) );
     if (!core_input->reactor) {
          D_ERROR( "DirectFB/Input: fusion_reactor_new() failed!\n" );
          result = DFB_FAILURE;
          goto errorExit;
     }

     fusion_reactor_add_permissions( core_input->reactor, 0, FUSION_REACTOR_PERMIT_ATTACH_DETACH );

     /* Attach local process function to the input hot-plug reactor. */
     result = fusion_reactor_attach(
                              core_input->reactor,
                              local_processing_hotplug,
                              (void*) core,
                              &local_processing_react );
     if (result) {
          D_ERROR( "DirectFB/Input: fusion_reactor_attach() failed!\n" );
          goto errorExit;
     }
#endif

     if (dfb_config->input_hub_service_qid)
          CoreInputHub_Create( dfb_config->input_hub_service_qid, &core_local->hub );

     init_devices( core );


     D_MAGIC_SET( data, DFBInputCore );
     D_MAGIC_SET( shared, DFBInputCoreShared );

     return DFB_OK;

#if FUSION_BUILD_MULTI
errorExit:
     /* Destroy the hot-plug reactor if it was created. */
     if (core_input->reactor)
          fusion_reactor_destroy(core_input->reactor);

     return result;
#endif
}

static DFBResult
dfb_input_core_join( CoreDFB            *core,
                     DFBInputCore       *data,
                     DFBInputCoreShared *shared )
{
     int i;
#if FUSION_BUILD_MULTI
     DFBResult result;
#endif

     D_DEBUG_AT( Core_Input, "dfb_input_core_join( %p, %p, %p )\n", core, data, shared );

     D_ASSERT( data != NULL );
     D_MAGIC_ASSERT( shared, DFBInputCoreShared );
     D_ASSERT( shared->reactor != NULL );

     core_local = data;   /* FIXME */
     core_input = shared; /* FIXME */

     data->core   = core;
     data->shared = shared;

#if FUSION_BUILD_MULTI
     /* Attach the local process function to the input hot-plug reactor. */
     result = fusion_reactor_attach( core_input->reactor,
                                     local_processing_hotplug,
                                     (void*) core,
                                     &local_processing_react );
     if (result) {
          D_ERROR( "DirectFB/Input: fusion_reactor_attach failed!\n" );
          return result;
     }
#endif

     for (i=0; i<core_input->num; i++) {
          CoreInputDevice *device;

          device = D_CALLOC( 1, sizeof(CoreInputDevice) );
          if (!device) {
               D_OOM();
               continue;
          }

          device->shared = core_input->devices[i];

#if FUSION_BUILD_MULTI
          /* Increase the reference counter. */
          fusion_ref_up( &device->shared->ref, false );
#endif

          /* add it to the list */
          direct_list_append( &data->devices, &device->link );

          D_MAGIC_SET( device, CoreInputDevice );
     }


     D_MAGIC_SET( data, DFBInputCore );

     return DFB_OK;
}

static DFBResult
dfb_input_core_shutdown( DFBInputCore *data,
                         bool          emergency )
{
     DFBInputCoreShared  *shared;
     DirectLink          *n;
     CoreInputDevice     *device;
     FusionSHMPoolShared *pool = dfb_core_shmpool( data->core );
     InputDriver         *driver;

     D_DEBUG_AT( Core_Input, "dfb_input_core_shutdown( %p, %semergency )\n", data, emergency ? "" : "no " );

     D_MAGIC_ASSERT( data, DFBInputCore );
     D_MAGIC_ASSERT( data->shared, DFBInputCoreShared );

     shared = data->shared;

     /* Stop each input provider's hot-plug thread that supports device hot-plugging. */
     direct_list_foreach_safe (driver, n, core_local->drivers) {
          if (driver->funcs->GetCapability && driver->funcs->StopHotplug) {
               if (IDC_HOTPLUG & driver->funcs->GetCapability()) {
                    D_DEBUG_AT( Core_Input, "Stopping hot-plug detection thread "
                                "within %s\n ", driver->module->name );
                    if (driver->funcs->StopHotplug()) {
                         D_ERROR( "DirectFB/Input: StopHotplug() failed with %s\n",
                                  driver->module->name );
                    }
               }
          }
     }

#if FUSION_BUILD_MULTI
     fusion_reactor_detach( core_input->reactor, &local_processing_react );
     fusion_reactor_destroy( core_input->reactor );
#endif

     direct_list_foreach_safe (device, n, data->devices) {
          InputDeviceShared *devshared;

          D_MAGIC_ASSERT( device, CoreInputDevice );

          driver = device->driver;
          D_ASSERT( driver != NULL );

          devshared = device->shared;
          D_ASSERT( devshared != NULL );

          CoreInputDevice_Deinit_Dispatch( &devshared->call );

          fusion_skirmish_destroy( &devshared->lock );

          if (device->driver_data != NULL) {
               void *driver_data;

               D_ASSERT( driver->funcs != NULL );
               D_ASSERT( driver->funcs->CloseDevice != NULL );

               D_DEBUG_AT( Core_Input, "  -> closing '%s' (%d) %d.%d (%s)\n",
                           devshared->device_info.desc.name, devshared->num + 1,
                           driver->info.version.major,
                           driver->info.version.minor, driver->info.vendor );

               driver_data = device->driver_data;
               device->driver_data = NULL;
               driver->funcs->CloseDevice( driver_data );

               if (data->hub)
                    CoreInputHub_RemoveDevice( data->hub, device->shared->id );
          }

          if (!--driver->nr_devices) {
               direct_module_unref( driver->module );
               D_FREE( driver );
          }

#if FUSION_BUILD_MULTI
          fusion_ref_destroy( &device->shared->ref );
#endif

          fusion_reactor_free( devshared->reactor );

          if (devshared->keymap.entries)
               SHFREE( pool, devshared->keymap.entries );
  
          if (devshared->axis_info)
               SHFREE( pool, devshared->axis_info );

          SHFREE( pool, devshared );

          D_MAGIC_CLEAR( device );

          D_FREE( device );
     }

     if (data->hub)
          CoreInputHub_Destroy( data->hub );

     D_MAGIC_CLEAR( data );
     D_MAGIC_CLEAR( shared );

     return DFB_OK;
}

static DFBResult
dfb_input_core_leave( DFBInputCore *data,
                      bool          emergency )
{
     DirectLink      *n;
     CoreInputDevice *device;

     D_DEBUG_AT( Core_Input, "dfb_input_core_leave( %p, %semergency )\n", data, emergency ? "" : "no " );

     D_MAGIC_ASSERT( data, DFBInputCore );
     D_MAGIC_ASSERT( data->shared, DFBInputCoreShared );

#if FUSION_BUILD_MULTI
     fusion_reactor_detach( core_input->reactor, &local_processing_react );
#endif

     direct_list_foreach_safe (device, n, data->devices) {
          D_MAGIC_ASSERT( device, CoreInputDevice );

#if FUSION_BUILD_MULTI
          /* Decrease the ref between shared device and local device. */
          fusion_ref_down( &device->shared->ref, false );
#endif

          D_FREE( device );
     }


     D_MAGIC_CLEAR( data );

     return DFB_OK;
}

static DFBResult
dfb_input_core_suspend( DFBInputCore *data )
{
     CoreInputDevice *device;
     InputDriver     *driver;

     D_DEBUG_AT( Core_Input, "dfb_input_core_suspend( %p )\n", data );

     D_MAGIC_ASSERT( data, DFBInputCore );
     D_MAGIC_ASSERT( data->shared, DFBInputCoreShared );

     D_DEBUG_AT( Core_Input, "  -> suspending...\n" );

     /* Go through the drivers list and attempt to suspend all of the drivers that
      * support the Suspend function.
      */
     direct_list_foreach (driver, core_local->drivers) {
          DFBResult ret;

          D_ASSERT( driver->funcs->Suspend != NULL );
          ret = driver->funcs->Suspend();

          if (ret != DFB_OK && ret != DFB_UNSUPPORTED) {
               D_DERROR( ret, "driver->Suspend failed during suspend (%s)\n",
                         driver->info.name );
          }
     }

     direct_list_foreach (device, data->devices) {
          InputDeviceShared *devshared;

          (void)devshared;
          
          D_MAGIC_ASSERT( device, CoreInputDevice );
          
          driver = device->driver;
          D_ASSERT( driver != NULL );
          
          devshared = device->shared;
          D_ASSERT( devshared != NULL );

          if (device->driver_data != NULL) {
               void *driver_data;

               D_ASSERT( driver->funcs != NULL );
               D_ASSERT( driver->funcs->CloseDevice != NULL );

               D_DEBUG_AT( Core_Input, "  -> closing '%s' (%d) %d.%d (%s)\n",
                           devshared->device_info.desc.name, devshared->num + 1,
                           driver->info.version.major,
                           driver->info.version.minor, driver->info.vendor );

               driver_data = device->driver_data;
               device->driver_data = NULL;
               driver->funcs->CloseDevice( driver_data );
          }

          flush_keys( device );
     }

     D_DEBUG_AT( Core_Input, "  -> suspended.\n" );

     return DFB_OK;
}

static DFBResult
dfb_input_core_resume( DFBInputCore *data )
{
     DFBResult        ret;
     CoreInputDevice *device;
     InputDriver     *driver;

     D_DEBUG_AT( Core_Input, "dfb_input_core_resume( %p )\n", data );

     D_MAGIC_ASSERT( data, DFBInputCore );
     D_MAGIC_ASSERT( data->shared, DFBInputCoreShared );

     D_DEBUG_AT( Core_Input, "  -> resuming...\n" );

     direct_list_foreach (device, data->devices) {
          D_MAGIC_ASSERT( device, CoreInputDevice );

          D_DEBUG_AT( Core_Input, "  -> reopening '%s' (%d) %d.%d (%s)\n",
                      device->shared->device_info.desc.name, device->shared->num + 1,
                      device->driver->info.version.major,
                      device->driver->info.version.minor,
                      device->driver->info.vendor );

          D_ASSERT( device->driver_data == NULL );

          ret = device->driver->funcs->OpenDevice( device, device->shared->num,
                                                   &device->shared->device_info,
                                                   &device->driver_data );
          if (ret) {
               D_DERROR( ret, "DirectFB/Input: Failed reopening device "
                         "during resume (%s)!\n", device->shared->device_info.desc.name );
               device->driver_data = NULL;
          }
     }

     /* Go through the drivers list and attempt to resume all of the drivers that
      * support the Resume function.
      */
     direct_list_foreach (driver, core_local->drivers) {
          D_ASSERT( driver->funcs->Resume != NULL );

          ret = driver->funcs->Resume();
          if (ret != DFB_OK && ret != DFB_UNSUPPORTED) {
               D_DERROR( ret, "driver->Resume failed during resume (%s)\n",
                         driver->info.name );
          }
     }

     D_DEBUG_AT( Core_Input, "  -> resumed.\n" );

     return DFB_OK;
}

void
dfb_input_enumerate_devices( InputDeviceCallback         callback,
                             void                       *ctx,
                             DFBInputDeviceCapabilities  caps )
{
     CoreInputDevice *device;

     D_ASSERT( core_input != NULL );

     direct_list_foreach (device, core_local->devices) {
          DFBInputDeviceCapabilities dev_caps;

          D_MAGIC_ASSERT( device, CoreInputDevice );
          D_ASSERT( device->shared != NULL );

          dev_caps = device->shared->device_info.desc.caps;

          /* Always match if unclassified */
          if (!dev_caps)
#ifndef DIRECTFB_DISABLE_DEPRECATED
               dev_caps = DICAPS_ALL;
#else
               dev_caps = DIDCAPS_ALL;
#endif

          if ((dev_caps & caps) && callback( device, ctx ) == DFENUM_CANCEL)
               break;
     }
}

DirectResult
dfb_input_attach( CoreInputDevice *device,
                  ReactionFunc     func,
                  void            *ctx,
                  Reaction        *reaction )
{
     D_DEBUG_AT( Core_Input, "%s( %p, %p, %p, %p )\n", __FUNCTION__, device, func, ctx, reaction );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     return fusion_reactor_attach( device->shared->reactor, func, ctx, reaction );
}

DirectResult
dfb_input_detach( CoreInputDevice *device,
                  Reaction        *reaction )
{
     D_DEBUG_AT( Core_Input, "%s( %p, %p )\n", __FUNCTION__, device, reaction );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     return fusion_reactor_detach( device->shared->reactor, reaction );
}

DirectResult
dfb_input_attach_global( CoreInputDevice *device,
                         int              index,
                         void            *ctx,
                         GlobalReaction  *reaction )
{
     D_DEBUG_AT( Core_Input, "%s( %p, %d, %p, %p )\n", __FUNCTION__, device, index, ctx, reaction );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     return fusion_reactor_attach_global( device->shared->reactor, index, ctx, reaction );
}

DirectResult
dfb_input_detach_global( CoreInputDevice *device,
                         GlobalReaction  *reaction )
{
     D_DEBUG_AT( Core_Input, "%s( %p, %p )\n", __FUNCTION__, device, reaction );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     return fusion_reactor_detach_global( device->shared->reactor, reaction );
}

const char *
dfb_input_event_type_name( DFBInputEventType type )
{
     switch (type) {
          case DIET_UNKNOWN:
               return "UNKNOWN";

          case DIET_KEYPRESS:
               return "KEYPRESS";

          case DIET_KEYRELEASE:
               return "KEYRELEASE";

          case DIET_BUTTONPRESS:
               return "BUTTONPRESS";

          case DIET_BUTTONRELEASE:
               return "BUTTONRELEASE";

          case DIET_AXISMOTION:
               return "AXISMOTION";

          default:
               break;
     }

     return "<invalid>";
}

void
dfb_input_dispatch( CoreInputDevice *device, DFBInputEvent *event )
{
     D_DEBUG_AT( Core_Input, "%s( %p, %p )\n", __FUNCTION__, device, event );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( event != NULL );

     /*
      * When a USB device is hot-removed, it is possible that there are pending events
      * still being dispatched and the shared field becomes NULL.
      */

     /*
      * 0. Sanity checks & debugging...
      */
     if (!device->shared) {
          D_DEBUG_AT( Core_Input, "  -> No shared data!\n" );
          return;
     }

     D_ASSUME( device->shared->reactor != NULL );

     if (!device->shared->reactor) {
          D_DEBUG_AT( Core_Input, "  -> No reactor!\n" );
          return;
     }

     D_DEBUG_AT( Core_InputEvt, "  -> (%02x) %s%s%s\n", event->type,
                 dfb_input_event_type_name( event->type ),
                 (event->flags & DIEF_FOLLOW) ? " [FOLLOW]" : "",
                 (event->flags & DIEF_REPEAT) ? " [REPEAT]" : "" );

#if D_DEBUG_ENABLED
     if (event->flags & DIEF_TIMESTAMP)
          D_DEBUG_AT( Core_InputEvt, "  -> TIMESTAMP  %lu.%06lu\n", event->timestamp.tv_sec, event->timestamp.tv_usec );
     if (event->flags & DIEF_AXISABS)
          D_DEBUG_AT( Core_InputEvt, "  -> AXISABS    %d at %d\n",  event->axis, event->axisabs );
     if (event->flags & DIEF_AXISREL)
          D_DEBUG_AT( Core_InputEvt, "  -> AXISREL    %d by %d\n",  event->axis, event->axisrel );
     if (event->flags & DIEF_KEYCODE)
          D_DEBUG_AT( Core_InputEvt, "  -> KEYCODE    %d\n",        event->key_code );
     if (event->flags & DIEF_KEYID)
          D_DEBUG_AT( Core_InputEvt, "  -> KEYID      0x%04x\n",    event->key_id );
     if (event->flags & DIEF_KEYSYMBOL)
          D_DEBUG_AT( Core_InputEvt, "  -> KEYSYMBOL  0x%04x\n",    event->key_symbol );
     if (event->flags & DIEF_MODIFIERS)
          D_DEBUG_AT( Core_InputEvt, "  -> MODIFIERS  0x%04x\n",    event->modifiers );
     if (event->flags & DIEF_LOCKS)
          D_DEBUG_AT( Core_InputEvt, "  -> LOCKS      0x%04x\n",    event->locks );
     if (event->flags & DIEF_BUTTONS)
          D_DEBUG_AT( Core_InputEvt, "  -> BUTTONS    0x%04x\n",    event->buttons );
     if (event->flags & DIEF_GLOBAL)
          D_DEBUG_AT( Core_InputEvt, "  -> GLOBAL\n" );
#endif

     /*
      * 1. Fixup event...
      */
     event->clazz     = DFEC_INPUT;
     event->device_id = device->shared->id;

     if (!(event->flags & DIEF_TIMESTAMP)) {
          gettimeofday( &event->timestamp, NULL );
          event->flags |= DIEF_TIMESTAMP;
     }

     switch (event->type) {
          case DIET_BUTTONPRESS:
          case DIET_BUTTONRELEASE:
               D_DEBUG_AT( Core_InputEvt, "  -> BUTTON     0x%04x\n", event->button );

               if (dfb_config->lefty) {
                    if (event->button == DIBI_LEFT)
                         event->button = DIBI_RIGHT;
                    else if (event->button == DIBI_RIGHT)
                         event->button = DIBI_LEFT;

                    D_DEBUG_AT( Core_InputEvt, "  -> lefty!  => 0x%04x <=\n", event->button );
               }
               /* fallthru */

          case DIET_AXISMOTION:
               fixup_mouse_event( device, event );
               break;

          case DIET_KEYPRESS:
          case DIET_KEYRELEASE:
               if (dfb_config->capslock_meta) {
                    if (device->shared->keymap.num_entries && (event->flags & DIEF_KEYCODE))
                         lookup_from_table( device, event, (DIEF_KEYID |
                                                            DIEF_KEYSYMBOL) & ~event->flags );

                    if (event->key_id == DIKI_CAPS_LOCK || event->key_symbol == DIKS_CAPS_LOCK) {
                         event->flags     |= DIEF_KEYID | DIEF_KEYSYMBOL;
                         event->key_code   = -1;
                         event->key_id     = DIKI_META_L;
                         event->key_symbol = DIKS_META;
                    }
               }

               fixup_key_event( device, event );
               break;

          default:
               ;
     }

#if D_DEBUG_ENABLED
     if (event->flags & DIEF_TIMESTAMP)
          D_DEBUG_AT( Core_InputEvt, "  => TIMESTAMP  %lu.%06lu\n", event->timestamp.tv_sec, event->timestamp.tv_usec );
     if (event->flags & DIEF_AXISABS)
          D_DEBUG_AT( Core_InputEvt, "  => AXISABS    %d at %d\n",  event->axis, event->axisabs );
     if (event->flags & DIEF_AXISREL)
          D_DEBUG_AT( Core_InputEvt, "  => AXISREL    %d by %d\n",  event->axis, event->axisrel );
     if (event->flags & DIEF_KEYCODE)
          D_DEBUG_AT( Core_InputEvt, "  => KEYCODE    %d\n",        event->key_code );
     if (event->flags & DIEF_KEYID)
          D_DEBUG_AT( Core_InputEvt, "  => KEYID      0x%04x\n",    event->key_id );
     if (event->flags & DIEF_KEYSYMBOL)
          D_DEBUG_AT( Core_InputEvt, "  => KEYSYMBOL  0x%04x\n",    event->key_symbol );
     if (event->flags & DIEF_MODIFIERS)
          D_DEBUG_AT( Core_InputEvt, "  => MODIFIERS  0x%04x\n",    event->modifiers );
     if (event->flags & DIEF_LOCKS)
          D_DEBUG_AT( Core_InputEvt, "  => LOCKS      0x%04x\n",    event->locks );
     if (event->flags & DIEF_BUTTONS)
          D_DEBUG_AT( Core_InputEvt, "  => BUTTONS    0x%04x\n",    event->buttons );
     if (event->flags & DIEF_GLOBAL)
          D_DEBUG_AT( Core_InputEvt, "  => GLOBAL\n" );
#endif

     if (core_local->hub)
          CoreInputHub_DispatchEvent( core_local->hub, device->shared->id, event );

     if (core_input_filter( device, event ))
          D_DEBUG_AT( Core_InputEvt, "  ****>> FILTERED\n" );
     else
          fusion_reactor_dispatch( device->shared->reactor, event, true, dfb_input_globals );
}

DFBInputDeviceID
dfb_input_device_id( const CoreInputDevice *device )
{
     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     return device->shared->id;
}

CoreInputDevice *
dfb_input_device_at( DFBInputDeviceID id )
{
     CoreInputDevice *device;

     D_ASSERT( core_input != NULL );

     direct_list_foreach (device, core_local->devices) {
          D_MAGIC_ASSERT( device, CoreInputDevice );

          if (device->shared->id == id)
               return device;
     }

     return NULL;
}

/* Get an input device's capabilities. */
DFBInputDeviceCapabilities
dfb_input_device_caps( const CoreInputDevice *device )
{
     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     return device->shared->device_info.desc.caps;
}

void
dfb_input_device_description( const CoreInputDevice     *device,
                              DFBInputDeviceDescription *desc )
{
     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     *desc = device->shared->device_info.desc;
}

DFBResult
dfb_input_device_get_keymap_entry( CoreInputDevice           *device,
                                   int                        keycode,
                                   DFBInputDeviceKeymapEntry *entry )
{
     DFBInputDeviceKeymapEntry *keymap_entry;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( entry != NULL );

     keymap_entry = get_keymap_entry( device, keycode );
     if (!keymap_entry)
          return DFB_FAILURE;

     *entry = *keymap_entry;

     return DFB_OK;
}

DFBResult
dfb_input_device_set_keymap_entry( CoreInputDevice                 *device,
                                   int                              keycode,
                                   const DFBInputDeviceKeymapEntry *entry )
{
     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( entry != NULL );

     return set_keymap_entry( device, keycode, entry );
}

DFBResult
dfb_input_device_load_keymap   ( CoreInputDevice           *device,
                                 char                      *filename )
{
     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( filename != NULL );

     return load_keymap( device, filename );
}

DFBResult
dfb_input_device_reload_keymap( CoreInputDevice *device )
{
     InputDeviceShared *shared;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     shared = device->shared;
     D_ASSERT( shared != NULL );

     D_INFO( "DirectFB/Input: Reloading keymap for '%s' [0x%02x]...\n",
             shared->device_info.desc.name, shared->id );

     return reload_keymap( device );
}

DFBResult
dfb_input_device_get_state( CoreInputDevice      *device,
                            CoreInputDeviceState *ret_state )
{
     InputDeviceShared *shared;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     shared = device->shared;
     D_ASSERT( shared != NULL );

     *ret_state = shared->state;

     return DFB_OK;
}

DFBResult
dfb_input_device_set_configuration( CoreInputDevice            *device,
                                    const DFBInputDeviceConfig *config )
{
     InputDriver *driver;

     D_DEBUG_AT( Core_Input, "%s( %p, %p )\n", __FUNCTION__, device, config );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     driver = device->driver;
     D_ASSERT( driver != NULL );

     if (!driver->funcs->SetConfiguration)
          return DFB_UNSUPPORTED;

     return driver->funcs->SetConfiguration( device, device->driver_data, config );
}

/** internal **/

static void
input_add_device( CoreInputDevice *device )
{
     D_DEBUG_AT( Core_Input, "%s( %p )\n", __FUNCTION__, device );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     if (core_input->num == MAX_INPUTDEVICES) {
          D_ERROR( "DirectFB/Input: Maximum number of devices reached!\n" );
          return;
     }

     direct_list_append( &core_local->devices, &device->link );

     core_input->devices[ core_input->num++ ] = device->shared;

     if (core_local->hub)
          CoreInputHub_AddDevice( core_local->hub, device->shared->id, &device->shared->device_info.desc );
}

static void
allocate_device_keymap( CoreDFB *core, CoreInputDevice *device )
{
     int                        i;
     DFBInputDeviceKeymapEntry *entries;
     FusionSHMPoolShared       *pool        = dfb_core_shmpool( core );
     InputDeviceShared         *shared      = device->shared;
     DFBInputDeviceDescription *desc        = &shared->device_info.desc;
     int                        num_entries = desc->max_keycode -
                                              desc->min_keycode + 1;

     D_DEBUG_AT( Core_Input, "%s( %p, %p )\n", __FUNCTION__, core, device );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );

     entries = SHCALLOC( pool, num_entries, sizeof(DFBInputDeviceKeymapEntry) );
     if (!entries) {
          D_OOSHM();
          return;
     }

     /* write -1 indicating entry is not fetched yet from driver */
     for (i=0; i<num_entries; i++)
          entries[i].code = -1;

     shared->keymap.min_keycode = desc->min_keycode;
     shared->keymap.max_keycode = desc->max_keycode;
     shared->keymap.num_entries = num_entries;
     shared->keymap.entries     = entries;

#if FUSION_BUILD_MULTI
     /* we need to fetch the whole map, otherwise a slave would try to */
     //####modified by marvell-bg2
     //for (i=desc->min_keycode; i<=desc->max_keycode; i++)
       //get_keymap_entry( device, i );
     for(i = 0x00; i < MRVL_NUM_KEY_MAP_KEYS; i++ )
         set_keymap_entry( device, dfb_keyboard_key_map_mrvl[i].entryvar.code, &(dfb_keyboard_key_map_mrvl[i].entryvar) );
	 //####end of marvell-bg2
#endif
}

static int
make_id( DFBInputDeviceID prefered )
{
     CoreInputDevice *device;

     D_DEBUG_AT( Core_Input, "%s( 0x%02x )\n", __FUNCTION__, prefered );

     D_ASSERT( core_input != NULL );

     direct_list_foreach (device, core_local->devices) {
          D_MAGIC_ASSERT( device, CoreInputDevice );

          if (device->shared->id == prefered)
               return make_id( (prefered < DIDID_ANY) ? DIDID_ANY : (prefered + 1) );
     }

     return prefered;
}

static DFBResult
reload_keymap( CoreInputDevice *device )
{
     int                i;
     InputDeviceShared *shared;

     D_DEBUG_AT( Core_Input, "%s( %p )\n", __FUNCTION__, device );

     D_MAGIC_ASSERT( device, CoreInputDevice );

     shared = device->shared;

     D_ASSERT( shared != NULL );

     if (shared->device_info.desc.min_keycode < 0 ||
         shared->device_info.desc.max_keycode < 0)
          return DFB_UNSUPPORTED;

     /* write -1 indicating entry is not fetched yet from driver */
     for (i=0; i<shared->keymap.num_entries; i++)
          shared->keymap.entries[i].code = -1;

     /* fetch the whole map */
     for (i=shared->keymap.min_keycode; i<=shared->keymap.max_keycode; i++)
          get_keymap_entry( device, i );

     D_INFO( "DirectFB/Input: Reloaded keymap for '%s' [0x%02x]\n",
             shared->device_info.desc.name, shared->id );

     return DFB_OK;
}

static DFBResult
init_axes( CoreInputDevice *device )
{
     int                     i, num;
     DFBResult               ret;
     InputDeviceShared      *shared;
     const InputDriverFuncs *funcs;

     D_DEBUG_AT( Core_Input, "%s( %p )\n", __FUNCTION__, device );

     D_MAGIC_ASSERT( device, CoreInputDevice );
     D_ASSERT( device->driver != NULL );

     funcs = device->driver->funcs;
     D_ASSERT( funcs != NULL );

     shared = device->shared;
     D_ASSERT( shared != NULL );

     if (shared->device_info.desc.max_axis < 0)
          return DFB_OK;

     num = shared->device_info.desc.max_axis + 1;

     shared->axis_info = SHCALLOC( dfb_core_shmpool(device->core), num, sizeof(DFBInputDeviceAxisInfo) );
     if (!shared->axis_info)
          return D_OOSHM();

     shared->axis_num = num;

     if (funcs->GetAxisInfo) {
          for (i=0; i<num; i++) {
               ret = funcs->GetAxisInfo( device, device->driver_data, i, &shared->axis_info[i] );
               if (ret)
                    D_DERROR( ret, "Core/Input: GetAxisInfo() failed for '%s' [%d] on axis %d!\n",
                              shared->device_info.desc.name, shared->id, i );
          }
     }

     return DFB_OK;
}

static void
init_devices( CoreDFB *core )
{
     DirectLink          *next;
     DirectModuleEntry   *module;
     FusionSHMPoolShared *pool = dfb_core_shmpool( core );
     external_keymap = NULL;//####modified by marvell-bg2

     D_DEBUG_AT( Core_Input, "%s( %p )\n", __FUNCTION__, core );

     D_ASSERT( core_input != NULL );

     direct_list_foreach_safe (module, next, dfb_input_modules.entries) {
          int                     n;
          InputDriver            *driver;
          const InputDriverFuncs *funcs;
          InputDriverCapability   driver_cap;
          DFBResult               result;

          driver_cap = IDC_NONE;

          funcs = direct_module_ref( module );
          if (!funcs)
               continue;

          driver = D_CALLOC( 1, sizeof(InputDriver) );
          if (!driver) {
               D_OOM();
               direct_module_unref( module );
               continue;
          }

          D_ASSERT( funcs->GetDriverInfo != NULL );

          funcs->GetDriverInfo( &driver->info );

          D_DEBUG_AT( Core_Input, "  -> probing '%s'...\n", driver->info.name );

          driver->nr_devices = funcs->GetAvailable();

          /*
           * If the input provider supports hot-plug, always load the module.
           */
          if (!funcs->GetCapability) {
               D_DEBUG_AT(Core_Input, "InputDriverFuncs::GetCapability is NULL\n");
          }
          else {
               driver_cap = funcs->GetCapability();
          }

          if (!driver->nr_devices && !(driver_cap & IDC_HOTPLUG)) {
               direct_module_unref( module );
               D_FREE( driver );
               continue;
          }

          D_DEBUG_AT( Core_Input, "  -> %d available device(s) provided by '%s'.\n",
                      driver->nr_devices, driver->info.name );

          driver->module = module;
          driver->funcs  = funcs;

          direct_list_prepend( &core_local->drivers, &driver->link );


          for (n=0; n<driver->nr_devices; n++) {
               char               buf[128];
               CoreInputDevice   *device;
               InputDeviceInfo    device_info;
               InputDeviceShared *shared;
               void              *driver_data;

               device = D_CALLOC( 1, sizeof(CoreInputDevice) );
               if (!device) {
                    D_OOM();
                    continue;
               }

               shared = SHCALLOC( pool, 1, sizeof(InputDeviceShared) );
               if (!shared) {
                    D_OOSHM();
                    D_FREE( device );
                    continue;
               }

               device->core = core;

               memset( &device_info, 0, sizeof(InputDeviceInfo) );

               device_info.desc.min_keycode = -1;
               device_info.desc.max_keycode = -1;

               D_MAGIC_SET( device, CoreInputDevice );

               if (funcs->OpenDevice( device, n, &device_info, &driver_data )) {
                    SHFREE( pool, shared );
                    D_MAGIC_CLEAR( device );
                    D_FREE( device );
                    continue;
               }

               D_DEBUG_AT( Core_Input, "  -> opened '%s' (%d) %d.%d (%s)\n",
                           device_info.desc.name, n + 1, driver->info.version.major,
                           driver->info.version.minor, driver->info.vendor );

               if (driver->nr_devices > 1)
                    snprintf( buf, sizeof(buf), "%s (%d)", device_info.desc.name, n+1 );
               else
                    snprintf( buf, sizeof(buf), "%s", device_info.desc.name );

               /* init skirmish */
               fusion_skirmish_init2( &shared->lock, buf, dfb_core_world(core), fusion_config->secure_fusion );

               /* create reactor */
               shared->reactor = fusion_reactor_new( sizeof(DFBInputEvent), buf, dfb_core_world(core) );

               fusion_reactor_direct( shared->reactor, false );

               fusion_reactor_add_permissions( shared->reactor, 0, FUSION_REACTOR_PERMIT_ATTACH_DETACH );

               fusion_reactor_set_lock( shared->reactor, &shared->lock );

               /* init call */
               CoreInputDevice_Init_Dispatch( core, device, &shared->call );

               /* initialize shared data */
               shared->id          = make_id(device_info.prefered_id);
               shared->num         = n;
               shared->device_info = device_info;
               shared->last_key    = DIKI_UNKNOWN;
               shared->first_press = true;
	           shared->external_keymap_loaded = false;//####modified by marvell-bg2

               /* initialize local data */
               device->shared      = shared;
               device->driver      = driver;
               device->driver_data = driver_data;

               D_INFO( "DirectFB/Input: %s %d.%d (%s)\n",
                       buf, driver->info.version.major,
                       driver->info.version.minor, driver->info.vendor );

#if FUSION_BUILD_MULTI
               /* Initialize the ref between shared device and local device. */
               snprintf( buf, sizeof(buf), "Ref of input device(%d)", shared->id );
               fusion_ref_init( &shared->ref, buf, dfb_core_world(core) );

               fusion_ref_add_permissions( &shared->ref, 0, FUSION_REF_PERMIT_REF_UNREF_LOCAL );

               /* Increase reference counter. */
               fusion_ref_up( &shared->ref, false );
#endif

               if (device_info.desc.min_keycode > device_info.desc.max_keycode) {
                    D_BUG("min_keycode > max_keycode");
                    device_info.desc.min_keycode = -1;
                    device_info.desc.max_keycode = -1;
               }
               else if (device_info.desc.min_keycode >= 0 &&
                        device_info.desc.max_keycode >= 0)
                    allocate_device_keymap( core, device );

               init_axes( device );

               /* add it to the list */
               input_add_device( device );
          }

          /*
           * If the driver supports hot-plug, launch its hot-plug thread to respond to
           * hot-plug events.  Failures in launching the hot-plug thread will only
           * result in no hot-plug feature being available.
           */
          if (driver_cap == IDC_HOTPLUG) {
               result = funcs->LaunchHotplug(core, (void*) driver);

               /* On failure, the input provider can still be used without hot-plug. */
               if (result) {
                    D_INFO( "DirectFB/Input: Failed to enable hot-plug "
                            "detection with %s\n ", driver->info.name);
               }
               else {
                    D_INFO( "DirectFB/Input: Hot-plug detection enabled with %s \n",
                            driver->info.name);
               }
          }
     }
}

/*
 * Create the DFB shared core input device, add the input device into the
 * local device list and shared dev array and broadcast the hot-plug in
 * message to all slaves.
 */
DFBResult
dfb_input_create_device(int device_index, CoreDFB *core_in, void *driver_in)
{
     char                    buf[128];
     CoreInputDevice        *device;
     InputDeviceInfo         device_info;
     InputDeviceShared      *shared;
     void                   *driver_data;
     InputDriver            *driver = NULL;
     const InputDriverFuncs *funcs;
     FusionSHMPoolShared    *pool;
     DFBResult               result;

     D_DEBUG_AT(Core_Input, "Enter: %s()\n", __FUNCTION__);

     driver = (InputDriver *)driver_in;

     pool = dfb_core_shmpool(core_in);
     funcs = driver->funcs;
     if (!funcs) {
          D_ERROR("DirectFB/Input: driver->funcs is NULL\n");
          goto errorExit;
     }

     device = D_CALLOC( 1, sizeof(CoreInputDevice) );
     if (!device) {
          D_OOM();
          goto errorExit;
     }
     shared = SHCALLOC( pool, 1, sizeof(InputDeviceShared) );
     if (!shared) {
          D_OOM();
          D_FREE( device );
          goto errorExit;
     }

     device->core = core_in;

     memset( &device_info, 0, sizeof(InputDeviceInfo) );

     device_info.desc.min_keycode = -1;
     device_info.desc.max_keycode = -1;

     D_MAGIC_SET( device, CoreInputDevice );

     if (funcs->OpenDevice( device, device_index, &device_info, &driver_data )) {
          SHFREE( pool, shared );
          D_MAGIC_CLEAR( device );
          D_FREE( device );
          D_DEBUG_AT( Core_Input,
                      "DirectFB/Input: Cannot open device in %s, at %d in %s\n",
                      __FUNCTION__, __LINE__, __FILE__);
          goto errorExit;
     }

     snprintf( buf, sizeof(buf), "%s (%d)", device_info.desc.name, device_index);

     /* init skirmish */
     result = fusion_skirmish_init2( &shared->lock, buf, dfb_core_world(device->core), fusion_config->secure_fusion );
     if (result) {
          funcs->CloseDevice( driver_data );
          SHFREE( pool, shared );
          D_MAGIC_CLEAR( device );
          D_FREE( device );
          D_ERROR("DirectFB/Input: fusion_skirmish_init() failed! in %s, at %d in %s\n",
                  __FUNCTION__, __LINE__, __FILE__);
          goto errorExit;
     }

     /* create reactor */
     shared->reactor = fusion_reactor_new( sizeof(DFBInputEvent),
                                           buf,
                                           dfb_core_world(device->core) );
     if (!shared->reactor) {
          funcs->CloseDevice( driver_data );
          SHFREE( pool, shared );
          D_MAGIC_CLEAR( device );
          D_FREE( device );
          fusion_skirmish_destroy(&shared->lock);
          D_ERROR("DirectFB/Input: fusion_reactor_new() failed! in %s, at %d in %s\n",
                  __FUNCTION__, __LINE__, __FILE__);
          goto errorExit;
     }

     fusion_reactor_direct( shared->reactor, false );

     fusion_reactor_add_permissions( shared->reactor, 0, FUSION_REACTOR_PERMIT_ATTACH_DETACH );

     fusion_reactor_set_lock( shared->reactor, &shared->lock );

     /* init call */
     CoreInputDevice_Init_Dispatch( device->core, device, &shared->call );

     /* initialize shared data */
     shared->id          = make_id(device_info.prefered_id);
     shared->num         = device_index;
     shared->device_info = device_info;
     shared->last_key    = DIKI_UNKNOWN;
     shared->first_press = true;

     /* initialize local data */
     device->shared      = shared;
     device->driver      = driver;
     device->driver_data = driver_data;

     D_INFO( "DirectFB/Input: %s %d.%d (%s)\n",
             buf, driver->info.version.major,
             driver->info.version.minor, driver->info.vendor );

#if FUSION_BUILD_MULTI
     snprintf( buf, sizeof(buf), "Ref of input device(%d)", shared->id);
     fusion_ref_init( &shared->ref, buf, dfb_core_world(core_in));
     fusion_ref_add_permissions( &shared->ref, 0, FUSION_REF_PERMIT_REF_UNREF_LOCAL );
     fusion_ref_up( &shared->ref, false );
#endif

     if (device_info.desc.min_keycode > device_info.desc.max_keycode) {
          D_BUG("min_keycode > max_keycode");
          device_info.desc.min_keycode = -1;
          device_info.desc.max_keycode = -1;
     }
     else if (device_info.desc.min_keycode >= 0 && device_info.desc.max_keycode >= 0)
          allocate_device_keymap( device->core, device );

     /* add it into local device list and shared dev array */
     D_DEBUG_AT(Core_Input,
                "In master, add a new device with dev_id=%d\n",
                shared->id);

     input_add_device( device );
     driver->nr_devices++;

     D_DEBUG_AT(Core_Input,
                "Successfully added new input device with dev_id=%d in shared array\n",
                shared->id);

     InputDeviceHotplugEvent    message;

     /* Setup the hot-plug in message. */
     message.is_plugin = true;
     message.dev_id = shared->id;
     gettimeofday (&message.stamp, NULL);

     /* Send the hot-plug in message */
#if FUSION_BUILD_MULTI
     fusion_reactor_dispatch(core_input->reactor, &message, true, NULL);
#else
     local_processing_hotplug((const void *) &message, (void*) core_in);
#endif
     return DFB_OK;

errorExit:
     return DFB_FAILURE;
}

/*
 * Tell whether the DFB input device handling of the system input device
 * indicated by device_index is already created.
 */
static CoreInputDevice *
search_device_created(int device_index, void *driver_in)
{
     DirectLink  *n;
     CoreInputDevice *device;

     D_ASSERT(driver_in != NULL);

     direct_list_foreach_safe(device, n, core_local->devices) {
          if (device->driver != driver_in)
               continue;

          if (device->driver_data == NULL) {
               D_DEBUG_AT(Core_Input,
                          "The device %d has been closed!\n",
                          device->shared->id);
               return NULL;
          }

          if (device->driver->funcs->IsCreated &&
              !device->driver->funcs->IsCreated(device_index, device->driver_data)) {
               return device;
          }
     }

     return NULL;
}

/*
 * Remove the DFB shared core input device handling of the system input device
 * indicated by device_index and broadcast the hot-plug out message to all slaves.
 */
DFBResult
dfb_input_remove_device(int device_index, void *driver_in)
{
     CoreInputDevice    *device;
     InputDeviceShared  *shared;
     FusionSHMPoolShared *pool;
     int                 i;
     int                 found = 0;
     int                 device_id;

     D_DEBUG_AT(Core_Input, "Enter: %s()\n", __FUNCTION__);

     D_ASSERT(driver_in !=NULL);

     device = search_device_created(device_index, driver_in);
     if (device == NULL) {
          D_DEBUG_AT(Core_Input,
                     "DirectFB/input: Failed to find the device[%d] or the device is "
                     "closed.\n",
                     device_index);
          goto errorExit;
     }

     shared = device->shared;
     pool = dfb_core_shmpool( device->core );
     device_id = shared->id;

     D_DEBUG_AT(Core_Input, "Find the device with dev_id=%d\n", device_id);

     device->driver->funcs->CloseDevice( device->driver_data );

     if (core_local->hub)
          CoreInputHub_RemoveDevice( core_local->hub, device->shared->id );

     device->driver->nr_devices--;

     InputDeviceHotplugEvent    message;

     /* Setup the hot-plug out message */
     message.is_plugin = false;
     message.dev_id = device_id;
     gettimeofday (&message.stamp, NULL);

     /* Send the hot-plug out message */
#if FUSION_BUILD_MULTI
     fusion_reactor_dispatch( core_input->reactor, &message, true, NULL);

     int    loop = CHECK_NUMBER;

     while (--loop) {
          if (fusion_ref_zero_trylock( &device->shared->ref ) == DR_OK) {
               fusion_ref_unlock(&device->shared->ref);
               break;
          }

          usleep(CHECK_INTERVAL);
     }

     if (!loop)
          D_DEBUG_AT(Core_Input, "Shared device might be connected to by others\n");

     fusion_ref_destroy(&device->shared->ref);
#else
     local_processing_hotplug((const void*) &message, (void*) device->core);
#endif

     /* Remove the device from shared array */
     for (i = 0; i < core_input->num; i++) {
          if (!found && (core_input->devices[i]->id == shared->id))
               found = 1;

          if (found)
               core_input->devices[i] = core_input->devices[(i + 1) % MAX_INPUTDEVICES];
     }
     if (found)
          core_input->devices[core_input->num -1] = NULL;

     core_input->num--;

     CoreInputDevice_Deinit_Dispatch( &shared->call );

     fusion_skirmish_destroy( &shared->lock );

     fusion_reactor_free( shared->reactor );

     if (shared->keymap.entries)
          SHFREE( pool, shared->keymap.entries );

     SHFREE( pool, shared );

     D_DEBUG_AT(Core_Input,
                "Successfully remove the device with dev_id=%d in shared array\n",
                device_id);

     return DFB_OK;

errorExit:
     return DFB_FAILURE;
}

/*
 * Create local input device and add it into the local input devices list.
 */
static CoreInputDevice *
add_device_into_local_list(int dev_id)
{
     int i;
     D_DEBUG_AT(Core_Input, "Enter: %s()\n", __FUNCTION__);
     for (i = 0; i < core_input->num; i++) {
          if (core_input->devices[i]->id == dev_id) {
               CoreInputDevice *device;

               D_DEBUG_AT(Core_Input,
                          "Find the device with dev_id=%d in shared array, and "
                          "allocate local device\n",
                          dev_id);

               device = D_CALLOC( 1, sizeof(CoreInputDevice) );
               if (!device) {
                   return NULL;
               }

               device->shared = core_input->devices[i];

#if FUSION_BUILD_MULTI
               /* Increase the reference counter. */
               fusion_ref_up( &device->shared->ref, false );
#endif

               /* add it to the list */
               direct_list_append( &core_local->devices, &device->link );

               D_MAGIC_SET( device, CoreInputDevice );
               return device;
          }
     }

     return NULL;
}

/*
 * Local input device function that handles hot-plug in/out messages.
 */
static ReactionResult
local_processing_hotplug( const void *msg_data, void *ctx )
{
     const InputDeviceHotplugEvent *message = msg_data;
     CoreInputDevice               *device = NULL;

     D_DEBUG_AT(Core_Input, "Enter: %s()\n", __FUNCTION__);

     D_DEBUG_AT(Core_Input,
                "<PID:%6d> hotplug-in:%d device_id=%d message!\n",
                getpid(),
                message->is_plugin,
                message->dev_id );

     if (message->is_plugin) {

          device = dfb_input_device_at(message->dev_id);

          if (!device){
               /* Update local device list according to shared devices array */
               if (!(device = add_device_into_local_list(message->dev_id))) {
                    D_ERROR("DirectFB/Input: update_local_devices_list() failed\n" );
                    goto errorExit;
               }
          }

          /* attach the device to event containers */
          containers_attach_device( device );
          /* attach the device to stack containers  */
          stack_containers_attach_device( device );
     }
     else {
          device = dfb_input_device_at(message->dev_id);
          if (device) {

               direct_list_remove(&core_local->devices, &device->link);

               containers_detach_device(device);
               stack_containers_detach_device(device);

#if FUSION_BUILD_MULTI
               /* Decrease reference counter. */
               fusion_ref_down( &device->shared->ref, false );
#endif
               D_MAGIC_CLEAR( device );
               D_FREE(device);
          }
          else
               D_ERROR("DirectFB/Input:Can't find the device to be removed!\n");

     }

     return DFB_OK;

errorExit:
     return DFB_FAILURE;
}

static DFBInputDeviceKeymapEntry *
get_keymap_entry( CoreInputDevice *device,
                  int              code )
{
     InputDeviceKeymap         *map;
     DFBInputDeviceKeymapEntry *entry;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );
	 //####modified by marvell-bg2
     if((external_keymap != NULL) && (device->shared->external_keymap_loaded == false))
     {
	     map = external_keymap;
     }
     else
     {
     map = &device->shared->keymap;
     }
	 //####end of marvell-bg2

     /* safety check */
     if (code < map->min_keycode || code > map->max_keycode)
          return NULL;

     /* point to right array index */
     entry = &map->entries[code - map->min_keycode];

     /* need to initialize? */
     if (entry->code != code) {
          DFBResult    ret;
          InputDriver *driver = device->driver;

          if (!driver) {
               D_BUG("seem to be a slave with an empty keymap");
               return NULL;
          }

          /* write keycode to entry */
          entry->code = code;

          /* fetch entry from driver */
          ret = driver->funcs->GetKeymapEntry( device,
                                               device->driver_data, entry );
          if (ret)
               return NULL;

          /* drivers may leave this blank */
          if (entry->identifier == DIKI_UNKNOWN)
               entry->identifier = symbol_to_id( entry->symbols[DIKSI_BASE] );

          if (entry->symbols[DIKSI_BASE_SHIFT] == DIKS_NULL)
               entry->symbols[DIKSI_BASE_SHIFT] = entry->symbols[DIKSI_BASE];

          if (entry->symbols[DIKSI_ALT] == DIKS_NULL)
               entry->symbols[DIKSI_ALT] = entry->symbols[DIKSI_BASE];

          if (entry->symbols[DIKSI_ALT_SHIFT] == DIKS_NULL)
               entry->symbols[DIKSI_ALT_SHIFT] = entry->symbols[DIKSI_ALT];
     }

     return entry;
}

/* replace a single keymap entry with the code-entry pair */
static DFBResult
set_keymap_entry( CoreInputDevice                 *device,
                  int                              code,
                  const DFBInputDeviceKeymapEntry *entry )
{
     InputDeviceKeymap         *map;

     D_ASSERT( device->shared != NULL );
     D_ASSERT( device->shared->keymap.entries != NULL );

     map = &device->shared->keymap;
	 //####modified by marvell-bg2
     device->shared->external_keymap_loaded = true;
     external_keymap = map;
	 //####end of marvell-bg2
     /* sanity check */
     if (code < map->min_keycode || code > map->max_keycode)
          return DFB_FAILURE;

     /* copy the entry to the map */
     map->entries[code - map->min_keycode] = *entry;
     
     return DFB_OK;
}

/* replace the complete current keymap with a keymap from a file.
 * the minimum-maximum keycodes of the driver are to be respected. 
 */
static DFBResult
load_keymap( CoreInputDevice           *device,
             char                      *filename )
{
     DFBResult                  ret       = DFB_OK;
     InputDeviceKeymap         *map       = 0;
     FILE                      *file      = 0;
     DFBInputDeviceLockState    lockstate = 0;

     D_ASSERT( device->shared != NULL );
     D_ASSERT( device->shared->keymap.entries != NULL );

     map = &device->shared->keymap;

     /* open the file */
     file = fopen( filename, "r" );
     if( !file )
     {
          return errno2result( errno );
     }

     /* read the file, line by line, and consume the mentioned scancodes */
     while(1)
     {
          int   i;
          int   dummy;
          char  buffer[201];
          int   keycode;
          char  diki[201];
          char  diks[4][201];
          char *b;
          
          DFBInputDeviceKeymapEntry entry = { .code = 0 };
          
          b = fgets( buffer, 200, file );
          if( !b ) {
               if( feof(file) ) {
                    fclose(file);
                    return DFB_OK;
               }
               fclose(file);
               return errno2result(errno);
          }

          /* comment or empty line */
          if( buffer[0]=='#' || strcmp(buffer,"\n")==0 )
               continue;

          /* check for lock state change */
          if( !strncmp(buffer,"capslock:",9) ) { lockstate |=  DILS_CAPS; continue; }
          if( !strncmp(buffer,":capslock",9) ) { lockstate &= ~DILS_CAPS; continue; }
          if( !strncmp(buffer,"numlock:",8)  ) { lockstate |=  DILS_NUM;  continue; }
          if( !strncmp(buffer,":numlock",8)  ) { lockstate &= ~DILS_NUM;  continue; }

          i = sscanf( buffer, " keycode %i = %s = %s %s %s %s %i\n",
                    &keycode, diki, diks[0], diks[1], diks[2], diks[3], &dummy );

          if( i < 3 || i > 6 ) {
               /* we want 1 to 4 key symbols */
               D_INFO( "DirectFB/Input: skipped erroneous input line %s\n", buffer );
               continue;
          }

          if( keycode > map->max_keycode || keycode < map->min_keycode ) {
               D_INFO( "DirectFB/Input: skipped keycode %d out of range\n", keycode );
               continue;
          }

          entry.code       = keycode;
          entry.locks      = lockstate;
          entry.identifier = lookup_keyidentifier( diki );

          switch( i ) {
               case 6:  entry.symbols[3] = lookup_keysymbol( diks[3] );
               case 5:  entry.symbols[2] = lookup_keysymbol( diks[2] );
               case 4:  entry.symbols[1] = lookup_keysymbol( diks[1] );
               case 3:  entry.symbols[0] = lookup_keysymbol( diks[0] );

                    /* fall through */
          }

          switch( i ) {
               case 3:  entry.symbols[1] = entry.symbols[0];
               case 4:  entry.symbols[2] = entry.symbols[0];
               case 5:  entry.symbols[3] = entry.symbols[1];

               /* fall through */
          }

          ret = CoreInputDevice_SetKeymapEntry( device, keycode, &entry );
          if( ret )
               return ret;
     }
}

static DFBInputDeviceKeySymbol lookup_keysymbol( char *symbolname )
{
     int i;

     /* we want uppercase */  
     for( i=0; i<strlen(symbolname); i++ )
          if( symbolname[i] >= 'a' && symbolname[i] <= 'z' )
               symbolname[i] = symbolname[i] - 'a' + 'A';

     for( i=0; i < sizeof (KeySymbolNames) / sizeof (KeySymbolNames[0]); i++ ) {
          if( strcmp( symbolname, KeySymbolNames[i].name ) == 0 )
               return KeySymbolNames[i].symbol;
     }
     
     /* not found, maybe starting with 0x for raw conversion.
      * We are already at uppercase.
      */
     if( symbolname[0]=='0' && symbolname[1]=='X' ) {
          int code=0;
          symbolname+=2;
          while(*symbolname) {
               if( *symbolname >= '0' && *symbolname <= '9' ) {
                    code = code*16 + *symbolname - '0';
               } else if( *symbolname >= 'A' && *symbolname <= 'F' ) {
                    code = code*16 + *symbolname - 'A' + 10;
               } else {
                    /* invalid character */
                    return DIKS_NULL;
               }
               symbolname++;
          }
          return code;
     }
     
     return DIKS_NULL;
}

static DFBInputDeviceKeyIdentifier lookup_keyidentifier( char *identifiername )
{
     int i;

     /* we want uppercase */  
     for( i=0; i<strlen(identifiername); i++ )
          if( identifiername[i] >= 'a' && identifiername[i] <= 'z' )
               identifiername[i] = identifiername[i] - 'a' + 'A';

     for( i=0; i < sizeof (KeyIdentifierNames) / sizeof (KeyIdentifierNames[0]); i++ ) {
          if( strcmp( identifiername, KeyIdentifierNames[i].name ) == 0 )
               return KeyIdentifierNames[i].identifier;
     }
     
     return DIKI_UNKNOWN;
}

static bool
lookup_from_table( CoreInputDevice    *device,
                   DFBInputEvent      *event,
                   DFBInputEventFlags  lookup )
{
     DFBInputDeviceKeymapEntry *entry;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );
     D_ASSERT( event != NULL );

     /* fetch the entry from the keymap, possibly calling the driver */
     entry = get_keymap_entry( device, event->key_code );
     if (!entry)
          return false;

     /* lookup identifier */
     if (lookup & DIEF_KEYID)
          event->key_id = entry->identifier;

     /* lookup symbol */
     if (lookup & DIEF_KEYSYMBOL) {
          DFBInputDeviceKeymapSymbolIndex index =
               (event->modifiers & DIMM_ALTGR) ? DIKSI_ALT : DIKSI_BASE;

          if (!(event->modifiers & DIMM_SHIFT) ^ !(entry->locks & event->locks))
               index++;

          /* don't modify modifiers */
          if (DFB_KEY_TYPE( entry->symbols[DIKSI_BASE] ) == DIKT_MODIFIER)
               event->key_symbol = entry->symbols[DIKSI_BASE];
          else
               event->key_symbol = entry->symbols[index];
     }

     return true;
}

static int
find_key_code_by_id( CoreInputDevice             *device,
                     DFBInputDeviceKeyIdentifier  id )
{
     int                i;
     InputDeviceKeymap *map;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

     //####modified by marvell-bg2
     if((external_keymap != NULL) && (device->shared->external_keymap_loaded == false))
     {
	     map = external_keymap;
     }
     else
     {
     map = &device->shared->keymap;
     }
	 //####end of marvell-bg2

     for (i=0; i<map->num_entries; i++) {
          DFBInputDeviceKeymapEntry *entry = &map->entries[i];

          if (entry->identifier == id)
               return entry->code;
     }

     return -1;
}

static int
find_key_code_by_symbol( CoreInputDevice         *device,
                         DFBInputDeviceKeySymbol  symbol )
{
     int                i;
     InputDeviceKeymap *map;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     D_ASSERT( core_input != NULL );
     D_ASSERT( device != NULL );
     D_ASSERT( device->shared != NULL );

	 //####modified by marvell-bg2
     if((external_keymap != NULL) && (device->shared->external_keymap_loaded == false))
     {
	     map = external_keymap;
     }
     else
     {
     map = &device->shared->keymap;
     }
	 //####end of marvell-bg2

     for (i=0; i<map->num_entries; i++) {
          int                        n;
          DFBInputDeviceKeymapEntry *entry = &map->entries[i];

          for (n=0; n<=DIKSI_LAST; n++)
               if (entry->symbols[n] == symbol)
                    return entry->code;
     }

     return -1;
}

#define FIXUP_KEY_FIELDS     (DIEF_MODIFIERS | DIEF_LOCKS | \
                              DIEF_KEYCODE | DIEF_KEYID | DIEF_KEYSYMBOL)

/*
 * Fill partially missing values for key_code, key_id and key_symbol by
 * translating those that are set. Fix modifiers/locks before if not set.
 *
 *
 * There are five valid constellations that give reasonable values.
 * (not counting the constellation where everything is set)
 *
 * Device has no translation table
 *   1. key_id is set, key_symbol not
 *      -> key_code defaults to -1, key_symbol from key_id (up-translation)
 *   2. key_symbol is set, key_id not
 *      -> key_code defaults to -1, key_id from key_symbol (down-translation)
 *
 * Device has a translation table
 *   3. key_code is set
 *      -> look up key_id and/or key_symbol (key_code being the index)
 *   4. key_id is set
 *      -> look up key_code and possibly key_symbol (key_id being searched for)
 *   5. key_symbol is set
 *      -> look up key_code and key_id (key_symbol being searched for)
 *
 * Fields remaining will be set to the default, e.g. key_code to -1.
 */
static void
fixup_key_event( CoreInputDevice *device, DFBInputEvent *event )
{
     int                 i;
     DFBInputEventFlags  valid   = event->flags & FIXUP_KEY_FIELDS;
     DFBInputEventFlags  missing = valid ^ FIXUP_KEY_FIELDS;
     InputDeviceShared  *shared  = device->shared;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     /* Add missing flags */
     event->flags |= missing;

     /*
      * Use cached values for modifiers/locks if they are missing.
      */
     if (missing & DIEF_MODIFIERS)
          event->modifiers = shared->state.modifiers_l | shared->state.modifiers_r;

     if (missing & DIEF_LOCKS)
          event->locks = shared->state.locks;

     /*
      * With translation table
      */
     if (device->shared->keymap.num_entries) {
          if (valid & DIEF_KEYCODE) {
               lookup_from_table( device, event, missing );

               missing &= ~(DIEF_KEYID | DIEF_KEYSYMBOL);
          }
          else if (valid & DIEF_KEYID) {
               event->key_code = find_key_code_by_id( device, event->key_id );

               if (event->key_code != -1) {
                    lookup_from_table( device, event, missing );

                    missing &= ~(DIEF_KEYCODE | DIEF_KEYSYMBOL);
               }
               else if (missing & DIEF_KEYSYMBOL) {
                    event->key_symbol = id_to_symbol( event->key_id,
                                                      event->modifiers,
                                                      event->locks );
                    missing &= ~DIEF_KEYSYMBOL;
               }
          }
          else if (valid & DIEF_KEYSYMBOL) {
               event->key_code = find_key_code_by_symbol( device,
                                                          event->key_symbol );

               if (event->key_code != -1) {
                    lookup_from_table( device, event, missing );

                    missing &= ~(DIEF_KEYCODE | DIEF_KEYID);
               }
               else {
                    event->key_id = symbol_to_id( event->key_symbol );
                    missing &= ~DIEF_KEYSYMBOL;
               }
          }
     }
     else {
          /*
           * Without translation table
           */
          if (valid & DIEF_KEYID) {
               if (missing & DIEF_KEYSYMBOL) {
                    event->key_symbol = id_to_symbol( event->key_id,
                                                      event->modifiers,
                                                      event->locks );
                    missing &= ~DIEF_KEYSYMBOL;
               }
          }
          else if (valid & DIEF_KEYSYMBOL) {
               event->key_id = symbol_to_id( event->key_symbol );
               missing &= ~DIEF_KEYID;
          }
     }

     /*
      * Clear remaining fields.
      */
     if (missing & DIEF_KEYCODE)
          event->key_code = -1;

     if (missing & DIEF_KEYID)
          event->key_id = DIKI_UNKNOWN;

     if (missing & DIEF_KEYSYMBOL)
          event->key_symbol = DIKS_NULL;

     /*
      * Update cached values for modifiers.
      */
     if (DFB_KEY_TYPE(event->key_symbol) == DIKT_MODIFIER) {
          if (event->type == DIET_KEYPRESS) {
               switch (event->key_id) {
                    case DIKI_SHIFT_L:
                         shared->state.modifiers_l |= DIMM_SHIFT;
                         break;
                    case DIKI_SHIFT_R:
                         shared->state.modifiers_r |= DIMM_SHIFT;
                         break;
                    case DIKI_CONTROL_L:
                         shared->state.modifiers_l |= DIMM_CONTROL;
                         break;
                    case DIKI_CONTROL_R:
                         shared->state.modifiers_r |= DIMM_CONTROL;
                         break;
                    case DIKI_ALT_L:
                         shared->state.modifiers_l |= DIMM_ALT;
                         break;
                    case DIKI_ALT_R:
                         shared->state.modifiers_r |= (event->key_symbol == DIKS_ALTGR) ? DIMM_ALTGR : DIMM_ALT;
                         break;
                    case DIKI_META_L:
                         shared->state.modifiers_l |= DIMM_META;
                         break;
                    case DIKI_META_R:
                         shared->state.modifiers_r |= DIMM_META;
                         break;
                    case DIKI_SUPER_L:
                         shared->state.modifiers_l |= DIMM_SUPER;
                         break;
                    case DIKI_SUPER_R:
                         shared->state.modifiers_r |= DIMM_SUPER;
                         break;
                    case DIKI_HYPER_L:
                         shared->state.modifiers_l |= DIMM_HYPER;
                         break;
                    case DIKI_HYPER_R:
                         shared->state.modifiers_r |= DIMM_HYPER;
                         break;
                    default:
                         ;
               }
          }
          else {
               switch (event->key_id) {
                    case DIKI_SHIFT_L:
                         shared->state.modifiers_l &= ~DIMM_SHIFT;
                         break;
                    case DIKI_SHIFT_R:
                         shared->state.modifiers_r &= ~DIMM_SHIFT;
                         break;
                    case DIKI_CONTROL_L:
                         shared->state.modifiers_l &= ~DIMM_CONTROL;
                         break;
                    case DIKI_CONTROL_R:
                         shared->state.modifiers_r &= ~DIMM_CONTROL;
                         break;
                    case DIKI_ALT_L:
                         shared->state.modifiers_l &= ~DIMM_ALT;
                         break;
                    case DIKI_ALT_R:
                         shared->state.modifiers_r &= (event->key_symbol == DIKS_ALTGR) ? ~DIMM_ALTGR : ~DIMM_ALT;
                         break;
                    case DIKI_META_L:
                         shared->state.modifiers_l &= ~DIMM_META;
                         break;
                    case DIKI_META_R:
                         shared->state.modifiers_r &= ~DIMM_META;
                         break;
                    case DIKI_SUPER_L:
                         shared->state.modifiers_l &= ~DIMM_SUPER;
                         break;
                    case DIKI_SUPER_R:
                         shared->state.modifiers_r &= ~DIMM_SUPER;
                         break;
                    case DIKI_HYPER_L:
                         shared->state.modifiers_l &= ~DIMM_HYPER;
                         break;
                    case DIKI_HYPER_R:
                         shared->state.modifiers_r &= ~DIMM_HYPER;
                         break;
                    default:
                         ;
               }
          }

          /* write back to event */
          if (missing & DIEF_MODIFIERS)
               event->modifiers = shared->state.modifiers_l | shared->state.modifiers_r;
     }

     /*
      * Update cached values for locks.
      */
     if (event->type == DIET_KEYPRESS) {

          /* When we receive a new key press, toggle lock flags */
          if (shared->first_press || shared->last_key != event->key_id) {
              switch (event->key_id) {
                   case DIKI_CAPS_LOCK:
                        shared->state.locks ^= DILS_CAPS;
                        break;
                   case DIKI_NUM_LOCK:
                        shared->state.locks ^= DILS_NUM;
                        break;
                   case DIKI_SCROLL_LOCK:
                        shared->state.locks ^= DILS_SCROLL;
                        break;
                   default:
                        ;
              }
          }

          /* write back to event */
          if (missing & DIEF_LOCKS)
               event->locks = shared->state.locks;

          /* store last pressed key */
          shared->last_key = event->key_id;
          shared->first_press = false;
     }
     else if (event->type == DIET_KEYRELEASE) {
          
          shared->first_press = true;
     }

     /* Handle dead keys. */
     if (DFB_KEY_TYPE(shared->last_symbol) == DIKT_DEAD) {
          for (i=0; i<D_ARRAY_SIZE(deadkey_maps); i++) {
               const DeadKeyMap *map = &deadkey_maps[i];

               if (map->deadkey == shared->last_symbol) {
                    for (i=0; map->combos[i].target; i++) {
                         if (map->combos[i].target == event->key_symbol) {
                              event->key_symbol = map->combos[i].result;
                              break;
                         }
                    }
                    break;
               }
          }

          if (event->type == DIET_KEYRELEASE &&
              DFB_KEY_TYPE(event->key_symbol) != DIKT_MODIFIER)
               shared->last_symbol = event->key_symbol;
     }
     else
          shared->last_symbol = event->key_symbol;
}

static void
fixup_mouse_event( CoreInputDevice *device, DFBInputEvent *event )
{
     InputDeviceShared *shared = device->shared;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     if (event->flags & DIEF_BUTTONS) {
          shared->state.buttons = event->buttons;
     }
     else {
          switch (event->type) {
               case DIET_BUTTONPRESS:
                    shared->state.buttons |= (1 << event->button);
                    break;
               case DIET_BUTTONRELEASE:
                    shared->state.buttons &= ~(1 << event->button);
                    break;
               default:
                    ;
          }

          /* Add missing flag */
          event->flags |= DIEF_BUTTONS;

          event->buttons = shared->state.buttons;
     }

     switch (event->type) {
          case DIET_AXISMOTION:
               if ((event->flags & DIEF_AXISABS) && event->axis >= 0 && event->axis < shared->axis_num) {
                    if (!(event->flags & DIEF_MIN) && (shared->axis_info[event->axis].flags & DIAIF_ABS_MIN)) {
                         event->min = shared->axis_info[event->axis].abs_min;

                         event->flags |= DIEF_MIN;
                    }
                         
                    if (!(event->flags & DIEF_MAX) && (shared->axis_info[event->axis].flags & DIAIF_ABS_MAX)) {
                         event->max = shared->axis_info[event->axis].abs_max;

                         event->flags |= DIEF_MAX;
                    }
               }
               break;

          default:
               break;
     }
}

static DFBInputDeviceKeyIdentifier
symbol_to_id( DFBInputDeviceKeySymbol symbol )
{
     if (symbol >= 'a' && symbol <= 'z')
          return DIKI_A + symbol - 'a';

     if (symbol >= 'A' && symbol <= 'Z')
          return DIKI_A + symbol - 'A';

     if (symbol >= '0' && symbol <= '9')
          return DIKI_0 + symbol - '0';

     if (symbol >= DIKS_F1 && symbol <= DIKS_F12)
          return DIKI_F1 + symbol - DIKS_F1;

     switch (symbol) {
          case DIKS_ESCAPE:
               return DIKI_ESCAPE;

          case DIKS_CURSOR_LEFT:
               return DIKI_LEFT;

          case DIKS_CURSOR_RIGHT:
               return DIKI_RIGHT;

          case DIKS_CURSOR_UP:
               return DIKI_UP;

          case DIKS_CURSOR_DOWN:
               return DIKI_DOWN;

          case DIKS_ALTGR:
               return DIKI_ALT_R;

          case DIKS_CONTROL:
               return DIKI_CONTROL_L;

          case DIKS_SHIFT:
               return DIKI_SHIFT_L;

          case DIKS_ALT:
               return DIKI_ALT_L;

          case DIKS_META:
               return DIKI_META_L;

          case DIKS_SUPER:
               return DIKI_SUPER_L;

          case DIKS_HYPER:
               return DIKI_HYPER_L;

          case DIKS_TAB:
               return DIKI_TAB;

          case DIKS_ENTER:
               return DIKI_ENTER;

          case DIKS_SPACE:
               return DIKI_SPACE;

          case DIKS_BACKSPACE:
               return DIKI_BACKSPACE;

          case DIKS_INSERT:
               return DIKI_INSERT;

          case DIKS_DELETE:
               return DIKI_DELETE;

          case DIKS_HOME:
               return DIKI_HOME;

          case DIKS_END:
               return DIKI_END;

          case DIKS_PAGE_UP:
               return DIKI_PAGE_UP;

          case DIKS_PAGE_DOWN:
               return DIKI_PAGE_DOWN;

          case DIKS_CAPS_LOCK:
               return DIKI_CAPS_LOCK;

          case DIKS_NUM_LOCK:
               return DIKI_NUM_LOCK;

          case DIKS_SCROLL_LOCK:
               return DIKI_SCROLL_LOCK;

          case DIKS_PRINT:
               return DIKI_PRINT;

          case DIKS_PAUSE:
               return DIKI_PAUSE;

          case DIKS_BACKSLASH:
               return DIKI_BACKSLASH;

          case DIKS_PERIOD:
               return DIKI_PERIOD;

          case DIKS_COMMA:
               return DIKI_COMMA;

          default:
               ;
     }

     return DIKI_UNKNOWN;
}

static DFBInputDeviceKeySymbol
id_to_symbol( DFBInputDeviceKeyIdentifier id,
              DFBInputDeviceModifierMask  modifiers,
              DFBInputDeviceLockState     locks )
{
     bool shift = !(modifiers & DIMM_SHIFT) ^ !(locks & DILS_CAPS);

     if (id >= DIKI_A && id <= DIKI_Z)
          return (shift ? DIKS_CAPITAL_A : DIKS_SMALL_A) + id - DIKI_A;

     if (id >= DIKI_0 && id <= DIKI_9)
          return DIKS_0 + id - DIKI_0;

     if (id >= DIKI_KP_0 && id <= DIKI_KP_9)
          return DIKS_0 + id - DIKI_KP_0;

     if (id >= DIKI_F1 && id <= DIKI_F12)
          return DIKS_F1 + id - DIKI_F1;

     switch (id) {
          case DIKI_ESCAPE:
               return DIKS_ESCAPE;

          case DIKI_LEFT:
               return DIKS_CURSOR_LEFT;

          case DIKI_RIGHT:
               return DIKS_CURSOR_RIGHT;

          case DIKI_UP:
               return DIKS_CURSOR_UP;

          case DIKI_DOWN:
               return DIKS_CURSOR_DOWN;

          case DIKI_CONTROL_L:
          case DIKI_CONTROL_R:
               return DIKS_CONTROL;

          case DIKI_SHIFT_L:
          case DIKI_SHIFT_R:
               return DIKS_SHIFT;

          case DIKI_ALT_L:
          case DIKI_ALT_R:
               return DIKS_ALT;

          case DIKI_META_L:
          case DIKI_META_R:
               return DIKS_META;

          case DIKI_SUPER_L:
          case DIKI_SUPER_R:
               return DIKS_SUPER;

          case DIKI_HYPER_L:
          case DIKI_HYPER_R:
               return DIKS_HYPER;

          case DIKI_TAB:
               return DIKS_TAB;

          case DIKI_ENTER:
               return DIKS_ENTER;

          case DIKI_SPACE:
               return DIKS_SPACE;

          case DIKI_BACKSPACE:
               return DIKS_BACKSPACE;

          case DIKI_INSERT:
               return DIKS_INSERT;

          case DIKI_DELETE:
               return DIKS_DELETE;

          case DIKI_HOME:
               return DIKS_HOME;

          case DIKI_END:
               return DIKS_END;

          case DIKI_PAGE_UP:
               return DIKS_PAGE_UP;

          case DIKI_PAGE_DOWN:
               return DIKS_PAGE_DOWN;

          case DIKI_CAPS_LOCK:
               return DIKS_CAPS_LOCK;

          case DIKI_NUM_LOCK:
               return DIKS_NUM_LOCK;

          case DIKI_SCROLL_LOCK:
               return DIKS_SCROLL_LOCK;

          case DIKI_PRINT:
               return DIKS_PRINT;

          case DIKI_PAUSE:
               return DIKS_PAUSE;

          case DIKI_KP_DIV:
               return DIKS_SLASH;

          case DIKI_KP_MULT:
               return DIKS_ASTERISK;

          case DIKI_KP_MINUS:
               return DIKS_MINUS_SIGN;

          case DIKI_KP_PLUS:
               return DIKS_PLUS_SIGN;

          case DIKI_KP_ENTER:
               return DIKS_ENTER;

          case DIKI_KP_SPACE:
               return DIKS_SPACE;

          case DIKI_KP_TAB:
               return DIKS_TAB;

          case DIKI_KP_EQUAL:
               return DIKS_EQUALS_SIGN;

          case DIKI_KP_DECIMAL:
               return DIKS_PERIOD;

          case DIKI_KP_SEPARATOR:
               return DIKS_COMMA;

          case DIKI_BACKSLASH:
               return DIKS_BACKSLASH;

          case DIKI_EQUALS_SIGN:
               return DIKS_EQUALS_SIGN;

          case DIKI_LESS_SIGN:
               return DIKS_LESS_THAN_SIGN;

          case DIKI_MINUS_SIGN:
               return DIKS_MINUS_SIGN;

          case DIKI_PERIOD:
               return DIKS_PERIOD;

          case DIKI_QUOTE_LEFT:
          case DIKI_QUOTE_RIGHT:
               return DIKS_QUOTATION;

          case DIKI_SEMICOLON:
               return DIKS_SEMICOLON;

          case DIKI_SLASH:
               return DIKS_SLASH;

          default:
               ;
     }

     return DIKS_NULL;
}

static void
release_key( CoreInputDevice *device, DFBInputDeviceKeyIdentifier id )
{
     DFBInputEvent evt;

     D_MAGIC_ASSERT( device, CoreInputDevice );

     evt.type = DIET_KEYRELEASE;

     if (DFB_KEY_TYPE(id) == DIKT_IDENTIFIER) {
          evt.flags  = DIEF_KEYID;
          evt.key_id = id;
     }
     else {
          evt.flags      = DIEF_KEYSYMBOL;
          evt.key_symbol = id;
     }

     dfb_input_dispatch( device, &evt );
}

static void
flush_keys( CoreInputDevice *device )
{
     D_MAGIC_ASSERT( device, CoreInputDevice );

     if (device->shared->state.modifiers_l) {
          if (device->shared->state.modifiers_l & DIMM_ALT)
               release_key( device, DIKI_ALT_L );

          if (device->shared->state.modifiers_l & DIMM_CONTROL)
               release_key( device, DIKI_CONTROL_L );

          if (device->shared->state.modifiers_l & DIMM_HYPER)
               release_key( device, DIKI_HYPER_L );

          if (device->shared->state.modifiers_l & DIMM_META)
               release_key( device, DIKI_META_L );

          if (device->shared->state.modifiers_l & DIMM_SHIFT)
               release_key( device, DIKI_SHIFT_L );

          if (device->shared->state.modifiers_l & DIMM_SUPER)
               release_key( device, DIKI_SUPER_L );
     }

     if (device->shared->state.modifiers_r) {
          if (device->shared->state.modifiers_r & DIMM_ALTGR)
               release_key( device, DIKS_ALTGR );

          if (device->shared->state.modifiers_r & DIMM_ALT)
               release_key( device, DIKI_ALT_R );

          if (device->shared->state.modifiers_r & DIMM_CONTROL)
               release_key( device, DIKI_CONTROL_R );

          if (device->shared->state.modifiers_r & DIMM_HYPER)
               release_key( device, DIKI_HYPER_R );

          if (device->shared->state.modifiers_r & DIMM_META)
               release_key( device, DIKI_META_R );

          if (device->shared->state.modifiers_r & DIMM_SHIFT)
               release_key( device, DIKI_SHIFT_R );

          if (device->shared->state.modifiers_r & DIMM_SUPER)
               release_key( device, DIKI_SUPER_R );
     }
}

static void
dump_primary_layer_surface( CoreDFB *core )
{
     CoreLayer        *layer = dfb_layer_at( DLID_PRIMARY );
     CoreLayerContext *context;

     /* Get the currently active context. */
     if (dfb_layer_get_active_context( layer, &context ) == DFB_OK) {
          CoreLayerRegion *region;

          /* Get the first region. */
          if (dfb_layer_context_get_primary_region( context,
                                                    false, &region ) == DFB_OK)
          {
               CoreSurface *surface;

               /* Lock the region to avoid tearing due to concurrent updates. */
               dfb_layer_region_lock( region );

               /* Get the surface of the region. */
               if (dfb_layer_region_get_surface( region, &surface ) == DFB_OK) {
                    /* Dump the surface contents. */
                    dfb_surface_dump_buffer( surface, CSBR_FRONT, dfb_config->screenshot_dir, "dfb" );

                    /* Release the surface. */
                    dfb_surface_unref( surface );
               }

               /* Unlock the region. */
               dfb_layer_region_unlock( region );

               /* Release the region. */
               dfb_layer_region_unref( region );
          }

          /* Release the context. */
          dfb_layer_context_unref( context );
     }
}

static bool
core_input_filter( CoreInputDevice *device, DFBInputEvent *event )
{
     D_MAGIC_ASSERT( device, CoreInputDevice );

     if (dfb_system_input_filter( device, event ))
          return true;

     if (event->type == DIET_KEYPRESS) {
          switch (event->key_symbol) {
               case DIKS_PRINT:
                    if (!event->modifiers && dfb_config->screenshot_dir) {
                         dump_primary_layer_surface( device->core );
                         return true;
                    }
                    break;

               case DIKS_BACKSPACE:
                    if (event->modifiers == DIMM_META)
                         direct_trace_print_stacks();

                    break;

               case DIKS_ESCAPE:
                    if (event->modifiers == DIMM_META) {
#if FUSION_BUILD_MULTI
                         DFBResult         ret;
                         CoreLayer        *layer = dfb_layer_at( DLID_PRIMARY );
                         CoreLayerContext *context;

                         /* Get primary (shared) context. */
                         ret = dfb_layer_get_primary_context( layer,
                                                              false, &context );
                         if (ret)
                              return false;

                         /* Activate the context. */
                         dfb_layer_activate_context( layer, context );

                         /* Release the context. */
                         dfb_layer_context_unref( context );

#else
                         direct_kill( 0, SIGINT );
#endif

                         return true;
                    }
                    break;

               default:
                    break;
          }
     }

     return false;
}

