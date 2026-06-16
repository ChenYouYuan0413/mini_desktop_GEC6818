# mini_desktop — GEC6818 Desktop System

800×480 framebuffer + touch desktop with photo album and IR remote control.

## Architecture

```
desktop → album     (swipe photo browser)
        → ir_app    (learn / send / receive IR codes)
```

## Non-blocking I/O pattern

All I/O uses `O_NONBLOCK` + drain-events-into-cache. Never block on `read()`.

Touch events are drained per-frame via `ts_read()` — after the `while(ts_read > 0)`
loop exits (no more events), remaining logic (serial polling, etc.) runs once,
then the outer loop repeats. This keeps the UI responsive to both touch and serial.

### Do NOT let `ts_read()` return 1 with stale data

`ts_read()` uses a `static int has_new` flag. It MUST be reset to `0` at the top
of every call. If you forget this, `has_new` sticks at `1` after the first touch,
every `ts_read()` call returns 1 forever, and `while (ts_read(...) > 0)` loops
spin endlessly — **code after the loop is never reached**. This bit us when
`page_learn()` could never read from the IR serial port.

## Build

```bash
docker run --platform linux/amd64 --rm \
  -v /path/to/toolchain:/opt/gec6818-toolchain:ro \
  -v /path/to/mini_desktop:/work -w /work \
  gec6818-dev:ubuntu18 bash -c 'make static'
```

## Deploy

```bash
ssh root@192.168.1.88 'killall mini_desktop 2>/dev/null; sleep 1'
scp -O mini_desktop root@192.168.1.88:/home/cyy/
```
