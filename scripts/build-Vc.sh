#!/bin/bash -e
set -x
set -e

if [[ -z ${AutoTuneTMP_source_me_sourced} ]]; then
    source source-me.sh
fi

if [ ! -d "Vc/" ]; then
    git clone https://github.com/STEllAR-GROUP/Vc.git
    cd Vc
    # git checkout pfandedd_inlining_AVX512
    # git checkout HEAD~1
    git checkout gcc8_fixes
    cd ..
fi
# else
#     cd Vc
#     git pull
#     cd ..
# fi

mkdir -p Vc/build
cd Vc/build
echo "building Vc"
#  > cmake_Vc.log 2>&1
cmake -DCMAKE_INSTALL_PREFIX="$Vc_ROOT" -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=release ../
#  > make_install_Vc.log 2>&1
make -j${PARALLEL_BUILD} VERBOSE=1 install
cd ../..
