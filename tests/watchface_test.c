// Host-side SDK mock: checks geometry and callback lifecycles, not hardware APIs.
#include <assert.h>
#define main watch_main
#include "../src/c/anticipate.c"
#undef main
static AppTimer timers[4];
static bool fail_timers, use_24h, subscribed;
static int color, rgb_calls, system_calls, saved[200];
static uint32_t rgb;
static int previews;
static unsigned char pixels[168][144];
void graphics_context_set_fill_color(GContext *ctx,int c) { (void)ctx; color=c; }
void graphics_fill_rect(GContext *ctx,GRect r,int radius,int corners) {
 (void)ctx;(void)radius;(void)corners;
 if(color) {
  assert(r.origin.x>=4 && r.origin.y>=4);
  assert(r.origin.x+r.size.w<=140 && r.origin.y+r.size.h<=164);
 }
 for(int y=r.origin.y;y<r.origin.y+r.size.h;y++)
  for(int x=r.origin.x;x<r.origin.x+r.size.w;x++) pixels[y][x]=color?255:0;
}
GRect layer_get_bounds(Layer *l) { (void)l; return GRect(0,0,144,168); }
bool clock_is_24h_style(void) { return use_24h; }
void layer_mark_dirty(Layer *l) { (void)l; }
AppTimer *app_timer_register(uint32_t delay,void (*callback)(void*),void *context) {
 if(fail_timers) return NULL;
 for(int i=0;i<4;i++) if(!timers[i].active) {
  timers[i]=(AppTimer){true,callback,context,delay}; return &timers[i];
 }
 assert(false);return NULL;
}
void app_timer_cancel(AppTimer *t) { t->active=false; }
static void fire(AppTimer *t) {
 assert(t && t->active);void (*fn)(void*)=t->callback;void *ctx=t->context;
 t->active=false;fn(ctx);
}
static void finish_animation(void) { while(s_frame_timer) fire(s_frame_timer); }
void accel_tap_service_subscribe(void (*f)(AccelAxisType,int32_t)) { (void)f; subscribed=true; }
void accel_tap_service_unsubscribe(void) { subscribed=false; }
void light_set_color_rgb888(uint32_t c) { rgb=c;rgb_calls++; }
void light_set_system_color(void) { system_calls++; }
int persist_write_int(int k,int v) { saved[k]=v;return 4; }
Tuple *dict_find(DictionaryIterator *it,uint32_t key) {
 for(int i=0;i<it->count;i++) if(it->tuples[i].key==key) return &it->tuples[i];
 return NULL;
}
static void send_int(uint32_t key,int value) {
 TupleValue v={.int32=value};Tuple t={key,TUPLE_INT,&v};DictionaryIterator it={&t,1};inbox(&it,NULL);
}
static void send_string(uint32_t key,const char *value) {
 TupleValue v;snprintf(v.cstring,sizeof(v.cstring),"%s",value);
 Tuple t={key,TUPLE_CSTRING,&v};DictionaryIterator it={&t,1};inbox(&it,NULL);
}
int app_message_outbox_begin(DictionaryIterator **it) { *it=NULL;return 1; }
int app_message_outbox_send(void) { return 0; }
void dict_write_uint8(DictionaryIterator *it,uint32_t k,uint8_t v) {(void)it;(void)k;(void)v;}
int persist_write_data(int k,const void *v,size_t n) {(void)k;(void)v;return n;}
void light_enable_interaction(void) {previews++;}
int main(void) {
 for(s_style=0;s_style<3;s_style++) for(int mode=0;mode<2;mode++) {
  use_24h=mode;
  for(int h=0;h<24;h++) for(int m=0;m<60;m++) {
   struct tm t={.tm_hour=h,.tm_min=m};tick(&t,MINUTE_UNIT);
   assert(s_hour==(mode?h:(h%12?h%12:12)) && s_minute==m);
   for(s_progress=0;s_progress<=1000;s_progress+=100) draw(NULL,NULL);
  }
 }
 s_style=0;
 for(s_percent=0;s_percent<=100;s_percent++)
  for(s_progress=0;s_progress<=1000;s_progress+=10) draw(NULL,NULL);
 s_percent=75;s_progress=0;s_mode=MODE_FLICK;subscribe_motion();assert(subscribed);
 wrist_flick(0,1);assert(s_hide_timer->delay==5330);finish_animation();assert(s_progress==1000);
 fire(s_hide_timer);fire(s_frame_timer);assert(s_progress<1000);
 int interrupted=s_progress;wrist_flick(0,1);assert(s_progress==interrupted);
 finish_animation();assert(s_progress==1000);fire(s_hide_timer);finish_animation();assert(s_progress==0);
 send_string(MESSAGE_KEY_BatterySeconds,"10");assert(s_seconds==10 && saved[102]==10);
 send_string(MESSAGE_KEY_BatterySeconds,"bad");assert(s_seconds==10);
 send_string(MESSAGE_KEY_BatterySeconds,"999999999999999999999");assert(s_seconds==10);
 send_int(MESSAGE_KEY_BacklightRed,123);send_int(MESSAGE_KEY_BacklightGreen,45);
 send_int(MESSAGE_KEY_BacklightBlue,67);send_int(MESSAGE_KEY_CustomBacklight,1);
#ifdef PBL_RGB_BACKLIGHT
 assert(rgb==0x7B2D43 && rgb_calls>0 && previews>0);
 int previous=rgb_calls;backlight_changed(true);assert(rgb_calls==previous+1);
#endif
 wrist_flick(0,1);finish_animation();focus_changed(false);
 assert(!s_frame_timer && !s_hide_timer && !subscribed && s_progress==0);
 int calls=rgb_calls;focus_changed(true);assert(subscribed);
#ifdef PBL_RGB_BACKLIGHT
 assert(rgb_calls==calls+1);
#else
 assert(rgb_calls==calls);
#endif
 send_string(MESSAGE_KEY_BatteryMode,"1");finish_animation();assert(s_progress==1000 && !subscribed);
 wrist_flick(0,1);assert(!s_hide_timer);
 send_int(MESSAGE_KEY_BatteryMode,0);finish_animation();assert(s_progress==0 && !subscribed);
 send_int(MESSAGE_KEY_CustomBacklight,0);
#ifdef PBL_RGB_BACKLIGHT
 assert(system_calls>0);
#else
 assert(system_calls==0 && rgb_calls==0);
#endif
 fail_timers=true;s_mode=MODE_FLICK;wrist_flick(0,1);assert(s_progress==0);
 // Weather arrives without hiding details or triggering a light preview.
 fail_timers=false;s_mode=MODE_FLICK;wrist_flick(0,1);finish_animation();
 AppTimer *hold=s_hide_timer;int preview_count=previews;
 TupleValue vals[5]={{.int32=25},{.int32=20},{.int32=12},{.int32=0},{.int32=(int)time(NULL)}};
 uint32_t keys[5]={MESSAGE_KEY_TEMP_HI,MESSAGE_KEY_TEMP_CUR,MESSAGE_KEY_TEMP_LO,MESSAGE_KEY_CONDITIONS,MESSAGE_KEY_WEATHER_AT};
 Tuple tuples[5];for(int i=0;i<5;i++) tuples[i]=(Tuple){keys[i],TUPLE_INT,&vals[i]};
 DictionaryIterator weather={tuples,5};inbox(&weather,NULL);
 assert(s_progress==1000 && s_hide_timer==hold && previews==preview_count && s_weather[1]==20);
 s_hour=12;s_minute=34;s_date.tm_mday=16;s_date.tm_mon=8;s_date.tm_wday=3;s_percent=75;s_progress=1000;draw(NULL,NULL);
 FILE *f=fopen("/tmp/anticipate-battery-preview.pgm","wb");assert(f);
 fprintf(f,"P5\n144 168\n255\n");fwrite(pixels,1,sizeof(pixels),f);fclose(f);
 for(s_style=0;s_style<3;s_style++) {
  draw(NULL,NULL);char name[80];snprintf(name,sizeof(name),"/tmp/anticipate-style-%d.pgm",s_style);
  f=fopen(name,"wb");fprintf(f,"P5\n144 168\n255\n");fwrite(pixels,1,sizeof(pixels),f);fclose(f);
 }
 puts("PASS: time formats, animation bounds, all battery levels, flick/reversal, settings, focus, RGB and timer failure.");
}
