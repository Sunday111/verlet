# verlet

Simple physics approximation using [Verlet Integration](https://en.wikipedia.org/wiki/Verlet_integration) approach.

Inspired by [this video](https://www.youtube.com/watch?v=lS_qeBy3aQI). Almost exactly the same but I wanted to play with it myself :).

You can see the demo here: https://www.youtube.com/watch?v=vgMczxau7VM

# for windows

```bash
git submodule update --init
python./deps/yae/scripts/make_project_files.py --project_dir=.
cmake -S . -B ./build
```
