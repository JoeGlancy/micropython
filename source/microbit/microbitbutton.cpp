/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "MicroBit.h"

extern "C" {

#include "py/runtime.h"
#include "modmicrobit.h"

typedef struct _microbit_button_obj_t {
    mp_obj_base_t base;
    MicroBitButton *button;
    /* Stores pressed count in top 31 bits and was_pressed in the low bit */
    mp_uint_t pressed;
    /* Stores the system clock at the time when this button was pushed down.
     * if the value is 0, it means that the button has since been let go (or
     * has never been pressed).
     */
    unsigned long button_down_start_time;
    uint8_t event_states;
} microbit_button_obj_t;

mp_obj_t microbit_button_is_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    return mp_obj_new_bool(self->button->isPressed());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_is_pressed_obj, microbit_button_is_pressed);

#ifdef MICROBIT_BUTTON_INCLUDE_IS_LONG_PRESSED
mp_obj_t microbit_button_is_long_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    if (self->button_down_start_time == 0) {
        return mp_obj_new_bool(0);
    }
    uint8_t was_long_pressed = (ticks > self->button_down_start_time + 1000) ? 1 : 0;
    if (was_long_pressed && !self->button->isPressed()) {
        /* Only reset the counter if the button has been let go */
        self->button_down_start_time = 0;
    }
    return mp_obj_new_bool((int) was_long_pressed);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_is_long_pressed_obj, microbit_button_is_long_pressed);
#endif

mp_obj_t microbit_button_get_presses(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    mp_obj_t n_presses = mp_obj_new_int(self->pressed >> 1);
    self->pressed &= 1;
    return n_presses;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_get_presses_obj, microbit_button_get_presses);

mp_obj_t microbit_button_was_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    mp_int_t pressed = self->pressed;
    mp_obj_t result = mp_obj_new_bool(pressed & 1);
    self->pressed = pressed & -2;
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_was_pressed_obj, microbit_button_was_pressed);

mp_obj_t button_was_event(microbit_button_obj_t *self, uint8_t val) {
    uint8_t b = self->event_states & (1 << val);
    if (b) {
        self->event_states ^= (1 << val);
        return mp_const_true;
    }
    return mp_const_false;
}

#ifdef MICROBIT_BUTTON_INCLUDE_WAS_CLICKED
mp_obj_t microbit_button_was_clicked(mp_obj_t self_in) {
    return button_was_event((microbit_button_obj_t*) self_in, MICROBIT_BUTTON_EVT_CLICK);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_was_clicked_obj, microbit_button_was_clicked);
#endif

#ifdef MICROBIT_BUTTON_INCLUDE_WAS_DOUBLE_CLICKED
mp_obj_t microbit_button_was_double_clicked(mp_obj_t self_in) {
    return button_was_event((microbit_button_obj_t*) self_in, MICROBIT_BUTTON_EVT_DOUBLE_CLICK);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_was_double_clicked_obj, microbit_button_was_double_clicked);
#endif

#ifdef MICROBIT_BUTTON_INCLUDE_WAS_LONG_CLICKED
mp_obj_t microbit_button_was_long_clicked(mp_obj_t self_in) {
    return button_was_event((microbit_button_obj_t*) self_in, MICROBIT_BUTTON_EVT_LONG_CLICK);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_was_long_clicked_obj, microbit_button_was_long_clicked);
#endif

#ifdef MICROBIT_BUTTON_INCLUDE_WAS_HOLD
mp_obj_t microbit_button_was_hold(mp_obj_t self_in) {
    return button_was_event((microbit_button_obj_t*) self_in, MICROBIT_BUTTON_EVT_HOLD);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_was_hold_obj, microbit_button_was_hold);
#endif

void button_event_listener(MicroBitEvent *evt, microbit_button_obj_t *button_obj) {
    if (evt->value >= MICROBIT_BUTTON_EVT_DOWN && evt->value <= MICROBIT_BUTTON_EVT_DOUBLE_CLICK) {
        if (evt->value == MICROBIT_BUTTON_EVT_DOWN) {
            button_obj->button_down_start_time = ticks;
            button_obj->pressed = (button_obj->pressed + 2) | 1;
        }
        button_obj->event_states |= 1 << evt->value;
    }
}

void button_a_listener(MicroBitEvent evt) {
    button_event_listener(&evt, &microbit_button_a_obj);
}

void button_b_listener(MicroBitEvent evt) {
    button_event_listener(&evt, &microbit_button_b_obj);
}

void microbit_button_init(void) {
    uBit.MessageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_EVT_ANY, button_a_listener, MESSAGE_BUS_LISTENER_IMMEDIATE);
    uBit.MessageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_EVT_ANY, button_b_listener, MESSAGE_BUS_LISTENER_IMMEDIATE);
}

STATIC const mp_map_elem_t microbit_button_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_pressed),          (mp_obj_t)&microbit_button_is_pressed_obj },
#ifdef MICROBIT_BUTTON_INCLUDE_IS_LONG_PRESSED
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_long_pressed),     (mp_obj_t)&microbit_button_is_long_pressed_obj },
#endif
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_pressed),         (mp_obj_t)&microbit_button_was_pressed_obj },
#ifdef MICROBIT_BUTTON_INCLUDE_WAS_CLICKED
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_clicked),         (mp_obj_t)&microbit_button_was_clicked_obj },
#endif
#ifdef MICROBIT_BUTTON_INCLUDE_WAS_DOUBLE_CLICKED
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_double_clicked),  (mp_obj_t)&microbit_button_was_double_clicked_obj },
#endif
#ifdef MICROBIT_BUTTON_INCLUDE_WAS_LONG_CLICKED
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_long_clicked),    (mp_obj_t)&microbit_button_was_long_clicked_obj },
#endif
#ifdef MICROBIT_BUTTON_INCLUDE_WAS_HOLD
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_hold),            (mp_obj_t)&microbit_button_was_hold_obj },
#endif
};

STATIC MP_DEFINE_CONST_DICT(microbit_button_locals_dict, microbit_button_locals_dict_table);

STATIC const mp_obj_type_t microbit_button_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitButton,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&microbit_button_locals_dict,
};

microbit_button_obj_t microbit_button_a_obj = {
    {&microbit_button_type},
    .button = &uBit.buttonA,
    .pressed = 0,
    .button_down_start_time = 0,
    .event_states = 0
};

microbit_button_obj_t microbit_button_b_obj = {
    {&microbit_button_type},
    .button = &uBit.buttonB,
    .pressed = 0,
    .button_down_start_time = 0,
    .event_states = 0
};

}
