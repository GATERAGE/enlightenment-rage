#include <Elementary.h>
#include <Emotion.h>
#include <Eio.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "winlist.h"
#include "dnd.h"
#include "browser.h"

static int pending_dir = 0;
static Eina_List *playlist = NULL;

void
_cb_drag_enter(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED)
{
}

void
_cb_drag_leave(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED)
{
}

void
_cb_drag_pos(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED, Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED, Elm_Xdnd_Action action EINA_UNUSED)
{
   /* printf("dnd at %i %i act:%i\n", x, y, action); */
}

static void
_dnd_finish(Evas_Object *win)
{
   Eina_List *l;
   char *path;

   if (!eina_list_count(playlist))
      goto end;

   EINA_LIST_FOREACH(playlist, l, path)
     {
        win_video_insert(win, path);
        free(path);
     }

   win_video_next(win);
   win_list_content_update(win);
   browser_hide(win);

end:
   pending_dir = 0;
   eina_list_free(playlist);
   playlist = NULL;
}

static int
_xtov(char x)
{
   if ((x >= '0') && (x <= '9')) return x - '0';
   if ((x >= 'a') && (x <= 'f')) return 10 + (x - 'a');
   if ((x >= 'A') && (x <= 'F')) return 10 + (x - 'A');
   return 0;
}

static char *
_escape_parse(const char *str)
{
   char *dest = malloc(strlen(str) + 1);
   char *d;
   const char *s;

   for (d = dest, s = str; *s; d++)
     {
        if (s[0] == '%' && !isspace(s[1]))
          {
             if (s[1] && s[2])
               {
                  *d = (_xtov(s[1]) << 4) | (_xtov(s[2]));
                  s += 3;
               }
             else s++;
          }
        else
          {
             *d = s[0];
             s++;
          }
     }
   *d = 0;
   return dest;
}

struct _recurse_data
{
   Evas_Object *win;
   Eina_List *list;
};

static int
_pathcmp(const char *p1, const char *p2)
{
   int i = 0;

   if (!p1)
      return 1;

   if (!p2)
      return -1;

   /* skip common path */
   while (p1[i] == p2[i])
      ++i;

   return strcasecmp(p1+i, p2+i);
}

static void
_cb_recurse_end(void *data, Eio_File *f EINA_UNUSED)
{
   struct _recurse_data *d = data;

   if (!eina_list_count(d->list))
      goto end;

   if (!playlist)
     {
        playlist = d->list;
        goto end;
     }

   playlist = eina_list_sorted_merge(playlist, d->list, EINA_COMPARE_CB(_pathcmp));

end:
   if (--pending_dir == 0)
       _dnd_finish(d->win);

   free(d);
}

static void
_cb_recurse_error(void *data EINA_UNUSED, Eio_File *f EINA_UNUSED, int error)
{
   printf("ERROR: I/O error while recursing: %i\n", error);
}

static Eina_Bool
_cb_recurse_filter(void *data EINA_UNUSED, Eio_File *f EINA_UNUSED, const Eina_File_Direct_Info *info)
{
   return info->type == EINA_FILE_REG || info->type == EINA_FILE_DIR;
}

static void
_cb_recurse(void *data, Eio_File *f EINA_UNUSED, const Eina_File_Direct_Info *info)
{
   struct _recurse_data *d = data;
   if (info->type == EINA_FILE_DIR)
      return;

   if (emotion_object_extension_may_play_get(info->path))
      d->list = eina_list_sorted_insert(d->list, EINA_COMPARE_CB(_pathcmp), _escape_parse(info->path));
}

static void
_recurse_dir(Evas_Object *win, const char *path)
{
   struct _recurse_data *data = calloc(1, sizeof(*data));
   data->win = win;
   eio_dir_stat_ls(path, _cb_recurse_filter, _cb_recurse, _cb_recurse_end,
                   _cb_recurse_error, data);
}

Eina_Bool
_cb_drop(void *data, Evas_Object *o EINA_UNUSED, Elm_Selection_Data *ev)
{
   Evas_Object *win = data;
   char **plist, **p, *esc, *tmp;

   if (!ev->data)
      return EINA_TRUE;

   tmp = malloc(ev->len + 1);
   if (!tmp) return EINA_TRUE;
   memcpy(tmp, ev->data, ev->len);
   tmp[ev->len] = 0;
   plist = eina_str_split(tmp, "\n", -1);
   for (p = plist; *p != NULL; ++p)
     {
        esc = _escape_parse(*p);
        if (!esc)
           continue;

        if (ecore_file_is_dir(esc))
          {
             pending_dir++;
             _recurse_dir(win, esc);
             free(esc);
             continue;
          }

        if (emotion_object_extension_may_play_get(esc))
           playlist = eina_list_sorted_insert(playlist, EINA_COMPARE_CB(_pathcmp), esc);
     }

   free(*plist);
   free(plist);
   free(tmp);

   if (!pending_dir)
      _dnd_finish(win);

   return EINA_TRUE;
}

void
dnd_init(Evas_Object *win, Evas_Object *tgt)
{
   eio_init();
   elm_drop_target_add(tgt,
                       ELM_SEL_FORMAT_TEXT | ELM_SEL_FORMAT_IMAGE,
                       _cb_drag_enter, win,
                       _cb_drag_leave, win,
                       _cb_drag_pos, win,
                       _cb_drop, win);
}

void
dnd_shutdown(void)
{
   eio_shutdown();
}
