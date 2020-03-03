#include <Elementary.h>
#include "main.h"
#include "config.h"

static Config *config = NULL;
static Eet_Data_Descriptor *edd_base = NULL;

void
config_init(void)
{
   Eet_Data_Descriptor_Class eddc;
   Eet_File *ef;
   char buf[PATH_MAX];

   elm_need_efreet();
   efreet_init();
   
   eet_eina_stream_data_descriptor_class_set
     (&eddc, sizeof(eddc), "Config", sizeof(Config));
   edd_base = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC
     (edd_base, Config, "version", version, EET_T_INT);
   snprintf(buf, sizeof(buf), "%s/rage/config/standard/base.cfg", efreet_config_home_get());
   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        config = eet_data_read(ef, edd_base, "config");
        eet_close(ef);
     }
   if (!config)
     {
        config = calloc(1, sizeof(Config));
        if (!config) abort();
        config->version = 1;
        config_save();
     }
}

void
config_shutdown(void)
{
   free(config);
   if (edd_base)
     {
        eet_data_descriptor_free(edd_base);
        edd_base = NULL;
     }
   efreet_shutdown();
}

Config *
config_get(void)
{
   return config;
}

void
config_save(void)
{
   Eet_File *ef;
   Eina_Bool ok;
   char buf[PATH_MAX], buf2[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/rage/config/standard/", efreet_config_home_get());
   ecore_file_mkpath(buf);
   snprintf(buf, sizeof(buf), "%s/rage/config/standard/base.cfg.tmp", efreet_config_home_get());
   snprintf(buf2, sizeof(buf2), "%s/rage/config/standard/base.cfg", efreet_config_home_get());
   ef = eet_open(buf, EET_FILE_MODE_WRITE);
   if (ef)
     {
        ok = eet_data_write(ef, edd_base, "config", config, 1);
        eet_close(ef);
        if (ok) ecore_file_mv(buf, buf2);
     }
}
