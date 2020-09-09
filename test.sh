#!/bin/sh

pushd ../examples

rm -f vert.spv
rm -f frag.spv

cmake --build ../build

../build/tsc -E vertex -T vertex -o vert.spv test.hlsl &&\
../build/tsc -E pixel -T fragment -o frag.spv test.hlsl &&\
	spirv-dis vert.spv &&\
	spirv-val vert.spv &&\
	spirv-dis frag.spv &&\
	spirv-val frag.spv
# od --width=4 -Ad -H a.spv

popd