inc = include_directories('.', '../..')
executable('rage', [
    'controls.c',   'controls.h',
    'dnd.c',        'dnd.h',
    'gesture.c',    'gesture.h',
    'key.c',        'key.h',
    'main.c',       'main.h',
    'video.c',      'video.h',
    'winvid.c',     'winvid.h',
    'win.c',        'win.h',
    'winlist.c',    'winlist.h',
    'config.c',     'config.h',
    'sha1.c',       'sha1.h',
    'videothumb.c', 'videothumb.h',
    'albumart.c',   'albumart.h',
    'browser.c',    'browser.h',
    'mpris.c',      'mpris.h',
    'util.c',       'util.h'
  ],
  include_directories: inc,
  dependencies       : deps,
  win_subsystem      : 'windows',
  install            : true)
executable('rage_thumb', [
    'thumb.c',
    'sha1.c',     'sha1.h',
    'albumart.c', 'albumart.h'
  ],
  include_directories: inc,
  dependencies       : deps,
  install_dir        : join_paths(dir_lib, proj, 'utils'),
  install            : true)
