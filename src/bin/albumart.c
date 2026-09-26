#include <Elementary.h>
#include "video.h"
#include "win.h"
#include "rage_config.h"
#include "config.h"
#include "albumart.h"
#include "sha1.h"

#define Q_START "https://www.google.com/search?as_st=y&tbm=isch&hl=en&as_q="
//#define Q_END "&as_epq=&as_oq=&as_eq=&cr=&as_sitesearch=&safe=images&tbs=iar:s,ift:jpg"
#define Q_END "&as_epq=&as_oq=&as_eq=&cr=&as_sitesearch=&safe=images&tbs=ift:jpg"

static Ecore_Con_Url *fetch = NULL;
static Ecore_Event_Handler *handle_data = NULL;
static Ecore_Event_Handler *handle_complete = NULL;
static Eina_Strbuf *sb_result = NULL;
static Eina_Bool fetch_image = EINA_FALSE;
static char *fetchfile = NULL;
static char *fetchpath = NULL;
static char *fetchpath2 = NULL;
static FILE *fout = NULL;
static void (*_fetch_done) (void *data) = NULL;
static void *_fetch_data = NULL;


static char *
_inpath(const char *file)
{
   char *realpath = NULL;

   if (!strncasecmp(file, "file:/", 6))
     {
        Efreet_Uri *uri = efreet_uri_decode(file);
        if (uri)
          {
             realpath = ecore_file_realpath(uri->path);
             efreet_uri_free(uri);
          }
     }
   else if ((!strncasecmp(file, "http:/", 6)) ||
            (!strncasecmp(file, "https:/", 7)))
     realpath = strdup(file);
   else
     realpath = ecore_file_realpath(file);
   if (realpath && (!realpath[0]))
     {
        free(realpath);
        return NULL;
     }
   return realpath;
}

static char *
_thumbpath(const char *file)
{
   char buf_base[PATH_MAX];
   char buf_file[PATH_MAX];
   unsigned char sum[20];

   if (!sha1((unsigned char *)file, strlen(file), sum)) return NULL;
   if ((size_t)snprintf(buf_base, sizeof(buf_base), "%s/rage/albumart/%02x",
                        efreet_cache_home_get(), sum[0]) >= sizeof(buf_base))
     {
        fprintf(stderr, "Not enough buffer space for thumb path");
        return NULL;
     }
   if (!ecore_file_mkpath(buf_base)) return NULL;
   if ((size_t)snprintf(buf_file, sizeof(buf_base),
                        "%s/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
                        "%02x%02x%02x%02x%02x%02x%02x%02x.jpg",
                        buf_base,
                        sum[1], sum[2], sum[3],
                        sum[4], sum[5], sum[6], sum[7],
                        sum[8], sum[9], sum[10], sum[11],
                        sum[12], sum[13], sum[14], sum[15],
                        sum[16], sum[17], sum[18], sum[19]) >= sizeof(buf_base))
     {
        fprintf(stderr, "Not enough buffer space for thumb path");
        return NULL;
     }
   return strdup(buf_file);
}

static Ecore_Con_Url *
_fetch(Eina_Strbuf *sb)
{
   Ecore_Con_Url *f;
   const char *qs;

   qs = eina_strbuf_string_get(sb);
   if (!qs) return NULL;
//   printf("GET %s\n", qs);
   f = ecore_con_url_new(qs);
   if (!f) return NULL;
   ecore_con_url_additional_header_add
   (f, "user-agent",
    "Mozilla/5.0 (Linux; Android 4.0.4; Galaxy Nexus Build/IMM76B) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.133 Mobile Safari/535.19");
   ecore_con_url_get(f);
   return f;
}

static Eina_Bool
_cb_http_data(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Con_Event_Url_Data *ev = event;

   if (ev->url_con != fetch) return EINA_TRUE;
//   printf("HTTP DATA %i (fetch image? %i)\n", (int)ev->size, fetch_image);
   if (fetch_image) fwrite(ev->data, ev->size, 1, fout);
   else if (sb_result)
     eina_strbuf_append_length(sb_result, (char *)ev->data, (size_t)ev->size);
   return EINA_FALSE;
}

static Eina_Bool
_delay_fetch(void *data)
{
   Eina_Strbuf *sb = data;
   fetch = _fetch(sb);
   eina_strbuf_free(sb);
   return EINA_FALSE;
}

static Eina_Bool
_cb_http_complete(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;
   Eina_Bool ok = EINA_FALSE;

   if (ev->url_con != fetch) return EINA_TRUE;
   if (fetch_image)
     {
        fetch_image = EINA_FALSE;
        fclose(fout);
        fout = NULL;
        if (ecore_file_size(fetchpath2) < 0)
          ecore_file_unlink(fetchpath2);
        else
          ecore_file_mv(fetchpath2, fetchpath);
        free(fetchpath);
        free(fetchpath2);
        fetchpath = NULL;
        fetchpath2 = NULL;
     }
   else
     {
        if (sb_result)
          {
             const char *res = eina_strbuf_string_get(sb_result);
             if (res)
               {
                  const char *p, *pe;
                  Eina_Strbuf *sb;

                  sb = eina_strbuf_new();
                  p = strstr(res, "data-src=\"http");
                  if (p)
                    {
                       p += 10;
                       pe = strchr(p, '"');
                       if (pe)
                         {
                            eina_strbuf_append_length(sb, p, pe - p);
                            ok = EINA_TRUE;
                         }
                    }
                  if (!ok)
                    {
                       p = strstr(res, "imgurl=");
                       if (p)
                         {
                            p += 7;
                            pe = strstr(p, "&amp;");
                            if (pe)
                              {
                                 eina_strbuf_append_length(sb, p, pe - p);
                                 ok = EINA_TRUE;
                              }
                         }
                    }
                  if (ok)
                    {
                       char *path, *path2;

                       ecore_con_url_free(fetch);
                       fetch = NULL;

                       path = _thumbpath(fetchfile);
                       if (path)
                         {
                            path2 = alloca(strlen(path) + 4  + 1);
                            sprintf(path2, "%s.tmo", path);

                            fout = fopen(path2, "wb");
                            if (fout)
                              {
                                 fetch_image = EINA_TRUE;
// delay this - it seems google doesn;'t like to respond if we ask too fast
//                                 fetch = _fetch(sb);
                                 ecore_timer_add(1.5, _delay_fetch, sb);
                                 sb = NULL;
                                 free(fetchpath);
                                 free(fetchpath2);
                                 fetchpath = strdup(path);
                                 fetchpath2 = strdup(path2);
                              }
                            free(path);
                         }
                    }
                  if (sb) eina_strbuf_free(sb);
               }
             eina_strbuf_free(sb_result);
             sb_result = NULL;
          }
     }
   if (!ok)
     {
        ecore_con_url_free(fetch);
        fetch = NULL;
        if (fetchfile)
          {
             free(fetchfile);
             fetchfile = NULL;
          }
        if (_fetch_done) _fetch_done(_fetch_data);
        _fetch_done = NULL;
        _fetch_data = NULL;
     }
   return EINA_FALSE;
}

static Eina_Bool
_search_append(Eina_Strbuf *sb, const char *str, Eina_Bool hadword)
{
   const char *s;
   Eina_Bool word = EINA_FALSE;

   for (s = str; *s; s++)
     {
        if (((*s >= 'a') && (*s <= 'z')) ||
            ((*s >= 'A') && (*s <= 'Z')) ||
            ((*s >= '0') && (*s <= '9')))
          {
             if (!word)
               {
                  if (hadword)
                    {
                       eina_strbuf_append_char(sb, '+');
                       word = EINA_FALSE;
                    }
               }
             eina_strbuf_append_char(sb, *s);
             word = EINA_TRUE;
             hadword = EINA_TRUE;
          }
        else word = EINA_FALSE;
        if (*s == '.') break;
     }
   return hadword;
}

void
albumart_find(const char *file,
              const char *artist, const char *album, const char *title,
              const char *extrastr,
              void (*fetch_done) (void *data), void *fetch_data)
{
   Eina_Strbuf *sb;
   char *path;

   if (fetchfile)
     {
        free(fetchfile);
        fetchfile = NULL;
     }
   if (fetch)
     {
        ecore_con_url_free(fetch);
        fetch = NULL;
     }
   if (fout)
     {
        fclose(fout);
        fout = NULL;
     }
   if (sb_result)
     {
        eina_strbuf_free(sb_result);
        sb_result = NULL;
     }
   fetch_image = EINA_FALSE;
   if (!file) return;

   if (!handle_data)
     handle_data = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                                           _cb_http_data, NULL);
   if (!handle_complete)
     handle_complete = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                                               _cb_http_complete, NULL);

   fetchfile = _inpath(file);

   _fetch_done = fetch_done;
   _fetch_data = fetch_data;

   path = _thumbpath(fetchfile);
   if (path)
     {
        if (ecore_file_exists(path))
          {
             if (fetch_done) fetch_done(fetch_data);
             free(path);
             free(fetchfile);
             fetchfile = NULL;
             return;
          }
        else free(path);
     }

   sb = eina_strbuf_new();
   eina_strbuf_append(sb, Q_START);

   if ((title) || (album) || (artist))
     {
        Eina_Bool had = EINA_FALSE;

        if (artist) had = _search_append(sb, artist, had);
        if (album) had = _search_append(sb, album, had);
        if (title) had = _search_append(sb, title, had);
     }
   else
     _search_append(sb, ecore_file_file_get(fetchfile), EINA_FALSE);

   if (extrastr) _search_append(sb, extrastr, EINA_TRUE);

   eina_strbuf_append(sb, Q_END);

   if (sb_result) eina_strbuf_free(sb_result);
   sb_result = eina_strbuf_new();

   fetch = _fetch(sb);

   eina_strbuf_free(sb);
}

char *
albumart_file_get(const char *file)
{
   char *tmp = alloca(strlen(file) + 1 + 100);
   char *dir, *fraw, *s;
   const char *fname, *e, *c;
   const char *ext[] = {
      "png", "PNG",
      "jpg", "JPG",
      "jpeg", "JPEG",
      "jpe", "JPE",
      NULL };
   const char *cover[] = {
      "cover",   "Cover",   "COVER",
      "front",   "Front",   "FRONT",
      "folder",  "Folder",  "FOLDER",
      ".cover",  ".Cover",  ".COVER",
      ".front",  ".Front",  ".FRONT",
      ".folder", ".Folder", ".FOLDER",
      NULL };
   int i, j;

   for (i = 0; (e = ext[i]) && e; i++)
     {
        sprintf(tmp, "%s.%s", file, e);
        if (ecore_file_exists(tmp)) return strdup(tmp);
     }

   dir = ecore_file_dir_get(file);
   if (!dir) dir = strdup(".");
   fname = ecore_file_file_get(file);
   if (!fname)
     {
        free(dir);
        return NULL;
     }
   fraw  = strdup(fname);
   if (!fraw)
     {
        free(dir);
        return NULL;
     }
   s = strrchr(fraw, '.');
   if (s) *s = 0;

   for (i = 0; (e = ext[i]) && e; i++)
     {
        sprintf(tmp, "%s/%s.%s", dir, fraw, e);
        if (ecore_file_exists(tmp)) goto found;
     }
   for (i = 0; (e = ext[i]) && e; i++)
     {
        sprintf(tmp, "%s/.%s.%s", dir, fname, e);
        if (ecore_file_exists(tmp)) goto found;
     }
   for (i = 0; (e = ext[i]) && e; i++)
     {
        sprintf(tmp, "%s/.%s.%s", dir, fraw, e);
        if (ecore_file_exists(tmp)) goto found;
     }
   for (i = 0; (e = ext[i]) && e; i++)
     {
        sprintf(tmp, "%s/.thumb/%s.%s", dir, fname, e);
        if (ecore_file_exists(tmp)) goto found;
     }
   for (i = 0; (e = ext[i]) && e; i++)
     {
        sprintf(tmp, "%s/.thumb/%s.%s", dir, fraw, e);
        if (ecore_file_exists(tmp)) goto found;
     }

   for (j = 0; (c = cover[j]) && c; j++)
     {
        for (i = 0; (e = ext[i]) && e; i++)
          {
             sprintf(tmp, "%s/%s.%s", dir, c, e);
             if (ecore_file_exists(tmp)) goto found;
          }
     }

   free(dir);
   free(fraw);
   return _thumbpath(file);
found:
   free(dir);
   free(fraw);
   return strdup(tmp);
}
