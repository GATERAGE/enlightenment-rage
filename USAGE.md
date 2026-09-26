# USAGE — building and running Rage

## 1. Build

Requirements: a C compiler, meson ≥ 0.60, ninja, pkg-config, and **EFL ≥
1.26.0**, which provides elementary, edje, emotion, eet, ecore, efreet and
eldbus. Playback goes through Emotion's default engine, which is GStreamer
1.x, so the codecs you can play are the GStreamer plugins you have
installed.

Debian / Ubuntu (the archive currently carries EFL 1.26.2):

```sh
sudo apt install build-essential meson ninja-build pkg-config libefl-all-dev \
     gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
     gstreamer1.0-plugins-ugly gstreamer1.0-libav
```

Build and install:

```sh
meson setup build              # older meson: `meson . build`
ninja -C build
sudo ninja -C build install    # default prefix /usr/local
```

This installs:

| Path | What |
|---|---|
| `$prefix/bin/rage` | the player |
| `$prefix/lib/rage/utils/rage_thumb` | thumbnail/artwork worker (spawned by `rage`, never run by hand) |
| `$prefix/share/rage/themes/default.edj` | the compiled Edje theme |
| `$prefix/share/applications/rage.desktop` | desktop entry (`Exec=rage %U`) |

`rage` finds `rage_thumb` and the theme through the compiled-in prefix. A
build run from `build/` without `install` will not find them. Use
`--prefix=$HOME/.local` if you do not want a system install. If edje_cc is
not next to the EFL prefix, pass `-Dedje-cc=/path/to/edje_cc` (the only
project option).

## 2. Run

```sh
rage                                  # browser mode over ~/Videos
rage -dir /srv/media                  # browser/scan another directory
rage a.mp4 b.mkv song.flac            # play a list
rage film.mp4 -sub film.en.srt        # -sub applies to the file BEFORE it
rage /path/to/dir                     # recursively queue every media file
rage -d 2 /path/to/dir                # ...no more than 2 levels deep (0 = unlimited, max 99)
rage dvd:/                            # DVD (needs a GStreamer DVD source)
rage http://host/stream               # network stream
rage -f file.mp4                      # start fullscreen
rage -rot 90 file.mp4                 # rotate output 0/90/180/270
```

Notes on argument handling (from `src/bin/main.c`):

- `-sub` attaches to the most recent file. If you give it before any file,
  it is silently ignored.
- `file:///…` URIs are decoded to local paths.
- A directory argument is scanned in a background thread. Only files with a
  known video or audio extension are queued (see `util.c` for the list). If
  the scan finds nothing, Rage exits.
- A directory scan will not recurse into `$HOME` itself or into `/`.
- If no file opens, the window is shown after a 10-second fallback timer
  anyway.

## 3. Controls

**Mouse / touch**

- Hovering at the right edge shows the playlist.
- Hovering over the position bar shows a preview frame for that position.
- Dragging horizontally scrubs the video. Half the window width equals 60
  seconds. Releasing with speed carries on with momentum, and playback
  resumes if it was playing.
- Dropping files onto the window adds them to the playlist.

**Keyboard** (the full table is in README.md)

| Key | Action |
|---|---|
| Space / p / Pause | play/pause |
| ← → or [ ] | rewind / fast-forward (DVD menu nav on discs) |
| ↑ ↓ or + − | volume |
| PgUp / PgDn, Home / End | previous/next, first/last file |
| m | mute |
| l | loop current file |
| f / F11 | fullscreen |
| n | resize window to the video's native size |
| z | zoom: fit ↔ fill |
| y | low-quality (fast) scaling |
| \\ | toggle playlist, or browser in browser mode |
| s / Backspace / Delete | stop |
| c | stop but stay in playback |
| e | eject |
| q / Esc | quit |
| Enter, Tab, 0–9, F1–F7, `,` `.` | DVD navigation |

## 4. Remote control over D-Bus (MPRIS2)

Rage owns `org.mpris.MediaPlayer2.rage` on the session bus (except on
Windows), so any MPRIS client works:

```sh
playerctl -p rage play-pause
playerctl -p rage next
playerctl -p rage volume 0.5
playerctl -p rage position 30+          # seek +30 s (Seek)
playerctl -p rage open file:///srv/a.mp4

# raw D-Bus
dbus-send --session --dest=org.mpris.MediaPlayer2.rage --print-reply \
  /org/mpris/MediaPlayer2 org.freedesktop.DBus.Properties.Get \
  string:org.mpris.MediaPlayer2.Player string:Metadata
```

Supported: Play, Pause, PlayPause, Stop, Next, Previous, Seek, OpenUri,
Raise, Quit, plus the Volume, LoopStatus (None/Track), Fullscreen and
Metadata properties. Not supported: SetPosition, TrackList, Shuffle,
playback Rate, and LoopStatus=Playlist.

Only the first running instance gets the bus name. A second `rage` runs
normally but cannot be controlled over MPRIS.

## 5. Files Rage writes

| Path | Contents |
|---|---|
| `$XDG_CONFIG_HOME/rage/config/standard/base.cfg` | eet config (currently only a version number) |
| `$XDG_CACHE_HOME/rage/thumb/XX/<sha1>.eet` | one JPEG frame (q70, 320 px wide) per 10 s of video |
| `$XDG_CACHE_HOME/rage/albumart/XX/<sha1>.jpg` | fetched or extracted artwork |

`<sha1>` is the SHA-1 of the file's real path, and `XX` is its first byte.
Deleting `~/.cache/rage` is safe and forces regeneration. Thumbnails are
regenerated when the media file is newer than its cache entry.

**Privacy:** for music without local art, and for videos in browser mode,
Rage sends the artist/album/title, or the filename, to Google Images over
plain HTTP. To keep it offline, put a `cover.jpg`, `folder.jpg` or
`<name>.jpg` next to the media. Rage looks locally first, so a local image
means no search is made.

## 6. Keeping in sync with upstream

```sh
git remote add upstream https://git.enlightenment.org/enlightenment/rage.git
git fetch upstream
git merge upstream/master        # upstream default branch is master
```

Report upstream bugs at https://git.enlightenment.org/enlightenment/rage/issues.
