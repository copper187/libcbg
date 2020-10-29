#ifndef CBG_H
#define CBG_H

#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <queue>
#include <vector>
#include <thread>
#include "cbgError.h"
#include "cbgFile.h"

using namespace std;

namespace
{
	struct cbgheader
	{
		unsigned short int width;
		unsigned short int height;
		unsigned int color_depth;
		unsigned int not_use_8byte_a;
		unsigned int not_use_8byte_b;
		unsigned int zero_comprlen;
		unsigned int zero_uncomprlen;
		unsigned int key;
		unsigned int encrypt_length;
		unsigned char sum_check;
		unsigned char xor_check;
		unsigned short int cbg_version;
		unsigned long long int filesize;
		unsigned long long int huffman_comprlen;
	};
	struct huffman_nodes
	{
		int Lchild = -1;
		int Rchild = -1;
		int parents = -1;
		int weight = 0x7fffffff;
		bool L_or_R_child = 0;
	};
	cbgheader cbgh;
	huffman_nodes node[511];
	static long long each_byte_num[256];
	static int root_location;
	static bool* buffer_pointer[3];
	static long long int buffer_size[3];
	static bool coding_complete[3] = { 0,0,0 };
	static long long num[3];
	static unsigned char decrypt_check_tag[2] = { 0,0 };
	static unsigned int decrypt_check_present_key = 0;
	static unsigned char check_sum_result = 0;
	static unsigned char check_xor_result = 0;
	static char encoder_information[8];
	static unsigned int encrypt_key;
	static bool mthread;
	static bool only_get_trans_pixels = false;
	static bool only_get_huffman_coding = false;
}

queue<unsigned char> q; //q is buffer for not zero bytes.
queue<unsigned char> q2;//q2 is buffer for compressed zero bytes data.
vector<unsigned char> v;//v is buffer for compressed zero bytes data.For calc each byte nums and huffman coding.
queue<unsigned char> q3;//q3 is buffer for encrypted huffman leaf nodes weight. 
//Why I don't use new? Because I have no way to know the not zero bytes size and compressed zero bytes data size. 
//So I cannot allocate static size memory.I need a dynamic memory container such as C++ STL queue and vector.

void coding_thread(int ID, long long int start, long long int end)
{
	int coding_cur = 0;
	int root_node = root_location;
	//v2 is huffman coding data buffer.
	bool* v2 = new bool[(cbgh.zero_comprlen / 2) * 8];
	//But why I use new to allocate huffman coding data buffer? Because I use multithreads to huffman coding.If I use vector,I will be can't cross border to write memory.
	//For more, You should read more information about C++ STL vector.
	for (long long int i = start; i < end; i++)
	{
		unsigned char present_byte = v[i];
		unsigned int present_node = v[i];
		unsigned int start_cur = coding_cur;
		for (;;)
		{
			if (present_node == root_node)
			{
				bool* temp_huffman_buffer = new bool[coding_cur - start_cur + 1];
				for (int k = start_cur, j = 0; k < coding_cur, j < coding_cur - start_cur; k++, j++)
				{
					temp_huffman_buffer[j] = v2[k];
				}
				for (int k = coding_cur - 1, j = 0; k >= start_cur, j < coding_cur - start_cur; k--, j++)
				{
					v2[k] = temp_huffman_buffer[j];
				}
				delete[] temp_huffman_buffer;
				num[ID - 1]++;
				break;
			}
			if (node[present_node].L_or_R_child == 0)
			{
				v2[coding_cur] = 0;
				coding_cur++;
				present_node = node[present_node].parents;
			}
			else
			{
				v2[coding_cur] = 1;
				coding_cur++;
				present_node = node[present_node].parents;
			}
		}
	}
	buffer_pointer[ID - 1] = v2;
	buffer_size[ID - 1] = coding_cur;
	coding_complete[ID - 1] = 1;
	return;
}

class cbg_codec
{
public:
	struct api
	{
		unsigned long long buffersize;
		unsigned char buffer[];
	};
	api *encoder(unsigned char* raw_pixel_buffer)
	{
		return encoder2(raw_pixel_buffer);
	}
private:
	api *encoder2(unsigned char* raw_pixel_buffer)
	{
		unsigned char* transed_pixel_buffer = new unsigned char[cbgh.height * cbgh.width * (cbgh.color_depth / 8)];
		trans_pixels(raw_pixel_buffer, transed_pixel_buffer);
		if (only_get_trans_pixels == true)
		{
			unsigned char* bitstream_buffer = new unsigned char[cbgh.height * cbgh.width * (cbgh.color_depth / 8)+8];
			unsigned long long buffersize = cbgh.height * cbgh.width * (cbgh.color_depth / 8);
			api* data;
			memcpy(bitstream_buffer, &buffersize, 8);
			memcpy(bitstream_buffer+8, transed_pixel_buffer, buffersize);
			delete[] raw_pixel_buffer;
			delete[] transed_pixel_buffer;
			data = (api*)bitstream_buffer;
			return data;
		}
		delete[] raw_pixel_buffer;
		compress_zero_bytes(transed_pixel_buffer);
		cbgh.zero_comprlen = q2.size();
		while (!q2.empty())
		{
			char c;
			c = q2.front();
			v.push_back(q2.front());
			q2.pop();
		}
		build_huffman_tree();
		unsigned char* v3 = encode_huffman_coding();
		if (only_get_huffman_coding == true)
		{
			unsigned char* bitstream_buffer = new unsigned char[cbgh.huffman_comprlen+8];
			unsigned long long buffersize = cbgh.huffman_comprlen;
			api* data;
			memcpy(bitstream_buffer, &buffersize, 8);
			memcpy(bitstream_buffer+8, v3, buffersize);
			delete[] v3;
			data = (api*)bitstream_buffer;
			return data;
		}
		shift7_leaf_nodes();
		int y = q3.size();
		unsigned char* encrypt_buffer = encrypt_leaf_nodes();
		calc_xor_and_sum(encrypt_buffer, y);
		api* bitstream_buffer = build_buffer(encrypt_buffer, v3, y);
		return bitstream_buffer;
	}
	api* build_buffer(unsigned char* encrypt_buffer, unsigned char* v3, unsigned int y)
	{
		unsigned char *bitstream_buffer = new unsigned char[0x30 + y + cbgh.huffman_comprlen+8];
		unsigned long long buffersize = 0x30 + y + cbgh.huffman_comprlen;
		//data->buffersize=buffersize;
		api* data;
		memcpy(bitstream_buffer, &buffersize, 8);
		char fileheader[] = "CompressedBG___"; //CompressedBG___
		memcpy(bitstream_buffer + 8, fileheader, 15);
		bitstream_buffer[15 + 8] = 0;
		memcpy(bitstream_buffer + 16 + 8, &cbgh.width, 2);
		memcpy(bitstream_buffer + 18 + 8, &cbgh.height, 2);
		bitstream_buffer[20 + 8] = (unsigned char)cbgh.color_depth;
		bitstream_buffer[21 + 8] = 0;
		bitstream_buffer[22 + 8] = 0;
		bitstream_buffer[23 + 8] = 0;
		memcpy(bitstream_buffer + 24 + 8, &encoder_information, 8);
		memcpy(bitstream_buffer + 32 + 8, &cbgh.zero_comprlen, 4);
		memcpy(bitstream_buffer + 36 + 8, &encrypt_key, 4);
		memcpy(bitstream_buffer + 40 + 8, &y,4);
		bitstream_buffer[44 + 8] = cbgh.sum_check;
		bitstream_buffer[45 + 8] = cbgh.xor_check;
		bitstream_buffer[46 + 8] = 0x01;
		bitstream_buffer[47 + 8] = 0x00;
		memcpy(bitstream_buffer + 0x30 + 8,encrypt_buffer, y);
		memcpy(bitstream_buffer + 0x30 + y + 8, v3, cbgh.huffman_comprlen);
		data =(api*)bitstream_buffer;
		return data;
	}
	void calc_xor_and_sum(unsigned char* encrypt_buffer, int y)
	{
		decrypt_check_present_key = encrypt_key;
		for (unsigned int i = 0; i < y; i++)
		{
			unsigned char calc_temp = 0;
			calc_temp = encrypt_buffer[i] - get_decrypt_check_reduce_num();
			check_sum_result += calc_temp;
			check_xor_result ^= calc_temp;
		}
		cbgh.xor_check = check_xor_result;
		cbgh.sum_check = check_sum_result;
	}
	unsigned char get_decrypt_check_reduce_num()
	{
		unsigned int calc_reduce_num_part_a = 0, calc_reduce_num_part_b = 0;

		calc_reduce_num_part_a = (decrypt_check_present_key & 0x0000ffff) * 20021;

		calc_reduce_num_part_b = (decrypt_check_tag[1] << 24) | (decrypt_check_tag[0] << 16) | (decrypt_check_present_key >> 16);

		calc_reduce_num_part_b = calc_reduce_num_part_b * 20021 + decrypt_check_present_key * 346;

		calc_reduce_num_part_b = (calc_reduce_num_part_b + (calc_reduce_num_part_a >> 16)) & 0x0000ffff;

		decrypt_check_present_key = (calc_reduce_num_part_b << 16) + (calc_reduce_num_part_a & 0x0000ffff) + 1;

		calc_reduce_num_part_b = calc_reduce_num_part_b & 0x00007ffff;

		return calc_reduce_num_part_b;
	}
	unsigned char* encrypt_leaf_nodes()
	{
		unsigned int target_reduce_num;
		unsigned int calc_reduce_num_part_a, calc_reduce_num_part_b;
		unsigned int present_key = encrypt_key;
		unsigned char* encrypt_buffer = new unsigned char[q3.size()];
		int y = q3.size();
		for (long long i = 0; i < y; i++)
		{
			calc_reduce_num_part_a = (present_key & 0x0000ffff) * 20021;
			calc_reduce_num_part_b = ((present_key >> 16) & 0x0000ffff) * 20021;

			target_reduce_num = (present_key * 346 + calc_reduce_num_part_b) + ((calc_reduce_num_part_a >> 16) & 0x0000ffff);

			present_key = (target_reduce_num << 16) + (calc_reduce_num_part_a & 0x0000ffff) + 1;

			encrypt_buffer[i] = q3.front() + (char)target_reduce_num;
			q3.pop();
		}
		return encrypt_buffer;
	}
	void shift7_leaf_nodes()
	{
		for (int i = 0; i < 256; i++)
		{
			unsigned int j = each_byte_num[i];
			unsigned int b;
			unsigned char c;
			char c2;
			b = j << 24;
			b >>= 24;
			if (j < 0x80)
			{
				c = b;
				c2 = c;
				q3.push(c2);
				continue;
			}
			b = b | 0x80;
			c = b;
			c2 = c;
			q3.push(c2);
			j >>= 7;
			b = j << 24;
			b >>= 24;
			if (j < 0x80)
			{
				c = b;
				c2 = c;
				q3.push(c2);
				continue;
			}
			b = b | 0x80;
			c = b;
			c2 = c;
			q3.push(c2);
			j >>= 7;
			b = j << 24;
			b >>= 24;
			if (j < 0x80)
			{
				c = b;
				c2 = c;
				q3.push(c2);
				continue;
			}
			b = b | 0x80;
			c = b;
			c2 = c;
			q3.push(c2);
			j >>= 7;
			b = j << 24;
			b >>= 24;
			if (j < 0x80)
			{
				c = b;
				c2 = c;
				q3.push(c2);
				continue;
			}
			b = b | 0x80;
			c = b;
			c2 = c;
			q3.push(c2);
		}
	}
	unsigned char* encode_huffman_coding()
	{
		int thread1_start = 0;
		int thread1_end = (cbgh.zero_comprlen / 3);
		int thread2_start = cbgh.zero_comprlen / 3;
		int thread2_end = (cbgh.zero_comprlen / 3) * 2;
		int thread3_start = (cbgh.zero_comprlen / 3) * 2;
		int thread3_end = cbgh.zero_comprlen;
		if (mthread)
		{
			thread t[3];
			t[0] = thread(coding_thread, 1, thread1_start, thread1_end);
			t[1] = thread(coding_thread, 2, thread2_start, thread2_end);
			t[2] = thread(coding_thread, 3, thread3_start, thread3_end);
			t[0].join();
			t[1].join();
			t[2].join();
		}
		else
		{
			coding_thread(1, thread1_start, thread1_end);
			coding_thread(2, thread2_start, thread2_end);
			coding_thread(3, thread3_start, thread3_end);
		}
		unsigned char* v3 = new unsigned char[(buffer_size[0] + buffer_size[1] + buffer_size[2]) / 8 + 100];
		//v3 is huffman coding data buffer.Trans to bytes.
		long long int k = 0;
		long long int m = 0;
		for (long long int i = 0; i < 3; i++)
		{
			for (long long int j = 0; j < buffer_size[i]; j++)
			{
				if (buffer_pointer[i][j] == true)
				{
					v3[k] += 1;
					m++;
					if (m >= 8)
					{
						k++;
						m = 0;
					}
					v3[k] <<= 1;
				}
				else
				{
					v3[k] += 0;
					m++;
					if (m >= 8)
					{
						k++;
						m = 0;
					}
					v3[k] <<= 1;
				}
			}
		}
		v3[k] <<= (7 - m);
		cbgh.huffman_comprlen = k;
		return v3;
	}
	void build_huffman_tree()
	{
		//calc each byte nums
		for (int i = 0; i < v.size(); i++)
		{
			each_byte_num[v[i]]++;
		}
		int starttag = 0;
		int j = 0;
		for (short i = 0; i < 256; i++)
		{
			if (!each_byte_num[i])
			{
				node[i].parents = 1;
				starttag++;
			}
		}
		int nodenum = 256;
		int nodenum2 = 511 - starttag;
		for (int i = 0; i < 256; i++)
		{
			node[i].weight = each_byte_num[i];
		}
		for (int i = 0; i < nodenum2 - nodenum; i++)
		{
			int min = 0x7fffffff;
			int pointer = 0;
			int tempweight = 0;
			for (int j = 0; j < nodenum2; j++)
			{
				if (node[j].parents == -1)
				{
					if (node[j].weight < min)
					{
						min = node[j].weight;
						pointer = j;
					}
				}
			}
			node[256 + i].Lchild = pointer;
			node[pointer].L_or_R_child = 0;
			tempweight += min;
			node[pointer].parents = 256 + i;
			min = 0x7fffffff;
			pointer = 0;
			for (int j = 0; j < nodenum2; j++)
			{
				if (node[j].parents == -1)
				{
					if (node[j].weight < min)
					{
						min = node[j].weight;
						pointer = j;
					}
				}
			}
			node[256 + i].Rchild = pointer;
			node[pointer].L_or_R_child = 1;
			tempweight += min;
			node[256 + i].weight = tempweight;
			node[pointer].parents = 256 + i;
			root_location = nodenum2 - 1;
		}
	}
	void compress_zero_bytes(unsigned char* transed_pixel_buffer)
	{
		bool is_comp_zero = false;
		int cur = 0;
		for (;;)
		{
			if (is_comp_zero == false)
			{
				for (int i = cur, j = 0;; i++, j++)
				{
					if (i == cbgh.height * cbgh.width * (cbgh.color_depth / 8))
					{
						return;
					}
					if (transed_pixel_buffer[i] == 0x00)
					{
						cur = i;
						calc_shift7_num(j);
						push_not_zero_bytes(j);
						is_comp_zero = true;
						break;
					}
					else
					{
						q.push(transed_pixel_buffer[i]);
					}
				}
			}
			else
			{
				for (int i = cur, j = 0;; i++, j++)
				{
					if (i == cbgh.height * cbgh.width * (cbgh.color_depth / 8))
					{
						return;
					}
					if (transed_pixel_buffer[i] != 0x00)
					{
						cur = i;
						calc_shift7_num(j);
						is_comp_zero = false;
						break;
					}
				}
			}
			if (cur == cbgh.height * cbgh.width * (cbgh.color_depth / 8))
			{
				return;
			}
		}
	}
	void push_not_zero_bytes(int j)
	{
		for (int i = 0; i < j; i++)
		{
			unsigned char c = q.front();
			q2.push(c);
			q.pop();
		}
		if (!q.empty())
		{
			cout << "bad!!" << endl;
		}
	}
	void calc_shift7_num(int j)
	{
		unsigned int b;
		unsigned char c;
		char c2;
		b = j << 24;
		b >>= 24;
		if (j < 0x80)
		{
			c = b;
			c2 = c;
			q2.push(c2);
			return;
		}
		b = b | 0x80;
		c = b;
		c2 = c;
		q2.push(c2);
		j >>= 7;
		b = j << 24;
		b >>= 24;
		if (j < 0x80)
		{
			c = b;
			c2 = c;
			q2.push(c2);
			return;
		}
		b = b | 0x80;
		c = b;
		c2 = c;
		q2.push(c2);
		j >>= 7;
		b = j << 24;
		b >>= 24;
		if (j < 0x80)
		{
			c = b;
			c2 = c;
			q2.push(c2);
			return;
		}
		b = b | 0x80;
		c = b;
		c2 = c;
		q2.push(c2);
		j >>= 7;
		b = j << 24;
		b >>= 24;
		if (j < 0x80)
		{
			c = b;
			c2 = c;
			q2.push(c2);
			return;
		}
		b = b | 0x80;
		c = b;
		c2 = c;
		q2.push(c2);
	}
	void trans_pixels(unsigned char* raw_pixel_buffer, unsigned char* transed_pixel_buffer)
	{
		if (cbgh.color_depth / 8 == 4)
		{
			//32 bits.
			transed_pixel_buffer[0] = raw_pixel_buffer[0] + 0;
			transed_pixel_buffer[1] = raw_pixel_buffer[1] + 0;
			transed_pixel_buffer[2] = raw_pixel_buffer[2] + 0;
			transed_pixel_buffer[3] = raw_pixel_buffer[3] + 0;

			unsigned char b = 0;
			unsigned char g = 0;
			unsigned char r = 0;
			unsigned char a = 0;
			unsigned long long cur = 0;
			for (long long i = 1; i < cbgh.width; i++)
			{
				cur = i * (cbgh.color_depth / 8);
				b = raw_pixel_buffer[cur] - raw_pixel_buffer[cur - 4];
				transed_pixel_buffer[cur] = b;

				g = raw_pixel_buffer[cur + 1] - raw_pixel_buffer[cur - 3];
				transed_pixel_buffer[cur + 1] = g;

				r = raw_pixel_buffer[cur + 2] - raw_pixel_buffer[cur - 2];
				transed_pixel_buffer[cur + 2] = r;

				a = raw_pixel_buffer[cur + 3] - raw_pixel_buffer[cur - 1];
				transed_pixel_buffer[cur + 3] = a;
			}
			b = 0;
			g = 0;
			r = 0;
			a = 0;
			cur = 0;

			for (long long i = 1; i < cbgh.height; i++)
			{

				cur = i * cbgh.width * (cbgh.color_depth / 8);
				unsigned int up_cur = (i - 1) * cbgh.width * (cbgh.color_depth / 8);

				b = raw_pixel_buffer[cur] - raw_pixel_buffer[up_cur];
				transed_pixel_buffer[cur] = b;

				g = raw_pixel_buffer[cur + 1] - raw_pixel_buffer[up_cur + 1];
				transed_pixel_buffer[cur + 1] = g;

				r = raw_pixel_buffer[cur + 2] - raw_pixel_buffer[up_cur + 2];
				transed_pixel_buffer[cur + 2] = r;

				a = raw_pixel_buffer[cur + 3] - raw_pixel_buffer[up_cur + 3];
				transed_pixel_buffer[cur + 3] = a;
				for (long long j = 1; j < cbgh.width; j++)
				{

					cur = i * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
					unsigned int up_cur = (i - 1) * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
					b = raw_pixel_buffer[cur] - (raw_pixel_buffer[cur - 4] + raw_pixel_buffer[up_cur]) / 2;
					transed_pixel_buffer[cur] = b;

					g = raw_pixel_buffer[cur + 1] - (raw_pixel_buffer[cur - 3] + raw_pixel_buffer[up_cur + 1]) / 2;
					transed_pixel_buffer[cur + 1] = g;

					r = raw_pixel_buffer[cur + 2] - (raw_pixel_buffer[cur - 2] + raw_pixel_buffer[up_cur + 2]) / 2;
					transed_pixel_buffer[cur + 2] = r;

					a = raw_pixel_buffer[cur + 3] - (raw_pixel_buffer[cur - 1] + raw_pixel_buffer[up_cur + 3]) / 2;
					transed_pixel_buffer[cur + 3] = a;
				}
			}
			return;
		}

		//24 bits.
		transed_pixel_buffer[0] = raw_pixel_buffer[0] + 0;
		transed_pixel_buffer[1] = raw_pixel_buffer[1] + 0;
		transed_pixel_buffer[2] = raw_pixel_buffer[2] + 0;

		unsigned char b = 0;
		unsigned char g = 0;
		unsigned char r = 0;
		unsigned long long cur = 0;
		for (long long i = 1; i < cbgh.width; i++)
		{
			cur = i * (cbgh.color_depth / 8);
			b = raw_pixel_buffer[cur] - raw_pixel_buffer[cur - 3];
			transed_pixel_buffer[cur] = b;

			g = raw_pixel_buffer[cur + 1] - raw_pixel_buffer[cur - 2];
			transed_pixel_buffer[cur + 1] = g;

			r = raw_pixel_buffer[cur + 2] - raw_pixel_buffer[cur - 1];
			transed_pixel_buffer[cur + 2] = r;
		}
		b = 0;
		g = 0;
		r = 0;
		cur = 0;

		for (long long i = 1; i < cbgh.height; i++)
		{

			cur = i * cbgh.width * (cbgh.color_depth / 8);
			unsigned int up_cur = (i - 1) * cbgh.width * (cbgh.color_depth / 8);

			b = raw_pixel_buffer[cur] - raw_pixel_buffer[up_cur];
			transed_pixel_buffer[cur] = b;

			g = raw_pixel_buffer[cur + 1] - raw_pixel_buffer[up_cur + 1];
			transed_pixel_buffer[cur + 1] = g;

			r = raw_pixel_buffer[cur + 2] - raw_pixel_buffer[up_cur + 2];
			transed_pixel_buffer[cur + 2] = r;

			for (long long j = 1; j < cbgh.width; j++)
			{

				cur = i * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
				unsigned int up_cur = (i - 1) * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
				b = raw_pixel_buffer[cur] - (raw_pixel_buffer[cur - 3] + raw_pixel_buffer[up_cur]) / 2;
				transed_pixel_buffer[cur] = b;

				g = raw_pixel_buffer[cur + 1] - (raw_pixel_buffer[cur - 2] + raw_pixel_buffer[up_cur + 1]) / 2;
				transed_pixel_buffer[cur + 1] = g;

				r = raw_pixel_buffer[cur + 2] - (raw_pixel_buffer[cur - 1] + raw_pixel_buffer[up_cur + 2]) / 2;
				transed_pixel_buffer[cur + 2] = r;

			}
		}
	}
};

cbg_codec::api* cbg1_enc_default(unsigned short int height, unsigned short int width, unsigned int color_depth, unsigned char* raw_pixel_buffer)
{
	unsigned int res = check_cbg1_enc_default(height, width, color_depth);
	if (res != 0)
	{
		return (cbg_codec::api*)res;
	}
	cbgh.height = height;
	cbgh.width = width;
	cbgh.color_depth = color_depth;
	char ff[] = "bylibcbg";
	memcpy(&encoder_information, &ff, 8);
	encrypt_key = 0x31676263;
	mthread = true;
	cbg_codec encode;
	cbg_codec::api* bitstream_buffer = encode.encoder(raw_pixel_buffer);
	return bitstream_buffer;
}
cbg_codec::api* cbg1_enc_advanced(unsigned short int height, unsigned short int width, 
			unsigned int color_depth, unsigned int key, char* cbg_encoder_information, 
			bool huffman_coding_settings, unsigned char* raw_pixel_buffer)
{
	unsigned int res = check_cbg1_enc_advanced(height, width, color_depth,cbg_encoder_information);
	if (res == 1||res == 2)
	{
		return (cbg_codec::api*)res;
	}
	cbgh.height = height;
	cbgh.width = width;
	cbgh.color_depth = color_depth;
	memcpy(&encoder_information, cbg_encoder_information, 8);
	encrypt_key = key;
	mthread = huffman_coding_settings;
	cbg_codec encode;
	cbg_codec::api* bitstream_buffer = encode.encoder(raw_pixel_buffer);
	return bitstream_buffer;
}
int cbg1_encToFile_default(unsigned short int height, unsigned short int width, unsigned int color_depth,
	unsigned char* raw_pixel_buffer, char* filename)
{
	unsigned int res = check_cbg1_enc_default(height, width, color_depth);
	if (res != 0)
	{
		return res;
	}
	cbgh.height = height;
	cbgh.width = width;
	cbgh.color_depth = color_depth;
	char ff[] = "bylibcbg";
	memcpy(&encoder_information, &ff, 8);
	encrypt_key = 0x31676263;
	mthread = true;
	cbg_codec encode;
	cbg_codec::api* bitstream_buffer = encode.encoder(raw_pixel_buffer);
	cbg_writefile(filename, bitstream_buffer->buffer, bitstream_buffer->buffersize);
	return 0;
}
int cbg1_encToFile_advanced(unsigned short int height, unsigned short int width, 
			unsigned int color_depth, unsigned int key, char* cbg_encoder_information, 
			bool huffman_coding_settings, unsigned char* raw_pixel_buffer, char* filename)
{
	unsigned int res = check_cbg1_enc_advanced(height, width, color_depth,cbg_encoder_information);
	if (res == 1||res == 2)
	{
		return res;
	}
	cbgh.height = height;
	cbgh.width = width;
	cbgh.color_depth = color_depth;
	memcpy(&encoder_information, cbg_encoder_information, 8);
	encrypt_key = key;
	mthread = huffman_coding_settings;
	cbg_codec encode;
	cbg_codec::api* bitstream_buffer = encode.encoder(raw_pixel_buffer);
	cbg_writefile(filename, bitstream_buffer->buffer, bitstream_buffer->buffersize);
	return 0;
}
cbg_codec::api* cbg1_get_trans_pixel(unsigned short int height, unsigned short int width, unsigned int color_depth, unsigned char* raw_pixel_buffer)
{
	only_get_trans_pixels = true;
	unsigned int res = check_cbg1_enc_default(height, width, color_depth);
	if (res != 0)
	{
		return (cbg_codec::api*)res;
	}
	cbgh.height = height;
	cbgh.width = width;
	cbgh.color_depth = color_depth;
	char ff[] = "bylibcbg";
	memcpy(&encoder_information, &ff, 8);
	encrypt_key = 0x31676263;
	mthread = true;
	cbg_codec encode;
	cbg_codec::api* bitstream_buffer = encode.encoder(raw_pixel_buffer);
	return bitstream_buffer;
}
cbg_codec::api* cbg1_get_huffman_stream(unsigned short int height, unsigned short int width, unsigned int color_depth, unsigned char* raw_pixel_buffer)
{
	only_get_huffman_coding = true;
	unsigned int res = check_cbg1_enc_default(height, width, color_depth);
	if (res != 0)
	{
		return (cbg_codec::api*)res;
	}
	cbgh.height = height;
	cbgh.width = width;
	cbgh.color_depth = color_depth;
	char ff[] = "bylibcbg";
	memcpy(&encoder_information, &ff, 8);
	encrypt_key = 0x31676263;
	mthread = true;
	cbg_codec encode;
	cbg_codec::api* bitstream_buffer = encode.encoder(raw_pixel_buffer);
	return bitstream_buffer;
}

#endif
