#ifndef _WIN_H__
#define _WIN_H__ 1

typedef struct _Inf Inf;

struct _Inf
{
   Evas_Object *vid, *lay, *event, *event2, *glayer, *vidthumb, *artimg, *focus;
   Eina_List *file_list, *file_cur;
   Ecore_Job *next_job;
   Ecore_Timer *show_timeout;
   Ecore_Timer *mouse_idle_timeout;
   Ecore_Timer *albumart_timeout;
   Ecore_Timer *down_timeout;
   Ecore_Animator *drag_anim;
   double last_action;
   double jump;
   double start_pos;
   double drag_time, drag_start;
   int zoom_mode;
   int drag_dist, drag_momentum;
   Evas_Coord down_x, down_y;
   Eina_Bool sized : 1;
   Eina_Bool last_action_rwind : 1;
   Eina_Bool playing : 1;
   Eina_Bool was_playing : 1;
   Eina_Bool dragging : 1;
   Eina_Bool browse_mode : 1;
   Eina_Bool down : 1;
};

// ui high level controls
void win_do_play(Evas_Object *win);
void win_do_pause(Evas_Object *win);
void win_do_play_pause(Evas_Object *win);
void win_do_prev(Evas_Object *win);
void win_do_next(Evas_Object *win);
void win_do_volume(Evas_Object *win, double vol);

// lower level controls
void win_video_restart(Evas_Object *win);
Eina_Bool win_video_next(Evas_Object *win);
void win_video_prev(Evas_Object *win);
void win_video_first(Evas_Object *win);
void win_video_last(Evas_Object *win);
void win_video_delete(Evas_Object *win);
Eina_Bool win_video_have_next(Evas_Object *win);
Eina_Bool win_video_have_prev(Evas_Object *win);
Evas_Object *win_add(void);
void win_focus(Evas_Object *win);
void win_title_update(Evas_Object *win);
void win_show(Evas_Object *win, int w, int h);
void win_art(Evas_Object *win, const char *path);
void win_aspect_adjust(Evas_Object *win);
void win_frame_decode(Evas_Object *win);

#endif
