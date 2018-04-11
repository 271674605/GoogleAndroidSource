#!/bin/bash

# Populates glide maven repository with current snapshot from google3

GLIDE_VERSION='4.0.0-SNAPSHOT'
DISKLRUCACHE_VERSION='1.0.0-SNAPSHOT'
GIFDECODER_VERSION='1.0.0-SNAPSHOT'

TARGET_DIR=`pwd`

p4 g4d -f sync_glide_google3_android
pushd /google/src/cloud/$USER/sync_glide_google3_android/google3 >> /dev/null

SYNCED_CL=`g4 sync | grep @ | sed s/.*@//`

blaze build \
   third_party/java_src/android_libs/glide/library/src/main:libglide.jar \
   third_party/java_src/android_libs/glide/library/src/main:libglide-src.jar \
   third_party/java_src/android_libs/glide/third_party/disklrucache:libdisklrucache.jar \
   third_party/java_src/android_libs/glide/third_party/disklrucache:libdisklrucache-src.jar \
   third_party/java_src/android_libs/glide/third_party/gif_decoder:libgif_decoder.jar \
   third_party/java_src/android_libs/glide/third_party/gif_decoder:libgif_decoder-src.jar

cp -f blaze-bin/third_party/java_src/android_libs/glide/library/src/main/libglide.jar \
   $TARGET_DIR/com/github/bumptech/glide/glide/$GLIDE_VERSION/glide-$GLIDE_VERSION.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/library/src/main/libglide-src.jar \
   $TARGET_DIR/com/github/bumptech/glide/glide/$GLIDE_VERSION/glide-$GLIDE_VERSION-sources.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/third_party/disklrucache/libdisklrucache.jar \
   $TARGET_DIR/com/github/bumptech/glide/disklrucache/$DISKLRUCACHE_VERSION/disklrucache-$DISKLRUCACHE_VERSION.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/third_party/disklrucache/libdisklrucache-src.jar \
   $TARGET_DIR/com/github/bumptech/glide/disklrucache/$DISKLRUCACHE_VERSION/disklrucache-$DISKLRUCACHE_VERSION-sources.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/third_party/gif_decoder/libgif_decoder.jar \
   $TARGET_DIR/com/github/bumptech/glide/gifdecoder/$GIFDECODER_VERSION/gifdecoder-$GIFDECODER_VERSION.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/third_party/gif_decoder/libgif_decoder-src.jar \
   $TARGET_DIR/com/github/bumptech/glide/gifdecoder/$GIFDECODER_VERSION/gifdecoder-$GIFDECODER_VERSION-sources.jar

echo "This maven repository was synced to google3 CL $SYNCED_CL" > $TARGET_DIR/README.txt
popd
