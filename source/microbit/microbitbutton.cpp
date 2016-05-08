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

#define EVENT_LIST_SIZE (16)

enum button_event {
    BTN_EVENT_DOWN,
    BTN_EVENT_UP,
    BTN_EVENT_CLICK,
    BTN_EVENT_LONG_CLICK,
    BTN_EVENT_HOLD,
    BTN_EVENT_DOUBLE_CLICK
};

typedef struct _microbit_button_obj_t {
    mp_obj_base_t base;
    MicroBitButton *button;
    /* Stores pressed count in top 31 bits and was_pressed in the low bit */
    mp_uint_t pressed;
    /* Stores the system clock at the time when this button was pushed down.
     * if the value is 0, it means that the button has since been let go (or
     * has never been pressed).
     */
    unsigned long downStartTime;
    uint8_t event_states;
    uint8_t event_current;
    uint8_t event_list[EVENT_LIST_SIZE];
    uint8_t last_event;
} microbit_button_obj_t;

void update_button_state(microbit_button_obj_t *button_obj) {
    // update the button state (shouldn't be doing it this way; TODO fix)
    button_obj->button->systemTick();
}

mp_obj_t microbit_button_is_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    return mp_obj_new_bool(self->button->isPressed());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_is_pressed_obj, microbit_button_is_pressed);

mp_obj_t microbit_button_is_long_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    if (self->downStartTime == 0) {
        return mp_obj_new_bool(0);
    }
    int wasLongPressed = (ticks > self->downStartTime + 1000) ? 1 : 0;
    if (wasLongPressed && !self->button->isPressed()) {
        /* Only reset the counter if the button has been let go */
        self->downStartTime = 0;
    }
    return mp_obj_new_bool(wasLongPressed);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_is_long_pressed_obj, microbit_button_is_long_pressed);


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

STATIC const qstr button_event_name_map[] = {
    [BTN_EVENT_DOWN] = MP_QSTR_down,
    [BTN_EVENT_UP] = MP_QSTR_up,
    [BTN_EVENT_CLICK] = MP_QSTR_click,
    [BTN_EVENT_LONG_CLICK] = MP_QSTR_long_space_click,
    [BTN_EVENT_HOLD] = MP_QSTR_hold,
    [BTN_EVENT_DOUBLE_CLICK] = MP_QSTR_double_space_click
};

STATIC button_event get_button_event_from_str(mp_obj_t event_in) {
    qstr event = mp_obj_str_get_qstr(event_in);
    for (uint8_t i=0; i<MP_ARRAY_SIZE(button_event_name_map); i++) {
        if (event == button_event_name_map[i]) {
            return (button_event) i;
        }
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "invalid event"));
}

mp_obj_t microbit_button_current_event(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*) self_in;
    self->last_event = 0;
    update_button_state(self);
    // events should have been raised and processed by the time we get here.
    if (self->last_event == 0) {
        return MP_OBJ_NEW_QSTR(MP_QSTR_NULL);
    }
    return MP_OBJ_NEW_QSTR(button_event_name_map[self->last_event]);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_current_event_obj, microbit_button_current_event);

mp_obj_t microbit_button_is_event(mp_obj_t self_in, mp_obj_t event_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*) self_in;
    self->last_event = 0;
    update_button_state(self);
    if (self->last_event == 0) {
        return mp_const_false;
    }
    button_event event = get_button_event_from_str(event_in);
    return mp_obj_new_bool(self->last_event == event);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_button_is_event_obj, microbit_button_is_event);

mp_obj_t microbit_button_was_event(mp_obj_t self_in, mp_obj_t event_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*) self_in;
    button_event event = get_button_event_from_str(event_in);
    update_button_state(self);
    mp_obj_t result = mp_obj_new_bool(self->event_states & (1 << event));
    self->event_states &= (~(1 << event));
    self->event_current = 0;
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_button_was_event_obj, microbit_button_was_event);

mp_obj_t microbit_button_get_events(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*) self_in;
    update_button_state(self);
    if (self->event_current == 0) {
        return mp_const_empty_tuple;
    }
    mp_obj_tuple_t *o = (mp_obj_tuple_t*) mp_obj_new_tuple(self->event_current, NULL);
    for (uint8_t i=0; i<self->event_current; i++) {
        uint8_t event = (self->event_list[i >> 1] >> (4 * (i & 1))) & 0x0f;
        o->items[i] = MP_OBJ_NEW_QSTR(button_event_name_map[event]);
    }
    self->event_current = 0;
    return o;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_get_events_obj, microbit_button_get_events);

void button_event_listener(MicroBitEvent *evt, microbit_button_obj_t *button_obj) {
    if (evt->value >= MICROBIT_BUTTON_EVT_DOWN && evt->value <= MICROBIT_BUTTON_EVT_DOUBLE_CLICK) {
        button_obj->last_event = evt->value;
        printf("[button event] %d\r\n", (int) evt->value);
        if (evt->value == MICROBIT_BUTTON_EVT_DOWN) {
            button_obj->downStartTime = ticks;
            button_obj->pressed = (button_obj->pressed + 2) | 1;
        }
        button_obj->event_states |= 1 << evt->value;
        if (button_obj->event_current < 2 * EVENT_LIST_SIZE) {
            button_obj->event_list[button_obj->event_current >> 1] |= evt->value << (4 * (button_obj->event_current & 1));
            button_obj->event_current++;
        }
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
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_pressed),      (mp_obj_t)&microbit_button_is_pressed_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_long_pressed), (mp_obj_t)&microbit_button_is_long_pressed_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_pressed),     (mp_obj_t)&microbit_button_was_pressed_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_current_event),   (mp_obj_t)&microbit_button_current_event_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_event),        (mp_obj_t)&microbit_button_is_event_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_event),       (mp_obj_t)&microbit_button_was_event_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_events),      (mp_obj_t)&microbit_button_get_events_obj },
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
    .downStartTime = 0,
    .event_states = 0,
    .event_current = 0,
    .event_list = {0},
    .last_event = 0
};

microbit_button_obj_t microbit_button_b_obj = {
    {&microbit_button_type},
    .button = &uBit.buttonB,
    .pressed = 0,
    .downStartTime = 0,
    .event_states = 0,
    .event_current = 0,
    .event_list = {0},
    .last_event = 0
};

}
