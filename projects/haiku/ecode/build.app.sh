#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit
cd ../../../ || exit
premake4 --with-mojoal gmake
cd make/haiku || exit
make -j"$(nproc)" config=release ecode
cd "$DIRPATH" || exit
rm -rf ./ecode.app
mkdir -p ecode.app/assets
mkdir -p ecode.app/libs
cp ./AppRun ecode.app/ecode
cp ../../../bin/assets/icon/ee.png ecode.app/ecode.png
cp ../../../libs/haiku/libeepp.so ecode.app/libs/
cp ../../../bin/ecode ecode.app/ecode.bin
cp -L /boot/system/lib/libSDL2-2.0.so.0 ecode.app/libs/
strip ecode.app/libs/libSDL2-2.0.so.0
mkdir -p ecode.app/assets/colorschemes
mkdir -p ecode.app/assets/fonts
cp -r ../../../bin/assets/colorschemes ecode.app/assets/
cp -r ../../../bin/assets/fonts/DejaVuSansMono.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMonoNerdFontComplete.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/codicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DroidSansFallbackFull.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/plugins ecode.app/assets/
mkdir -p ecode.app/assets/icon/
cp -r ../../../bin/assets/icon/ee.png ecode.app/assets/icon/
mkdir ecode.app/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode.app/assets/ui/

VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')
ECODE_NAME=ecode-haiku-"$ECODE_MAJOR_VERSION"."$ECODE_MINOR_VERSION"."$ECODE_PATCH_LEVEL"-"$(uname -m)"

mv ecode.app ecode
tar -czf "$ECODE_NAME".tar.gz ecode
mv ecode ecode.app
