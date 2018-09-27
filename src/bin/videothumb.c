#include <Elementary.h>
#include "videothumb.h"
#include "sha1.h"
#include "albumart.h"

typedef struct _Videothumb Videothumb;
typedef struct _Pending Pending;

struct _Videothumb
{
   Evas_Object_Smart_Clipped_Data __clipped_data;
   Evas_Object *o_img, *o_img2;
   Pending *pending;
   Ecore_Timer *cycle_timer;
   Ecore_Timer *launch_timer;
   const char *file;
   const char *realfile;
   char *realpath;
   double pos;
   unsigned int realpos;
   int iw, ih;
   Evas_Coord w, h;
   int tried;
   Eina_Bool seen : 1;
   Eina_Bool poster_mode : 1;
   Eina_Bool poster : 1;
};

struct _Pending
{
   Ecore_Exe *exe;
   Evas_Object *obj;
   Eina_Stringshare *realpath;
};

static Evas_Smart *_smart = NULL;
static Evas_Smart_Class _parent_sc = EVAS_SMART_CLASS_INIT_NULL;

static Ecore_Event_Handler *exe_handler = NULL;
static Eina_List *busy_thumbs = NULL;
static Eina_List *vidthumbs = NULL;

static Eina_Bool _cb_thumb_exe(void *data, int type EINA_UNUSED, void *event);
static void _videothumb_image_load(Evas_Object *obj);
static void _videothumb_eval(Evas_Object *obj, Eina_Bool force);
static void _smart_calculate(Evas_Object *obj);

static Pending *
_busy_realpath_find(const char *realpath)
{
   Eina_List *l;
   Pending *p;

   EINA_LIST_FOREACH(busy_thumbs, l, p)
     {
        if (!strcmp(realpath, p->realpath)) return p;
     }
   return NULL;
}

static Pending *
_busy_exe_find(Ecore_Exe *exe)
{
   Eina_List *l;
   Pending *p;

   EINA_LIST_FOREACH(busy_thumbs, l, p)
     {
        if (exe == p->exe) return p;
     }
   return NULL;
}

static Pending *
_busy_add(Evas_Object *obj, Ecore_Exe *exe, const char *realpath)
{
   Pending *p;

   p = calloc(1, sizeof(Pending));
   if (!p) return NULL;
   p->exe = exe;
   p->obj = obj;
   p->realpath = eina_stringshare_add(realpath);
   busy_thumbs = eina_list_append(busy_thumbs, p);
   return p;
}

static void
_busy_pending_del(Pending *p)
{
   eina_stringshare_del(p->realpath);
   busy_thumbs = eina_list_remove(busy_thumbs, p);
   free(p);
}

static Eina_Bool
_busy_del(Ecore_Exe *exe)
{
   Eina_List *l;
   Pending *p;

   EINA_LIST_FOREACH(busy_thumbs, l, p)
     {
        if (p->exe == exe)
          {
             _busy_pending_del(p);
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

static void
_busy_kill(Pending *p)
{
   ecore_exe_kill(p->exe);
   ecore_exe_free(p->exe);
   _busy_pending_del(p);
}

static Eina_Bool
_busy_obj_del(Evas_Object *obj)
{
   Eina_List *l;
   Pending *p;

   EINA_LIST_FOREACH(busy_thumbs, l, p)
     {
        if (p->obj == obj)
          {
             _busy_kill(p);
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

static void
_thumb_update(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   Evas_Object *o;
   char buf[PATH_MAX];

   if (!sd) return;
   snprintf(buf, sizeof(buf), "%u", sd->realpos);
   o = sd->o_img;
   if (o)
     {
        evas_object_image_file_set(o, NULL, NULL);
        evas_object_image_file_set(o, sd->realfile,
                                   sd->poster ? NULL : buf);
        evas_object_image_preload(o, EINA_FALSE);
        evas_object_smart_callback_call(obj, "loaded", NULL);
     }
   _videothumb_image_load(obj);
}

static void
_thumb_match_update(Evas_Object *obj, const char *file)
{
   Videothumb *sd = evas_object_smart_data_get(obj);

   if (!sd) return;
   if (!sd->realpath) return;
   if (!strcmp(sd->realpath, file)) _thumb_update(obj);
}

static void
_videothumb_launch_do(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   char buf[PATH_MAX];
   const char *libdir;
   char *s;

   if (!sd) return;
   ecore_exe_run_priority_set(10);
   if (sd->pending)
     {
        while (_busy_obj_del(obj));
        sd->pending = NULL;
     }
   // tried too many times BEFORE... DON'T TRY AGAIN.
   if (sd->tried > 5) return;
   s = ecore_file_escape_name(sd->realpath);
   if (s)
     {
        libdir = elm_app_lib_dir_get();
        if (libdir)
          {
             if (!_busy_realpath_find(sd->realpath))
               {
                  Ecore_Exe *exe;

                  if (!exe_handler)
                    exe_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                                          _cb_thumb_exe, NULL);
                  snprintf(buf, sizeof(buf),
                           "%s/rage/utils/rage_thumb %s 10000 %i",
                           libdir, s, sd->poster_mode ? 1 : 0);
                  exe = ecore_exe_pipe_run(buf,
                                           ECORE_EXE_TERM_WITH_PARENT |
                                           ECORE_EXE_NOT_LEADER,
                                           obj);
                  if (exe)
                    {
                       sd->pending = _busy_add(obj, exe, sd->realpath);
                    }
               }
             else return;
          }
        free(s);
     }
}

static Eina_Bool
_have_active_thumb(const char *path)
{
   Evas_Object *o;
   Eina_List *l;

   EINA_LIST_FOREACH(vidthumbs, l, o)
     {
        Videothumb *sd = evas_object_smart_data_get(o);

        if (sd)
          {
             if ((sd->pending) && (!strcmp(path, sd->realpath)))
               return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

static Eina_Bool
_cb_videothumb_delay(void *data)
{
   Evas_Object *obj = data;
   Videothumb *sd = evas_object_smart_data_get(obj);
   unsigned int maxnum = (eina_cpu_count() / 2) + 1;
   if (maxnum > 4) maxnum = 4;
   if (!sd) return EINA_FALSE;
   if (eina_list_count(busy_thumbs) < maxnum)
     {
        if (!_have_active_thumb(sd->realpath))
          {
             sd->launch_timer = NULL;
             _videothumb_launch_do(obj);
             return EINA_FALSE;
          }
     }
   if ((sd->iw > 0) && (sd->ih > 0))
     {
        sd->launch_timer = NULL;
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static void
_videothumb_launch(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);

   if (!sd) return;
   if (sd->launch_timer) return;
   sd->launch_timer = ecore_timer_add(0.2, _cb_videothumb_delay, obj);
}

static Eina_Bool
_cb_thumb_exe(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Del *ev = event;
   Pending *p = _busy_exe_find(ev->exe);

   if (p)
     {
        Eina_List *l;
        Evas_Object *o, *o2 = p->obj;

        if (o2)
          {
             Videothumb *sd = evas_object_smart_data_get(o2);
             if (sd)
               {
                  sd->pending = NULL;
                  sd->tried++;
               }
          }

        EINA_LIST_FOREACH(vidthumbs, l, o)
          {
             _thumb_match_update(o, p->realpath);
          }
        _busy_del(p->exe);
     }
   return EINA_TRUE;
}

static void
_cb_preload(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   Videothumb *sd = evas_object_smart_data_get(data);

   if (!sd) return;
   if (sd->o_img) evas_object_del(sd->o_img);
   sd->o_img = sd->o_img2;
   sd->o_img2 = NULL;
   sd->iw = 0;
   sd->ih = 0;
   if (sd->o_img) evas_object_image_size_get(sd->o_img, &(sd->iw), &(sd->ih));
   if ((sd->iw <= 0) || (sd->ih <= 0))
     {
        if (sd->cycle_timer)
          {
             if (sd->pos == 0.0)
               {
                  _videothumb_launch(data);
               }
             else
               {
                  sd->pos = 0.0;
                  if (!sd->pending) _videothumb_eval(data, EINA_TRUE);
               }
          }
        else
          {
             evas_object_del(sd->o_img);
             sd->o_img = NULL;
             evas_object_smart_callback_call(data, "failed", NULL);
          }
     }
   else
     {
        Eina_Bool ok = EINA_FALSE;
        struct stat st1, st2;

        if (stat(sd->realpath, &st1) == 0)
          {
             if (stat(sd->realfile, &st2) == 0)
               {
                  if (st1.st_mtime < st2.st_mtime) ok = EINA_TRUE;
               }
          }
        if (ok)
          {
             evas_object_show(sd->o_img);
             evas_object_smart_callback_call(data, "data", NULL);
          }
        else _videothumb_launch(data);
     }
   _smart_calculate(data);
}

static void
_videothumb_image_load(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   char buf_base[PATH_MAX];
   char buf_file[PATH_MAX];
   char buf[PATH_MAX];
   char *artfile;
   unsigned char sum[20];
   Eina_Bool found = EINA_FALSE;

   if (!sd) return;
   if (!sd->file) return;
   if (sd->pending) return;
   sd->o_img2 = evas_object_image_filled_add(evas_object_evas_get(obj));
   evas_object_image_load_head_skip_set(sd->o_img2, EINA_TRUE);
   evas_object_smart_member_add(sd->o_img2, obj);
   if (sd->poster_mode)
     {
        artfile = albumart_file_get(sd->realpath);
        if (artfile)
          {
             if (ecore_file_exists(artfile))
               {
                  sd->realfile = eina_stringshare_add(artfile);
                  sd->poster = EINA_TRUE;
                  found = EINA_TRUE;
               }
             free(artfile);
          }
     }
   if (!found)
     {
        if (!sha1((unsigned char *)sd->realpath, strlen(sd->realpath), sum))
          return;
        snprintf(buf_base, sizeof(buf_base), "%s/rage/thumb/%02x",
                 efreet_cache_home_get(), sum[0]);
        snprintf(buf_file, sizeof(buf_base),
                 "%s/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
                 "%02x%02x%02x%02x%02x%02x%02x%02x.eet",
                 buf_base,
                 sum[1], sum[2], sum[3],
                 sum[4], sum[5], sum[6], sum[7],
                 sum[8], sum[9], sum[10], sum[11],
                 sum[12], sum[13], sum[14], sum[15],
                 sum[16], sum[17], sum[18], sum[19]);
        if (sd->realfile) eina_stringshare_del(sd->realfile);
        sd->realfile = eina_stringshare_add(buf_file);
        sd->realpos = (((unsigned int)(sd->pos * 1000.0)) / 10000) * 10000;
        snprintf(buf, sizeof(buf), "%u", sd->realpos);
     }
   evas_object_event_callback_add(sd->o_img2,
                                  EVAS_CALLBACK_IMAGE_PRELOADED,
                                  _cb_preload, obj);
   evas_object_image_file_set(sd->o_img2, sd->realfile,
                              sd->poster ? NULL : buf);
   evas_object_image_preload(sd->o_img2, EINA_FALSE);
   _smart_calculate(obj);
}

static void
_videothumb_eval(Evas_Object *obj, Eina_Bool force)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh, vw, vh;
   Eina_Bool seen = EINA_FALSE;

   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   evas_output_viewport_get(evas_object_evas_get(obj), NULL, NULL, &vw, &vh);
   // XXX: not listening to canvas resizes! :(
   if (ELM_RECTS_INTERSECT(ox, oy, ow, oh, 0, 0, vw, vh)) seen = EINA_TRUE;
   // XXX: fix - wrokaround and always visible
   seen = EINA_TRUE;
   if (force)
     {
        sd->seen = seen;
        if (sd->o_img2)
          {
             evas_object_del(sd->o_img2);
             sd->o_img2 = NULL;
          }
        _videothumb_image_load(obj);
        if (sd->o_img2)
          {
             evas_object_move(sd->o_img2, ox, oy);
             evas_object_resize(sd->o_img2, ow, oh);
          }
     }
   else
     {
        if (seen != sd->seen)
          {
             sd->seen = seen;
             if (sd->seen)
               {
                  if (sd->o_img2)
                    {
                       evas_object_del(sd->o_img2);
                       sd->o_img2 = NULL;
                    }
                  _videothumb_image_load(obj);
                  if (sd->o_img2)
                    {
                       evas_object_move(sd->o_img2, ox, oy);
                       evas_object_resize(sd->o_img2, ow, oh);
                    }
               }
             else
               {
                  if (sd->o_img2)
                    {
                       evas_object_del(sd->o_img2);
                       sd->o_img2 = NULL;
                    }
               }
          }
     }
}

static void
_smart_add(Evas_Object *obj)
{
   Videothumb *sd;

   sd = calloc(1, sizeof(Videothumb));
   EINA_SAFETY_ON_NULL_RETURN(sd);
   evas_object_smart_data_set(obj, sd);
   _parent_sc.add(obj);
}

static void
_smart_del(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);

   if (!sd) return;
   vidthumbs = eina_list_remove(vidthumbs, obj);
   while (_busy_obj_del(obj));
   if (sd->launch_timer) ecore_timer_del(sd->launch_timer);
   if (sd->file) eina_stringshare_del(sd->file);
   if (sd->realfile) eina_stringshare_del(sd->realfile);
   if (sd->realpath) free(sd->realpath);
   if (sd->o_img) evas_object_del(sd->o_img);
   if (sd->o_img2) evas_object_del(sd->o_img2);
   if (sd->cycle_timer) ecore_timer_del(sd->cycle_timer);

   sd->pending = NULL;
   sd->file = NULL;
   sd->realfile = NULL;
   sd->realpath = NULL;
   sd->o_img = NULL;
   sd->o_img2 = NULL;
   sd->cycle_timer = NULL;

   _parent_sc.del(obj);
}

static void
_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh;
   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   evas_object_smart_changed(obj);
}

static void
_smart_calculate(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh;

   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   sd->w = ow;
   sd->h = oh;
   _videothumb_eval(obj, EINA_FALSE);
   if (sd->o_img)
     {
        evas_object_move(sd->o_img, ox, oy);
        evas_object_resize(sd->o_img, ow, oh);
     }
   if (sd->o_img2)
     {
        evas_object_move(sd->o_img2, ox, oy);
        evas_object_resize(sd->o_img2, ow, oh);
     }
}

static void
_smart_move(Evas_Object *obj, Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_smart_changed(obj);
}

static void
_smart_init(void)
{
   static Evas_Smart_Class sc;

   evas_object_smart_clipped_smart_set(&_parent_sc);
   sc           = _parent_sc;
   sc.name      = "videothumb";
   sc.version   = EVAS_SMART_CLASS_VERSION;
   sc.add       = _smart_add;
   sc.del       = _smart_del;
   sc.resize    = _smart_resize;
   sc.move      = _smart_move;
   sc.calculate = _smart_calculate;
   _smart = evas_smart_class_new(&sc);
}

Evas_Object *
videothumb_add(Evas_Object *parent)
{
   Evas *e;
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   e = evas_object_evas_get(parent);
   if (!e) return NULL;
   if (!_smart) _smart_init();
   obj = evas_object_smart_add(e, _smart);
   vidthumbs = eina_list_prepend(vidthumbs, obj);
   return obj;
}

void
videothumb_poster_mode_set(Evas_Object *obj, Eina_Bool poster_mode)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->poster_mode = poster_mode;
}

void
videothumb_file_set(Evas_Object *obj, const char *file, double pos)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->file == file) && (sd->pos == pos)) return;
   if (sd->file) eina_stringshare_del(sd->file);
   sd->file = eina_stringshare_add(file);
   if (!strncasecmp(sd->file, "file:/", 6))
     {
        Efreet_Uri *uri = efreet_uri_decode(sd->file);
        if (uri)
          {
             sd->realpath = ecore_file_realpath(uri->path);
             efreet_uri_free(uri);
          }
     }
   else if ((!strncasecmp(sd->file, "http:/", 6)) ||
            (!strncasecmp(sd->file, "https:/", 7)))
     sd->realpath = strdup(sd->file);
   else
     sd->realpath = ecore_file_realpath(sd->file);
   sd->pos = pos;
   _videothumb_eval(obj, EINA_TRUE);
}

static Eina_Bool
_cb_cycle(void *data)
{
   Evas_Object *obj = data;
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   if (sd->poster)
     {
        sd->cycle_timer = NULL;
        return EINA_FALSE;
     }
   sd->pos += 10.0;
   if (!sd->pending)
     {
        _videothumb_eval(obj, EINA_TRUE);
        if (sd->iw <= 0)
          {
             sd->pos = 0;
             _videothumb_eval(obj, EINA_TRUE);
          }
     }
   return EINA_TRUE;
}

void
videothumb_autocycle_set(Evas_Object *obj, Eina_Bool enabled)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!enabled)
     {
        if (sd->cycle_timer) ecore_timer_del(sd->cycle_timer);
        sd->cycle_timer = NULL;
        return;
     }
   if (sd->cycle_timer) return;
   sd->cycle_timer = ecore_timer_add(0.5, _cb_cycle, obj);
}

void
videothumb_size_get(Evas_Object *obj, int *w, int *h)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   *w = 1;
   *h = 1;
   if (!sd) return;
   *w = sd->iw;
   *h = sd->ih;
}
