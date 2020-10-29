# libcbg

![image](https://i.loli.net/2020/09/06/gi4PxlFknpwZbYC.png)
A Ultra Fast Ethornell/BGI CompressedBG Image Format Codec.

Use multithreads huffman coding. All code write by C/C++. Ultra speed assurance.

Now this codec can encode and decode cbg1 image.(The CompressedBG Image Format have two version. include cbg1 and cbg2.)

I will not write cbg2 part. Because most base Ethornell/BGI Engine games only use cbg1. even if it use cbg2, You don't need to recompress the image. Just use a non-compress image also can change the game image.

Read this [PDF](https://github.com/copper187/libcbg/blob/master/libcbgDevelopmentDocumentsV3.pdf) to view full encoder APIs development documents.

(Not include decoder. Because you can find lots of great decoder project (such as GARbro by morkt --> https://github.com/morkt/GARbro/blob/master/ArcFormats/Ethornell/ImageCBG.cs). 

Of course you also can use my decoder if you like it. :)

