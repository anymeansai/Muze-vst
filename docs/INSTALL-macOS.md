# Installing Vocalyst on macOS

This zip contains universal (Apple Silicon + Intel) builds of **Vocalyst** and **Vocalyst Auto Key**:

- `VST3/Vocalyst.vst3`, `VST3/Vocalyst Auto Key.vst3`
- `AU/Vocalyst.component`, `AU/Vocalyst Auto Key.component` (Logic, GarageBand)
- `Standalone/Vocalyst.app`, `Standalone/Vocalyst Auto Key.app`

## 1. Copy the plugins

Copy the bundles into your user plugin folders (create the folders if they do not exist):

```
~/Library/Audio/Plug-Ins/VST3/
~/Library/Audio/Plug-Ins/Components/
```

In Finder: press Cmd+Shift+G, paste the path, drop the bundle in.

## 2. Remove the download quarantine (important)

These builds are signed ad-hoc, not notarized by Apple, so macOS blocks them after a browser download
("cannot be opened because the developer cannot be verified"). Clear the quarantine flag once:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Vocalyst.vst3
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Vocalyst Auto Key.vst3"
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Vocalyst.component
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/Components/Vocalyst Auto Key.component"
```

(Open Terminal, paste the lines, press Enter.)

## 3. Rescan plugins

- **Ableton Live**: Settings > Plug-Ins > Rescan (enable VST3 custom folder if needed).
- **Logic Pro / GarageBand**: restart Logic; if the AU does not show, run `auval -a` in Terminal or open Logic's Plug-in Manager and reset.
- **FL Studio**: Options > Manage plugins > Find more plugins.
- **Reaper**: Preferences > Plug-ins > VST > Re-scan.

## 4. Use it

1. Put **Vocalyst** on the vocal track, pick a preset in the PRESETS tab.
2. Put **Vocalyst Auto Key** on the beat or master bus, press SEND TO VOCALYST (or leave AUTO SEND on).
3. In Vocalyst press **AUTO KEY** > "Pull key from Vocalyst Auto Key", or detect the key from the vocal itself.
4. Press **ANALYZE**, play the vocal, apply the suggestions, then shape it with the SIMPLE macros or the ADVANCED chain.

Trouble? The Standalone app in this zip runs without a DAW (right-click > Open the first time).
