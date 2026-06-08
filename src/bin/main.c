#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "winlist.h"
#include "browser.h"
#include "config.h"
#include "mpris.h"
#include "util.h"

#define DEPTH_DEFAULT 99

typedef struct {
   Evas_Object *win;
   char *realpath;
   int depth;
   int depth_max;
   Eina_Bool have_media_files;
} Recursion_Data;

static Eina_Bool
_check_recursion(const char *path, unsigned int depth, unsigned int limit)
{
   const char *home = eina_environment_home_get();

   if (limit == 0)
     return EINA_TRUE;

   if (depth >= limit)
     return EINA_FALSE;

   if (!path[0] || !path[1])
     return EINA_FALSE;

   if ((home) && (!strcmp(home, path)))
     return EINA_FALSE;

   return EINA_TRUE;
}

static int
_cb_sort(const void *d1, const void *d2)
{
   const char *text = d1;
   const char *text2 = d2;

   if (!text) return 1;
   if (!text2) return -1;

   return strcmp(text, text2);
}

static void
_files_recursion(Ecore_Thread *thread, const char *path, Recursion_Data *recursion)
{
   Eina_Iterator *it;
   Eina_File_Direct_Info *info;
   char *dir_path;
   Eina_List *files = NULL, *dirs = NULL;

   if (!ecore_file_is_dir(path)) return;

   if (!_check_recursion(path, recursion->depth, recursion->depth_max)) return;

   it = eina_file_stat_ls(path);
   EINA_ITERATOR_FOREACH(it, info)
     {
        if (info->type == EINA_FILE_DIR)
          {
             dirs = eina_list_append(dirs, eina_stringshare_add(info->path));
          }
        if (info->type == EINA_FILE_REG)
          {
             files = eina_list_append(files, eina_stringshare_add(info->path));
          }
     }

   eina_iterator_free(it);

   if (files)
     {
        files = eina_list_sort(files, eina_list_count(files), _cb_sort);
        ecore_thread_feedback(thread, files);
     }

   dirs = eina_list_sort(dirs, eina_list_count(dirs), _cb_sort);

   EINA_LIST_FREE(dirs, dir_path)
     {
        recursion->depth++;
        _files_recursion(thread, dir_path, recursion);
        recursion->depth--;
        eina_stringshare_del(dir_path);
     }
}

static void
_cb_start_recursion(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Recursion_Data *recursion = data;

   _files_recursion(thread, recursion->realpath, recursion);
}

static void
_cb_end_recursion(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Recursion_Data *recursion;
   Eina_Bool have_media;

   recursion = data;
   have_media = recursion->have_media_files;

   free(recursion->realpath);
   free(recursion);
   if (!have_media)
     elm_exit();
}

static void
_cb_feedback_recursion(void *data, Ecore_Thread *thread EINA_UNUSED, void *msg)
{
   Recursion_Data *recursion;
   Eina_List *list;
   const char *path;
   Eina_Bool update_content = EINA_FALSE;

   list = msg;
   recursion = data;

   EINA_LIST_FREE(list, path)
     {
        if (util_video_ok(path) || util_audio_ok(path))
         {
            update_content = EINA_TRUE;
            win_list_hide(recursion->win);
            win_video_insert_append(recursion->win, path);
            if (!recursion->have_media_files)
              {
                 win_video_first(recursion->win);
                 recursion->have_media_files = EINA_TRUE;
              }
         }
        eina_stringshare_del(path);
     }

   if (update_content)
     win_list_content_update(recursion->win);
}

static Eina_Bool
_cb_show_timeout(void *data)
{
   Evas_Object *win = data;
   Inf *inf = evas_object_data_get(win, "inf");

   inf->show_timeout = NULL;
   evas_object_show(win);
   return EINA_FALSE;
}

static void
_cb_resize(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   win_list_size_update(obj);
   browser_size_update(obj);
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   Evas_Object *win;
   char buf[4096];
   int i;
// XXX: to do later
//   Config *config;
   Inf *inf;
   Eina_Bool fullscreen = EINA_FALSE;
   int rotation = 0;
   int depth_max = DEPTH_DEFAULT;
   Recursion_Data *recursion = NULL;
   Winvid_Entry *vid = NULL;
   Eina_List *list = NULL;

   elm_need_efreet();
   config_init();
// XXX: to do later
//   config = config_get();
   for (i = 1; i < argc; i++)
     {
        if ((!strcmp(argv[i], "-h")) ||
            (!strcmp(argv[i], "-help")) ||
            (!strcmp(argv[i], "--help")))
          {
             printf("Usage: rage [OPTIONS] [file1] [file2] [...]\n"
                    "  Where OPTIONS can be\n"
                    "    -h | -help | --help\n"
                    "      This help\n"
                    "\n"
                    "    -f\n"
                    "      Enable fullscreen mode at start\n"
                    "\n"
                    "    -sub SUBTITLE_FILE\n"
                    "      This sets the subtitle file to use for the\n"
                    "      previously given filename such as:\n"
                    "\n"
                    "        rage file.mp4 -sub subs.srt file2.mp4 ...\n"
                    "    -rot 0/90/180/270\n"
                    "      Rotate output by the given rotation\n"
                    "    -d DEPTH\n"
                    "      Set maximum level of recursion.\n"
                    "      The default is %d. Set to 0 for unlimited.\n"
                    "    -dir directory/to/scan\n"
                    "      Use this directory to scan for videos instead\n"
                    "      of ~/Videos\n"
                    , DEPTH_DEFAULT);
             exit(0);
          }
        else if (!strcmp(argv[i], "-f"))
          {
             fullscreen = EINA_TRUE;
          }
        else if (!strcmp(argv[i], "-d"))
          {
             if (i < (argc - 1))
               {
                  i++;
                  depth_max = atoi(argv[i]);
                  if (depth_max < 0 || depth_max > DEPTH_DEFAULT)
                    depth_max = DEPTH_DEFAULT;
               }
          }
        else if (!strcmp(argv[i], "-rot"))
          {
             if (i < (argc - 1))
               {
                  i++;
                  rotation = atoi(argv[i]);
               }
          }
        else if (!strcmp(argv[i], "-sub"))
          {
             if (i < (argc - 1))
               {
                  i++;
                  if (vid) eina_stringshare_replace(&(vid->sub), argv[i]);
               }
          }
        else if (!strcmp(argv[i], "-dir"))
          {
             if (i < (argc - 1))
               {
                 i++;
                 util_videos_dir_set(argv[i]);
               }
          }
        else
          {
             char *realpath = strdup(argv[i]);

             if (!strncasecmp(realpath, "file:/", 6))
               {
                  Efreet_Uri *uri = efreet_uri_decode(realpath);
                  if (uri)
                    {
                       free(realpath);
                       realpath = ecore_file_realpath(uri->path);
                       efreet_uri_free(uri);
                    }
               }
             if (ecore_file_is_dir(realpath))
               {
                  recursion = calloc(1, sizeof(Recursion_Data));
                  recursion->realpath = realpath;
                  recursion->depth_max = depth_max;
                  recursion->depth = 1;
               }
             else
               {
                  vid = calloc(1, sizeof(Winvid_Entry));
                  if (vid)
                    {
                       vid->file = eina_stringshare_add(realpath);
                       list = eina_list_append(list, vid);
                    }
                  free(realpath);
               }
          }
     }

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_lib_dir_set(PACKAGE_LIB_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
#if HAVE_GETTEXT && ENABLE_NLS
   elm_app_compile_locale_set(LOCALEDIR);
#endif
   elm_app_info_set(elm_main, "rage", "themes/default.edj");

#if (ELM_VERSION_MAJOR > 1) || (ELM_VERSION_MINOR >= 10)
   elm_config_accel_preference_set("accel");
#endif

   snprintf(buf, sizeof(buf), "%s/themes/default.edj", elm_app_data_dir_get());
   elm_theme_overlay_add(NULL, buf);

   win = win_add();
   if (!win)
     {
        printf("ERROR: cannot create window!\n");
        goto end;
     }

   evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _cb_resize, NULL);
   evas_object_resize(win,
                      600 * elm_config_scale_get(),
                      360 * elm_config_scale_get());
   elm_win_rotation_set(win, rotation);

   win_video_init(win);
   inf = evas_object_data_get(win, "inf");

   if (!list && !recursion)
     inf->browse_mode = EINA_TRUE;

   if (fullscreen) elm_win_fullscreen_set(win, EINA_TRUE);

   if (list)
     {
        win_video_file_list_set(win, list);
        EINA_LIST_FREE(list, vid)
          {
             if (vid->file) eina_stringshare_del(vid->file);
             if (vid->sub) eina_stringshare_del(vid->sub);
             if (vid->uri) efreet_uri_free(vid->uri);
             free(vid);
          }
     }
   if (recursion)
     {
        recursion->win = win;
        ecore_thread_feedback_run(_cb_start_recursion, _cb_feedback_recursion,
                                  _cb_end_recursion, NULL, recursion, EINA_FALSE);
     }

   if (inf->browse_mode)
     {
        inf->sized = EINA_TRUE;
        browser_show(win);
//        elm_layout_signal_emit(inf->lay, "about,show", "rage");
        evas_object_show(win);
     }
   else
     {
        inf->show_timeout = ecore_timer_add(10.0, _cb_show_timeout, win);
     }

   mpris_init(win);

   elm_run();

end:
   mpris_shutdown();
   config_shutdown();
   return 0;
}
ELM_MAIN()
