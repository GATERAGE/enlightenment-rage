#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "video.h"
#include "key.h"
#include "winlist.h"
#include "browser.h"
#include "mpris.h"

void
key_handle(Evas_Object *win, Evas_Event_Key_Down *ev)
{
   Inf *inf = evas_object_data_get(win, "inf");

   if ((!strcmp(ev->key, "Left")) ||
       (!strcmp(ev->key, "bracketleft")))
     {
        if (browser_visible()) return;
        if ((video_spu_button_count(inf->vid) > 0) &&
            (!strcmp(ev->key, "Left")))
          video_event_send(inf->vid, EMOTION_EVENT_LEFT);
        else
          {
             double pos = video_position_get(inf->vid);

             if (!inf->last_action_rwind)
               {
                  double t = ecore_time_get();
                  if ((t - inf->last_action) < 1.0) inf->jump *= 1.2;
                  else inf->jump = -5.0;
                  inf->last_action = t;
               }
             else inf->jump = -5.0;
             if (inf->jump < -300.0) inf->jump = -300.0;
             inf->last_action_rwind = EINA_FALSE;
             pos += inf->jump;
             if (pos < 0.0) pos = 0.0;
             video_position_set(inf->vid, pos);
             elm_layout_signal_emit(inf->lay, "action,rewind", "rage");
          }
     }
   else if ((!strcmp(ev->key, "Right")) ||
            (!strcmp(ev->key, "bracketright")))
     {
        double pos = video_position_get(inf->vid);
        double len = video_length_get(inf->vid);

        if (browser_visible()) return;
        if ((video_spu_button_count(inf->vid) > 0) &&
            (!strcmp(ev->key, "Right")))
          video_event_send(inf->vid, EMOTION_EVENT_RIGHT);
        else
          {
             if (inf->last_action_rwind)
               {
                  double t = ecore_time_get();
                  if ((t - inf->last_action) < 1.0) inf->jump *= 1.2;
                  else inf->jump = 5.0;
                  inf->last_action = t;
               }
             else inf->jump = 5.0;
             if (inf->jump > 300.0) inf->jump = 300.0;
             inf->last_action_rwind = EINA_TRUE;
             pos += inf->jump;
             if (pos < (len - 1.0)) video_position_set(inf->vid, pos);
               elm_layout_signal_emit(inf->lay, "action,forward", "rage");
          }
     }
   else if ((!strcmp(ev->key, "Up")) ||
            (!strcmp(ev->key, "plus")) ||
            (!strcmp(ev->key, "equal")))
     {
        if (browser_visible()) return;
        if ((video_spu_button_count(inf->vid) > 0) &&
            (!strcmp(ev->key, "Up")))
          video_event_send(inf->vid, EMOTION_EVENT_UP);
        else
          {
             double vol = video_volume_get(inf->vid) + 0.05;
             if (vol > 1.0) vol = 1.0;
             win_do_volume(win, vol);
          }
     }
   else if ((!strcmp(ev->key, "Down")) ||
            (!strcmp(ev->key, "minus")))
     {
        if (browser_visible()) return;
        if ((video_spu_button_count(inf->vid) > 0) &&
            (!strcmp(ev->key, "Down")))
          video_event_send(inf->vid, EMOTION_EVENT_DOWN);
        else
          {
             double vol = video_volume_get(inf->vid) - 0.05;
             if (vol < 0.0) vol = 0.0;
             win_do_volume(win, vol);
          }
     }
   else if ((!strcmp(ev->key, "space")) ||
            (!strcmp(ev->key, "Pause")) ||
            (!strcmp(ev->keyname, "p")) ||
            (!strcmp(ev->key, "XF86AudioPlay")))
     {
        if (browser_visible()) return;
        win_do_play_pause(win);
     }
   else if ((!strcmp(ev->keyname, "s")) ||
            (!strcmp(ev->key, "XF86AudioStop")) ||
            (!strcmp(ev->key, "BackSpace")) ||
            (!strcmp(ev->key, "Delete")))
     {
        if (browser_visible()) return;
        video_stop(inf->vid);
        elm_layout_signal_emit(inf->lay, "action,stop", "rage");
        elm_layout_signal_emit(inf->lay, "state,default", "rage");
        if (inf->browse_mode) browser_show(win);
        else evas_object_del(win);
        mpris_playback_status_change();
     }
   else if (!strcmp(ev->keyname, "c"))
     {
        video_stop(inf->vid);
        elm_layout_signal_emit(inf->lay, "action,stop", "rage");
        mpris_playback_status_change();
        win_video_free(win);
     }
   else if ((!strcmp(ev->key, "Prior")) ||
            (!strcmp(ev->key, "XF86AudioPrev")))
     {
        if (browser_visible()) return;
        win_do_prev(win);
     }
   else if ((!strcmp(ev->key, "Next")) ||
            (!strcmp(ev->key, "XF86AudioNext")))
     {
        if (browser_visible()) return;
        win_do_next(win);
     }
   else if (!strcmp(ev->key, "Home"))
     {
        if (browser_visible()) return;
        if (win_video_have_prev(win)) win_video_first(win);
        else win_video_prev(win);
     }
   else if (!strcmp(ev->key, "End"))
     {
        if (browser_visible()) return;
        if (win_video_have_next(win)) win_video_last(win);
        else win_video_next(win);
     }
   else if (!strcmp(ev->key, "Delete"))
     {
        if (browser_visible()) return;
        win_video_delete(win);
     }
   else if ((!strcmp(ev->keyname, "m")) ||
            (!strcmp(ev->key, "XF86AudioMute")))
     {
        if (browser_visible()) return;
        video_mute_set(inf->vid, !video_mute_get(inf->vid));
        if (video_mute_get(inf->vid))
          elm_layout_signal_emit(inf->lay, "action,mute", "rage");
        else
          elm_layout_signal_emit(inf->lay, "action,unmute", "rage");
        mpris_volume_change();
     }
   else if (!strcmp(ev->keyname, "l"))
     {
        if (browser_visible()) return;
        video_loop_set(inf->vid, !video_loop_get(inf->vid));
        if (video_loop_get(inf->vid))
          elm_layout_signal_emit(inf->lay, "action,loop", "rage");
        else
          elm_layout_signal_emit(inf->lay, "action,sequential", "rage");
        mpris_loop_status_change();
     }
   else if ((!strcmp(ev->keyname, "q")) ||
            (!strcmp(ev->key, "Escape")))
     {
        evas_object_del(win);
     }
   else if ((!strcmp(ev->keyname, "f")) ||
            (!strcmp(ev->key, "F11")))
     {
        elm_win_fullscreen_set(win, !elm_win_fullscreen_get(win));
     }
   else if (!strcmp(ev->keyname, "n"))
     {
        int w, h;

        if (browser_visible()) return;
        video_ratio_size_get(inf->vid, &w, &h);
        if ((w > 1) && (h > 1)) evas_object_resize(win, w, h);
        else
          {
             Evas_Coord mw = 1, mh = 1;

             elm_layout_signal_emit(inf->lay, "pref,size,on", "rage");
             edje_object_message_signal_process(elm_layout_edje_get(inf->lay));
             edje_object_size_min_calc(elm_layout_edje_get(inf->lay), &mw, &mh);
             elm_layout_signal_emit(inf->lay, "pref,size,off", "rage");
             w = mw;
             h = mh;
             if ((w > 1) && (h > 1)) evas_object_resize(win, w, h);
          }
     }
   else if (!strcmp(ev->keyname, "backslash"))
     {
        if (browser_visible()) return;
        if (inf->browse_mode) browser_toggle(win);
        else win_list_toggle(win);
     }
   else if (!strcmp(ev->keyname, "y"))
     {
        if (browser_visible()) return;
        video_lowquality_set(inf->vid, !video_lowquality_get(inf->vid));
     }
   else if (!strcmp(ev->keyname, "z"))
     {
        if (browser_visible()) return;
        if (inf->zoom_mode == 0) inf->zoom_mode = 1;
        else inf->zoom_mode = 0;
        win_aspect_adjust(win);
        if (inf->zoom_mode == 1)
          elm_layout_signal_emit(inf->lay, "action,zoom_fill", "rage");
        else
          elm_layout_signal_emit(inf->lay, "action,zoom_fit", "rage");
     }
   else if (!strcmp(ev->keyname, "e"))
     {
        if (browser_visible()) return;
        video_eject(inf->vid);
        elm_layout_signal_emit(inf->lay, "action,eject", "rage");
     }
   else if ((!strcmp(ev->key, "Return")) ||
            (!strcmp(ev->key, "KP_Enter")))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_SELECT);
     }
   else if ((!strcmp(ev->key, "comma")) ||
            (!strcmp(ev->key, "less")))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_ANGLE_PREV);
     }
   else if ((!strcmp(ev->key, "period")) ||
            (!strcmp(ev->key, "greater")))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_ANGLE_NEXT);
     }
   else if (!strcmp(ev->key, "Tab"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_FORCE);
     }
   else if (!strcmp(ev->key, "0"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_0);
     }
   else if (!strcmp(ev->key, "1"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_1);
     }
   else if (!strcmp(ev->key, "2"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_2);
     }
   else if (!strcmp(ev->key, "3"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_3);
     }
   else if (!strcmp(ev->key, "4"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_4);
     }
   else if (!strcmp(ev->key, "5"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_5);
     }
   else if (!strcmp(ev->key, "6"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_6);
     }
   else if (!strcmp(ev->key, "7"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_7);
     }
   else if (!strcmp(ev->key, "8"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_8);
     }
   else if (!strcmp(ev->key, "9"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_9);
     }
   else if ((!strcmp(ev->key, "grave")) ||
            (!strcmp(ev->key, "asciitilde")))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_10);
     }
   else if (!strcmp(ev->key, "F1"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_MENU1);
     }
   else if (!strcmp(ev->key, "F2"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_MENU2);
     }
   else if (!strcmp(ev->key, "F3"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_MENU3);
     }
   else if (!strcmp(ev->key, "F4"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_MENU4);
     }
   else if (!strcmp(ev->key, "F5"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_MENU5);
     }
   else if (!strcmp(ev->key, "F6"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_MENU6);
     }
   else if (!strcmp(ev->key, "F7"))
     {
        if (browser_visible()) return;
        video_event_send(inf->vid, EMOTION_EVENT_MENU7);
     }
}
