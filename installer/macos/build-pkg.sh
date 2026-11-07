#!/bin/bash
# Builds the macOS installer package for Vocalyst + Vocalyst Auto Key.
#
#   installer/macos/build-pkg.sh <build-dir> <output-dir> [version]
#
# Produces <output-dir>/Vocalyst-<version>-macOS-Installer.pkg with three selectable parts
# (VST3, Audio Unit, Standalone apps) plus a final step that refreshes the AU registry and
# offers to move the downloaded installer to the Trash.
set -euo pipefail

BUILD_DIR="${1:?usage: build-pkg.sh <build-dir> <output-dir> [version]}"
OUT_DIR="${2:?usage: build-pkg.sh <build-dir> <output-dir> [version]}"
VERSION="${3:-}"

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
if [ -z "$VERSION" ]; then
    VERSION=$(sed -nE 's/^project\(Vocalyst VERSION ([0-9.]+).*/\1/p' "$REPO/CMakeLists.txt")
fi
[ -n "$VERSION" ] || { echo "could not determine version" >&2; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/root/vst3" "$WORK/root/au" "$WORK/root/apps" "$WORK/pkgs" "$WORK/resources" "$OUT_DIR"

copy_bundle() {
    local src="$1" dst="$2"
    [ -d "$src" ] || { echo "missing bundle: $src" >&2; exit 1; }
    ditto "$src" "$dst/$(basename "$src")"
}
copy_bundle "$BUILD_DIR/Vocalyst_artefacts/Release/VST3/Vocalyst.vst3"                     "$WORK/root/vst3"
copy_bundle "$BUILD_DIR/VocalystAutoKey_artefacts/Release/VST3/Vocalyst Auto Key.vst3"     "$WORK/root/vst3"
copy_bundle "$BUILD_DIR/Vocalyst_artefacts/Release/AU/Vocalyst.component"                  "$WORK/root/au"
copy_bundle "$BUILD_DIR/VocalystAutoKey_artefacts/Release/AU/Vocalyst Auto Key.component"  "$WORK/root/au"
copy_bundle "$BUILD_DIR/Vocalyst_artefacts/Release/Standalone/Vocalyst.app"                "$WORK/root/apps"
copy_bundle "$BUILD_DIR/VocalystAutoKey_artefacts/Release/Standalone/Vocalyst Auto Key.app" "$WORK/root/apps"

# One component package per install location. Bundles are marked non-relocatable so the
# installer always writes to the chosen folder instead of "updating" a stray copy elsewhere.
component_pkg() {
    local name="$1" root="$2" location="$3"
    local plist="$WORK/$name.plist"
    pkgbuild --analyze --root "$root" "$plist" > /dev/null
    local count
    count=$(/usr/libexec/PlistBuddy -c "Print" "$plist" | grep -c "BundleIsRelocatable" || true)
    local i
    for ((i = 0; i < count; i++)); do
        /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$plist"
    done
    pkgbuild --root "$root" --component-plist "$plist" \
             --identifier "ai.anymeans.vocalyst.$name" --version "$VERSION" \
             --install-location "$location" "$WORK/pkgs/$name.pkg"
}
component_pkg vst3 "$WORK/root/vst3" "/Library/Audio/Plug-Ins/VST3"
component_pkg au   "$WORK/root/au"   "/Library/Audio/Plug-Ins/Components"
component_pkg apps "$WORK/root/apps" "/Applications"

chmod +x "$HERE/scripts/postinstall"
pkgbuild --nopayload --scripts "$HERE/scripts" \
         --identifier "ai.anymeans.vocalyst.finish" --version "$VERSION" "$WORK/pkgs/finish.pkg"

cp "$HERE/resources/"*.html "$WORK/resources/"
cp "$REPO/LICENSE" "$WORK/resources/license.txt"
sed "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"

PKG="$OUT_DIR/Vocalyst-$VERSION-macOS-Installer.pkg"
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK/pkgs" \
             --resources "$WORK/resources" --version "$VERSION" "$PKG"
echo "built $PKG"
