#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "video.h"
#include "controls.h"
#include "winlist.h"
#include "mpris.h"

static void
_cb_drag(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   double pos = 0.0;
   
   edje_object_part_drag_value_get(elm_layout_edje_get(inf->lay),
                                   "rage.dragable.pos", &pos, NULL);
   video_position_set(inf->vid, pos * video_length_get(inf->vid));
}

static void
_cb_pos_prev(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_do_prev(data);
}

static void
_cb_pos_next(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_do_next(data);
}

static void
_cb_pos_play(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_do_play(data);
}

static void
_cb_pos_pause(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_do_pause(data);
}

static void
_cb_options(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   elm_layout_signal_emit(inf->lay, "about,show", "rage");
   //   win_do_options(data);
}

static void
_cb_list_show(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   if (eina_list_count(inf->file_list) <= 1) return;
   win_list_show(data);
}

static void
_cb_list_hide(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_list_hide(data);
}

static void
_cb_win_fullscreen(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   elm_win_fullscreen_set(elm_object_top_widget_get(obj), EINA_TRUE);
}

static void
_cb_win_unfullscreen(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   elm_win_fullscreen_set(elm_object_top_widget_get(obj), EINA_FALSE);
}

static void
_cb_win_close(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   evas_object_del(elm_object_top_widget_get(obj));
}

static void
_cb_vol(void *data, Evas_Object *obj, void *info EINA_UNUSED)
{
   win_do_volume(data, elm_slider_value_get(obj) / 100.0);
}

static void
_cb_action_vol(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   video_mute_set(inf->vid, !video_mute_get(inf->vid));
   if (video_mute_get(inf->vid))
     elm_layout_signal_emit(inf->lay, "action,mute", "rage");
   else
     elm_layout_signal_emit(inf->lay, "action,unmute", "rage");
}

static void
_time_print(double t, double max, int size, char *buf, Eina_Bool subsec)
{
   double tsf;
   long long tl;
   int ts, tm, th;

   tl = t;
   ts = tl % 60;
   tm = (tl / 60) % 60;
   th = (tl / (60 * 60));
   tsf = fmod(t, 60.0);
   if (subsec)
     {
        if (tsf < 10.0)
          {
             if (max >= (60 * 60))
               snprintf(buf, size, "%i:%02i:0%1.02f", th, tm, tsf);
             else if (max >= 60)
               snprintf(buf, size, "%i:0%1.02f", tm, tsf);
             else
               snprintf(buf, size, "%1.02f", tsf);
          }
        else
          {
             if (max >= (60 * 60))
               snprintf(buf, size, "%i:%02i:%2.02f", th, tm, tsf);
             else if (max >= 60)
               snprintf(buf, size, "%i:%2.02f", tm, tsf);
             else
               snprintf(buf, size, "%2.02f", tsf);
          }
     }
   else
     {
        if (max >= (60 * 60))
          snprintf(buf, size, "%i:%02i:%02i", th, tm, ts);
        else if (max >= 60)
          snprintf(buf, size, "%i:%02i", tm, ts);
        else
          snprintf(buf, size, "%i", ts);
     }
}

void
controls_init(Evas_Object *win, Evas_Object *lay)
{
   Evas_Object *o;
   Evas_Coord sz;
   
   elm_layout_signal_callback_add(lay, "drag", "rage.dragable.pos",
                                  _cb_drag, win);
   elm_layout_signal_callback_add(lay, "pos,action,prev", "rage",
                                  _cb_pos_prev, win);
   elm_layout_signal_callback_add(lay, "pos,action,next", "rage",
                                  _cb_pos_next, win);
   elm_layout_signal_callback_add(lay, "pos,action,play", "rage",
                                  _cb_pos_play, win);
   elm_layout_signal_callback_add(lay, "pos,action,pause", "rage",
                                  _cb_pos_pause, win);
   elm_layout_signal_callback_add(lay, "pos,action,options", "rage",
                                  _cb_options, win);
   elm_layout_signal_callback_add(lay, "pos,action,vol", "rage",
                                  _cb_action_vol, win);
   elm_layout_signal_callback_add(lay, "list,show", "rage",
                                  _cb_list_show, win);
   elm_layout_signal_callback_add(lay, "list,hide", "rage",
                                  _cb_list_hide, win);
   elm_layout_signal_callback_add(lay, "win,fullscreen", "rage",
                                  _cb_win_fullscreen, win);
   elm_layout_signal_callback_add(lay, "win,unfullscreen", "rage",
                                  _cb_win_unfullscreen, win);
   elm_layout_signal_callback_add(lay, "win,close", "rage",
                                  _cb_win_close, win);
   sz = 0;
   elm_coords_finger_size_adjust(1, &sz, 1, &sz);

#define FINGER_SIZE(_nam) \
      o = evas_object_rectangle_add(evas_object_evas_get(win)); \
      evas_object_color_set(o, 0, 0, 0, 0); \
      evas_object_pass_events_set(o, EINA_TRUE); \
      evas_object_size_hint_min_set(o, sz, sz); \
      elm_object_part_content_set(lay, _nam, o)

   FINGER_SIZE("rage.pos.swallow");
//   FINGER_SIZE("rage.vol.swallow");
   FINGER_SIZE("rage.options.swallow");
   FINGER_SIZE("rage.win.fullscreen");
   FINGER_SIZE("rage.win.close");
   FINGER_SIZE("rage.pos.prev.swallow");
   FINGER_SIZE("rage.pos.play.swallow");
   FINGER_SIZE("rage.pos.next.swallow");

   o = elm_slider_add(win);
   elm_object_focus_allow_set(o, EINA_FALSE);
   elm_slider_min_max_set(o, 0, 100);
   elm_slider_indicator_format_set(o, "%1.0f");
   evas_object_smart_callback_add(o, "changed", _cb_vol, win);
   elm_object_part_content_set(lay, "rage.vol.swallow", o);
   evas_object_show(o);
}

void
controls_update(Evas_Object *lay, Evas_Object *vid)
{
   char buf[256];
   double p;

   _time_print(video_position_get(vid), video_length_get(vid),
               sizeof(buf), buf, EINA_FALSE);
   elm_object_part_text_set(lay, "rage.pos", buf);
   _time_print(video_length_get(vid), video_length_get(vid),
               sizeof(buf), buf, EINA_FALSE);
   elm_object_part_text_set(lay, "rage.length", buf);
   elm_layout_signal_emit(lay, "action,frame", "rage");

   if (video_length_get(vid) > 0.0)
     p = video_position_get(vid) / video_length_get(vid);
   else p = 0.0;
   edje_object_part_drag_value_set(elm_layout_edje_get(lay),
                                   "rage.dragable.pos", p, 0.0);
   mpris_position_change(video_position_get(vid));
}
