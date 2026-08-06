# verlet

Simple physics approximation using [Verlet Integration](https://en.wikipedia.org/wiki/Verlet_integration) approach.

Inspired by [this video](https://www.youtube.com/watch?v=lS_qeBy3aQI). Almost exactly the same but I wanted to play with it myself :).

You can see the demo here: https://www.youtube.com/watch?v=vgMczxau7VM and here https://www.youtube.com/watch?v=vgMczxau7VM.

It is also possible to generate video that results into some predefined image (verlet_video project): https://youtu.be/NFWb60gZgKY

# Building

```bash
git clone https://github.com/Sunday111/yae
git clone https://github.com/Sunday111/verlet
cd verlet
../yae/yae build
```

If `yae` is installed on `PATH`, run `yae build` from the project root instead. Machine-specific CMake overrides go in
an ignored `local-config.json` next to `yae_project.json`.

# verlet_video

`verlet_video` runs the simulation with every object coloured by where it will eventually come to rest, so the settled
pile reproduces a target image.

The picture is `--image`, and any PNG or JPEG will do:

```bash
yae run verlet_video -- --image ~/pictures/van.jpg --klvk-diagnostics video.json
```

It works out the colours itself: on startup it simulates `settle_frames` frames with no rendering, reads back where
each object ended up, samples the image there, then resets the solver and emitters and runs again with those colours.
Both passes run single threaded from the same state, so object *i* lands where the first pass said it would, and a run
is reproducible frame for frame.

| Option | Default | What it is |
| --- | --- | --- |
| `--preset` | `VerletAppPreset.json` next to the executable | Window size, object budget, and emitters. Written by the **Save Preset** button. |
| `--image` | `content/target_image.png` | The picture to reproduce. Sampled at each settled position. |
| `--positions` | simulate instead | Read settled positions from a dump written by the **Save positions** button rather than simulating them. |

Recording is klvk's, not this project's — pass a diagnostic configuration containing a `video` block:

```bash
yae run verlet_video -- --klvk-diagnostics video.json
```

```json
{
  "version": 1,
  "presentation": "offscreen",
  "framebuffer_size": [1920, 1080],
  "clock": {"mode": "fixed", "step_ns": 16666667},
  "video": {"path": "verlet.mp4", "encoding": "h264", "compression_level": 3, "include_ui": false},
  "exit": {"frame": 1800},
  "application": {"settle_frames": 1800}
}
```

Video requires `offscreen` presentation, a fixed clock, and **even** framebuffer dimensions — an odd width or height
fails at startup rather than producing a broken file. The clock step is the output frame rate, and `exit` decides how
long the recording runs. Encoding is `av1`, `h264`, or `mpeg4`, on `cpu` or `gpu`; see klvk's readme for the full set
of options.

`application.settle_frames` is how long the first pass simulates before reading positions back, so it is the frame the
picture appears on. It defaults to 3600 and is a separate decision from `exit.frame`, which is where the recording
stops: settling earlier than the end holds the finished picture on screen for the remaining frames, and the two are
equal only when the picture is meant to land on the very last one.

# World space

Screen size decides how much world there is. One world unit is `kPixelsPerWorldUnit` pixels, so an object covers the
same few pixels at every resolution and a bigger window simulates a bigger world rather than the same world drawn
larger.

Nothing in a preset is written in world units, so none of it has to be recomputed for a different resolution.
Emitters are placed in **relative coordinates**, where -1 and 1 are the edges of the world on each axis and the origin
is its centre: `{"X": 0.99, "Y": 0.4}` is a point just inside the right wall, forty percent of the way up. A relative
length — a radial emitter's radius — is measured against the shorter half of the world, so a ring stays a ring at any
aspect ratio.

Two things stay in world units on purpose, because they are distances between *objects* and objects are the same size
whatever the world: a flat emitter's `Spacing`, and `SpeedFactor`.

The object budget is stated one of two ways, and a preset carries exactly one of them:

| Key | Meaning |
| --- | --- |
| `MaxObjectsCount` | A literal number of objects. The same preset fills a small window and looks sparse in a large one. |
| `MaxObjectsSaturation` | A share of what the world holds, from 0 to 1, where 1 is objects packed as tightly as circles go. Means the same thing at any resolution. |

Saturation is converted to a count from the world's area whenever the world changes, so it follows a resize on its
own. The **Limit by saturation** checkbox switches between the two and carries the current budget across, and the one
in force is the one written back by **Save Preset**.

A worked example ships in `content/`, with its recording configuration beside it. `fill_2244x6864.json` is a tall
2244x6864 world lined with three flat emitters, one along each surface bounding the top 30%. They fill it to saturation
1.0 in 28 seconds, the picture is composed at 33 seconds, and the recording runs on to 38 seconds so the finished image
holds on screen.

```bash
yae run verlet_video -- \
    --klvk-diagnostics src/verlet_video/content/fill_2244x6864_video.json \
    --preset src/verlet_video/content/fill_2244x6864.json \
    --image src/verlet_video/content/target_image.png
```

A preset's `WindowSize` sizes the window an interactive session opens; a render takes its resolution from the
configuration's `framebuffer_size` instead. The two agree in the example, but they no longer have to: everything the
preset places is relative, so the same file renders at any resolution and describes the same picture.

# Emitters

`Radial` spawns outward from a ring, along a sector of it. `Position` is relative and `Radius` is relative to the
shorter half of the world; `Radius` and `SectorDegrees` together decide how many objects leave per tick, and
`PhaseDegrees` points the sector, measuring zero as straight up.

`Flat` spawns from a straight surface, all of it moving the same way. It is given as its two ends, `Start` and `End`,
both relative — which is how a surface says "the whole top edge" without naming a size. `DirectionDegrees` points the
emission with the same convention as a sector's phase, and the surface normally lies across it. `Spacing` is the
world-unit distance between neighbouring spawn points along the surface, so object diameter packs them solid and
larger values emit proportionally fewer.

A flat emitter fills a rectangle evenly, where a radial one is a point source that builds a cone and spreads only
through collisions.
