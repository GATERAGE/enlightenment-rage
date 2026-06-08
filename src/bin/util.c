#include <Elementary.h>
#include "main.h"
#include "util.h"

Eina_Bool
util_video_ok(const char *path)
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

Eina_Bool
util_audio_ok(const char *path)
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

static char *_vid_dir = NULL;

void
util_videos_dir_set(const char *dir)
{
  if (_vid_dir) free(_vid_dir);
  _vid_dir = NULL;
  if (dir) _vid_dir = strdup(dir);
}

char *
util_videos_dir_get(void)
{
  char buf[PATH_MAX];
  const char *vids, *home;
  char *vidsreal = NULL, *homereal = NULL;

  if (_vid_dir)
    { // if explicitly set - use this
      vidsreal = ecore_file_realpath(_vid_dir);
      if (vidsreal) return vidsreal;
    }
  vids = efreet_videos_dir_get();
  if (vids)
    { // if efreet has a video dir - use that
      vidsreal = ecore_file_realpath(vids);
      if (vidsreal) return vidsreal;
    }
  // fallback - homedir + Videos
  home = eina_environment_home_get();
  if (home) homereal = ecore_file_realpath(home);
  if (!homereal) return strdup("/tmp");
  snprintf(buf, sizeof(buf), "%s/Videos", homereal);
  free(homereal);
  return strdup(buf);
}

