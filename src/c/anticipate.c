#include <pebble.h>
#include <stdlib.h>
#include "time_digits.h"
#include "column_sprites.h"
#include "alt_digits.h"
#include "lcd_digits.h"

// Both states retain the original centred 144x168 footprint and four-pixel inset.
enum { FACE_WIDTH=144, FACE_HEIGHT=168, RIGHT=140, BOTTOM=164,
       MODE_OFF=0, MODE_ALWAYS=1, MODE_FLICK=2, FRAME_MS=33, FRAMES=10 };
static Window *s_window;
static Layer *s_layer;
static AppTimer *s_frame_timer, *s_hide_timer;
static int s_hour, s_minute, s_percent;
static int s_style, s_date_format, s_fahrenheit;
enum { DATE_DAY_MONTH=0, DATE_MONTH_DAY=1, DATE_DAY=2, DATE_MONTH=3 };
static int s_bg_rgb=0x000000, s_digit_rgb=0xffffff, s_detail_rgb=0xffffff;
static struct tm s_date;
static int s_weather[5]={0,0,0,-1,0}; // high, current, low, condition, timestamp
static time_t s_last_request;
static void request_weather(void);
static int s_progress, s_start, s_target, s_frame;
static int s_mode=MODE_FLICK, s_seconds=5, s_red=255, s_green=255, s_blue=255;
static bool s_custom_light, s_focused=true, s_tap_subscribed;

static int clamp(int n,int lo,int hi) { return n<lo?lo:(n>hi?hi:n); }
// Colour platforms honour the chosen palette; black-and-white watches keep the original look.
static GColor theme(int rgb,GColor fallback) {
  return PBL_IF_COLOR_ELSE(GColorFromHEX(rgb),fallback);
}
#define BG_COLOR theme(s_bg_rgb,GColorBlack)
#define DIGIT_COLOR theme(s_digit_rgb,GColorWhite)
#define DETAIL_COLOR theme(s_detail_rgb,GColorWhite)
static void dirty(void) { if(s_layer) layer_mark_dirty(s_layer); }
static void cancel_timer(AppTimer **timer) {
  if(*timer) { app_timer_cancel(*timer); *timer=NULL; }
}
static void apply_light(void) {
#ifdef PBL_RGB_BACKLIGHT
  if(s_custom_light) light_set_color_rgb888(((uint32_t)s_red<<16)|((uint32_t)s_green<<8)|s_blue);
  else light_set_system_color();
#endif
}
#ifdef PBL_RGB_BACKLIGHT
static void backlight_changed(bool on) { if(on && s_focused) apply_light(); }
#endif
// Explicit clipping prevents sliding content touching the original outer margins.
static void rect(GContext *ctx,int x,int y,int w,int h) {
  int right=clamp(x+w,4,RIGHT), bottom=clamp(y+h,4,BOTTOM);
  x=clamp(x,4,RIGHT); y=clamp(y,4,BOTTOM);
  if(right>x && bottom>y) graphics_fill_rect(ctx,GRect(x,y,right-x,bottom-y),0,GCornerNone);
}
enum { STYLE_ANTICIPATE=0, STYLE_NAIVE=1, STYLE_BRUTAL=2, STYLE_LCD=3 };
static const ColumnSprite *alt_digit(int digit,int y) {
  if(s_style==STYLE_NAIVE) return y==4?naive_hours_digits[digit]:naive_minutes_digits[digit];
  if(s_style==STYLE_LCD) return lcd_digits[digit];
  return brutal_digits[digit];
}
// Width of the artwork's full cell; the drawn size w maps onto this.
static int source_width(void) {
  return s_style==STYLE_LCD?91:(s_style?60:46);
}
static int digit_width(int digit,int y) {
  return s_style?alt_digit(digit,y)->w:DIGIT_WIDTHS[digit];
}
static void draw_digit(GContext *ctx,int digit,int x,int y,int w,int h) {
  const ColumnSprite *alt=s_style?alt_digit(digit,y):NULL;
  int source_w=source_width(), source_h=alt?alt->h:71;
  int width=alt?alt->w:DIGIT_WIDTHS[digit], stride=alt?alt->stride:6;
  const uint8_t *bits=alt?alt->bits:&DIGIT_ROWS[digit][0][0];
  for(int sy=0;sy<source_h;sy++) {
    int top=y+sy*h/source_h, bottom=y+(sy+1)*h/source_h;
    for(int sx=0;sx<width;) {
      if(!(bits[sy*stride+sx/8] & (1<<(sx%8)))) {sx++;continue;}
      int start=sx;
      while(sx<width && (bits[sy*stride+sx/8] & (1<<(sx%8)))) sx++;
      int left=x+start*w/source_w, right=x+sx*w/source_w;
      rect(ctx,left,top,right-left,bottom-top);
    }
  }
}
// Unlit LCD segments: the all-segments 8 sampled as a dot field on even face pixels,
// like the original artwork's dotted ghost, so dots stay put while digits resize.
static void draw_ghost(GContext *ctx,int x,int y,int w,int h) {
  const ColumnSprite *all=lcd_digits[8];
  int top=clamp(y,4,BOTTOM), bottom=clamp(y+h,4,BOTTOM);
  int left=clamp(x,4,RIGHT), right=clamp(x+w,4,RIGHT);
  for(int py=top+(top&1);py<bottom;py+=2) {
    int sy=(py-y)*all->h/h;
    for(int px=left+(left&1);px<right;px+=2) {
      int sx=(px-x)*all->w/w;
      if(all->bits[sy*all->stride+sx/8] & (1<<(sx%8)))
        graphics_fill_rect(ctx,GRect(px,py,1,1),0,GCornerNone);
    }
  }
}
static GColor ghost_color(void) {
#ifdef PBL_COLOR
  // Halfway between background and digits, per channel.
  int mix=0;
  for(int shift=0;shift<24;shift+=8)
    mix|=((((s_bg_rgb>>shift)&255)+((s_digit_rgb>>shift)&255))/2)<<shift;
  return GColorFromHEX(mix);
#else
  return GColorWhite;
#endif
}
static void draw_pair(GContext *ctx,int value,int y,int w,int h) {
  int ones=value%10,tens=value/10,source_w=source_width();
  int ones_x=RIGHT-digit_width(ones,y)*w/source_w;
  int tens_x=ones_x-4-digit_width(tens,y)*w/source_w;
  if(s_style==STYLE_LCD) {
    graphics_context_set_fill_color(ctx,ghost_color());
    draw_ghost(ctx,ones_x,y,w,h); draw_ghost(ctx,tens_x,y,w,h);
    graphics_context_set_fill_color(ctx,DIGIT_COLOR);
  }
  draw_digit(ctx,ones,ones_x,y,w,h);
  draw_digit(ctx,tens,tens_x,y,w,h);
}
static void sprite(GContext *ctx,const ColumnSprite *im,int x,int y) {
  for(int sy=0;sy<im->h;sy++) for(int sx=0;sx<im->w;) {
    if(!(im->bits[sy*im->stride+sx/8] & (1<<(sx%8)))) {sx++;continue;}
    int start=sx;
    while(sx<im->w && (im->bits[sy*im->stride+sx/8] & (1<<(sx%8)))) sx++;
    rect(ctx,x+start,y+sy,sx-start,1);
  }
}
static int glyph_index(char c) { return c=='-'?10:(c=='*'?11:c-'0'); }
static void column_text(GContext *ctx,const char *str,int x,int y,bool large) {
  const ColumnSprite *const *font=large?s_sprites:xs_sprites;
  int width=0;
  for(const char *c=str;*c;c++) width+=font[glyph_index(*c)]->w+1;
  int limit=36;
  // Scale very long step counts/temperatures into the original 36px column.
  int actual=width?width-1:0, fit=actual>limit?limit:actual;
  int pen=0,origin=x+(limit-fit)/2;
  for(const char *c=str;*c;c++) {
    const ColumnSprite *im=font[glyph_index(*c)];
    for(int sy=0;sy<im->h;sy++) for(int sx=0;sx<im->w;sx++) {
      if(!(im->bits[sy*im->stride+sx/8] & (1<<(sx%8)))) continue;
      int left=(pen+sx)*fit/actual,right=(pen+sx+1)*fit/actual;
      rect(ctx,origin+left,y+sy,right-left,1);
    }
    pen+=im->w+1;
  }
}
static bool weather_fresh(void) {
  time_t now=time(NULL);return s_weather[4]>0 && now>=s_weather[4] && now-s_weather[4]<10800;
}
// Heavy 9x13 weekday lettering with 3px strokes, matching the pill's temperature numerals.
static void draw_weekday(GContext *ctx,int x,int y) {
  static const char alphabet[]="MONTUEWDHFRISA";
  static const uint16_t rows[][13]={
    {455,495,511,511,471,455,455,455,455,455,455,455,455}, {254,511,511,455,455,455,455,455,455,455,511,511,254},
    {455,487,503,511,479,463,455,455,455,455,455,455,455}, {511,511,511,56,56,56,56,56,56,56,56,56,56},
    {455,455,455,455,455,455,455,455,455,455,511,511,511}, {511,511,511,448,448,508,508,508,448,448,511,511,511},
    {455,455,455,455,455,455,455,455,471,511,511,495,455}, {510,511,511,455,455,455,455,455,455,455,511,511,510},
    {455,455,455,455,455,511,511,511,455,455,455,455,455}, {511,511,511,448,448,508,508,508,448,448,448,448,448},
    {510,511,511,455,455,511,511,510,476,462,455,455,455}, {511,511,511,56,56,56,56,56,56,56,511,511,511},
    {511,511,511,448,448,511,511,511,7,7,511,511,511}, {254,511,511,455,455,511,511,511,455,455,455,455,455}
  };
  static const char *const days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
  const char *day=days[clamp(s_date.tm_wday,0,6)];
  for(int i=0;i<3;i++) {
    const char *letter=strchr(alphabet,day[i]);
    if(!letter) continue;
    int index=(int)(letter-alphabet), left=x+3+i*10;
    for(int row=0;row<13;row++) for(int col=0;col<9;) {
      if(!(rows[index][row] & (1<<(8-col)))) {col++;continue;}
      int start=col;
      while(col<9 && (rows[index][row] & (1<<(8-col)))) col++;
      rect(ctx,left+start,y+row,col-start,1);
    }
  }
}
static void draw_column(GContext *ctx) {
  int x=4-40*(1000-s_progress)/1000;
  graphics_context_set_fill_color(ctx,DETAIL_COLOR);
  sprite(ctx,&sprite_background,x,4);
  char buf[16];
  static const char *const formats[]={"%d-%m","%m-%d","%d","%m"};
  strftime(buf,sizeof(buf),formats[clamp(s_date_format,0,3)],&s_date);
  column_text(ctx,buf,x,10,false);
  graphics_context_set_fill_color(ctx,BG_COLOR);
  draw_weekday(ctx,x,33);
  for(int i=0;i<3;i++) {
    graphics_context_set_fill_color(ctx,i==1?BG_COLOR:DETAIL_COLOR);
    if(!weather_fresh()) strcpy(buf,"--*");
    else snprintf(buf,sizeof(buf),"%d*",s_fahrenheit?s_weather[i]*9/5+32:s_weather[i]);
    column_text(ctx,buf,x,58+25*i,true);
  }
  graphics_context_set_fill_color(ctx,DETAIL_COLOR);
  if(weather_fresh() && s_weather[3]>=0 && s_weather[3]<10) sprite(ctx,condition_sprites[s_weather[3]],x,128);
  else column_text(ctx,"--",x,141,false);
}
// A compact 3x5 pixel alphabet for the gauge's small labels (last glyph is %).
static const uint8_t SMALL[11][5]={
 {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
 {7,4,7,1,7},{7,4,7,5,7},{7,1,2,2,2},{7,5,7,5,7},{7,5,7,1,7},
 {5,1,2,4,5}
};
static void small_text(GContext *ctx,const char *s,int x,int y,int scale) {
  for(;*s;s++,x+=4*scale) {
    int d=*s=='%'?10:*s-'0';
    if(d<0 || d>10) continue;
    for(int row=0;row<5;row++) for(int col=0;col<3;col++)
      if(SMALL[d][row] & (1<<(2-col))) rect(ctx,x+col*scale,y+row*scale,scale,scale);
  }
}
static void draw_battery(GContext *ctx,int top) {
  // Track spans x=4..139; marker follows the fill boundary, including 0/100%.
  int left=4+40*s_progress/1000, span=RIGHT-left;
  rect(ctx,left,top,span,1); rect(ctx,left,top+3,span,1);
  rect(ctx,left,top,1,4); rect(ctx,139,top,1,4);
  int fill=(span-2)*s_percent/100, marker=clamp(left+1+fill,left,139);
  rect(ctx,left+1,top+1,fill,2);
  // Upward triangle below the track, with one clear row between them.
  rect(ctx,marker,top+5,1,1);
  rect(ctx,marker-1,top+6,3,1);
  rect(ctx,marker-2,top+7,5,1);
  char label[5]; snprintf(label,sizeof(label),"%d%%",s_percent);
  // The percentage is drawn at double size; endpoints stay small and share its baseline.
  int width=(int)strlen(label)*8-2;
  int label_x=clamp(marker-width/2,left,RIGHT-width);
  small_text(ctx,label,label_x,top+9,2);
  // Near endpoints, the percentage itself replaces the endpoint label.
  if(label_x>left+5) small_text(ctx,"0",left,top+14,1);
  if(label_x+width<127) small_text(ctx,"100",129,top+14,1);
}
static void draw(Layer *layer,GContext *ctx) {
  graphics_context_set_fill_color(ctx,BG_COLOR);
  graphics_fill_rect(ctx,layer_get_bounds(layer),0,GCornerNone);
  int w=66-20*s_progress/1000, h=78-12*s_progress/1000;
  if(s_progress) draw_column(ctx);
  graphics_context_set_fill_color(ctx,DIGIT_COLOR);
  draw_pair(ctx,s_hour,4,w,h); draw_pair(ctx,s_minute,8+h,w,h);
  graphics_context_set_fill_color(ctx,DETAIL_COLOR);
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
  request_weather(); apply_light();
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
  (void)changed; s_date=*t; s_hour=t->tm_hour;
  if(!clock_is_24h_style()) { s_hour%=12; if(!s_hour) s_hour=12; }
  s_minute=t->tm_min;
  if(s_mode!=MODE_OFF) {request_weather();}
  dirty();
}
static void request_weather(void) {
  time_t now=time(NULL);
  if(s_last_request && now>=s_last_request && now-s_last_request<60) return;
  if(weather_fresh() && now-s_weather[4]<1800) return;
  DictionaryIterator *it;
  if(app_message_outbox_begin(&it)!=APP_MSG_OK || !it) return;
  dict_write_uint8(it,MESSAGE_KEY_REQUEST_WEATHER,1);
  if(app_message_outbox_send()==APP_MSG_OK) s_last_request=now;
}
// Independent persistence keys avoid interpreting the upstream settings struct.
static int read_setting(int key,int fallback,int lo,int hi) {
  return persist_exists(key)?clamp(persist_read_int(key),lo,hi):fallback;
}
// AppMessage integers may occupy 1, 2 or 4 bytes, not always four.
static bool tuple_number(const Tuple *tuple,int64_t *number) {
  if(!tuple || (tuple->type!=TUPLE_INT && tuple->type!=TUPLE_UINT)) return false;
  switch(tuple->length) {
    case 1: *number=tuple->type==TUPLE_INT?(int64_t)tuple->value->int8:(int64_t)tuple->value->uint8;return true;
    case 2: *number=tuple->type==TUPLE_INT?(int64_t)tuple->value->int16:(int64_t)tuple->value->uint16;return true;
    case 4: *number=tuple->type==TUPLE_INT?(int64_t)tuple->value->int32:(int64_t)tuple->value->uint32;return true;
    default:return false;
  }
}
static void receive_setting(DictionaryIterator *iter,uint32_t key,int storage,
                            int *value,int lo,int hi) {
  Tuple *tuple=dict_find(iter,key);
  if(!tuple) return;
  int64_t n;
  if(tuple->type==TUPLE_CSTRING) {
    // Parse by hand: newlib strtol reads an unrelocated global and faults on watch.
    const char *s=tuple->value->cstring; uint16_t len=tuple->length;
    if(!len || len>12) return;
    uint16_t i=0; bool neg=false; long parsed=0;
    if(s[i]=='-') {neg=true;i++;}
    if(i>=len || s[i]<'0' || s[i]>'9') return;
    for(;i<len && s[i]>='0' && s[i]<='9';i++) parsed=parsed*10+(s[i]-'0');
    if(i>=len || s[i]!='\0') return;
    if(neg) parsed=-parsed;
    if(parsed<lo || parsed>hi) return;
    n=parsed;
  } else if(!tuple_number(tuple,&n)) return;
  n=n<lo?lo:(n>hi?hi:n);
  *value=(int)n; persist_write_int(storage,(int)n);
}
static void inbox(DictionaryIterator *iter,void *context) {
  (void)context;
  Tuple *at=dict_find(iter,MESSAGE_KEY_WEATHER_AT);
  if(at) {
    const uint32_t keys[]={MESSAGE_KEY_TEMP_HI,MESSAGE_KEY_TEMP_CUR,MESSAGE_KEY_TEMP_LO,MESSAGE_KEY_CONDITIONS,MESSAGE_KEY_WEATHER_AT};
    int values[5];bool valid=true;
    for(int i=0;i<5;i++) {
      Tuple *t=dict_find(iter,keys[i]);
      int64_t n;
      if(!tuple_number(t,&n) || n<INT32_MIN || n>INT32_MAX) {valid=false;break;}
      values[i]=(int)n;
      if(i<3 && (values[i]<-100 || values[i]>100)) valid=false;
    }
    if(valid && values[3]>=-1 && values[3]<10 && values[4]>0) {
      memcpy(s_weather,values,sizeof(values));persist_write_data(120,s_weather,sizeof(s_weather));dirty();
    }
    // Weather updates must never cancel an active reveal or preview the light.
    return;
  }
  receive_setting(iter,MESSAGE_KEY_TimeStyle,109,&s_style,0,3);
  receive_setting(iter,MESSAGE_KEY_DateFormat,111,&s_date_format,0,3);
  receive_setting(iter,MESSAGE_KEY_BackgroundColor,112,&s_bg_rgb,0,0xffffff);
  receive_setting(iter,MESSAGE_KEY_DigitColor,113,&s_digit_rgb,0,0xffffff);
  receive_setting(iter,MESSAGE_KEY_DetailColor,114,&s_detail_rgb,0,0xffffff);
  if(s_window) window_set_background_color(s_window,BG_COLOR);
  receive_setting(iter,MESSAGE_KEY_Fahrenheit,108,&s_fahrenheit,0,1);
  receive_setting(iter,MESSAGE_KEY_BatteryMode,101,&s_mode,0,2);
  receive_setting(iter,MESSAGE_KEY_BatterySeconds,102,&s_seconds,2,30);
  int enabled=s_custom_light;
  receive_setting(iter,MESSAGE_KEY_CustomBacklight,103,&enabled,0,1);
  s_custom_light=enabled!=0;
  receive_setting(iter,MESSAGE_KEY_BacklightRed,104,&s_red,0,255);
  receive_setting(iter,MESSAGE_KEY_BacklightGreen,105,&s_green,0,255);
  receive_setting(iter,MESSAGE_KEY_BacklightBlue,106,&s_blue,0,255);
  if(dict_find(iter,MESSAGE_KEY_BacklightColor)) {
    int rgb=(s_red<<16)|(s_green<<8)|s_blue;
    receive_setting(iter,MESSAGE_KEY_BacklightColor,110,&rgb,0,0xffffff);
    s_red=(rgb>>16)&255;s_green=(rgb>>8)&255;s_blue=rgb&255;
    persist_write_int(104,s_red);persist_write_int(105,s_green);persist_write_int(106,s_blue);
  }
  cancel_timer(&s_hide_timer); subscribe_motion();
  transition(s_mode==MODE_ALWAYS);
  if(s_focused) {
    apply_light();
#ifdef PBL_RGB_BACKLIGHT
    if(dict_find(iter,MESSAGE_KEY_BacklightColor) || dict_find(iter,MESSAGE_KEY_CustomBacklight) || dict_find(iter,MESSAGE_KEY_BacklightRed) ||
       dict_find(iter,MESSAGE_KEY_BacklightGreen) || dict_find(iter,MESSAGE_KEY_BacklightBlue)) light_enable_interaction();
#endif
  }
  APP_LOG(APP_LOG_LEVEL_INFO,"Settings applied: style=%d mode=%d RGB=%d/%d/%d enabled=%d",
          s_style,s_mode,s_red,s_green,s_blue,s_custom_light);
  request_weather();dirty();
}
static void inbox_dropped(AppMessageResult reason,void *context) {
  (void)context;APP_LOG(APP_LOG_LEVEL_ERROR,"AppMessage dropped: %d",reason);
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
  s_style=read_setting(109,0,0,3);s_fahrenheit=read_setting(108,0,0,1);
  // Older builds stored only "month before day" (key 107); use it until a format is chosen.
  s_date_format=read_setting(111,read_setting(107,0,0,1)?DATE_MONTH_DAY:DATE_DAY_MONTH,0,3);
  s_bg_rgb=read_setting(112,0x000000,0,0xffffff);
  s_digit_rgb=read_setting(113,0xffffff,0,0xffffff);
  s_detail_rgb=read_setting(114,0xffffff,0,0xffffff);
  if(persist_get_size(120)==sizeof(s_weather)) persist_read_data(120,s_weather,sizeof(s_weather));
  s_mode=read_setting(101,MODE_FLICK,0,2); s_seconds=read_setting(102,5,2,30);
  s_custom_light=read_setting(103,0,0,1)!=0;
  s_red=read_setting(104,255,0,255); s_green=read_setting(105,255,0,255);
  s_blue=read_setting(106,255,0,255);
  s_progress=s_mode==MODE_ALWAYS?1000:0;
  s_percent=clamp(battery_state_service_peek().charge_percent,0,100);
  s_window=window_create(); if(!s_window) return 1;
  window_set_background_color(s_window,BG_COLOR);
  window_set_window_handlers(s_window,(WindowHandlers){.load=window_load,.unload=window_unload});
  window_stack_push(s_window,true);
  tick_timer_service_subscribe(MINUTE_UNIT,tick);
  battery_state_service_subscribe(battery_changed);
  app_focus_service_subscribe(focus_changed);
  app_message_register_inbox_received(inbox);
  app_message_register_inbox_dropped(inbox_dropped);
  AppMessageResult opened=app_message_open(512,64);
  if(opened!=APP_MSG_OK) APP_LOG(APP_LOG_LEVEL_ERROR,"AppMessage open failed: %d",opened);
#ifdef PBL_RGB_BACKLIGHT
  backlight_service_subscribe(backlight_changed);
#endif
  subscribe_motion(); apply_light(); app_event_loop();
  cancel_timer(&s_frame_timer); cancel_timer(&s_hide_timer);
  if(s_tap_subscribed) accel_tap_service_unsubscribe();
  battery_state_service_unsubscribe(); app_focus_service_unsubscribe();
  tick_timer_service_unsubscribe(); app_message_deregister_callbacks();
#ifdef PBL_RGB_BACKLIGHT
  backlight_service_unsubscribe();light_set_system_color();
#endif
  window_destroy(s_window); return 0;
}
