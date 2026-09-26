# TECHNICAL — how Rage works

This document is a reading of Rage 0.4.0 (upstream commit `5663526`,
2026-06-08). File and line references are to `src/bin/`.

## 1. Stack

```
rage (C99/gnu99, one process, one main loop)
 ├─ Elementary   windows, layouts, scroller, table, gesture layer, DnD
 ├─ Edje         the whole look: data/themes/default.edc → default.edj
 ├─ Emotion      playback object (default engine: GStreamer 1.x)
 ├─ Evas         canvas; smart objects for video + thumbnails
 ├─ Ecore        main loop, timers, animators, threads, child processes, HTTP
 ├─ Eet          config + thumbnail archives
 ├─ Efreet       XDG dirs (Videos, cache, config), URI decoding
 └─ Eldbus       MPRIS2 server
rage_thumb (separate executable, spawned per file, buffer engine, no window)
```

The build is meson (`meson.build`, `src/bin/meson.build`,
`data/*/meson.build`). It produces two executables. The Edje theme is
compiled with `edje_cc`, and images come from `data/themes/images`.

## 2. Source map

| File | Lines | Responsibility |
|---|---:|---|
| `main.c` | 367 | argv parsing, recursive directory scan thread, window creation, choice between browser and playlist mode, MPRIS init |
| `win.c` / `win.h` | 857 | the `Inf` state struct (hung off the window as data `"inf"`), high-level play/pause/next/volume, title, aspect, art display |
| `winvid.c` | 333 | the playlist model (`Eina_List` of `Winvid_Entry {file, uri, sub}`), open/next/prev, autosubtitle |
| `winlist.c` | 340 | the slide-in playlist panel |
| `video.c` | 1214 | `video` smart object that wraps an Emotion object, plus letterboxing, fill/fit, low-quality mode, metadata/chapter/channel accessors |
| `controls.c` | 223 | Edje signal wiring for on-screen buttons and the position bar |
| `key.c` | 349 | the keyboard map (README table) → `win_do_*` / `video_*` |
| `gesture.c` | 143 | momentum-drag scrubbing |
| `dnd.c` | 229 | drag-and-drop of files/URIs into the playlist |
| `browser.c` | 1221 | the library wall: threaded scan, virtualised tile grid, keyboard selection |
| `videothumb.c` | 679 | `videothumb` smart object: finds/creates cached frames or posters and cycles them |
| `thumb.c` | 289 | `rage_thumb` worker process: renders frames or fetches art, then exits |
| `albumart.c` | 433 | local art discovery + remote image search + cache paths |
| `mpris.c` | 1190 | `org.mpris.MediaPlayer2` + `.Player` interfaces |
| `config.c` | 75 | eet-backed config (currently only `version`) |
| `sha1.c` | 104 | SHA-1 used for cache keys |
| `util.c` | 83 | media extension tables, videos-dir resolution |

## 3. Start-up and modes

`elm_main` (`main.c`) walks argv. File arguments become `Winvid_Entry`
records, and a directory argument becomes a `Recursion_Data`. If there are
no inputs, `inf->browse_mode` is set.

- **Playlist mode.** `win_video_file_list_set()` loads the list, and the
  window is shown when the first video reports its size (`win.c:784`).
  A 10 s timer shows it regardless.
- **Directory mode.** `ecore_thread_feedback_run()` walks the tree off the
  main loop. Each directory's files are sorted and sent back with
  `ecore_thread_feedback`. The main-loop callback filters them by extension
  (`util_video_ok` / `util_audio_ok`), appends them, and starts playing the
  first one. `_check_recursion` stops at the depth limit (`-d`, default and
  maximum 99, 0 = unlimited) and refuses to descend into `$HOME` or `/`.
  If nothing is found, the program exits.
- **Browser mode.** `browser_show()` scans `util_videos_dir_get()`: `-dir`,
  then the XDG Videos dir, then `~/Videos`, then `/tmp`.

Hardware acceleration is requested with
`elm_config_accel_preference_set("accel")`.

## 4. Playback (`video.c`, `winvid.c`)

`video_add()` creates a clipped smart object around
`emotion_object_add()`. No engine is named, so Emotion picks its default
(GStreamer). The wrapper listens to `frame_decode`, `frame_resize`,
`decode_stop`, `progress_change`, `ref_change` and `open_done`, and exposes
a flat API (`video.h`). The API covers position/length, volume/mute, loop,
fill vs fit, low-quality scaling, chapters, audio/video/SPU channel
enumeration, DVD navigation events and metadata tags.

Emotion can enumerate channels, but TODO.md notes there is no UI to switch
them during play.

**Subtitles.** `video_file_autosub_set()` takes an explicit `-sub` file if
there is one. Otherwise it replaces the text after the last dot of the media
path with `.srt/.SRT/.sub/.SUB`. If that misses, it tries the same after the
first dot, which catches `film.en.mp4` → `film.srt`.

**Scrubbing** (`gesture.c`). An Elementary gesture layer delivers
`MOMENTUM` events. Once the horizontal travel exceeds a finger size, Rage
pauses and sets `pos = start + dx * 60 / (width/2)`. On release, an
`ecore_animator_timeline` continues along the flick's momentum for
`|v|/1000` seconds and then restores play state.

## 5. The thumbnail and artwork pipeline

This is the most interesting part of the codebase.

```
videothumb (UI)                      rage_thumb <path> 10000 <poster>
  │ file_set(path, pos)                (child process, nice 10,
  │                                     TERM_WITH_PARENT)
  ├─ poster mode? albumart_file_get()     │
  │     local art exists → show it        ├─ poster: embedded artwork (Emotion)
  ├─ else key = sha1(realpath)            │     → albumart cache; exit
  │   ~/.cache/rage/thumb/XX/<sha>.eet    ├─ open with Emotion, wait open_done
  │   entry key = ⌊pos·1000/10000⌋·10000  ├─ audio → albumart_find(artist,album,title)
  ├─ cache missing/stale (mtime)          ├─ movie (poster && 4:3…4:1 && 1–5 h)
  │   → spawn rage_thumb (≤5 retries,     │     → albumart_find(title + "film poster")
  │     de-duplicated per realpath)       ├─ other video in poster mode → albumart_find(title)
  └─ ECORE_EXE_EVENT_DEL → reload         └─ else: for pos = 0,10000,20000… ms
                                                load frame via Evas generic loader
                                                (file, key="<ms>"), scale to
                                                320 px wide, write JPEG q70 into
                                                <sha>.eet.tmp → rename → exit
```

Design points:

- **Out-of-process.** Decoding runs in a child with its own EFL instance on
  the `buffer` engine, rendering into an inlined-image sub-window. A crashing
  codec therefore kills `rage_thumb`, not the player. There is a 360 s
  watchdog (`_cb_timeout`).
- **Atomic cache writes.** Every artefact is written to `.tmp`/`.tmo` and
  renamed into place, so readers never see partial files.
- **Content addressing by path.** The cache key is SHA-1 of the realpath,
  sharded on the first byte. Freshness is checked by comparing mtimes
  (`videothumb.c:343`).
- **Virtualised browser.** `_entry_files_pop_eval` creates tile objects
  only for cells within 80 px of the viewport. It creates and destroys at
  most 32 per frame (`CREATED_MAX`/`DESTROYED_MAX`) and defers the rest
  with an animator, so huge libraries scroll without stalls. The scan
  thread and main loop hand off entries with a semaphore (`step_sema`) and
  a per-entry `Eina_Lock`.

**Artwork resolution order** (`albumart_file_get`):

1. `<file>.{png,jpg,jpeg,jpe}` (all cases)
2. `<dir>/<name-without-ext>.<ext>`
3. `<dir>/.<file>.<ext>` and `<dir>/.<name>.<ext>`
4. `<dir>/.thumb/<file|name>.<ext>`
5. `<dir>/{cover,front,folder}.<ext>` (plain, dotted and case variants)
6. the cache path `~/.cache/rage/albumart/XX/<sha1>.jpg`, filled by embedded
   artwork or by the remote search below

**Remote search** (`albumart_find`). Rage builds
`http://www.google.com/search?tbm=isch&as_q=<words>…&tbs=ift:jpg` from
alphanumeric runs of the metadata, sent with a 2012 Android Chrome
user-agent. It takes the first `data-src="http…"` (or `imgurl=`) from the
HTML, waits 1.5 s ("google doesn't like to respond if we ask too fast"),
and downloads that image. `rage_thumb` then deletes the result if Evas
cannot decode it.

## 6. MPRIS (`mpris.c`)

The server owns `org.mpris.MediaPlayer2.rage` at `/org/mpris/MediaPlayer2`
(this code is compiled out on `_WIN32`).

- **Root interface.** Quit, Raise, and the Fullscreen property (read/write),
  plus Identity, DesktopEntry, SupportedMimeTypes and SupportedUriSchemes.
- **Player interface.** Next, Previous, Pause, PlayPause, Stop, Play,
  Seek(offset µs) and OpenUri (insert, jump to it, hide the browser).
  Properties: PlaybackStatus, LoopStatus (None/Track), Volume, Metadata,
  Position, and the Can* flags. Rate and Shuffle are accepted but are
  no-ops.
- **Signals.** The server emits `Seeked` and `PropertiesChanged` for
  Fullscreen, Volume, LoopStatus, PlaybackStatus, Position and Metadata.
- **Missing.** SetPosition and the TrackList interface. The bus-name reply
  is ignored (`_cb_name_request` returns immediately), so a second instance
  fails silently.

## 7. Theme

`data/themes/default.edc` (2,236 lines) defines every group the C code
loads: the main layout, controls, position bar with thumbnail popup,
playlist panel, browser item `rage/browser/item`, and the about screen.
Behaviour and look are joined by Edje signals such as `rage,selected`,
`state,fullscreen` and `about,show`. Restyling Rage therefore means editing
the EDC, not the C.

## 8. Issues noticed while reading

These come from reading the code only. **None of this was built or run
here** (EFL is not installed on the machine that wrote this).

| # | Where | Observation | Effect |
|---|---|---|---|
| 1 | `albumart.c` `Q_START`, `_fetch` | Scrapes Google Images over `http://`, with a fixed 2012 UA and HTML-string matching | Likely broken or brittle against current Google markup. It also leaks media metadata or filenames to a third party in cleartext |
| 2 | `thumb.c` `_cb_loaded` poster branch | `else ratio = iw / ih;` is integer division, and `ih` may be 0 | A 16:9 video with no Emotion ratio computes 1.0, so it is never classed as a movie. If `ih == 0`, the child gets SIGFPE (only `rage_thumb` dies, and it is retried up to 5 times) |
| 3 | `thumb.c` `elm_main` | Audio detection uses `strchr(file, '.')` (the first dot), not `strrchr` | `my.song.mp3` is not flagged as audio up front. This is harmless in practice, because `_cb_loaded` reclassifies via `emotion_object_audio_handled_get` |
| 4 | `thumb.c` `_cb_loaded` | A local `int iw, ih` shadows the file-scope `iw, ih` | Only confusing to read; no effect |
| 5 | `main.c` `-sub` | Ignored when it comes before any file | Silent user error |
| 6 | `mpris.c` | Bus-name ownership result is ignored | A second instance is invisible to MPRIS clients |
| 7 | `config.c` | Config only has `version` | Nothing persists between runs: volume, last position, zoom mode, engine choice |

Upstream is where fixes belong
(https://git.enlightenment.org/enlightenment/rage/issues). Items 1 and 2
are the most worth a patch: an HTTPS, opt-in artwork provider (for example
MusicBrainz Cover Art Archive for audio) and a float ratio with an `ih > 0`
guard.
