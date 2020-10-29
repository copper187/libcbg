# libcbg

![image](https://i.loli.net/2020/09/06/gi4PxlFknpwZbYC.png)
A Ultra Fast Ethornell/BGI CompressedBG Image Format Codec.

Use multithreads huffman coding. All code write by C/C++. Ultra speed assurance.

Now this codec can encode and decode cbg1 image.(The CompressedBG Image Format have two version. include cbg1 and cbg2.)

I will not write cbg2 part. Because most base Ethornell/BGI Engine games only use cbg1. even if it use cbg2, You don't need to recompress the image. Just use a non-compress image also can change the game image.

Here are full encoder APIs guide.
(Not include decoder. Because you can find lots of great decoder project (such as GARbro by morkt --> https://github.com/morkt/GARbro/blob/master/ArcFormats/Ethornell/ImageCBG.cs). 

Of course you also can use my decoder if you like it. :)

cbg_codec::api* cbg1_enc_ default

( 

int height,

int width,

int color_depth,

unsigned char/BYTE *raw_pixel_buffer

)

Explanation:

Encode to cbg image format by pixels in raw pixel buffer and use height, width and depth you pass. Write the cbg bit stream (include “CompressedBG___” magic bytes and all file header information.) in a bitstream buffer.

libcbg will return a “cbg_codec::api*” type struct pointer. Change to P15 to read more about this struct.

libcbg will use default key (0x31676263 or “cbg1”) and write default encoder information (“bylibcbg”).

libcbg will use default huffman coding settings(multithreads).

Error codes:

0x1: Height or width are wrong. (negative or zero).

Tips: check height and width and pass a correct num.

0x2: Wrong or not support color depth.

Tips: check color depth and pass a correct num.


cbg_codec::api* cbg1_enc_advanced
( int height,
int width,
int color_depth,
  unsigned int/DWORD key,
  char *encoder_information,
  bool huffman_coding_settings,
  unsigned char/BYTE *raw_pixel_buffer )
Explanation:
	Encode to cbg image format by pixels in raw pixel buffer and use height, width, depth and key you pass. Write the cbg bit stream(include “CompressedBG___” magic bytes and all file header information.) in a bitstream buffer.
	libcbg will return a “cbg_codec::api*” type struct pointer. Change to P15 to read more about this struct.	
	libcbg will write encoder information you pass.
	libcbg will use singlethread if the huffman coding settings is false,
and will use multithreads if it is true.
Tips: libcbg will not use less memory if you use singlethread coding. Maybe support in next version? (maybe).
Error codes:
	0x1: Height or width are wrong.(negative or zero).
Tips: check height and width and pass a correct num.
	0x1: Wrong or not support color depth.
Tips: check color depth and pass a correct num.
Warning codes:
	0x101: Encoder information is too long(Out of 8 bytes).
Tips: libcbg will only be use first 8 bytes character.
















int cbg1_encToFile_ default
( int height,
int width,
int color_depth,
unsigned char/BYTE *raw_pixel_buffer,
char *filename)
Explanation:
	Encode to cbg image format by pixels in raw pixel buffer and use height, width and depth you pass. Write the cbg bit stream (include “CompressedBG___” magic bytes and all file header information.) in bit stream buffer. Write the bit stream in a file you designated and only return a successed code(0x0). 
	libcbg will use default key (0x31676263 or “cbg1”) and write default encoder information (“bylibcbg”).
	libcbg will use default huffman coding settings(multithreads).
Error codes:
	0x1: Height or width are wrong.(negative or zero).
Tips: check height and width and pass a correct num.
	0x2: Wrong or not support color depth.
Tips: check color depth and pass a correct num.


int cbg1_encToFile_advanced
( int height,
int width,
int color_depth,
  unsigned int/DWORD key,
  char *encoder_information,
  bool huffman_coding_settings,
  unsigned char/BYTE *raw_pixel_buffer,
char *filename)
Explanation:
	Encode to cbg image format by pixels in raw pixel buffer and use height, width, depth and key you pass. Write the cbg bit stream(include “CompressedBG___” magic bytes and all file header information.) in bit stream buffer. Write the bit stream in a file you designated and only return a successed code(0x0). 
	libcbg will write encoder information you pass.
	libcbg will use singlethread if the huffman coding settings is false,
and will use multithreads if it is true.
Tips: libcbg will not use less memory if you use singlethread coding. Maybe support in next version? (maybe).
Error codes:
	0x1: Height or width are wrong.(negative or zero).
Tips: check height and width and pass a correct num.
	0x2: Wrong or not support color depth.
Tips: check color depth and pass a correct num.
Warning codes:
	0x101: Encoder information is too long(Out of 8 bytes).
Tips: libcbg will only be use first 8 bytes character.
















cbg_codec::api* cbg1_get_trans_pixel 
( int height,
int width,
int color_depth,
unsigned char/BYTE *raw_pixel_buffer)
Explanation:
	Transform the pixels from raw pixel buffer to cbg format used pixels. Use height, width and depth you pass. Write the transformed pixel stream in a bitstream buffer.
	libcbg will return a “cbg_codec::api*” type struct pointer. Change to P15 to read more about this struct.	
Error codes:
	0x1: Height or width are wrong.(negative or zero).
Tips: check height and width and pass a correct num.
	0x2: Wrong or not support color depth.
Tips: check color depth and pass a correct num.






cbg_codec::api* cbg1_get_huffman_stream
( int height,
int width,
int color_depth,
unsigned char/BYTE *raw_pixel_buffer)
Explanation:
	Encode the pixels to cbg format style huffman coding(cbg format is using a special huffman tree).Use height, width and depth you pass. Write the huffman coding bit stream in a bitstream buffer.
	libcbg will return a “cbg_codec::api*” type struct pointer. Change to P15 to read more about this struct.	
	libcbg will use default huffman coding settings(multithreads).
Error codes:
	0x1: Height or width are wrong. (negative or zero).
Tips: check height and width and pass a correct num.
	0x2: Wrong or not support color depth.
Tips: check color depth and pass a correct num.



cbg_codec::api*
{
		unsigned long long buffersize;
		unsigned char buffer[];
}
	unsigned long long buffersize;
The size of this buffer.
	unsigned char buffer[];
The binary stream buffer pointer. Size of this buffer is recorded in “unsigned long long buffersize”.
Example:
● call the function to encode…

cbg_codec::api* data = cbg1_enc_default(…)

● Get the data…

unsigned long long size;
unsigned char stream[];
/*some other codes…*/
size = data->buffersize;
stream = data->buffer;
