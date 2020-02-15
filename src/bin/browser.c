#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "browser.h"
#include "videothumb.h"
#include "key.h"
#include "dnd.h"

typedef struct _Message Message;
typedef struct _Entry Entry;

typedef enum _Type
{
   TYPE_NEW,
   TYPE_UPDATE,
   TYPE_FINISH
} Type;

struct _Message
{
   Type type;
   Entry *entry;
};

struct _Entry
{
   Eina_Lock lock;
   Entry *parent; // what is above
   Eina_Stringshare *path; // full path
   Eina_List *dirs; // entries fo subdir entires
   Eina_List *files; // strings of just filenames in path dir
   Eina_List *sels;
   Evas_Object *base;
   Evas_Object *box;
   Evas_Object *table;
   Evas_Object *sizer;
   Evas_Coord iw, ih;
   Evas_Object **file_obj;
   int cols, rows;
   int sel_x, sel_y;
   Eina_Bool sel : 1;
};

typedef struct
{
   char *videos;
   Evas_Object *win;
} Fill_Data;

#define CREATED_MAX      32
#define DESTROYED_MAX    32
#define NORMAL_COLS      12
#define NORMAL_ROWS       4
#define FULLSCREEN_COLS   6
#define FULLSCREEN_ROWS   3
// these gets scaled by scaling factor anyway...
#define ITEM_MIN_W      100
#define ITEM_MIN_H      100
#define ITEM_MAX_W      140
#define ITEM_MAX_H      140
#define ITEM_ASPECT_MIN  50
#define ITEM_ASPECT_MAX 150

static char *selfile = NULL;
static Entry *selentry = NULL;
static int seli = 0;

static Evas_Object *bx = NULL;
static Evas_Object *sc, *bt;
static Ecore_Thread *fill_thread = NULL;
static Entry *dir_entry = NULL;
static Eina_List *entries = NULL;
static Ecore_Timer *_browser_hide_focus_restore_timer = NULL;
static Eina_Semaphore step_sema;
static Ecore_Timer *initial_update_timer = NULL;
static Ecore_Animator *pop_eval_redo = NULL;


static void _pop_eval_delay(Evas_Object *win, int created, int destroyed);
static void _entry_files_pop_eval(Evas_Object *win, Entry *entry);
static void _sel_go(Evas_Object *win EINA_UNUSED, Entry *base_entry, int x, int y);

static void
_item_size_get(Evas_Object *win, Evas_Coord *w, Evas_Coord *h)
{
   Evas_Coord sz = 0;
   Evas_Coord minw = ITEM_MIN_W, minh = ITEM_MIN_H;
   Evas_Coord maxw = ITEM_MAX_W, maxh = ITEM_MAX_H;

   minw = elm_config_scale_get() * (double)minw;
   minh = elm_config_scale_get() * (double)minh;
   maxw = elm_config_scale_get() * (double)maxw;
   maxh = elm_config_scale_get() * (double)maxh;
   elm_coords_finger_size_adjust(1, &sz, 1, &sz);
   evas_object_geometry_get(win, NULL, NULL, w, h);
   if (elm_win_fullscreen_get(win))
     {
        *w = (double)(*w) / (double)FULLSCREEN_COLS;
        *h = (double)(*h) / (double)FULLSCREEN_ROWS;
        maxw = 0;
        maxh = 0;
     }
   else
     {
        *w = (double)(*w) / (double)NORMAL_COLS;
        *h = (double)(*h) / (double)NORMAL_ROWS;
     }
   if (*w < minw) *w = minw;
   if (*h < minh) *h = minh;
   if ((maxw > 0) && (*w > maxw)) *w = maxw;
   if ((maxh > 0) && (*h > maxh)) *h = maxh;
   if (((*w * 100) / *h) < ITEM_ASPECT_MIN)
     *w = (*h * ITEM_ASPECT_MIN) / 100;
   else if (((*w * 100) / *h) > ITEM_ASPECT_MAX)
     *w = (*h * ITEM_ASPECT_MAX) / 100;
   if (*w < minw) *w = minw;
   if (*h < minh) *h = minh;
   if (*w < sz) *w = sz;
   if (*h < sz) *h = sz;
}

static Eina_Bool
_video_ok(const char *path)
{
   const char *exts[] =
     {
        ".asf", ".avi", ".bdm", ".bdmv", ".clpi", ".cpi", ".dv", ".fla",
        ".flv", ".m1v", ".m2t", ".m2v", ".m4v", ".mkv", ".mov", ".mp2",
        ".mp2ts", ".mp4", ".mpe", ".mpeg", ".mpg", ".mpl", ".mpls", ".mts",
        ".mxf", ".nut", ".nuv", ".ogg", ".ogm", ".ogv", ".qt", ".rm", ".rmj",
        ".rmm", ".rms", ".rmvb", ".rmx", ".rv", ".swf", ".ts", ".weba",
        ".webm", ".wmv", ".3g2", ".3gp", ".3gp2", ".3gpp", ".3gpp2", ".3p2",
        ".264",
        NULL
     };
   int i;
   const char *ext = strrchr(path, '.');
   if (!ext) return EINA_FALSE;
   for (i = 0; exts[i]; i++)
     {
        if (!strcasecmp(ext, exts[i])) return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Eina_Bool
_audio_ok(const char *path)
{
   const char *exts[] =
     {
        ".mp3", ".m4a", ".oga", ".aac", ".flac", ".wav",
        NULL
     };
   int i;
   const char *ext = strrchr(path, '.');
   if (!ext) return EINA_FALSE;
   for (i = 0; exts[i]; i++)
     {
        if (!strcasecmp(ext, exts[i])) return EINA_TRUE;
     }
   return EINA_FALSE;
}

static char *
_videos_dir_get(void)
{
   char buf[PATH_MAX];
   const char *vids, *home;
   char *vidsreal = NULL, *homereal = NULL;

   vids = efreet_videos_dir_get();
   if (vids) vidsreal = ecore_file_realpath(vids);
   home = eina_environment_home_get();
   if (home) homereal = ecore_file_realpath(home);
   if ((vidsreal) && (homereal))
     {
        if (!strcmp(vidsreal, homereal)) vids = NULL;
     }
   free(vidsreal);
   free(homereal);
   if (vids)
     snprintf(buf, sizeof(buf), "%s", vids);
   else
     snprintf(buf, sizeof(buf), "%s/Videos", eina_environment_home_get());
   return strdup(buf);
}

static void
_fill_message(Ecore_Thread *th, Type type, Entry *entry)
{
   Message *message = calloc(1, sizeof(Message));
   if (!message) return;
   message->type = type;
   message->entry = entry;
   ecore_thread_feedback(th, message);
   // take semaphore lock after sending message so we wait if the mainloop
   // had not processed the previous message
   eina_semaphore_lock(&step_sema);
}

static Entry *
_fill_scan(Ecore_Thread *th, Entry *parent, const char *dir)
{
   Eina_List *files;
   char *file;
   Entry *entry = NULL;

   entry = calloc(1, sizeof(Entry));
   if (!entry) return NULL;
   files = ecore_file_ls(dir);
   if (!files)
     {
        if (entry == selentry) selentry = NULL;
        free(entry);
        return NULL;
     }
   eina_lock_new(&(entry->lock));
   entry->parent = parent;
   entry->path = eina_stringshare_add(dir);
   if (parent)
     {
        eina_lock_take(&(parent->lock));
        parent->dirs = eina_list_append(parent->dirs, entry);
        eina_lock_release(&(parent->lock));
     }
   _fill_message(th, TYPE_NEW, entry);
   EINA_LIST_FREE(files, file)
     {
        if (!ecore_thread_check(th))
          {
             if (file[0] != '.')
               {
                  char buf[PATH_MAX];

                  snprintf(buf, sizeof(buf), "%s/%s", dir, file);
                  if (ecore_file_is_dir(buf))
                    {
                       _fill_scan(th, entry, buf);
                       _fill_message(th, TYPE_UPDATE, entry);
                    }
                  else
                    {
                       if (_video_ok(file) || _audio_ok(file))
                         {
                            eina_lock_take(&(entry->lock));
                            entry->files = eina_list_append
                              (entry->files, eina_stringshare_add(file));
                            eina_lock_release(&(entry->lock));
                            _fill_message(th, TYPE_UPDATE, entry);
                         }
                    }
               }
          }
        free(file);
     }
   _fill_message(th, TYPE_FINISH, entry);
   return entry;
}

static void
_fill_thread(void *data, Ecore_Thread *th)
{
   Fill_Data *fdat = data;
   _fill_scan(th, NULL, fdat->videos);
}

static void
_cb_vidthumb_data(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   int w, h;

   videothumb_size_get(obj, &w, &h);
   evas_object_size_hint_aspect_set(obj, EVAS_ASPECT_CONTROL_NEITHER, w, h);
}

static void
_activate(Evas_Object *win, Entry *entry, const char *file)
{
   Eina_List *list = NULL;
   Winvid_Entry *vid;
   char buf[PATH_MAX];

   vid = calloc(1, sizeof(Winvid_Entry));
   if (vid)
     {
        snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
        vid->file = eina_stringshare_add(buf);
        list = eina_list_append(list, vid);
     }
   win_video_file_list_set(win, list);
   EINA_LIST_FREE(list, vid)
     {
        if (vid->file) eina_stringshare_del(vid->file);
        if (vid->sub) eina_stringshare_del(vid->sub);
        if (vid->uri) efreet_uri_free(vid->uri);
        free(vid);
     }
   browser_hide(win);
}

static void
_cb_file_selected(void *data, Evas_Object *obj, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   Evas_Object *win = data;
   Entry *entry = evas_object_data_get(obj, "entry");
   char buf[PATH_MAX];
   const char *file = evas_object_data_get(obj, "file");

   elm_layout_signal_emit(obj, "rage,state,selected", "rage");
   evas_object_raise(obj);
   _activate(win, entry, file);
   snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
   if (selfile) free(selfile);
   selfile = strdup(buf);
}

static void
_entry_files_pop_clear(Entry *entry)
{
   Evas_Object *win = evas_object_data_get(entry->table, "win");
   int i, k, destroyed = 0;

   // if we had any content at all before - nuke it all
   if ((entry->sels) && (entry->file_obj))
     {
        k = entry->cols * entry->rows;
        for (i = 0; i < k; i++)
          {
             if (entry->file_obj[i])
               {
                  entry->sels = eina_list_remove(entry->sels, entry->file_obj[i]);
                  evas_object_del(entry->file_obj[i]);
                  entry->file_obj[i] = NULL;
                  destroyed++;
                  if (destroyed >= DESTROYED_MAX) break;
               }
          }
        _pop_eval_delay(win, 0, destroyed);
     }
}

static Eina_Bool
_cb_pop_eval_redo(void *data)
{
   Evas_Object *win = data;
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   Entry *entry;
   pop_eval_redo = NULL;
   if (!inf) return EINA_FALSE;
   if (!bx) return EINA_FALSE;
   EINA_LIST_FOREACH(entries, l, entry)
     {
        if (pop_eval_redo) break;
        _entry_files_pop_eval(win, entry);
     }
   return EINA_FALSE;
}

static void
_pop_eval_delay(Evas_Object *win, int created, int destroyed)
{
   if ((created >= CREATED_MAX) || (destroyed >= DESTROYED_MAX))
     {
        if (pop_eval_redo)
          {
             ecore_animator_del(pop_eval_redo);
             pop_eval_redo = NULL;
          }
        pop_eval_redo = ecore_animator_add(_cb_pop_eval_redo, win);
     }
}

static void
_entry_files_pop_eval(Evas_Object *win, Entry *entry)
{
   const char *file;
   Eina_List *l;
   Evas_Object **obj;
   Evas_Coord win_w, win_h, ent_x, ent_y, ent_w, ent_h;
   int i = 0, j = 0, created = 0, destroyed = 0;
   Eina_Rectangle win_rect, file_rect;

   if (pop_eval_redo) return;
   evas_object_geometry_get(entry->table, &ent_x, &ent_y, &ent_w, &ent_h);
   evas_object_geometry_get(win, NULL, NULL, &win_w, &win_h);
   win_rect.x = 0;
   win_rect.y = 0;
   win_rect.w = win_w;
   win_rect.h = win_h;

   // if we're not in the viewport at all empty all content
   file_rect.x = ent_x;
   file_rect.y = ent_y;
   file_rect.w = ent_w;
   file_rect.h = ent_h;
   file_rect.x -= 80;
   file_rect.y -= 80;
   file_rect.w += 160;
   file_rect.h += 160;
   if (!eina_rectangles_intersect(&win_rect, &file_rect))
     {
        _entry_files_pop_clear(entry);
        return;
     }
   // walk files to find which intersect the window
   EINA_LIST_FOREACH(entry->files, l, file)
     {
        if ((created >= CREATED_MAX) && (destroyed >= DESTROYED_MAX)) break;
        file_rect.x = ent_x + ((i * ent_w) / entry->cols);
        file_rect.y = ent_y + ((j * ent_h) / entry->rows);
        file_rect.w = (ent_w / entry->cols);
        file_rect.h = (ent_h / entry->rows);
        file_rect.x -= 80;
        file_rect.y -= 80;
        file_rect.w += 160;
        file_rect.h += 160;
        obj = &(entry->file_obj[(j * entry->cols) + i]);
        if (eina_rectangles_intersect(&win_rect, &file_rect))
          {
             if ((!(*obj)) && (created < CREATED_MAX))
               {
                  Evas_Object *o, *base;
                  char buf[PATH_MAX], *p;

                  base = o = elm_layout_add(win);
                  *obj = o;
                  entry->sels = eina_list_append(entry->sels, o);
                  evas_object_data_set(o, "entry", entry);
                  evas_object_data_set(o, "file", file);
                  elm_object_focus_allow_set(o, EINA_FALSE);
                  snprintf(buf, sizeof(buf), "%s/themes/default.edj",
                           elm_app_data_dir_get());
                  elm_layout_file_set(o, buf, "rage/browser/item");
                  if (elm_win_fullscreen_get(win))
                    elm_layout_signal_emit(base, "state,fullscreen", "rage");
                  else
                    elm_layout_signal_emit(base, "state,normal", "rage");
                  snprintf(buf, sizeof(buf), "%s", file);
                  for (p = buf; *p; p++)
                    {
                       // nuke stupid characters from the label that may be
                       // in filename
                       if ((*p == '_') || (*p == '#') || (*p == '$') ||
                           (*p == '%') || (*p == '*') || (*p == '+') ||
                           (*p == '[') || (*p == ']') || (*p == ';') ||
                           (*p == '<') || (*p == '=') || (*p == '>') ||
                           (*p == '^') || (*p == '`') || (*p == '{') ||
                           (*p == '}') || (*p == '|') || (*p == '~') ||
                           (*p == 127) || (*p == '\'') || (*p == '\\'))
                         {
                            *p = ' ';
                         }
                       else if (*p == '.')
                         {
                            *p = 0;
                            break;
                         }
                    }
                  elm_object_part_text_set(o, "rage.title", buf);
                  evas_object_size_hint_weight_set(o, 0.0, 0.0);
                  evas_object_size_hint_align_set(o,
                                                  EVAS_HINT_FILL,
                                                  EVAS_HINT_FILL);
                  elm_table_pack(entry->table, o, i, j, 1, 1);
                  evas_object_show(o);

                  elm_layout_signal_callback_add(o, "rage,selected", "rage",
                                                 _cb_file_selected, win);

                  o = videothumb_add(win);
                  videothumb_poster_mode_set(o, EINA_TRUE);
                  evas_object_smart_callback_add(o, "data",
                                                 _cb_vidthumb_data, base);
                  evas_object_data_set(o, "entry", entry);
                  evas_object_data_set(o, "file", file);
                  snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
                  videothumb_file_set(o, buf, 0.0);
                  videothumb_autocycle_set(o, EINA_TRUE);
                  elm_object_part_content_set(base, "rage.content", o);
                  evas_object_show(o);

                  if ((entry->sel) &&
                      (entry->sel_x == i) && (entry->sel_y == j))
                    {
                       elm_layout_signal_emit(base, "rage,state,selected",
                                              "rage");
                       evas_object_raise(base);
                    }
                  created++;
               }
          }
        else
          {
             if ((*obj) && (destroyed < DESTROYED_MAX))
               {
                  entry->sels = eina_list_remove(entry->sels, *obj);
                  evas_object_del(*obj);
                  *obj = NULL;
                  destroyed++;
               }
          }
        i++;
        if (i == entry->cols)
          {
             i = 0;
             j++;
          }
     }
   _pop_eval_delay(win, created, destroyed);
}

static void
_entry_update(Evas_Object *win, Entry *entry)
{
   eina_lock_take(&(entry->lock));
   _entry_files_pop_eval(win, entry);
   eina_lock_release(&(entry->lock));
}

static void
_cb_sel_job(void *data)
{
   Evas_Object *win = data;
   Entry *entry = selentry;
   if ((!dir_entry) || (!entry)) return;
   eina_lock_take(&(entry->lock));
   entry->sel = EINA_TRUE;
   if (entry->cols > 0) entry->sel_y = seli / entry->cols;
   entry->sel_x = seli - (entry->sel_y * entry->cols);
   eina_lock_release(&(entry->lock));
   _sel_go(win, dir_entry, 0, 0);
}

static void
_entry_files_redo(Evas_Object *win, Entry *entry)
{
   Evas_Coord x, y,w, h, iw = 1, ih = 1, ww, wh, sw, sh;
   int num, cols, rows;

   eina_lock_take(&(entry->lock));
   if (elm_win_fullscreen_get(win))
     elm_layout_signal_emit(entry->base, "state,fullscreen", "rage");
   else
     elm_layout_signal_emit(entry->base, "state,normal", "rage");

   num = eina_list_count(entry->files);
   evas_object_geometry_get(win, NULL, NULL, &ww, &wh);
   evas_object_geometry_get(entry->table, &x, &y, &w, &h);
   elm_scroller_region_get(sc, NULL, NULL, &sw, &sh);
   if (sw < w) w = sw;

   _entry_files_pop_clear(entry);
   free(entry->file_obj);
   entry->file_obj = NULL;

   _item_size_get(win, &iw, &ih);
   cols = w / iw;
   if (cols < 1) cols = 1;
   rows = (num + (cols - 1)) / cols;

   entry->iw = iw - 1;
   entry->ih = ih - 1;
   entry->cols = cols;
   entry->rows = rows;

   entry->file_obj = calloc(entry->cols * entry->rows,
                            sizeof(Evas_Object *));
   if ((entry->cols > 0) && (entry->rows > 0))
     elm_table_pack(entry->table, entry->sizer, 0, 0,
                    entry->cols, entry->rows);
   else
     elm_table_pack(entry->table, entry->sizer, 0, 0, 1, 1);
   evas_object_size_hint_min_set(entry->sizer,
                                 entry->cols * entry->iw,
                                 entry->rows * entry->ih);
   _entry_files_pop_eval(win, entry);

   if (selfile)
     {
        Eina_List *l;
        const char *file;
        char buf[PATH_MAX];
        int i;

        i = 0;
        EINA_LIST_FOREACH(entry->files, l, file)
          {
             snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
             if (!strcmp(buf, selfile))
               {
                  selentry = entry;
                  seli = i;
                  ecore_job_add(_cb_sel_job, win);
                  break;
               }
             i++;
          }
     }
   eina_lock_release(&(entry->lock));
}

static void
_cb_entry_table_move(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *info EINA_UNUSED)
{
   Entry *entry = data;
   Evas_Object *win = evas_object_data_get(obj, "win");
   if (initial_update_timer) return;
   _entry_update(win, entry);
}

static void
_cb_entry_table_resize(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *info EINA_UNUSED)
{
   Entry *entry = data;
   Evas_Object *win = evas_object_data_get(obj, "win");
   if (initial_update_timer) return;
   _entry_files_redo(win, entry);
}

static void
_cb_scroller_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   Eina_List *l;
   Entry *entry;
   if ((!inf) || (!bx)) return;
   if (initial_update_timer) return;
   EINA_LIST_FOREACH(entries, l, entry)
     {
        _entry_files_redo(data, entry);
     }
}

static Eina_Bool
_cb_initial_update_timer(void *data)
{
   Inf *inf = evas_object_data_get(data, "inf");
   Eina_List *l;
   Entry *entry;

   initial_update_timer = NULL;
   if ((!inf) || (!bx)) return EINA_FALSE;
   EINA_LIST_FOREACH(entries, l, entry)
     {
        _entry_files_redo(data, entry);
     }
   return EINA_FALSE;
}

static void
_fill_feedback(void *data, Ecore_Thread *th, void *msg)
{
   Fill_Data *fdat = data;
   Evas_Object *win = fdat->win;
   Message *message = msg;
   Evas_Object *o;
   char buf[PATH_MAX];

   if ((th == fill_thread) && (bx))
     {
        Entry *entry = message->entry;

        if ((message->type == TYPE_NEW) && (!dir_entry)) dir_entry = entry;

        if (message->type == TYPE_NEW)
          {
             eina_lock_take(&(entry->lock));
//             if ((entry->dirs) || (entry->files))
               {
                  if (!entry->base)
                    {
                       const char *file;

                       entry->base = o = elm_layout_add(win);
                       elm_object_focus_allow_set(o, EINA_FALSE);
                       snprintf(buf, sizeof(buf), "%s/themes/default.edj", elm_app_data_dir_get());
                       elm_layout_file_set(o, buf, "rage/browser/entry");
                       if (elm_win_fullscreen_get(win))
                         elm_layout_signal_emit(entry->base, "state,fullscreen", "rage");
                       else
                         elm_layout_signal_emit(entry->base, "state,normal", "rage");
                       evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
                       evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);

                       if (entry->parent)
                         {
                            file = entry->path + strlen(dir_entry->path) + 1;
                            elm_object_part_text_set(o, "rage.title", file);
                         }

                       entry->box = o = elm_box_add(win);
                       evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
                       evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
                       elm_object_part_content_set(entry->base, "rage.content", o);
                       evas_object_show(o);

                       entry->table = o = elm_table_add(win);
                       elm_table_homogeneous_set(o, EINA_TRUE);
                       evas_object_data_set(o, "win", win);
                       evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, _cb_entry_table_move, entry);
                       evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _cb_entry_table_resize, entry);
                       evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
                       evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
                       elm_box_pack_end(entry->box, o);
                       evas_object_show(o);

                       entry->sizer = o = evas_object_rectangle_add(evas_object_evas_get(win));
                       evas_object_color_set(o, 0, 0, 0, 0);
                       evas_object_size_hint_min_set(o, 10, 10);
                       elm_table_pack(entry->table, o, 0, 0, 1, 1);

                       if ((!entry->parent) ||
                           ((entry->parent) && (!entry->parent->parent)) ||
                           (!entry->parent->box))
                         elm_box_pack_end(bx, entry->base);
                       else
                         elm_box_pack_end(entry->parent->box, entry->base);
                       evas_object_show(entry->base);

                    }
               }
             entries = eina_list_append(entries, entry);
             eina_lock_release(&(entry->lock));
          }
        else if ((message->type == TYPE_FINISH) && (entry->parent))
          {
             if (!initial_update_timer)
               _entry_files_redo(win, entry);
             else
               {
                  int iw, ih, cols, rows, num;
                  Evas_Coord w, h;

                  eina_lock_take(&(entry->lock));
                  num = eina_list_count(entry->files);
                  evas_object_geometry_get(entry->table, NULL, NULL, &w, &h);
                  _item_size_get(win, &iw, &ih);
                  cols = w / iw;
                  if (cols < 1) cols = 1;
                  rows = (num + (cols - 1)) / cols;

                  entry->iw = iw - 1;
                  entry->ih = ih - 1;
                  entry->cols = cols;
                  entry->rows = rows;

                  evas_object_size_hint_min_set(entry->sizer,
                                                entry->cols * entry->iw,
                                                entry->rows * entry->ih);
                  if ((entry->cols > 0) && (entry->rows > 0))
                    elm_table_pack(entry->table, entry->sizer, 0, 0,
                                   entry->cols, entry->rows);
                  else
                    elm_table_pack(entry->table, entry->sizer, 0, 0, 1, 1);
                  eina_lock_release(&(entry->lock));
               }
          }
     }
   // allow the freedback thread to step more
   eina_semaphore_release(&step_sema, 1);
   free(msg);
}

static void
_fill_end(void *data, Ecore_Thread *th)
{
   Fill_Data *fdat = data;
   if (th == fill_thread) fill_thread = NULL;
   eina_semaphore_free(&step_sema);
   free(fdat->videos);
   free(fdat);
}

static void
_fill_cancel(void *data EINA_UNUSED, Ecore_Thread *th)
{
   Fill_Data *fdat = data;
   if (th == fill_thread) fill_thread = NULL;
   free(fdat->videos);
   free(fdat);
}

static void
_entry_free(Entry *entry)
{
   Entry *subentry;
   Eina_Stringshare *str;
   if (!entry) return;
   eina_lock_take(&(entry->lock));
   entry->sels = eina_list_free(entry->sels);
   free(entry->file_obj);
   EINA_LIST_FREE(entry->files, str) eina_stringshare_del(str);
   EINA_LIST_FREE(entry->dirs, subentry) _entry_free(subentry);
   if (entry->base) evas_object_del(entry->base);
   eina_stringshare_del(entry->path);
   if (entry == selentry) selentry = NULL;
   entries = eina_list_remove(entries, entry);
   eina_lock_release(&(entry->lock));
   eina_lock_free(&(entry->lock));
   free(entry);
}

static void
_fill(Evas_Object *win)
{
   Fill_Data *fdat;

   if (fill_thread)
     {
        ecore_thread_cancel(fill_thread);
        ecore_thread_wait(fill_thread, 10.0);
     }
   _entry_free(dir_entry);
   dir_entry = NULL;
   fdat = malloc(sizeof(Fill_Data));
   if (!fdat) return;
   fdat->videos = _videos_dir_get();
   fdat->win = win;
   eina_semaphore_new(&step_sema, 0);
   fill_thread = ecore_thread_feedback_run(_fill_thread, _fill_feedback,
                                           _fill_end, _fill_cancel,
                                           fdat, EINA_TRUE);
}

static Entry *
_sel_find(Entry *entry)
{
   Eina_List *l;
   Entry *subentry, *tmpentry;

   eina_lock_take(&(entry->lock));
   if (entry->sel)
     {
        eina_lock_release(&(entry->lock));
        return entry;
     }
   EINA_LIST_FOREACH(entry->dirs, l, subentry)
     {
        tmpentry = _sel_find(subentry);
        if (tmpentry)
          {
             eina_lock_release(&(entry->lock));
             return tmpentry;
          }
     }
   eina_lock_release(&(entry->lock));
   return NULL;
}

static Evas_Object *
_sel_object_find(Entry *entry)
{
   int num = (entry->sel_y * entry->cols) + entry->sel_x;
   if (!entry->file_obj) return NULL;
   Evas_Object *o = entry->file_obj[num];
   return o;
}

static const char *
_sel_file_find(Entry *entry)
{
   int num = (entry->sel_y * entry->cols) + entry->sel_x;
   const char *file = eina_list_nth(entry->files, num);
   return file;
}

static Eina_List *
_entry_list_flatten(Eina_List *list, Entry *entry)
{
   Eina_List *l;
   Entry *subentry;

   eina_lock_take(&(entry->lock));
   if (entry->files) list = eina_list_append(list, entry);
   EINA_LIST_FOREACH(entry->dirs, l, subentry)
     {
        list = _entry_list_flatten(list, subentry);
     }
   eina_lock_release(&(entry->lock));
   return list;
}

static void
_sel_go(Evas_Object *win EINA_UNUSED, Entry *base_entry, int x, int y)
{
   Evas_Object *o;
   Evas_Coord bxx, bxy, tbx, tby;
   const char *file;

   if (!base_entry) return;
   evas_object_geometry_get(bx, &bxx, &bxy, NULL, NULL);
   Entry *entry = _sel_find(base_entry);
   if (!entry)
     {
        Eina_List *flatlist = _entry_list_flatten(NULL, base_entry);

        if (flatlist)
          {
             entry = flatlist->data;
             eina_lock_take(&(entry->lock));
             entry->sel = EINA_TRUE;
             entry->sel_x = 0;
             entry->sel_y = 0;
             o = _sel_object_find(entry);
             elm_layout_signal_emit(o, "rage,state,selected", "rage");
             evas_object_raise(o);
             eina_lock_release(&(entry->lock));
             eina_list_free(flatlist);
          }
     }
   else
     {
        int sel_x, sel_y, num;
        Entry *subentry;
        Eina_List *l;
        Eina_List *flatlist = _entry_list_flatten(NULL, base_entry);

        eina_lock_take(&(entry->lock));
        sel_x = entry->sel_x + x;
        sel_y = entry->sel_y + y;
        if (sel_x >= entry->cols)
          {
             sel_y++;
             sel_x = 0;
          }
        else if (sel_x < 0)
          {
             sel_y--;
             sel_x = entry->cols - 1;
          }
        num = (sel_y * entry->cols) + sel_x;
        if (num < 0)
          {
             EINA_LIST_FOREACH(flatlist, l, subentry)
               {
                  if (subentry == entry)
                    {
                       if (l->prev)
                         {
                            subentry = l->prev->data;
                            entry->sel = EINA_FALSE;
                            subentry->sel = EINA_TRUE;
                            if (sel_x < subentry->cols)
                              subentry->sel_x = sel_x;
                            else
                              subentry->sel_x = subentry->cols - 1;
                            subentry->sel_y = subentry->rows - 1;
                            num = eina_list_count(subentry->files) - 1 -
                              ((subentry->sel_y * subentry->cols) +
                               subentry->sel_x);
                            if (num < 0) subentry->sel_x += num;
                            evas_object_geometry_get(subentry->table, &tbx, &tby, NULL, NULL);
                            o = _sel_object_find(entry);
                            if (o) elm_layout_signal_emit(o, "rage,state,unselected", "rage");
                            o = _sel_object_find(subentry);
                            if (o)
                              {
                                 elm_layout_signal_emit(o, "rage,state,selected", "rage");
                                 evas_object_raise(o);
                              }
                            elm_scroller_region_bring_in
                              (sc,
                               (tbx - bxx) + (subentry->sel_x * subentry->iw),
                               (tby - bxy) + (subentry->sel_y * subentry->ih),
                               entry->iw, entry->ih);
                         }
                       break;
                    }
               }
          }
        else if (num >= (int)eina_list_count(entry->files))
          {
             EINA_LIST_FOREACH(flatlist, l, subentry)
               {
                  if (subentry == entry)
                    {
                       if (l->next)
                         {
                            subentry = l->next->data;
                            entry->sel = EINA_FALSE;
                            subentry->sel = EINA_TRUE;
                            if (sel_x < subentry->cols)
                              subentry->sel_x = sel_x;
                            else
                              subentry->sel_x = subentry->cols - 1;
                            subentry->sel_y = 0;
                            num = eina_list_count(subentry->files) - 1 -
                              ((subentry->sel_y * subentry->cols) +
                               subentry->sel_x);
                            if (num < 0) subentry->sel_x += num;
                            evas_object_geometry_get(subentry->table, &tbx, &tby, NULL, NULL);
                            o = _sel_object_find(entry);
                            if (o) elm_layout_signal_emit(o, "rage,state,unselected", "rage");
                            o = _sel_object_find(subentry);
                            if (o)
                              {
                                 elm_layout_signal_emit(o, "rage,state,selected", "rage");
                                 evas_object_raise(o);
                              }
                            elm_scroller_region_bring_in
                              (sc,
                               (tbx - bxx) + (subentry->sel_x * subentry->iw),
                               (tby - bxy) + (subentry->sel_y * subentry->ih),
                               entry->iw, entry->ih);
                         }
                       break;
                    }
               }
          }
        else
          {
             o = _sel_object_find(entry);
             if (o) elm_layout_signal_emit(o, "rage,state,unselected", "rage");
             entry->sel_x = sel_x;
             entry->sel_y = sel_y;
             evas_object_geometry_get(entry->table, &tbx, &tby, NULL, NULL);
             o = _sel_object_find(entry);
             if (o)
               {
                  elm_layout_signal_emit(o, "rage,state,selected", "rage");
                  evas_object_raise(o);
               }
             elm_scroller_region_bring_in
               (sc,
                (tbx - bxx) + (entry->sel_x * entry->iw),
                (tby - bxy) + (entry->sel_y * entry->ih),
                entry->iw, entry->ih);
          }
        eina_lock_release(&(entry->lock));
        eina_list_free(flatlist);
     }
   entry = _sel_find(base_entry);
   if (entry)
     {
        file = _sel_file_find(entry);
        if (file)
          {
             char buf[PATH_MAX];
             snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
             if (selfile) free(selfile);
             selfile = strdup(buf);
          }
     }
}

static void
_sel_do(Evas_Object *win, Entry *base_entry)
{
   Entry *entry;

   if (!base_entry) return;
   entry = _sel_find(base_entry);
   if (entry)
     {
        eina_lock_take(&(entry->lock));
        Evas_Object *o = _sel_object_find(entry);
        const char *file = _sel_file_find(entry);
        if (file)
          {
             char buf[PATH_MAX];

             elm_layout_signal_emit(o, "rage,state,selected", "rage");
             evas_object_raise(o);
             _activate(win, entry, file);
             snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
             if (selfile) free(selfile);
             selfile = strdup(buf);
         }
        eina_lock_release(&(entry->lock));
     }
}

static void
_cb_key_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   Evas_Object *win = data;

   if ((!strcmp(ev->key, "Left")) ||
       (!strcmp(ev->key, "bracketleft")))
     {
        _sel_go(win, dir_entry, -1, 0);
     }
   else if ((!strcmp(ev->key, "Right")) ||
            (!strcmp(ev->key, "bracketright")))
     {
        _sel_go(win, dir_entry, 1, 0);
     }
   else if ((!strcmp(ev->key, "Up")) ||
            (!strcmp(ev->key, "XF86AudioPrev")))
     {
        _sel_go(win, dir_entry, 0, -1);
     }
   else if ((!strcmp(ev->key, "Prior")))
     {
        _sel_go(win, dir_entry, 0, -10);
     }
   else if ((!strcmp(ev->key, "Down")) ||
            (!strcmp(ev->key, "XF86AudioNext")))
     {
        _sel_go(win, dir_entry, 0, 1);
     }
   else if ((!strcmp(ev->key, "Next")))
     {
        _sel_go(win, dir_entry, 0, 10);
     }
   else if ((!strcmp(ev->key, "space")) ||
            (!strcmp(ev->key, "Pause")) ||
            (!strcmp(ev->keyname, "p")) ||
            (!strcmp(ev->key, "XF86AudioPlay")) ||
            (!strcmp(ev->key, "Return")) ||
            (!strcmp(ev->key, "KP_Enter")))
     {
        _sel_do(win, dir_entry);
     }
   else if ((!strcmp(ev->keyname, "q")) ||
            (!strcmp(ev->key, "Escape")) ||
            (!strcmp(ev->key, "e")))
     {
        browser_hide(win);
     }
   else key_handle(win, ev);
   elm_object_focus_set(bt, EINA_TRUE);
}

static Ecore_Timer *focus_timer = NULL;

static Eina_Bool
_browser_focus_timer_cb(void *data)
{
   focus_timer = NULL;
   elm_object_focus_set(data, EINA_TRUE);
   return EINA_FALSE;
}

Eina_Bool
browser_visible(void)
{
   if (bx) return EINA_TRUE;
   return EINA_FALSE;
}

void
browser_show(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   if (_browser_hide_focus_restore_timer) ecore_timer_del(_browser_hide_focus_restore_timer);
   _browser_hide_focus_restore_timer = NULL;
   if (!bx)
     {
        bx = elm_box_add(win);

        sc = elm_scroller_add(win);
        evas_object_event_callback_add(sc, EVAS_CALLBACK_RESIZE, _cb_scroller_resize, win);
        dnd_init(win, sc);
        elm_object_style_set(sc, "noclip");
        elm_object_focus_allow_set(sc, EINA_FALSE);
        evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(sc, EVAS_HINT_FILL, EVAS_HINT_FILL);
//        elm_scroller_content_min_limit(sc, EINA_TRUE, EINA_FALSE);

        bx = elm_box_add(win);
        elm_object_focus_allow_set(bx, EINA_FALSE);
        evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, 0.5);
        elm_object_content_set(sc, bx);
        evas_object_show(bx);

        elm_object_part_content_set(inf->lay, "rage.browser", sc);

        _fill(win);

        // a dummy button to collect key events and have focus
        bt = elm_button_add(win);
        elm_object_focus_highlight_style_set(bt, "blank");
        evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add(win, bt);
        evas_object_lower(bt);
        evas_object_show(bt);
        evas_object_event_callback_add(bt, EVAS_CALLBACK_KEY_DOWN,
                                       _cb_key_down, win);

        if (focus_timer) ecore_timer_del(focus_timer);
        focus_timer = ecore_timer_add(0.7, _browser_focus_timer_cb, bt);
     }
   elm_layout_signal_emit(inf->lay, "browser,state,visible", "rage");
   initial_update_timer = ecore_timer_add(0.2, _cb_initial_update_timer, win);
}

static void
_cb_hidden(void *data, Evas_Object *obj, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   elm_layout_signal_callback_del(obj, "browser,state,hidden,finished", "rage",
                                  _cb_hidden);
   if (fill_thread) ecore_thread_cancel(fill_thread);
   if (dir_entry) _entry_free(dir_entry);
   dir_entry = NULL;
   evas_object_del(bx);
   bx = NULL;
   evas_object_del(bt);
   bt = NULL;
   evas_object_del(sc);
   sc = NULL;
   elm_object_focus_next(data, ELM_FOCUS_PREVIOUS);
}

static Eina_Bool
_browser_hide_focus_restore_cb(void *data)
{
   win_focus(data);
   _browser_hide_focus_restore_timer = NULL;
   return EINA_FALSE;
}

void
browser_hide(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   if (!bx) return;
   if (initial_update_timer)
     {
        ecore_timer_del(initial_update_timer);
        initial_update_timer = NULL;
     }
   if (focus_timer) ecore_timer_del(focus_timer);
   focus_timer = NULL;
   elm_layout_signal_callback_add(inf->lay, "browser,state,hidden,finished", "rage",
                                  _cb_hidden, win);
   elm_layout_signal_emit(inf->lay, "browser,state,hidden", "rage");
   if (_browser_hide_focus_restore_timer) ecore_timer_del(_browser_hide_focus_restore_timer);
   _browser_hide_focus_restore_timer = ecore_timer_add(0.2, _browser_hide_focus_restore_cb, win);
}

void
browser_toggle(Evas_Object *win)
{
   if (bx) browser_hide(win);
   else browser_show(win);
}

void
browser_size_update(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   Entry *entry;
   if (!inf) return;
   if (!bx) return;
   if (initial_update_timer) return;
   EINA_LIST_FOREACH(entries, l, entry)
     {
        _entry_files_redo(win, entry);
     }
}

void
browser_fullscreen(Evas_Object *win, EINA_UNUSED Eina_Bool enabled)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   Entry *entry;
   if (!inf) return;
   if (!bx) return;
   if (initial_update_timer) return;
   EINA_LIST_FOREACH(entries, l, entry)
     {
        _entry_files_redo(win, entry);
     }
}
