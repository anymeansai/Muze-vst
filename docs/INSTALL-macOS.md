# Installing Vocalyst on macOS

## The easy way: the installer

1. On the Releases page, download **Vocalyst-x.y.z-macOS-Installer.pkg**.
2. Double-click it. The installer offers three parts you can tick or untick:
   - **VST3 plug-ins** (Ableton Live, FL Studio, Reaper, Cubase, Studio One, Bitwig ...) -> `/Library/Audio/Plug-Ins/VST3`
   - **Audio Unit plug-ins** (Logic Pro, GarageBand) -> `/Library/Audio/Plug-Ins/Components`
   - **Standalone apps** -> `/Applications`
3. Enter your Mac password when asked. At the end it refreshes Logic's plug-in list and asks whether to move the
   downloaded installer to the Trash.
4. Open your DAW and rescan plug-ins if needed (Logic Pro: just restart it).

### "Apple could not verify ..." when opening the installer

The installer is not notarized by Apple (that needs a paid Apple Developer account), so macOS blocks it once.
This is the only hurdle:

- **macOS 15 Sequoia or newer**: double-click the .pkg, click **Done** on the warning, then open
  **System Settings > Privacy & Security**, scroll down to the message about the installer and click **Open Anyway**.
  Double-click the .pkg again.
- **macOS 13 / 14**: right-click (or Control-click) the .pkg, choose **Open**, then click **Open** in the dialog.

Plug-ins placed by the installer are not quarantined, so your DAW loads them without further warnings.

## The manual way: the zip

`Vocalyst-macOS.zip` contains the same bundles. Copy them yourself into:

```
~/Library/Audio/Plug-Ins/VST3/
~/Library/Audio/Plug-Ins/Components/
```

Because they came from a browser download, clear the quarantine flag once in Terminal, otherwise the DAW refuses them:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Vocalyst.vst3
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Vocalyst Auto Key.vst3"
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Vocalyst.component
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/Components/Vocalyst Auto Key.component"
```

## Using it

1. Put **Vocalyst** on the vocal track, pick a preset in the PRESETS tab.
2. Put **Vocalyst Auto Key** on the beat or master bus (AUTO SEND is on by default).
3. In Vocalyst press **AUTO KEY** > "Pull key from Vocalyst Auto Key", or detect the key from the vocal itself.
4. Press **ANALYZE**, play the vocal, apply the suggestions, then shape it with the SIMPLE macros or the ADVANCED chain.
