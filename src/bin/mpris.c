#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "browser.h"
#include "video.h"
#include "winvid.h"
#include "winlist.h"
#include "config.h"
#include "mpris.h"

static Evas_Object *mainwin = NULL;
static Eldbus_Connection *conn = NULL;
static Eldbus_Service_Interface *iface = NULL;
static Eldbus_Service_Interface *iface_player = NULL;

#define SERVICE "org.mpris.MediaPlayer2.rage"
#define PATH    "/org/mpris/MediaPlayer2"

static void
_cb_name_request(void *data EINA_UNUSED, const Eldbus_Message *msg EINA_UNUSED,
                 Eldbus_Pending *pending EINA_UNUSED)
{
   return;
/*
   unsigned int flag;

   if (eldbus_message_error_get(msg, NULL, NULL))
     {
        fprintf(stderr, "Could not request bus name");
        return;
     }
   if (!eldbus_message_arguments_get(msg, "u", &flag))
     {
        fprintf(stderr, "Could not get arguments on on_name_request");
        return;
     }
   if (!(flag & ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER))
     fprintf(stderr, "Name already in use\n");
 */
}

/* Implementing almost all of:
 * 
https://specifications.freedesktop.org/mpris-spec/latest/Player_Interface.html
 * 
 * Not implemented:
 * Tracklist objects
 * SetPosition (requires Tracklist objects)
 * 
 * In rage generally and here:
 * 
 * Loop playlist vs just loop current
 * Shuffle play
 * Playback rate
 */

#define API(fn) \
static Eldbus_Message * \
_cb_api_##fn(const Eldbus_Service_Interface *iface EINA_UNUSED, \
              const Eldbus_Message *msg)
#define GETTER(fn) \
static Eina_Bool \
_cb_prop_##fn##_get(const Eldbus_Service_Interface *iface EINA_UNUSED, \
                        const char *propname EINA_UNUSED, \
                        Eldbus_Message_Iter *iter, \
                        const Eldbus_Message *request_msg EINA_UNUSED, \
                        Eldbus_Message **error EINA_UNUSED)

#define SETTER(fn) \
static Eldbus_Message * \
_cb_prop_##fn##_set(const Eldbus_Service_Interface *iface EINA_UNUSED, \
                        const char *propname EINA_UNUSED, \
                        Eldbus_Message_Iter *iter, \
                        const Eldbus_Message *msg)
#define PROP_RO(str, type, prop) \
   { str, type, _cb_prop_##prop##_get, NULL, 0 }
#define PROP_RW(str, type, prop) \
   { str, type, _cb_prop_##prop##_get, _cb_prop_##prop##_set, 0 }
#define METHOD(str, type, fn) \
   { str,  NULL, type, _cb_api_##fn, 0 }





/////////////////////////////////////////////////////////////////////////////

GETTER(can_quit)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(fullscreen)
{
   Eina_Bool fs = elm_win_fullscreen_get(mainwin);
   eldbus_message_iter_arguments_append(iter, "b", fs);
   return EINA_TRUE;
}

SETTER(fullscreen)
{
   Eina_Bool fs = EINA_FALSE;
   eldbus_message_iter_arguments_get(iter, "d", &fs);
   elm_win_fullscreen_set(mainwin, fs);
   return eldbus_message_method_return_new(msg);
}

GETTER(can_set_fullscreen)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_raise)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(has_track_list)
{
   // XXX: Implement Tracklist
   eldbus_message_iter_arguments_append(iter, "b", EINA_FALSE);
   return EINA_TRUE;
}

GETTER(identity)
{
   char buf[] = "Rage";
   eldbus_message_iter_arguments_append(iter, "s", buf);
   return EINA_TRUE;
}

GETTER(desktop_entry)
{
   char buf[] = "rage";
   eldbus_message_iter_arguments_append(iter, "s", buf);
   return EINA_TRUE;
}

GETTER(supported_mime_types)
{
   Eldbus_Message_Iter *array = NULL;

   eldbus_message_iter_arguments_append(iter, "as", &array);
#define A(s) eldbus_message_iter_arguments_append(array, "s", s)
   A("application/ogg");

   A("audio/1d-interleaved-parityfec");
   A("audio/32kadpcm");
   A("audio/3gpp");
   A("audio/3gpp2");
   A("audio/AMR");
   A("audio/AMR-WB");
   A("audio/ATRAC-ADVANCED-LOSSLESS");
   A("audio/ATRAC-X");
   A("audio/ATRAC3");
   A("audio/BV16");
   A("audio/BV32");
   A("audio/CN");
   A("audio/DAT12");
   A("audio/DV");
   A("audio/DVI4");
   A("audio/EVRC");
   A("audio/EVRC-QCP");
   A("audio/EVRC0");
   A("audio/EVRC1");
   A("audio/EVRCB");
   A("audio/EVRCB0");
   A("audio/EVRCB1");
   A("audio/EVRCWB");
   A("audio/EVRCWB0");
   A("audio/EVRCWB1");
   A("audio/G719");
   A("audio/G722");
   A("audio/G7221");
   A("audio/G723");
   A("audio/G726-16");
   A("audio/G726-24");
   A("audio/G726-32");
   A("audio/G726-40");
   A("audio/G728");
   A("audio/G729");
   A("audio/G7291");
   A("audio/G729D");
   A("audio/G729E");
   A("audio/GSM");
   A("audio/GSM-EFR");
   A("audio/GSM-HR-08");
   A("audio/L16");
   A("audio/L20");
   A("audio/L24");
   A("audio/L8");
   A("audio/LPC");
   A("audio/MP4A-LATM");
   A("audio/MPA");
   A("audio/PCMA");
   A("audio/PCMA-WB");
   A("audio/PCMU");
   A("audio/PCMU-WB");
   A("audio/QCELP");
   A("audio/RED");
   A("audio/SMV");
   A("audio/SMV-QCP");
   A("audio/SMV0");
   A("audio/UEMCLIP");
   A("audio/VDVI");
   A("audio/VMR-WB");
   A("audio/aac");
   A("audio/ac3");
   A("audio/adpcm");
   A("audio/amr");
   A("audio/amr-wb");
   A("audio/amr-wb+");
   A("audio/annodex");
   A("audio/asc");
   A("audio/basic");
   A("audio/bv16");
   A("audio/bv32");
   A("audio/clearmode");
   A("audio/cn");
   A("audio/csound");
   A("audio/dat12");
   A("audio/dls");
   A("audio/dsr-es201108");
   A("audio/dsr-es202050");
   A("audio/dsr-es202211");
   A("audio/dsr-es202212");
   A("audio/dvi4");
   A("audio/eac3");
   A("audio/evrc");
   A("audio/evrc-qcp");
   A("audio/evrc0");
   A("audio/evrc1");
   A("audio/evrcb");
   A("audio/evrcb0");
   A("audio/evrcb1");
   A("audio/evrcwb");
   A("audio/evrcwb0");
   A("audio/evrcwb1");
   A("audio/example");
   A("audio/flac");
   A("audio/fwdred");
   A("audio/g.722.1");
   A("audio/g719");
   A("audio/g722");
   A("audio/g7221");
   A("audio/g723");
   A("audio/g726-16");
   A("audio/g726-24");
   A("audio/g726-32");
   A("audio/g726-40");
   A("audio/g728");
   A("audio/g729");
   A("audio/g7291");
   A("audio/g729d");
   A("audio/g729e");
   A("audio/gsm");
   A("audio/gsm-efr");
   A("audio/iLBC");
   A("audio/ilbc");
   A("audio/ip-mr_v2.5");
   A("audio/l16");
   A("audio/l20");
   A("audio/l24");
   A("audio/l8");
   A("audio/lpc");
   A("audio/midi");
   A("audio/mobile-xmf");
   A("audio/mp2");
   A("audio/mp4");
   A("audio/mp4a-latm");
   A("audio/mpa");
   A("audio/mpa-robust");
   A("audio/mpeg");
   A("audio/mpeg4-generic");
   A("audio/mpegurl");
   A("audio/ogg");
   A("audio/parityfec");
   A("audio/pcma");
   A("audio/pcma-wb");
   A("audio/pcmu");
   A("audio/pcmu-wb");
   A("audio/prs.sid");
   A("audio/qcelp");
   A("audio/raptorfec");
   A("audio/red");
   A("audio/rtp-enc-aescm128");
   A("audio/rtp-midi");
   A("audio/rtx");
   A("audio/s3m");
   A("audio/silk");
   A("audio/smv");
   A("audio/smv-qcp");
   A("audio/smv0");
   A("audio/sp-midi");
   A("audio/speex");
   A("audio/t140c");
   A("audio/t38");
   A("audio/telephone-event");
   A("audio/tone");
   A("audio/ulpfec");
   A("audio/vdvi");
   A("audio/vmr-wb");
   A("audio/vnd.3gpp.iufp");
   A("audio/vnd.4SB");
   A("audio/vnd.4sb");
   A("audio/vnd.CELP");
   A("audio/vnd.audiokoz");
   A("audio/vnd.celp");
   A("audio/vnd.cisco.nse");
   A("audio/vnd.cmles.radio-events");
   A("audio/vnd.cns.anp1");
   A("audio/vnd.cns.inf1");
   A("audio/vnd.dece.audio");
   A("audio/vnd.digital-winds");
   A("audio/vnd.dlna.adts");
   A("audio/vnd.dolby.heaac.1");
   A("audio/vnd.dolby.heaac.2");
   A("audio/vnd.dolby.mlp");
   A("audio/vnd.dolby.mps");
   A("audio/vnd.dolby.pl2");
   A("audio/vnd.dolby.pl2x");
   A("audio/vnd.dolby.pl2z");
   A("audio/vnd.dolby.pulse.1");
   A("audio/vnd.dra");
   A("audio/vnd.dts");
   A("audio/vnd.dts.hd");
   A("audio/vnd.dvb.file");
   A("audio/vnd.everad.plj");
   A("audio/vnd.hns.audio");
   A("audio/vnd.lucent.voice");
   A("audio/vnd.rn-realaudio");
   A("audio/vnd.ms-playready.media.pya");
   A("audio/vnd.nokia.mobile-xmf");
   A("audio/vnd.nortel.vbk");
   A("audio/vnd.nuera.ecelp4800");
   A("audio/vnd.nuera.ecelp7470");
   A("audio/vnd.nuera.ecelp9600");
   A("audio/vnd.octel.sbc");
   A("audio/vnd.qcelp");
   A("audio/vnd.rhetorex.32kadpcm");
   A("audio/vnd.rip");
   A("audio/vnd.sealedmedia.softseal.mpeg");
   A("audio/vnd.vmx.cvsd");
   A("audio/vorbis");
   A("audio/vorbis-config");
   A("audio/webm");
   A("audio/x-aac");
   A("audio/x-adpcm");
   A("audio/x-aifc");
   A("audio/x-aiff");
   A("audio/x-amzxml");
   A("audio/x-ape");
   A("audio/x-caf");
   A("audio/x-flac");
   A("audio/x-flac+ogg");
   A("audio/x-gsm");
   A("audio/x-iriver-pla");
   A("audio/x-it");
   A("audio/x-m4b");
   A("audio/x-matroska");
   A("audio/x-minipsf");
   A("audio/x-mo3");
   A("audio/x-mod");
   A("audio/x-mpegurl");
   A("audio/x-ms-asx");
   A("audio/x-ms-wax");
   A("audio/x-ms-wma");
   A("audio/x-musepack");
   A("audio/x-opus+ogg");
   A("audio/x-pn-audibleaudio");
   A("audio/x-pn-realaudio");
   A("audio/x-pn-realaudio-plugin");
   A("audio/x-psf");
   A("audio/x-psflib");
   A("audio/x-realaudio");
   A("audio/x-riff");
   A("audio/x-s3m");
   A("audio/x-scpls");
   A("audio/x-sd2");
   A("audio/x-speex");
   A("audio/x-speex+ogg");
   A("audio/x-stm");
   A("audio/x-tta");
   A("audio/x-voc");
   A("audio/x-vorbis+ogg");
   A("audio/x-wav");
   A("audio/x-wavpack");
   A("audio/x-wavpack-correction");
   A("audio/x-xi");
   A("audio/x-xm");
   A("audio/x-xmf");
   A("audio/xm");

   A("video/1d-interleaved-parityfec");
   A("video/3gpp");
   A("video/3gpp-tt");
   A("video/3gpp2");
   A("video/BMPEG");
   A("video/BT656");
   A("video/CelB");
   A("video/DV");
   A("video/H261");
   A("video/H263");
   A("video/H263-1998");
   A("video/H263-2000");
   A("video/H264");
   A("video/H264-RCDO");
   A("video/H264-SVC");
   A("video/JPEG");
   A("video/MJ2");
   A("video/MP1S");
   A("video/MP2P");
   A("video/MP2T");
   A("video/MP4V-ES");
   A("video/MPV");
   A("video/SMPTE292M");
   A("video/annodex");
   A("video/bmpeg");
   A("video/bt656");
   A("video/celb");
   A("video/dl");
   A("video/dv");
   A("video/example");
   A("video/fli");
   A("video/gl");
   A("video/h261");
   A("video/h263");
   A("video/h263-1998");
   A("video/h263-2000");
   A("video/h264");
   A("video/isivideo");
   A("video/jpeg");
   A("video/jpeg2000");
   A("video/jpm");
   A("video/mj2");
   A("video/mp1s");
   A("video/mp2p");
   A("video/mp2t");
   A("video/mp4");
   A("video/mp4v-es");
   A("video/mpeg");
   A("video/mpeg4-generic");
   A("video/mpv");
   A("video/nv");
   A("video/ogg");
   A("video/parityfec");
   A("video/pointer");
   A("video/quicktime");
   A("video/raptorfec");
   A("video/raw");
   A("video/rtp-enc-aescm128");
   A("video/rtx");
   A("video/smpte292m");
   A("video/ulpfec");
   A("video/vc1");
   A("video/vnd.CCTV");
   A("video/vnd.cctv");
   A("video/vnd.dece.hd");
   A("video/vnd.dece.mobile");
   A("video/vnd.dece.mp4");
   A("video/vnd.dece.pd");
   A("video/vnd.dece.sd");
   A("video/vnd.dece.video");
   A("video/vnd.directv.mpeg");
   A("video/vnd.directv.mpeg-tts");
   A("video/vnd.dlna.mpeg-tts");
   A("video/vnd.dvb.file");
   A("video/vnd.fvt");
   A("video/vnd.hns.video");
   A("video/vnd.iptvforum.1dparityfec-1010");
   A("video/vnd.iptvforum.1dparityfec-2005");
   A("video/vnd.iptvforum.2dparityfec-1010");
   A("video/vnd.iptvforum.2dparityfec-2005");
   A("video/vnd.iptvforum.ttsavc");
   A("video/vnd.iptvforum.ttsmpeg2");
   A("video/vnd.motorola.video");
   A("video/vnd.motorola.videop");
   A("video/vnd.mpegurl");
   A("video/vnd.ms-playready.media.pyv");
   A("video/vnd.mts");
   A("video/vnd.nokia.interleaved-multimedia");
   A("video/vnd.nokia.videovoip");
   A("video/vnd.objectvideo");
   A("video/vnd.rn-realvideo");
   A("video/vnd.sealed.mpeg1");
   A("video/vnd.sealed.mpeg4");
   A("video/vnd.sealed.swf");
   A("video/vnd.sealedmedia.softseal.mov");
   A("video/vnd.uvvu.mp4");
   A("video/vnd.vivo");
   A("video/wavelet");
   A("video/webm");
   A("video/x-anim");
   A("video/x-f4v");
   A("video/x-fli");
   A("video/x-flic");
   A("video/x-flv");
   A("video/x-javafx");
   A("video/x-la-asf");
   A("video/x-m4v");
   A("video/x-matroska");
   A("video/x-matroska-3d");
   A("video/x-mng");
   A("video/x-ms-asf");
   A("video/x-ms-vob");
   A("video/x-ms-wm");
   A("video/x-ms-wmp");
   A("video/x-ms-wmv");
   A("video/x-ms-wmx");
   A("video/x-ms-wvx");
   A("video/x-msvideo");
   A("video/x-nsv");
   A("video/x-ogm+ogg");
   A("video/x-sgi-movie");
   A("video/x-smv");
   A("video/x-theora+ogg");

   // hope the other end understands globs?
   A("video/*");
   A("audio/*");
   eldbus_message_iter_container_close(iter, array);
   return EINA_TRUE;
}

GETTER(supported_uri_schemes)
{
   Eldbus_Message_Iter *array = NULL;

   eldbus_message_iter_arguments_append(iter, "as", &array);
   A("file");
   A("http");
   A("https");
   A("rtsp");
   eldbus_message_iter_container_close(iter, array);
   return EINA_TRUE;
}

static const Eldbus_Property properties[] =
{
   PROP_RO("CanQuit",             "b",  can_quit),
   PROP_RW("Fullscreen",          "b",  fullscreen),
   PROP_RO("CanSetFullscreen",    "b",  can_set_fullscreen),
   PROP_RO("CanRaise",            "b",  can_raise),
   PROP_RO("HasTrackList",        "b",  has_track_list),
   PROP_RO("Identity",            "s",  identity),
   PROP_RO("DesktopEntry",        "s",  desktop_entry),
   PROP_RO("SupportedMimeTypes",  "as", supported_mime_types),
   PROP_RO("SupportedUriSchemes", "as", supported_uri_schemes),
   { NULL, NULL, NULL, NULL, 0 }
};

API(quit)
{
   evas_object_del(mainwin);
   return eldbus_message_method_return_new(msg);
}

API(raize)
{
   elm_win_raise(mainwin);
   return eldbus_message_method_return_new(msg);
}

static const Eldbus_Method methods[] =
{
   METHOD("Quit",  NULL, quit),
   METHOD("Raise", NULL, raize),
   { NULL, NULL, NULL, NULL, 0 }
};

static const Eldbus_Service_Interface_Desc desc = {
   "org.mpris.MediaPlayer2", methods, NULL, properties, NULL, NULL
};





/////////////////////////////////////////////////////////////////////////////

GETTER(playback_status)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   const char *buf = "Stopped";

   if (!inf) goto end;
   if (browser_visible()) goto end;
   if (video_play_get(inf->vid)) buf = "Playing";
   else buf = "Paused";
end:
   eldbus_message_iter_arguments_append(iter, "s", buf);
   return EINA_TRUE;
}

GETTER(loop_status)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   const char *buf = "None";

   if (!inf) goto end;
   if (browser_visible()) goto end;
   if (video_loop_get(inf->vid)) buf = "Track";
   else buf = "None";
//   "Playlist";
end:
   eldbus_message_iter_arguments_append(iter, "s", buf);
   return EINA_TRUE;
}

SETTER(loop_status)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   const char *buf = NULL;

   if (!inf) goto end;
   if (browser_visible()) goto end;
   eldbus_message_iter_arguments_get(iter, "s", &buf);
   if (buf)
     {
        if (!strcmp(buf, "None"))
          {
             video_loop_set(inf->vid, EINA_FALSE);
             mpris_loop_status_change();
          }
        else if (!strcmp(buf, "Track"))
          {
             video_loop_set(inf->vid, EINA_TRUE);
             mpris_loop_status_change();
          }
        else if (!strcmp(buf, "Playlist"))
          {
             fprintf(stderr, "Cannot do playlist loop yet.\n");
          }
        if (video_loop_get(inf->vid))
          elm_layout_signal_emit(inf->lay, "action,loop", "rage");
        else
          elm_layout_signal_emit(inf->lay, "action,sequential", "rage");
     }
end:
   return eldbus_message_method_return_new(msg);
}

GETTER(rate)
{
   double rate = 1.0;
   // XXX: Implement playback rate
   eldbus_message_iter_arguments_append(iter, "d", rate);
   return EINA_TRUE;
}

SETTER(rate)
{
   double rate = 1.0;
   // XXX: Implement playback rate
   eldbus_message_iter_arguments_get(iter, "d", &rate);
   return eldbus_message_method_return_new(msg);
}

GETTER(shuffle)
{
   Eina_Bool shuffle = EINA_FALSE;
   // XXX: Implement shuffle mode
   eldbus_message_iter_arguments_append(iter, "b", shuffle);
   return EINA_TRUE;
}

SETTER(shuffle)
{
   Eina_Bool shuffle = EINA_FALSE;
   // XXX: Implement shuffle mode
   fprintf(stderr, "Cannot set shuffle mode yet.\n");
   eldbus_message_iter_arguments_get(iter, "b", &shuffle);
   return eldbus_message_method_return_new(msg);
}

GETTER(metadata)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   Eldbus_Message_Iter *array = NULL, *entry = NULL, *var, *var2;
   uint64_t len = 0;
   char *buf = NULL, *tmp = NULL, *p;
   const char *s, *file;

   if (!inf) return EINA_FALSE;
   // XXX: TODO:
   // mpris:trackid

   eldbus_message_iter_arguments_append(iter, "a{sv}", &array);

   s = video_file_get(inf->vid);
   if (s)
     {
        if (s[0] == '/')
          {
             buf = alloca(strlen(s) + sizeof("file://") + 1);
             sprintf(buf, "file://%s", s);
          }
        else if (strstr(s, "://"))
          {
             buf = alloca(strlen(s) + 1);
             strcpy(buf, s);
          }
        else
          {
             char cwd[PATH_MAX];

             if (getcwd(cwd, sizeof(cwd) - 1))
               {
                  cwd[sizeof(cwd) - 1] = 0;
                  buf = alloca(strlen(cwd) + 1 + strlen(s) + sizeof("file://") + 1);
                  sprintf(buf, "file://%s/%s", cwd, s);
               }
          }
     }
   if (buf)
     {
        eldbus_message_iter_arguments_append(array, "{sv}", &entry);
        eldbus_message_iter_basic_append(entry, 's', "xesam:url");
        var = eldbus_message_iter_container_new(entry, 'v', "s");
        eldbus_message_iter_basic_append(var, 's', buf);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(array, entry);
     }

   s = video_artfile_get(inf->vid);
   if (s)
     {
        buf = alloca(strlen(s) + sizeof("file://") + 1);
        sprintf(buf, "file://%s", s);
        eldbus_message_iter_arguments_append(array, "{sv}", &entry);
        eldbus_message_iter_basic_append(entry, 's', "mpris:artUrl");
        var = eldbus_message_iter_container_new(entry, 'v', "s");
        eldbus_message_iter_basic_append(var, 's', buf);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(array, entry);
     }

   s = video_title_get(inf->vid);
   if (!s) s = video_meta_title_get(inf->vid);
   if ((!s) && (inf->file_cur))
     {
        Winvid_Entry *vid = inf->file_cur->data;
        if (vid)
          {
             file = vid->file;
             s = ecore_file_file_get(vid->uri ? vid->uri->path : file);
             if (s)
               {
                  tmp = alloca(strlen(s) + 1);
                  strcpy(tmp, s);
                  for (p = tmp; *p; p++)
                    {
                       // nuke stupid characters from the label that may be in filename
                       if ((*p == '_') || (*p == '#') || (*p == '$') || (*p == '%') ||
                           (*p == '*') || (*p == '+') || (*p == '[') || (*p == ']') ||
                           (*p == ';') || (*p == '<') || (*p == '=') || (*p == '>') ||
                           (*p == '^') || (*p == '`') || (*p == '{') || (*p == '}') ||
                           (*p == '|') || (*p == '~') || (*p == 127) ||
                           (*p == '\'') || (*p == '\\'))
                         {
                            *p = ' ';
                         }
                       else if (*p == '.')
                         {
                            *p = 0;
                            break;
                         }
                    }
                  s = tmp;
               }
          }
     }
   if (s)
     {
        eldbus_message_iter_arguments_append(array, "{sv}", &entry);
        eldbus_message_iter_basic_append(entry, 's', "xesam:title");
        var = eldbus_message_iter_container_new(entry, 'v', "s");
        eldbus_message_iter_basic_append(var, 's', s);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(array, entry);
     }

   s = video_meta_album_get(inf->vid);
   if (s)
     {
        eldbus_message_iter_arguments_append(array, "{sv}", &entry);
        eldbus_message_iter_basic_append(entry, 's', "xesam:album");
        var = eldbus_message_iter_container_new(entry, 'v', "s");
        eldbus_message_iter_basic_append(var, 's', buf);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(array, entry);
     }

   s = video_meta_artist_get(inf->vid);
   if (s)
     {
        eldbus_message_iter_arguments_append(array, "{sv}", &entry);
        eldbus_message_iter_basic_append(entry, 's', "xesam:artist");
        var = eldbus_message_iter_container_new(entry, 'v', "as");
        eldbus_message_iter_arguments_append(var, "as", &var2);
        eldbus_message_iter_basic_append(var2, 's', s);
        eldbus_message_iter_container_close(var, var2);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(array, entry);
     }

   s = video_meta_comment_get(inf->vid);
   if (s)
     {
        eldbus_message_iter_arguments_append(array, "{sv}", &entry);
        eldbus_message_iter_basic_append(entry, 's', "xesam:comment");
        var = eldbus_message_iter_container_new(entry, 'v', "as");
        eldbus_message_iter_arguments_append(var, "as", &var2);
        eldbus_message_iter_basic_append(var2, 's', s);
        eldbus_message_iter_container_close(var, var2);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(array, entry);
     }

   s = video_meta_genre_get(inf->vid);
   if (s)
     {
        eldbus_message_iter_arguments_append(array, "{sv}", &entry);
        eldbus_message_iter_basic_append(entry, 's', "xesam:genre");
        var = eldbus_message_iter_container_new(entry, 'v', "as");
        eldbus_message_iter_arguments_append(var, "as", &var2);
        eldbus_message_iter_basic_append(var2, 's', s);
        eldbus_message_iter_container_close(var, var2);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(array, entry);
     }

   len = video_length_get(inf->vid) * 1000000.0;
   eldbus_message_iter_arguments_append(array, "{sv}", &entry);
   eldbus_message_iter_basic_append(entry, 's', "mpris:length");
   var = eldbus_message_iter_container_new(entry, 'v', "x");
   eldbus_message_iter_basic_append(var, 'x', len);
   eldbus_message_iter_container_close(entry, var);
   eldbus_message_iter_container_close(array, entry);

   eldbus_message_iter_container_close(iter, array);
   return EINA_TRUE;
}

GETTER(volume)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   double vol = 1.0;

   if (!inf) goto end;
   if (browser_visible()) goto end;
   vol = video_volume_get(inf->vid);
end:
   eldbus_message_iter_arguments_append(iter, "d", vol);
   return EINA_TRUE;
}

SETTER(volume)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   double vol = 1.0; // max

   if (!inf) goto end;
   if (browser_visible()) goto end;
   eldbus_message_iter_arguments_get(iter, "d", &vol);
   if (vol < 0.0) vol = 0.0;
   if (vol > 1.0) vol = 1.0;
   video_volume_set(inf->vid, vol);
   mpris_volume_change();
end:
   return eldbus_message_method_return_new(msg);
}

GETTER(position)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   uint64_t pos = 0;

   if (!inf) goto end;
   if (browser_visible()) goto end;
   pos = video_position_get(inf->vid) * 1000000.0;
end:
   eldbus_message_iter_arguments_append(iter, "x", pos);
   return EINA_TRUE;
}

GETTER(minimum_rate)
{
   eldbus_message_iter_arguments_append(iter, "d", 1.0);
   return EINA_TRUE;
}

GETTER(maximum_rate)
{
   eldbus_message_iter_arguments_append(iter, "d", 1.0);
   return EINA_TRUE;
}

GETTER(can_go_next)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_go_previous)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_play)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_pause)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_seek)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_control)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

static const Eldbus_Property properties_player[] =
{
   PROP_RO("PlaybackStatus", "s", playback_status),
   PROP_RW("LoopStatus",     "s", loop_status),
   PROP_RW("Rate",           "d", rate),
   PROP_RW("Shuffle",        "b", shuffle),
   PROP_RO("Metadata",       "a{sv}", metadata),
   PROP_RW("Volume",         "d", volume),
   PROP_RO("Position",       "x", position),
   PROP_RO("MinimumRate",    "d", minimum_rate),
   PROP_RO("MaximumRate",    "d", maximum_rate),
   PROP_RO("CanGoNext",      "b", can_go_next),
   PROP_RO("CanGoPrevious",  "b", can_go_previous),
   PROP_RO("CanPlay",        "b", can_play),
   PROP_RO("CanPause",       "b", can_pause),
   PROP_RO("CanSeek",        "b", can_seek),
   PROP_RO("CanControl",     "b", can_control),
   { NULL, NULL, NULL, NULL, 0 }
};

API(next)
{
   if (browser_visible()) goto end;
   win_do_next(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(previous)
{
   if (browser_visible()) goto end;
   win_do_prev(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(pause)
{
   if (browser_visible()) goto end;
   win_do_pause(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(play_pause)
{
   if (browser_visible()) goto end;
   win_do_play_pause(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(stop)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");

   if (!inf) goto end;
   if (browser_visible()) goto end;
   video_stop(inf->vid);
   elm_layout_signal_emit(inf->lay, "action,stop", "rage");
   elm_layout_signal_emit(inf->lay, "state,default", "rage");
   if (inf->browse_mode) browser_show(mainwin);
   else evas_object_del(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(play)
{
   if (browser_visible()) goto end;
   win_do_play(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(seek)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   uint64_t pos = 0;

   if (!inf) goto end;
   if (browser_visible()) goto end;
   if (!eldbus_message_arguments_get(msg, "x", &pos))
     return eldbus_message_error_new(msg, "Invalid arguments", "Error getting position");
   video_position_set(inf->vid, (double)pos / 1000000.0);
end:
   return eldbus_message_method_return_new(msg);
}

/*
API(set_position)
{
   fprintf(stderr, "Cannot do set_position yet.\n");
   // XXX: get track + position
   return eldbus_message_method_return_new(msg);
}
*/

API(open_uri)
{
   char *uri = NULL;

   if (!eldbus_message_arguments_get(msg, "s", &uri))
     return eldbus_message_error_new(msg, "Invalid arguments", "Error getting URI string");
   win_video_insert(mainwin, uri);
   win_video_next(mainwin);
   win_list_content_update(mainwin);
   browser_hide(mainwin);
   return eldbus_message_method_return_new(msg);
}

static const Eldbus_Method methods_player[] =
{
   METHOD("Next", NULL, next),
   METHOD("Previous", NULL, previous),
   METHOD("Pause", NULL, pause),
   METHOD("PlayPause", NULL, play_pause),
   METHOD("Stop", NULL, stop),
   METHOD("Play", NULL, play),
   METHOD("Seek", ELDBUS_ARGS({"x", "Offset"}), seek),
//   METHOD("SetPosition", ELDBUS_ARGS({ "ox", "Path,Position"}), set_position),
   METHOD("OpenUri", ELDBUS_ARGS({"s", "Uri"}), open_uri),
   { NULL, NULL, NULL, NULL, 0 }
};

static const Eldbus_Signal signals_player[] =
{
   [ 0 ] = { "Seeked", ELDBUS_ARGS({ "x", "Position" }), 0 },
   { NULL, NULL, 0 }
};

static const Eldbus_Service_Interface_Desc desc_player = {
   "org.mpris.MediaPlayer2.Player", methods_player, signals_player, properties_player, NULL, NULL
};

void
mpris_fullscreen_change(void)
{
   if (!iface) return;
   eldbus_service_property_changed(iface, "Fullscreen");
}

void
mpris_volume_change(void)
{
   if (!iface_player) return;
   eldbus_service_property_changed(iface_player, "Volume");
}

void
mpris_loop_status_change(void)
{
   if (!iface_player) return;
   eldbus_service_property_changed(iface_player, "LoopStatus");
}

void
mpris_playback_status_change(void)
{
   if (!iface_player) return;
   eldbus_service_property_changed(iface_player, "PlaybackStatus");
}

void
mpris_position_change(double pos)
{
   uint64_t p = pos * 1000000.0;

   if (!iface_player) return;
   eldbus_service_signal_emit(iface_player, 0, p);
   eldbus_service_property_changed(iface_player, "Position");
}

void
mpris_metadata_change(void)
{
   if (!iface_player) return;
   eldbus_service_property_changed(iface_player, "Metadata");
}

void
mpris_init(Evas_Object *win)
{
   elm_need_eldbus();
   conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!conn) return;
   mainwin = win;
   eldbus_name_request(conn, SERVICE, 0, _cb_name_request, NULL);
   iface = eldbus_service_interface_register(conn, PATH, &desc);
   iface_player = eldbus_service_interface_register(conn, PATH, &desc_player);
}

void
mpris_shutdown(void)
{
   if (!conn) return;
   mainwin = NULL;
//   if (iface_player) eldbus_service_object_unregister(iface_player);
   iface_player = NULL;
//   if (iface) eldbus_service_object_unregister(iface);
   iface = NULL;
//   eldbus_name_release(conn, SERVICE, NULL, NULL);
//   eldbus_connection_unref(conn);
   conn = NULL;
}
