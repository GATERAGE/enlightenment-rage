#include <Elementary.h>
#include <math.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "video.h"
#include "gesture.h"

static Eina_Bool
_cb_momentum(void *data, double pos)
{
   Inf *inf = evas_object_data_get(data, "inf");
   Evas_Coord w, dx;
   Eina_Bool end = EINA_FALSE;
   
   evas_object_geometry_get(inf->event, NULL, NULL, &w, NULL);
   if (pos >= 1.0) end = EINA_TRUE;
   pos = ecore_animator_pos_map(pos, ECORE_POS_MAP_DECELERATE_FACTOR, 3.0, 0.0);

   dx = inf->drag_dist + (inf->drag_momentum * pos);
   
   pos = inf->start_pos + (((dx) * 60.0) / (double)(w / 2));
   video_position_set(inf->vid, pos);
   if (end)
     {
        inf->drag_anim = NULL;
        if (inf->was_playing) win_do_play(data);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static Evas_Event_Flags
_cb_long_move(void *data EINA_UNUSED, void *event EINA_UNUSED)
{
   // longpress - options
   return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags
_cb_move_start(void *data, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   
   if (inf->drag_anim)
     {
        ecore_animator_del(inf->drag_anim);
        inf->drag_anim = NULL;
     }
   inf->dragging = EINA_FALSE;
   return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags
_cb_move_move(void *data, void *event)
{
   Elm_Gesture_Momentum_Info *p = event;
   Inf *inf = evas_object_data_get(data, "inf");
   Evas_Coord w, sz = 0, dx;
   double pos;

   elm_coords_finger_size_adjust(1, &sz, 1, &sz);
   evas_object_geometry_get(inf->event, NULL, NULL, &w, NULL);
   if (w < 2) w = 2;
   dx = p->x2 - p->x1;
   if (abs(dx) < sz) return 0;
   if (!inf->dragging)
     {
        inf->was_playing = inf->playing;
        inf->start_pos = video_position_get(inf->vid);
        win_do_pause(data);
     }
   inf->dragging = EINA_TRUE;
   pos = inf->start_pos + (((dx) * 60.0) / (double)(w / 2));
   video_position_set(inf->vid, pos);
   return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags
_cb_move_end(void *data, void *event)
{
   Elm_Gesture_Momentum_Info *p = event;
   Inf *inf = evas_object_data_get(data, "inf");

   if (inf->dragging)
     {
        double tim = sqrt((p->mx * p->mx) + (p->my * p->my)) / 1000.0;
        
        if (tim > 0.0)
          {
             inf->drag_dist = p->x2 - p->x1;
             inf->drag_momentum = p->mx;
             inf->drag_time = tim;
             inf->drag_start = ecore_loop_time_get();
             inf->drag_anim =
               ecore_animator_timeline_add(tim, _cb_momentum, data);
          }
        else
          {
             if (inf->was_playing) win_do_play(data);
          }
     }
   else
     {
        if (inf->was_playing) win_do_play(data);
     }
   return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Event_Flags
_cb_move_abort(void *data, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");

   if (inf->was_playing) win_do_play(data);
   return EVAS_EVENT_FLAG_ON_HOLD;
}

void
gesture_init(Evas_Object *win, Evas_Object *tgt)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Evas_Object *g;
   
   inf->glayer = g = elm_gesture_layer_add(win);
   elm_gesture_layer_attach(g, tgt);
   
   elm_gesture_layer_cb_set(g, ELM_GESTURE_N_LONG_TAPS,
                            ELM_GESTURE_STATE_MOVE, _cb_long_move,
                            win);
   elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM,
                            ELM_GESTURE_STATE_START, _cb_move_start,
                            win);
   elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM,
                            ELM_GESTURE_STATE_MOVE, _cb_move_move,
                            win);
   elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM,
                            ELM_GESTURE_STATE_END, _cb_move_end,
                            win);
   elm_gesture_layer_cb_set(g, ELM_GESTURE_MOMENTUM,
                            ELM_GESTURE_STATE_ABORT, _cb_move_abort,
                            win);
}
