#!/bin/bash

err_exit() {
  echo $1 1>&2
  exit 1
}

if [ ! -z "$EMSDK" ]; then
  source $EMSDK/emsdk_env.sh
else
  source /opt/emsdk/emsdk_env.sh || err_exit "Can not setup EMSDK environment"
fi

SWD=$(cd $(dirname $0) && pwd)
cd $SWD

CC=$EMSDK/upstream/emscripten/node_modules/google-closure-compiler-linux/compiler
if [ ! -f "$CC" ]; then
  err_exit "Closure compiler is not included in the EMSDK"
fi

rm -rf dist || err_exit "Error deleting 'dist' folder"
mkdir -p dist || err_exit "Error recreating 'dist' folder"
echo "Bundling ..."
cd src/
rollup clsTargomanPdfViewer.mjs --file ../dist/bundle.js --format iife -n clsTargomanPdfViewer || err_exit "Error compiling the bundled js code"
# minify clsTargomanPdfViewer.css > ../dist/webpdfui.min.css || err_exit "Error minifying stylesheet"
cp clsTargomanPdfViewer.css ../dist/webpdfui.min.css
cd ..
echo "Compiling ..."
#$CC --js dist/bundle.js --js_output_file dist/webpdfui.min.js || err_exit "Error minifying bundled js code"
cp dist/bundle.js dist/webpdfui.min.js
echo "Copying worker ..."
cp ../out/bin/webpdf.* dist/
echo "Cleaning up ..."
rm -v dist/bundle.js || err_exit "Error removing uncompressed bundled js code"
echo "Done!"