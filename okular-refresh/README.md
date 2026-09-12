# Actual Okular automatic reload regression

The real Okular Part and PDF generator automatically reloaded deleted-and-recreated PDFs in this Linux test. No production Okular code changed; no manual reload or injected watcher signal was used.

`auto-reload-regression.patch` adds a data-driven test to `autotests/parttest.cpp` at [Okular](https://invent.kde.org/graphics/okular) commit `b9a5786fc939330808b7aa8e795dc31c4cf9dd1e`. It follows that file's GPL-2.0-or-later license. Astra investigated, Terra implemented the regression, and Codex reviewed, built and executed it.

The test enables file watching on a temporary one-page PDF, then deletes and recreates it three times, alternating 40, 1 and 40 pages. The rapid row recreates before processing events; the delayed row waits 1,100 ms with the event loop running and checks the old document remains available. Each replacement must load the expected page count within 10 seconds, retain the URL and leave the document open.

| Execution | Rapid | Delayed | Log |
| --- | --- | --- | --- |
| Default Linux watcher | Pass | Pass | [default-watcher.txt](default-watcher.txt) |
| Requested QFileSystemWatcher backend | Pass | Pass | [qfswatch.txt](qfswatch.txt) |
| QFileSystemWatcher diagnostic repeat | Pass | Pass | [qfswatch-debug.txt](qfswatch-debug.txt) |

Each execution completed six automatic replacements. QTest reports four passes: two rows plus initialization and cleanup. The diagnostic repeat verifies `preferred= QFSWatch` and records deletion/creation notifications.

Environment: Ubuntu 26.04.1 arm64, Qt 6.10.2, KCoreAddons 6.24.0, Poppler Qt6 26.01.0. Ubuntu image digest: `sha256:513c074113a871b51a8d16ab445c88779d6452d937a164fb5cc479f32668a41d`. A focused, uninstalled Debug build emitted missing drawing-tool XML warnings; annotation controls were outside this test.

## Reproduce

In an Ubuntu 26.04 container, enable `deb-src`, run `apt-get update`, then install `apt-get build-dep -y --no-install-recommends okular` and `apt-get install -y --no-install-recommends ninja-build xvfb xauth dbus-x11`. Check out the pinned revision at `/src`, apply the patch, and run:

```sh
cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DCMAKE_INSTALL_PREFIX=/opt/okular-test
cmake --build /build --target parttest okularGenerator_poppler --parallel 3
LANG=C.UTF-8 QT_PLUGIN_PATH=/build/bin dbus-run-session -- xvfb-run -a /build/bin/parttest testAutoReloadAfterDeleteAndRecreate
LANG=C.UTF-8 QT_PLUGIN_PATH=/build/bin KDIRWATCH_METHOD=QFSWatch dbus-run-session -- xvfb-run -a /build/bin/parttest testAutoReloadAfterDeleteAndRecreate
```

This proves application reload behavior on Linux. Separate Windows GooFile tests verify the sharing candidate. A combined Windows 11 Okular build, sponsor acceptance and earned bounty remain unverified.
