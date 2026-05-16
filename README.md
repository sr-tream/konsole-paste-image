# konsole-paste-image

A Konsole plugin that turns `Ctrl+V` into a smart paste:

- **Clipboard holds an image** → the PNG is written to
  `/tmp/konsole-clip-<sha256[:16]>.png` (deduped by content hash) and the
  path is sent into the active session.
- **Clipboard holds text** → the session controller's normal `edit_paste`
  action runs (multiline confirm, bracketed paste, etc. all preserved).

When the running program enabled DEC mode 2004 (bracketed paste), the path
is wrapped in the `\e[200~ … \e[201~` markers, so REPLs like Claude Code
that auto-attach pasted image paths see a paste event rather than typed
input.

A second action, **Send Ctrl+V to Terminal** (default `Ctrl+Shift+V`),
writes ASCII `SYN` (0x16) into the pty — keeping vim visual-block, shell
quoted-insert, etc. reachable now that the literal `Ctrl+V` chord is owned
by the plugin. Konsole's built-in paste remains on `Shift+Insert`.

Both actions appear under **Settings → Configure Keyboard Shortcuts** and
can be rebound.

## Install (Arch / CachyOS)

```sh
git clone https://github.com/sr-tream/konsole-paste-image.git
cd konsole-paste-image
makepkg -si
```

The `PKGBUILD`'s `pkgver()` pins the package to the Konsole release
currently in `[extra]`, because Konsole's `PluginManager` rejects plugins
whose `Version` metadata doesn't match its own `RELEASE_SERVICE_VERSION`.
Rebuild after every `pacman -Syu` that bumps Konsole.

## Install (manual)

```sh
git clone https://github.com/sr-tream/konsole-paste-image.git
cd konsole-paste-image
./build.sh                          # clones matching konsole source, builds
pkexec cmake --install build        # installs the .so under /usr/lib/qt6/plugins/konsoleplugins
```

Quit every Konsole window (the running process won't pick up new
plugins) and reopen.

### Build dependencies

`cmake`, `extra-cmake-modules`, `git`, `qt6-base`, `kcoreaddons`, `ki18n`,
`kxmlgui`, plus the installed `konsole` itself (for the `libkonsoleapp` /
`libkonsoleprivate` runtimes the plugin links against).

## Why an out-of-tree CMake build

Konsole's plugin headers (`IKonsolePlugin.h`, `MainWindow.h`, …) aren't
shipped by distro packages. `build.sh` and the `PKGBUILD`'s `build()`
both clone the Konsole source at the matching release tag, point CMake at
it via `-DKONSOLE_SOURCE_DIR=`, and link against the installed
`libkonsoleapp.so.*` / `libkonsoleprivate.so.*` (CMake stubs the missing
generated `*_export.h` / `config-konsole.h`).

## License

MIT — see [LICENSE](LICENSE).
