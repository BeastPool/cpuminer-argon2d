This file is included in the Windows binary package. Compile instructions
for Linux and Windows can be found in RELEASE_NOTES.

cpuminer is a console program that is executed from a DOS command prompt.
There is no GUI and no mouse support.

Choose the exe that best matches you CPU's features or use trial and
error to find the fastest one that doesn't crash. Pay attention to
the features listed at cpuminer startup to ensure you are mining at
optimum speed using all the available features.

Architecture names and compile options used are only provided for Intel
Core series. Pentium and Celeron often have fewer features.

AMD CPUs older than Piledriver, including Athlon x2 and Phenom II x4, are not
supported by cpuminer-opt due to an incompatible implementation of SSE2 on
these CPUs. Some algos may crash the miner with an invalid instruction.
Users are recommended to use an unoptimized miner such as cpuminer-multi.

Exe name                Compile flags              Arch name

cpuminer-sse2.exe      "-march=core2"              Core2, Nehalem   
cpuminer-aes-sse42.exe "-maes -msse4.2"            Westmere
cpuminer-aes-avx.exe   "-march=corei7-avx"         Sandybridge, Ivybridge
cpuminer-avx2.exe      "-march=core-avx2"          Haswell...
cpuminer-avx2-sha.exe  "-march=core-avx2 -msha"    Ryzen

If you like this software feel free to donate:

BTC: 12tdvfF7KmAsihBXQXynT6E6th2c2pByTT
ETH: 0x72122edabcae9d3f57eab0729305a425f6fef6d0
LTC: LdUwoHJnux9r9EKqFWNvAi45kQompHk6e8
BCH: 1QKYkB6atn4P7RFozyziAXLEnurwnUM1cQ
BTG: GVUyECtRHeC5D58z9F3nGGfVQndwnsPnHQ


