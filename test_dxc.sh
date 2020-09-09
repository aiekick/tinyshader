#!/bin/sh

pushd ../examples

rm -f vert_dxc.spv
rm -f frag_dxc.spv

dxc test.hlsl -Od -E vertex -T vs_6_6 -Fo vert_dxc.spv -spirv  &&\
dxc test.hlsl -Od -E pixel -T ps_6_6 -Fo frag_dxc.spv -spirv &&\
spirv-dis vert_dxc.spv &&\
spirv-dis frag_dxc.spv

popd