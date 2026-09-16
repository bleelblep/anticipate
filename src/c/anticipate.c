#include <pebble.h>
#include <stdlib.h>
#include "time_digits.h"

// Both states retain the original centred 144x168 footprint and four-pixel inset.
enum { FACE_WIDTH=144, FACE_HEIGHT=168, RIGHT=140, BOTTOM=164,
       MODE_OFF=0, MODE_ALWAYS=1, MODE_FLICK=2, FRAME_MS=33, FRAMES=10 };
static Window *s_window;
static Layer *s_layer;
static AppTimer *s_frame_timer, *s_hide_timer;
static int s_hour, s_minute, s_percent;
static int s_progress, s_start, s_target, s_frame;
static int s_mode=MODE_FLICK, s_seconds=5, s_red=255, s_green=255, s_blue=255;
static bool s_custom_light, s_focused=true, s_tap_subscribed;

static int clamp(int n,int lo,int hi) { return n<lo?lo:(n>hi?hi:n); }
static void dirty(void) { if(s_layer) layer_mark_dirty(s_layer); }
static void cancel_timer(AppTimer **timer) {
  if(*timer) { app_timer_cancel(*timer); *timer=NULL; }
}
static void apply_light(void) {
#ifdef PBL_PLATFORM_EMERY
  if(s_custom_light) light_set_color_rgb888(((uint32_t)s_red<<16)|((uint32_t)s_green<<8)|s_blue);
  else light_set_system_color();
#endif
}
// Explicit clipping prevents sliding content touching the original outer margins.
static void rect(GContext *ctx,int x,int y,int w,int h) {
  int right=clamp(x+w,4,RIGHT), bottom=clamp(y+h,4,BOTTOM);
  x=clamp(x,4,RIGHT); y=clamp(y,4,BOTTOM);
  if(right>x && bottom>y) graphics_fill_rect(ctx,GRect(x,y,right-x,bottom-y),0,GCornerNone);
}
static bool digit_pixel(int digit,int x,int y) {
  return (DIGIT_ROWS[digit][y][x/8] & (1<<(x%8)))!=0;
}
static void draw_digit(GContext *ctx,int digit,int x,int y,int w,int h) {
  for(int sy=0;sy<71;sy++) {
    int top=y+sy*h/71, bottom=y+(sy+1)*h/71;
    for(int sx=0;sx<DIGIT_WIDTHS[digit];) {
      if(!digit_pixel(digit,sx,sy)) { sx++; continue; }
      int start=sx;
      while(sx<DIGIT_WIDTHS[digit] && digit_pixel(digit,sx,sy)) sx++;
      int left=x+start*w/46, right=x+sx*w/46;
      rect(ctx,left,top,right-left,bottom-top);
    }
  }
}
static void draw_pair(GContext *ctx,int value,int y,int w,int h) {
  int ones=value%10, tens=value/10;
  int ones_x=RIGHT-DIGIT_WIDTHS[ones]*w/46;
  draw_digit(ctx,ones,ones_x,y,w,h);
  draw_digit(ctx,tens,ones_x-4-DIGIT_WIDTHS[tens]*w/46,y,w,h);
}
// A compact 3x5 pixel alphabet for the gauge's small labels (last glyph is %).
static const uint8_t SMALL[11][5]={
 {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
 {7,4,7,1,7},{7,4,7,5,7},{7,1,2,2,2},{7,5,7,5,7},{7,5,7,1,7},
 {5,1,2,4,5}
};
static void small_text(GContext *ctx,const char *s,int x,int y) {
  for(;*s;s++,x+=4) {
    int d=*s=='%'?10:*s-'0';
    if(d<0 || d>10) continue;
    for(int row=0;row<5;row++) for(int col=0;col<3;col++)
      if(SMALL[d][row] & (1<<(2-col))) rect(ctx,x+col,y+row,1,1);
  }
}
static void draw_battery(GContext *ctx,int top) {
  // Track spans x=4..139; marker follows the fill boundary, including 0/100%.
  rect(ctx,4,top,136,1); rect(ctx,4,top+3,136,1);
  rect(ctx,4,top,1,4); rect(ctx,139,top,1,4);
  int fill=134*s_percent/100, marker=clamp(5+fill,4,139);
  rect(ctx,5,top+1,fill,2);
  // Upward triangle below the track, with one clear row between them.
  rect(ctx,marker,top+5,1,1);
  rect(ctx,marker-1,top+6,3,1);
  rect(ctx,marker-2,top+7,5,1);
  char label[5]; snprintf(label,sizeof(label),"%d%%",s_percent);
  int width=(int)strlen(label)*4-1;
  int label_x=clamp(marker-width/2,4,RIGHT-width);
  small_text(ctx,label,label_x,top+10);
  // Near endpoints, the percentage itself replaces the endpoint label.
  if(label_x>11) small_text(ctx,"0",4,top+10);
  if(label_x+width<125) small_text(ctx,"100",129,top+10);
}
static void draw(Layer *layer,GContext *ctx) {
  graphics_context_set_fill_color(ctx,GColorBlack);
  graphics_fill_rect(ctx,layer_get_bounds(layer),0,GCornerNone);
  graphics_context_set_fill_color(ctx,GColorWhite);
  int w=66-10*s_progress/1000, h=78-12*s_progress/1000;
  draw_pair(ctx,s_hour,4,w,h); draw_pair(ctx,s_minute,8+h,w,h);
  if(s_progress) draw_battery(ctx,169-24*s_progress/1000);
}
static void animate_frame(void *context) {
  (void)context; s_frame_timer=NULL;
  s_frame++;
  // Smoothstep ease-in/out, with no floating point or continuous idle timer.
  int t=clamp(s_frame*1000/FRAMES,0,1000);
  int ease=(int)((int64_t)t*t*(3000-2*t)/1000000);
  s_progress=s_start+(s_target-s_start)*ease/1000;
  dirty();
  if(s_frame<FRAMES) {
    s_frame_timer=app_timer_register(FRAME_MS,animate_frame,NULL);
    if(!s_frame_timer) { s_progress=s_target; dirty(); }
  }
}
static void transition(bool visible) {
  cancel_timer(&s_frame_timer);
  s_target=visible?1000:0; s_start=s_progress; s_frame=0;
  if(s_start==s_target) return;
  s_frame_timer=app_timer_register(FRAME_MS,animate_frame,NULL);
  if(!s_frame_timer) { s_progress=s_target; dirty(); }
}
static void hide_battery(void *context) {
  (void)context; s_hide_timer=NULL;
  if(s_mode==MODE_FLICK) transition(false);
}
static void wrist_flick(AccelAxisType axis,int32_t direction) {
  (void)axis; (void)direction;
  if(!s_focused || s_mode!=MODE_FLICK) return;
  cancel_timer(&s_hide_timer);
  transition(true);
  // Restart the hold interval on another flick. Include the reveal duration.
  s_hide_timer=app_timer_register(s_seconds*1000+FRAME_MS*FRAMES,hide_battery,NULL);
  if(!s_hide_timer) transition(false);
}
static void subscribe_motion(void) {
  bool wanted=s_focused && s_mode==MODE_FLICK;
  if(wanted && !s_tap_subscribed) accel_tap_service_subscribe(wrist_flick);
  else if(!wanted && s_tap_subscribed) accel_tap_service_unsubscribe();
  s_tap_subscribed=wanted;
}
static void focus_changed(bool focused) {
  s_focused=focused;
  cancel_timer(&s_hide_timer); cancel_timer(&s_frame_timer);
  s_progress=s_target=s_mode==MODE_ALWAYS?1000:0;
  subscribe_motion();
  if(focused) { apply_light(); dirty(); }
}
static void battery_changed(BatteryChargeState state) {
  s_percent=clamp(state.charge_percent,0,100); dirty();
}
static void tick(struct tm *t,TimeUnits changed) {
  (void)changed; s_hour=t->tm_hour;
  if(!clock_is_24h_style()) { s_hour%=12; if(!s_hour) s_hour=12; }
  s_minute=t->tm_min; dirty();
}
// Independent persistence keys avoid interpreting the upstream settings struct.
static int read_setting(int key,int fallback,int lo,int hi) {
  return persist_exists(key)?clamp(persist_read_int(key),lo,hi):fallback;
}
static void receive_setting(DictionaryIterator *iter,uint32_t key,int storage,
                            int *value,int lo,int hi) {
  Tuple *tuple=dict_find(iter,key);
  if(!tuple) return;
  int n;
  if(tuple->type==TUPLE_CSTRING) {
    char *end; long parsed=strtol(tuple->value->cstring,&end,10);
    if(end==tuple->value->cstring || *end || parsed<lo || parsed>hi) return;
    n=(int)parsed;
  } else if(tuple->type==TUPLE_INT) n=clamp(tuple->value->int32,lo,hi);
  else if(tuple->type==TUPLE_UINT) n=tuple->value->uint32>(uint32_t)hi?hi:clamp((int)tuple->value->uint32,lo,hi);
  else return;
  *value=n; persist_write_int(storage,n);
}
static void inbox(DictionaryIterator *iter,void *context) {
  (void)context;
  receive_setting(iter,MESSAGE_KEY_BatteryMode,101,&s_mode,0,2);
  receive_setting(iter,MESSAGE_KEY_BatterySeconds,102,&s_seconds,2,30);
  int enabled=s_custom_light;
  receive_setting(iter,MESSAGE_KEY_CustomBacklight,103,&enabled,0,1);
  s_custom_light=enabled!=0;
  receive_setting(iter,MESSAGE_KEY_BacklightRed,104,&s_red,0,255);
  receive_setting(iter,MESSAGE_KEY_BacklightGreen,105,&s_green,0,255);
  receive_setting(iter,MESSAGE_KEY_BacklightBlue,106,&s_blue,0,255);
  cancel_timer(&s_hide_timer); subscribe_motion();
  transition(s_mode==MODE_ALWAYS);
  if(s_focused) apply_light();
}
static void window_load(Window *window) {
  Layer *root=window_get_root_layer(window); GRect bounds=layer_get_bounds(root);
  s_layer=layer_create(GRect((bounds.size.w-FACE_WIDTH)/2,
                            (bounds.size.h-FACE_HEIGHT)/2,FACE_WIDTH,FACE_HEIGHT));
  if(!s_layer) return;
  layer_set_update_proc(s_layer,draw); layer_add_child(root,s_layer);
  time_t now=time(NULL); tick(localtime(&now),MINUTE_UNIT);
}
static void window_unload(Window *window) {
  (void)window; cancel_timer(&s_frame_timer); cancel_timer(&s_hide_timer);
  if(s_layer) layer_destroy(s_layer);
  s_layer=NULL;
}
int main(void) {
  s_mode=read_setting(101,MODE_FLICK,0,2); s_seconds=read_setting(102,5,2,30);
  s_custom_light=read_setting(103,0,0,1)!=0;
  s_red=read_setting(104,255,0,255); s_green=read_setting(105,255,0,255);
  s_blue=read_setting(106,255,0,255);
  s_progress=s_mode==MODE_ALWAYS?1000:0;
  s_percent=clamp(battery_state_service_peek().charge_percent,0,100);
  s_window=window_create(); if(!s_window) return 1;
  window_set_background_color(s_window,GColorBlack);
  window_set_window_handlers(s_window,(WindowHandlers){.load=window_load,.unload=window_unload});
  window_stack_push(s_window,true);
  tick_timer_service_subscribe(MINUTE_UNIT,tick);
  battery_state_service_subscribe(battery_changed);
  app_focus_service_subscribe(focus_changed);
  app_message_register_inbox_received(inbox); app_message_open(256,64);
  subscribe_motion(); apply_light(); app_event_loop();
  cancel_timer(&s_frame_timer); cancel_timer(&s_hide_timer);
  if(s_tap_subscribed) accel_tap_service_unsubscribe();
  battery_state_service_unsubscribe(); app_focus_service_unsubscribe();
  tick_timer_service_unsubscribe(); app_message_deregister_callbacks();
#ifdef PBL_PLATFORM_EMERY
  light_set_system_color();
#endif
  window_destroy(s_window); return 0;
}
