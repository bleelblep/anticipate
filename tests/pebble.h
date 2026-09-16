#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef int TimeUnits;
#define MINUTE_UNIT 1
typedef struct { int x,y; } GPoint;
typedef struct { int w,h; } GSize;
typedef struct { GPoint origin; GSize size; } GRect;
#define GRect(x,y,w,h) ((GRect){{x,y},{w,h}})
typedef struct { int unused; } Layer;
typedef struct { int unused; } Window;
typedef struct { int unused; } GContext;
typedef struct { bool active; void (*callback)(void*); void *context; uint32_t delay; } AppTimer;
typedef int AccelAxisType;
typedef struct { int charge_percent; bool is_charging, is_plugged; } BatteryChargeState;
typedef struct { void (*load)(Window*); void (*unload)(Window*); } WindowHandlers;
enum { TUPLE_CSTRING, TUPLE_INT, TUPLE_UINT };
typedef union { char cstring[32]; int32_t int32; uint32_t uint32; } TupleValue;
typedef struct { uint32_t key; int type; TupleValue *value; } Tuple;
typedef struct { Tuple *tuples; int count; } DictionaryIterator;
enum { MESSAGE_KEY_BatteryMode, MESSAGE_KEY_BatterySeconds, MESSAGE_KEY_CustomBacklight,
 MESSAGE_KEY_BacklightRed, MESSAGE_KEY_BacklightGreen, MESSAGE_KEY_BacklightBlue };
#define GColorBlack 0
#define GColorWhite 1
#define GCornerNone 0
void graphics_fill_rect(GContext*,GRect,int,int);
void graphics_context_set_fill_color(GContext*,int);
GRect layer_get_bounds(Layer*);
bool clock_is_24h_style(void);
void layer_mark_dirty(Layer*);
Layer *window_get_root_layer(Window*);
Layer *layer_create(GRect);
void layer_set_update_proc(Layer*,void (*)(Layer*,GContext*));
void layer_add_child(Layer*,Layer*);
void layer_destroy(Layer*);
Window *window_create(void);
void window_set_background_color(Window*,int);
void window_set_window_handlers(Window*,WindowHandlers);
void window_stack_push(Window*,bool);
void tick_timer_service_subscribe(TimeUnits,void (*)(struct tm*,TimeUnits));
void app_event_loop(void);
void tick_timer_service_unsubscribe(void);
void window_destroy(Window*);
AppTimer *app_timer_register(uint32_t,void (*)(void*),void*);
void app_timer_cancel(AppTimer*);
void light_set_color_rgb888(uint32_t);
void light_set_system_color(void);
void accel_tap_service_subscribe(void (*)(AccelAxisType,int32_t));
void accel_tap_service_unsubscribe(void);
bool persist_exists(int);
int persist_read_int(int);
int persist_write_int(int,int);
Tuple *dict_find(DictionaryIterator*,uint32_t);
BatteryChargeState battery_state_service_peek(void);
void battery_state_service_subscribe(void (*)(BatteryChargeState));
void battery_state_service_unsubscribe(void);
void app_focus_service_subscribe(void (*)(bool));
void app_focus_service_unsubscribe(void);
void app_message_register_inbox_received(void (*)(DictionaryIterator*,void*));
void app_message_open(int,int);
void app_message_deregister_callbacks(void);
