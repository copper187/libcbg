# libcbg

![image](https://i.loli.net/2020/09/06/gi4PxlFknpwZbYC.png)

## Version 3.x
### V3.1 Update --- Incoming
I have confirmed we can use gf2p8affineqb to improve the int8 shifting. It can use in pixel transform. I have implemented a function prototype and confirmed a good speedup(150ms -> 45ms).

The code will be updated soon. 

The gf2p8affineqb instruction, In GFNI instructions set. Support since IceLake CPUs (3rd Gen Intel Xeon Scalable processors). Although is introduced alongside avx512, It is still supported on newer CPUs such as AlderLake and later processors(12th/13th/14th/Ultra100/Ultra200 Intel core) that do not support avx512. It just does not support 512-bit width and EVEX encoding.

For AMD CPUs, Support since Zen4.

I will also add an APX version to further optimize the use of GPRs.

Very thanks for these articles!

https://wunkolo.github.io/post/2020/11/gf2p8affineqb-int8-shifting/

https://wunkolo.github.io/post/2020/11/gf2p8affineqb-bit-reversal/

and thanks for icx and clang compiler for specific optimization ideas.
### V3.0.0 Update --- 2022.05.30

* AVX2 and AVX512 assembly optimize.
* New method for transform pixels.

Special notices for AVX512

You need AVX512(F, VL, DQ, BW)

Cannot run on Xeon Phi processors (Knights Landing / Knights Mill, etc).

Test passed on 11th Core Engineering Sample chip (Rocket Lake-S. Revision A0). 

## Version 2.x

### V2.0.1.0 Update --- 2021.02.17

* Fixed lots of memory leak bugs. Now this encoder can uses 75% or more less memory than the 1.x encoder.

* Fixed some other bugs.

<br>
A Ultra Fast Ethornell/BGI CompressedBG Image Format Codec. Licensed under GNU LGPL v3 license.

Use multithreads huffman coding. All code write by C/C++. Ultra speed assurance.

Now this codec can encode and decode cbg1 image.(The CompressedBG Image Format have two version. include cbg1 and cbg2.)

~~I will not write cbg2 part. Because most base Ethornell/BGI Engine games only use cbg1. even if it use cbg2, You don't need to recompress the image. Just use a non-compress image also can change the game image.~~

The cbg2 compression rate has a greatly improved with cbg1. And since 2012 or 2013, Most of Ethornell/BGI Engine were compatible cbg1/cbg2 image format. So I decide write cbg2 part.

Read this [PDF](https://github.com/copper187/libcbg/blob/master/libcbgDevelopmentDocumentsV4.pdf) to view full encoder APIs development documents.

(Not include decoder. Because you can find lots of great decoder project (such as GARbro by morkt --> https://github.com/morkt/GARbro/blob/master/ArcFormats/Ethornell/ImageCBG.cs). 

Of course you also can use my decoder if you like it. :)

