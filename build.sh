sed -i 's/"-lpthreadGC2"/"-lpthread"/g' configure.ac

mkdir release_linux

make distclean || echo clean
rm -f config.status
./autogen.sh || echo done
CFLAGS="-O3 -march=core-avx2 -Wall" ./configure --with-curl
make -j8
strip -s cpuminer
mv cpuminer release_linux/cpuminer-avx2

make clean || echo clean
rm -f config.status
CFLAGS="-O3 -march=core-avx2 -mavx512f -fno-asynchronous-unwind-tables -Wall" ./configure --with-curl
make -j8
strip -s cpuminer
mv cpuminer release_linux/cpuminer-avx512f

make clean || echo clean
rm -f config.status
CFLAGS="-O3 -march=corei7-avx -Wall" ./configure --with-curl
make -j8
strip -s cpuminer
mv cpuminer release_linux/cpuminer-aes-avx

make clean || echo clean
rm -f config.status
CFLAGS="-O3 -march=core2 -Wall" ./configure --with-curl
make -j8
strip -s cpuminer
mv cpuminer release_linux/cpuminer-sse2
make distclean || echo clean



