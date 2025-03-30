##### project
project('rage', 'c',
        version        : '0.4.0',
        license        : 'BSD 2 clause',
        default_options: [ 'buildtype=plain', 'c_std=gnu99' ],
        meson_version  : '>= 0.60.0')
base_url = 'https://www.enlightenment.org/about-'

##### convenience variables for later
proj     = meson.project_name()
ver      = meson.project_version()

##### dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
efl_version = '>= 1.26.0'
deps = [ dependency('elementary', version: efl_version), m_dep ]
edje = dependency('edje',       version: efl_version)

##### edje_cc binary compiler tool
edje_cmd = get_option('edje-cc')
if edje_cmd == ''
  edje_cmd = join_paths(edje.get_variable(pkgconfig: 'prefix'),
                        'bin', 'edje_cc')
endif

##### dir locations
dir_prefix = get_option('prefix')
dir_bin    = join_paths(dir_prefix, get_option('bindir'))
dir_lib    = join_paths(dir_prefix, get_option('libdir'))
dir_data   = join_paths(dir_prefix, get_option('datadir'))
dir_locale = join_paths(dir_prefix, get_option('localedir'))

##### config.h
cfg = configuration_data()
cfg.set_quoted('PACKAGE'                 , proj)
cfg.set_quoted('PACKAGE_NAME'            , proj)
cfg.set_quoted('PACKAGE_VERSION'         , ver)
cfg.set_quoted('PACKAGE_STRING'          , proj + ' ' + ver)
cfg.set_quoted('PACKAGE_URL'             , base_url + proj)
cfg.set_quoted('PACKAGE_BIN_DIR'         , dir_bin)
cfg.set_quoted('PACKAGE_LIB_DIR'         , dir_lib)
cfg.set_quoted('PACKAGE_DATA_DIR'        , join_paths(dir_data, proj))
cfg.set_quoted('LOCALEDIR'               , dir_locale)
cfg.set       ('_GNU_SOURCE'             , 1)
cfg.set       ('__EXTENSIONS__'          , 1)
cfg.set       ('_POSIX_PTHREAD_SEMANTICS', 1)
cfg.set       ('_ALL_SOURCE'             , 1)
cfg.set       ('_POSIX_SOURCE'           , 1)
cfg.set       ('_POSIX_1_SOURCE'         , 1)
configure_file(output: 'rage_config.h', configuration: cfg)

##### subdirs
subdir('src/bin')
subdir('data')
