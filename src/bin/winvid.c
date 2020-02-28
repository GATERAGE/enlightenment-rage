#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "video.h"
#include "winlist.h"
#include "winvid.h"
#include "videothumb.h"
#include "browser.h"
#include "mpris.h"

static void
_cb_resize(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_aspect_adjust(data);
   win_title_update(data);
}

static void
_cb_decode(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_frame_decode(data);
}

static void
_cb_stop_next(void *data)
{
   Inf *inf = evas_object_data_get(data, "inf");
   inf->next_job = NULL;
   if (!win_video_next(data))
     {
        elm_layout_signal_emit(inf->lay, "state,default", "rage");
        mpris_playback_status_change();
     }
}

static void
_cb_stop(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   if (win_video_have_next(data))
     {
        if (inf->next_job) ecore_job_del(inf->next_job);
        inf->next_job = ecore_job_add(_cb_stop_next, data);
     }
   else
     {
        elm_layout_signal_emit(inf->lay, "state,default", "rage");
        if (inf->browse_mode) browser_show(data);
        else elm_exit();
     }
   mpris_playback_status_change();
}

static void
_cb_opened(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_aspect_adjust(data);
   win_frame_decode(data);
   win_title_update(data);
}

static void
_cb_length(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_frame_decode(data);
   win_title_update(data);
}

static void
_cb_title(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
#if 0
   printf("title change........\n");
   printf("  meta title:   %s\n", video_meta_title_get(obj));
   printf("  meta album:   %s\n", video_meta_album_get(obj));
   printf("  meta artist:  %s\n", video_meta_artist_get(obj));
   printf("  meta year:    %s\n", video_meta_year_get(obj));
   printf("  meta genre:   %s\n", video_meta_genre_get(obj));
   printf("  meta comment: %s\n", video_meta_comment_get(obj));

   printf("  chapt: %i/%i [%s]\n", video_chapter_get(obj), video_chapter_count(obj), video_chapter_name_get(obj, video_chapter_get(obj)));
   printf("  title: %s\n", video_title_get(obj));
   printf("  audio: %i/%i [%s]\n", video_audio_channel_get(obj), video_audio_channel_count(obj), video_audio_channel_name_get(obj, video_audio_channel_get(obj)));
   printf("  video: %i/%i [%s]\n", video_video_channel_get(obj), video_video_channel_count(obj), video_video_channel_name_get(obj, video_video_channel_get(obj)));
   printf("  spu  : %i/%i [%s]\n", video_spu_channel_get(obj), video_spu_channel_count(obj), video_spu_channel_name_get(obj, video_spu_channel_get(obj)));
#endif
   win_title_update(data);
}

static void
_cb_audio(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   Evas_Object *sl = elm_object_part_content_get(inf->lay, "rage.vol.swallow");
   double vol = video_volume_get(inf->vid);
   elm_slider_value_set(sl, vol * 100);
   win_title_update(data);
}

static void
_cb_channels(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_title_update(data);
}

static void
_cb_play_start(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   win_aspect_adjust(data);
   win_frame_decode(data);
   win_title_update(data);
   inf->playing = EINA_TRUE;
   mpris_playback_status_change();
}

static void
_cb_play_finish(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   if (!inf->playing)
     {
        Evas_Coord mw = 1, mh = 1;

        elm_layout_signal_emit(inf->lay, "pref,size,on", "rage");
        edje_object_message_signal_process(elm_layout_edje_get(inf->lay));
        edje_object_size_min_calc(elm_layout_edje_get(inf->lay), &mw, &mh);
        elm_layout_signal_emit(inf->lay, "pref,size,off", "rage");
        win_show(data, mw, mh);
     }
   inf->playing = EINA_FALSE;
   mpris_playback_status_change();
}

static void
_cb_button_num(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_title_update(data);
}

static void
_cb_button(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_title_update(data);
}

static void
_cb_vidthumb_loaded(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");

   if (!inf) return;
}

static void
_cb_vidthumb_failed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");

   if (!inf) return;
}

static void
_cb_vidthumb_data(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   int w, h;
   Inf *inf = evas_object_data_get(data, "inf");

   if (!inf) return;
   videothumb_size_get(inf->vidthumb, &w, &h);
   w = ((double)w * elm_config_scale_get()) / 2.0;
   h = ((double)h * elm_config_scale_get()) / 2.0;
   evas_object_size_hint_min_set(inf->vidthumb, w, h);
}

static void
_cb_layout_message(void *data, Evas_Object *obj EINA_UNUSED, Edje_Message_Type type, int id, void *msg)
{
   Inf *inf = evas_object_data_get(data, "inf");

   if (!inf) return;
   if (type == EDJE_MESSAGE_FLOAT)
     {
        if (id == 10)
          {
             Edje_Message_Float *m = msg;
             double len = video_length_get(inf->vid);
             video_position_set(inf->vid, len * m->val);
          }
        else if (id == 13)
          {
             Edje_Message_Float *m = msg;
             double len = video_length_get(inf->vid);
             videothumb_file_set(inf->vidthumb, video_file_get(inf->vid),
                                 len * m->val);
          }
     }
}

void
win_video_init(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Evas_Object *o;
   Evas_Coord sz;

   o = video_add(win);
   video_fill_set(o, EINA_TRUE);
   inf->vid = o;
   evas_object_smart_callback_add(o, "frame_resize", _cb_resize, win);
   evas_object_smart_callback_add(o, "frame_decode", _cb_decode, win);
   evas_object_smart_callback_add(o, "stop", _cb_stop, win);
   evas_object_smart_callback_add(o, "opened", _cb_opened, win);
   evas_object_smart_callback_add(o, "length", _cb_length, win);
   evas_object_smart_callback_add(o, "title", _cb_title, win);
   evas_object_smart_callback_add(o, "audio", _cb_audio, win);
   evas_object_smart_callback_add(o, "channels", _cb_channels, win);
   evas_object_smart_callback_add(o, "play_start", _cb_play_start, win);
   evas_object_smart_callback_add(o, "play_finish", _cb_play_finish, win);
   evas_object_smart_callback_add(o, "button_num", _cb_button_num, win);
   evas_object_smart_callback_add(o, "button", _cb_button, win);
   elm_object_part_content_set(inf->lay, "rage.content", o);
   evas_object_show(o);

   o = videothumb_add(win);
   inf->vidthumb = o;
   evas_object_smart_callback_add(o, "loaded", _cb_vidthumb_loaded, win);
   evas_object_smart_callback_add(o, "failed", _cb_vidthumb_failed, win);
   evas_object_smart_callback_add(o, "data", _cb_vidthumb_data, win);
   sz = 80;
   sz = (double)sz * elm_config_scale_get();
   evas_object_size_hint_min_set(o, sz, sz);
   elm_object_part_content_set(inf->lay, "rage.dragable.content", o);
   evas_object_show(o);
   edje_object_message_handler_set(elm_layout_edje_get(inf->lay),
                                   _cb_layout_message, win);
}

void
win_video_file_list_set(Evas_Object *win, Eina_List *list)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l, *list2 = NULL;
   Winvid_Entry *vid;

   inf->file_cur = NULL;
   EINA_LIST_FREE(inf->file_list, vid)
     {
        if (vid->file) eina_stringshare_del(vid->file);
        if (vid->sub) eina_stringshare_del(vid->sub);
        if (vid->uri) efreet_uri_free(vid->uri);
        free(vid);
     }

   EINA_LIST_FOREACH(list, l, vid)
     {
        Winvid_Entry *vid2;

        vid2 = calloc(1, sizeof(Winvid_Entry));
        if (vid2)
          {
             if (vid->file) vid2->file = eina_stringshare_add(vid->file);
             if (vid->sub) vid2->sub = eina_stringshare_add(vid->sub);
             vid2->uri = efreet_uri_decode(vid->file);
             list2 = eina_list_append(list2, vid2);
          }
     }
   inf->file_list = list2;
   win_video_first(win);
}

void
win_video_insert(Evas_Object *win, const char *file)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Winvid_Entry *vid;

   vid = calloc(1, sizeof(Winvid_Entry));
   vid->file = eina_stringshare_add(file);
   vid->uri = efreet_uri_decode(vid->file);
   inf->file_list = eina_list_append_relative_list(inf->file_list, vid,
                                                   inf->file_cur);
   evas_object_data_set(win, "file_list", inf->file_list);
}

void
win_video_insert_append(Evas_Object *win, const char *file)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Winvid_Entry *vid;

   vid = calloc(1, sizeof(Winvid_Entry));
   vid->file = eina_stringshare_add(file);
   vid->uri = efreet_uri_decode(vid->file);

   inf->file_list = eina_list_append(inf->file_list, vid);
   evas_object_data_set(win, "file_list", inf->file_list);
}

void
win_video_free(Evas_Object *win)
{
   Winvid_Entry *vid;
   Inf *inf = evas_object_data_get(win, "inf");
   if (!inf->file_list) return;

   EINA_LIST_FREE(inf->file_list, vid)
     {
        if (vid->file) eina_stringshare_del(vid->file);
        if (vid->sub) eina_stringshare_del(vid->sub);
        if (vid->uri) efreet_uri_free(vid->uri);
        free(vid);
     }

   // Clean up inf state and reinit window
   // to not confuse rage
   inf->file_cur = NULL;
   inf->vid = NULL;
   win_video_init(win);

   // Refresh to get rid of playlist icons/preview videos
   win_list_content_update(win);
}

void
win_video_goto(Evas_Object *win, Eina_List *l)
{
   Inf *inf = evas_object_data_get(win, "inf");

   inf->file_cur = l;
   win_video_restart(win);
   win_list_sel_update(win);
}
