# EXPLANATION — what Rage is and why it is here

## What it is

Rage is the video and audio player from the Enlightenment project
(https://www.enlightenment.org/download). Carsten "Rasterman" Haitzler, the
author of Enlightenment and EFL, wrote it in 2014. It is a small C program
(about 8,400 lines across 17 source files) built on the Enlightenment
Foundation Libraries (EFL).

It does four things:

1. **Plays media.** You pass it files, directories, `dvd:/` or `http://`
   streams, and it plays them in a borderless, keyboard-driven window.
2. **Browses a library.** With no arguments it scans `~/Videos` (or your
   XDG video directory) and shows a scrolling wall of animated thumbnails,
   grouped by directory.
3. **Makes previews.** A separate helper process (`rage_thumb`) renders a
   frame every 10 seconds of each video. Hovering over the seek bar then
   shows the frame at that point, and the browser tiles cycle through them.
4. **Finds artwork.** It uses embedded tags, then cover files next to the
   media, then (as a last resort) an image search for album covers and film
   posters.

It also exposes itself on D-Bus as an MPRIS2 player
(`org.mpris.MediaPlayer2.rage`). Desktop media keys, panels and `playerctl`
can therefore control it.

## Where this copy comes from

| | |
|---|---|
| Upstream | https://git.enlightenment.org/enlightenment/rage (Gitea; no GitHub mirror) |
| Version | 0.4.0 (meson.build); the latest tag is `v0.4.0` |
| History | 283 commits, 2014 → 2026-06-08, with full upstream history kept |
| Authors | Carsten Haitzler, Eduardo Lima (Etrunko), Alastair Poole (netstar) |
| Licence | **BSD-2-Clause** (`COPYING`). This is OSI open source, so it passes the estate's "open source or go away" rule |

Because upstream is not on GitHub, this repository is an import rather than
a GitHub fork. The `upstream` remote in a working clone points at
git.enlightenment.org. It is named `enlightenment-rage` because
`GATERAGE/rage` would collide with the existing `GATERAGE/RAGE`, as GitHub
repository names are case-insensitive.

## "Rage" vs RAGE — making sense of the name

GATERAGE's own RAGE is the **Retrieval Augmented Generative Engine**, the
org's AI/RAG work (rage.pythai.net). Enlightenment's Rage is unrelated: the
name is a coincidence. The two are kept apart by name (`enlightenment-rage`)
and by licence: this code stays BSD-2-Clause with its copyright notice. Any
GATERAGE code that links or copies it must carry `COPYING` forward.

Rage is still a good fit here:

- **It shows how to build a media front end on EFL.** It covers smart
  objects, Edje themes, Emotion playback, gesture layers, threaded directory
  scans, and an out-of-process worker for heavy jobs.
- **Its thumbnail/artwork pipeline is a small retrieval engine.** It keys
  media by a hash of its path, caches derived artefacts (frames, posters)
  under XDG cache, and falls back from local evidence (tags, `cover.jpg`) to
  remote search. The same pattern applies to building a media index for a
  RAG system.
- **MPRIS gives a ready control surface.** Agents and scripts can drive
  playback over D-Bus without touching the UI.

## Current state (as read from the code, 2026-09-26)

- It is actively maintained, with 2026 commits fixing out-of-bounds accesses
  in the browser. Most development happened in 2014–2017.
- Configuration is a stub. `Config` holds only a `version` field, and every
  behaviour is hard-coded or set on the command line.
- The artwork search scrapes Google Images HTML over plain HTTP with a 2012
  mobile user-agent. That scraping is brittle, and it sends your
  artist/album/title (or the filename) to Google. See TECHNICAL.md.
- TODO.md lists the open wishes: audio/video/subtitle track selection during
  play, loop-all, a volume on-screen display, handling of non-seekable
  streams, and a right-click control panel.

See **USAGE.md** for how to build and drive it, and **TECHNICAL.md** for
how it works inside.
