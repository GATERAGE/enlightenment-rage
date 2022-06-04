![Rage](/data/icons/rage.png)
# Rage

-----

*FOR ANY ISSUES PLEASE EMAIL:*

`enlightenment-devel@lists.sourceforge.net`

-----

This is a Video + Audio player along the lines of mplayer. It's simple
yet decent looking. It finds album art for you for music files,
generates thumbnails for videos so you can select logaction by
thumbnail and more. It accepts file lists on the cmdline ala:

```
  rage file1.mp3 file2.mp3 file3.avi file4.mp4 file5.ogg
  rage dvd:/
  rage http://www.blah.com/stream
```

You can also specify subtitle files with -sub:

```
  rage file.mp4 -sub mysubs.srt
```

And of course you can run it with no arguments:

```
  rage
```

Which goes into "Browser mode" where Rage will index/list your
~/Videos directory as a library (and subdirectories).

This depends on video engine support to display them, so it may not
work. You might have toinstall more codec plugins for Gstreamer. Also
Rage will use a srt or sub format file if found, automatically. If the
filename is video.mp4 or video.avi, video.mov etc. then Rage looks for
video.srt or video.sub and automatically will use it.

If you DND files onto its window, they are added to the playlist.
Mouse over the right of the window brings up the playlist. Mouse over
the position bar at the bottom, if it's a video, brings up a video
thumbnail of that position (it may take a while to generate thumbnails
for that video - they are cached).

If you run Rage without arguments at all it goes into "browser" mode
where it will index all your media files in "$HOME/Videos" or whatever
you have configured your XDG video directory to be. It will flatten
out all directories that it recursively scans in this videos director
with a header with the directory name and a list of images per video
and the filename with extensions removed. It will either generate a
series of images to flip as the video thumb or may actually search for
a poster file dependig on length of video file (if it is more than 60
minutes long and less than 5 hours and the image ratio is between 4:3
and 4:1 it'll be assumed to be a movie for example). So while it is
fetching or generating thumbnails for the first time, be a little
patient. They will be cached so they only need to be generated or
fetched once (or if the file changes).

-----

## Key controls

It's fully key controlled too. Key controls are:

| KEY             | Action
| ----------------|----------------------------------------------------------
| Left            | rewind or DVD nav left
| [               | rewind
| Right           | fast forward or DVD nav right
| ]               | fast forward
| Up              | Volume up or DVD nav up
| +               | Volume up
| =               | Volume up
| Down            | Volume down DVD nav down
| -               | Volume down
| space           | Pause toggle
| Pause           | Pause toggle
| p               | Pause toggle
| XF86AudioPlay   | pause toggle
| s               | Stop
| XF86AudioStop   | Stop
| BackSpace       | Stop
| Delete          | Stop
| c               | Stop but don't exit playback
| PgUp            | Previous file
| XF86AudioPrev   | Previous file
| PgDn            | Next file
| XF86AudioNext   | Next file
| Home            | First file
| End             | Last file
| m               | Mute toggle
| XF86AudioMute   | Mute toggle
| l               | Loop 1 mode toggle
| q               | Exit
| Escape          | Exit
| f               | Fullscreen toggle
| F11             | Fullscreen toggle
| n               | Normalize window size to video size
| \               | Toggle File list or browser mode depending on launch mode
| y               | Video low quality toggle
| z               | Video zoom fit/fill modes toggle
| e               | Eject file/media
| Return          | DVD nav select
| Enter           | DVD nav select
| ,               | DVD nav angle prev
| <               | DVD nav angle prev
| .               | DVD nav angle next
| >               | DVD nav angle next
| Tab             | DVD nav force
| 0               | DVD nav 0
| 1               | DVD nav 1
| 2               | DVD nav 2
| 3               | DVD nav 3
| 4               | DVD nav 4
| 5               | DVD nav 5
| 6               | DVD nav 6
| 7               | DVD nav 7
| 8               | DVD nav 8
| 9               | DVD nav 9
| `               | DVD nav 10
| ~               | DVD nav 10
| F1              | DVD nav menu 1
| F2              | DVD nav menu 2
| F3              | DVD nav menu 3
| F4              | DVD nav menu 4
| F5              | DVD nav menu 5
| F6              | DVD nav menu 6
| F7              | DVD nav menu 7

-----

## Requirements

* [efl](https://git.enlightenment.org/enlightenment/efl)

-----

## Compiling and Installing

Meson is the build system used for this project. For more information
please see [mesonbuild.com](https://mesonbuild.com)

You will need normal build tooling installed such as a compiler (gcc
or clang for example), pkg-config, ninja, any relevant package-dev or
package-devel packages if your distribution splits out development
headers (e.g. libc6-dev) etc.

Depending on where dependencies (like efl) are installed, you might have to
set your `PKG_CONFIG_PATH` environment variable like:
```
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig
```

Also note that some distributions like to add extra arch directories
to your library locations so you might have to have more like:
```
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/lib64/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig
```

You will need to enusre that the destination library directory (e.g.
`/usr/local/lib` is in your /etc/ld.so.conf or /etc/ld.so.conf.d/
files and after insgtaling anything that installes libraries you
re-run `ldconfig`. Please see relevant documentation on ldconfig and
ld.so.conf for your distribution.

You might also want to add the destination bin dir to your environment
variable PATH (see documentation on your shell PATH variable) such as:
```
export PATH=/usr/local/bin:/usr/bin:/bin
```

Normal compilation in /usr/local:
```
meson . build
ninja -C build
sudo ninja -C build install
```

For meson build generic options:
```
meson --help
```

For a list of project specific options supported:
```
cat meson_options.txt
```

To set 1 or more project specific options:
```
meson --prefix=/path/to -Doption=value [-Dother=value2] [...] . build
```

To display current configuration:
```
meson configure build
```

The above will only work after at least the following is done:
```
meson . build
```

### Quick build help

How clean out the build and config and start fresh:
```
rm -rf build
```

How to make a dist tarball and check its build:
(must do it from git tree clone and commit all changes to git first)
```
ninja -C build dist
```

How to change prefix:
```
meson --prefix=/path/to/prefix . build
```

How to install in a specific destination directory for packaging:
```
DESTDIR=/path/to/destdir ninja -C build install
```

How to build with verbose output (full commands run):
```
ninja -C build -v
```
