#ifndef CBGERROR_H
#define CBGERROR_H
#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>

using namespace std;
unsigned int check_cbg1_enc_default(int height, int width, int color_depth)
{
	if (height <= 0 || width <= 0)
	{
		cout << "libcbg: Error. Height or width are wrong. code:0x1." << endl;
		return 0x1;
	}
	if (color_depth != 32 && color_depth != 24)
	{
		cout << "libcbg: Error. Wrong or not support color depth. code:0x2." << endl;
		return 0x2;
	}
	return 0;
}
unsigned int check_cbg1_enc_advanced(int height, int width, int color_depth,char* encoder_information)
{
	if (height <= 0 || width <= 0)
	{
		cout << "libcbg: Error. Height or width are wrong. code:0x1." << endl;
		return 0x1;
	}
	if (color_depth != 32 && color_depth != 24)
	{
		cout << "libcbg: Error. Wrong or not support color depth. code:0x2." << endl;
		return 0x2;
	}
	if (encoder_information[8] != NULL)
	{
		cout << "libcbg: Warning. Encoder information is too long(Out of 8 bytes) code:0x101." << endl;
	}
	return 0;
}
#endif
