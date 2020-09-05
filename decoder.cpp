//libcbgx-cbg1 (cbgx/Part I). Powered by copper187
//build with mingw-gcc.



#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <algorithm>

using namespace std;

ifstream fin("testfile.cbg",ios::binary);

struct cbgheader
{
	char fileheader[16]; //CompressedBG___
	unsigned short int width;
	unsigned short int height;
	unsigned int color_depth;
	unsigned int not_use_8byte_a;
	unsigned int not_use_8byte_b;
	unsigned int zero_comprlen;
	unsigned int key;
	unsigned int encrypt_length;
	unsigned char sum_check;
	unsigned char xor_check;
	unsigned short int cbg_version;
	unsigned int filesize;
};

struct huffman_nodes
{
	int Lchild = -1;
	int Rchild = -1;
	int parents = -1;
	int weight = 0x7fffffff;
//	bool used = 0;
};

cbgheader cbgh;
huffman_nodes node[511];
int k2=0;
class cbg_codec
{
	public:
		unsigned char decrypt_check_tag[2]={0,0};
		unsigned int decrypt_check_present_key = 0;
		unsigned char check_sum_result = 0;
		unsigned char check_xor_result = 0;
		unsigned int leaf_nodes_weight[256];

		int decoder(unsigned char *pixel_buffer,unsigned int pixel_size)
		{
			//temp debug point 2.
			/*
			cout<<"check the pixel_buffer pointer and size are get "<<pixel_buffer[999]<<" "<<pixel_size<<endl;
			--check the struct work--
			cout<<cbgh.fileheader;
			*/

			char *temp_encrypt_buffer = new char[cbgh.encrypt_length];

			memset(temp_encrypt_buffer, 0, sizeof(cbgh.encrypt_length));

			fin.read(temp_encrypt_buffer, cbgh.encrypt_length);

			unsigned char *encrypt_buffer = new unsigned char[cbgh.encrypt_length];
			memset(encrypt_buffer, 0, sizeof(cbgh.encrypt_length));

			for(long long i=0; i<cbgh.encrypt_length; i++)
			{
				encrypt_buffer[i] = (unsigned char)temp_encrypt_buffer[i];
			}

			delete[] temp_encrypt_buffer;

			unsigned int act_decomplen,i;
			unsigned char *decrypt_buffer = new unsigned char[cbgh.encrypt_length];
			memset(decrypt_buffer, 0, sizeof(cbgh.encrypt_length));

			unsigned int max = 0xffffffff;  // 4 Byte MAX.
			cout<<"Check sum and xor......"<<endl;
			decrypt_check(encrypt_buffer);
			if(check_sum_result != cbgh.sum_check || check_xor_result != cbgh.xor_check)
			{
				cout<<"The sum or the xor check is error! Exit"<<endl;
				exit(0);
			}
			else
			{
				cout<<"sum and xor check OK."<<endl<<endl;
			}
			cout<<"Decrypt huffman leaf nodes weight......";
			decrypt(encrypt_buffer, decrypt_buffer);
			delete[] encrypt_buffer;
			cout<<"complete."<<endl;

			cout<<"Load huffman leaf nodes weight......";
			load_huffman_leaf_nodes_weight(decrypt_buffer);
			delete[] decrypt_buffer;
			cout<<"complete."<<endl;

			cout<<"Create the huffman tree......";//<<endl;
			create_huffman_tree();
			cout<<"complete."<<endl;

			cout<<"Decode the huffman coding......";//<<endl;
			unsigned char *decode_buffer = decode_huffman_coding();
			cout<<"complete."<<endl;

			cout<<"Decompress the zero compressed part......";//<<endl;
			unsigned char *pre_pixel_buffer = decompress_zero_compressed(decode_buffer);
			delete[] decode_buffer;
			cout<<"complete."<<endl;
			
			cout<<"Transform the pixels......";//<<endl;
			trans_pixel(pre_pixel_buffer,pixel_buffer);
			delete[] pre_pixel_buffer;
			cout<<"complete."<<endl;
			
			cout<<"Write bmp......";//<<endl;
			write_bmp(pixel_buffer);
			cout<<"complete."<<endl;
			
			cout<<"Decode complete."<<endl;
		}
		void write_bmp(unsigned char *pixel_buffer)
		{
			ofstream fout("testfile.bmp",ios::binary);
			fout.put(0x42);
			fout.put(0x4d);
			unsigned int filesize = cbgh.width * cbgh.height * (cbgh.color_depth / 8) + 0x40;
			for(int i=0; i<4; i++)
			{
				int j=8*i;
				char c = filesize >> j;
				fout.put(c);
			}
			for(int i=0; i<4; i++)
			{
				fout.put(0x00);
			}
			fout.put(0x36);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x28);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			for(int i=0; i<2; i++)
			{
				int j=8*i;
				char c = cbgh.width >> j;
				fout.put(c);
			}
			fout.put(0x00);
			fout.put(0x00);
			for(int i=0; i<2; i++)
			{
				int j=8*i;
				char c = cbgh.height >> j;
				fout.put(c);
			}
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x01);
			fout.put(0x00);
			if(cbgh.color_depth == 24)
			{
				fout.put(0x18);
			}
			else
			{
				fout.put(0x20);
			}
			fout.put(0x00);
			
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			fout.put(0x00);
			for(long long i = cbgh.height - 1; i >=0; i--)
			{
				for(long long j = 0; j<cbgh.width; j++)
				{
					for(short k = 0; k< cbgh.color_depth/8;k++)
					{
						unsigned int cur = i * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8) + k;
						fout.put((char)pixel_buffer[cur]);
					}
				}
			}
			delete[] pixel_buffer;
		}
		void trans_pixel(unsigned char *pre_pixel_buffer,unsigned char *pixel_buffer)
		{
			if(cbgh.color_depth/8 == 4)
			{
				
				//32 bits.
			
				pixel_buffer[0] = pre_pixel_buffer[0] + 0;
				pixel_buffer[1] = pre_pixel_buffer[1] + 0;
				pixel_buffer[2] = pre_pixel_buffer[2] + 0;
				pixel_buffer[3] = pre_pixel_buffer[3] + 0;
				
				unsigned char b = 0;
				unsigned char g = 0;
				unsigned char r = 0;
				unsigned char a = 0;
				unsigned long long cur = 0;
				for(long long i = 1; i< cbgh.width; i++)
				{
			//		unsigned int avg = 0;
					cur = i * (cbgh.color_depth / 8);
					b = pre_pixel_buffer[cur] += pixel_buffer[cur - 4];
					pixel_buffer[cur] = b;
				//	cur ++;
					g = pre_pixel_buffer[cur+1] += pixel_buffer[cur - 3];
					pixel_buffer[cur+1] = g;
					//cur ++;
					r = pre_pixel_buffer[cur+2] += pixel_buffer[cur - 2];
					pixel_buffer[cur+2] = r;
					//cur ++;
					a = pre_pixel_buffer[cur+3] += pixel_buffer[cur - 1];
					pixel_buffer[cur+3] = a;
				}
				b = 0;
				g = 0;
				r = 0;
				a = 0;
				cur = 0;
				
				for(long long i = 1; i < cbgh.height; i++)
				{
					
					cur = i * cbgh.width * (cbgh.color_depth / 8);
					unsigned int up_cur = (i -1) * cbgh.width * (cbgh.color_depth / 8);
					b = pre_pixel_buffer[cur] += pixel_buffer[up_cur];
					pixel_buffer[cur] = b;
					g = pre_pixel_buffer[cur+1] += pixel_buffer[up_cur + 1];
					pixel_buffer[cur+1] = g;
					r = pre_pixel_buffer[cur+2] += pixel_buffer[up_cur + 2];
					pixel_buffer[cur+2] = r;
					a = pre_pixel_buffer[cur+3] += pixel_buffer[up_cur + 3];
					pixel_buffer[cur+3] = a;
					for(long long j = 1; j < cbgh.width; j++)
					{
						
						cur = i * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
						unsigned int up_cur = (i -1) * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
						b = pre_pixel_buffer[cur] += (pixel_buffer[cur - 4] + pixel_buffer[up_cur]) / 2;
						pixel_buffer[cur] = b;
						//cur ++;
					//	up_cur ++;
						g = pre_pixel_buffer[cur+1] += (pixel_buffer[cur - 3] + pixel_buffer[up_cur+1]) / 2;
						pixel_buffer[cur+1] = g;
						//cur ++;
						//up_cur ++;
						r = pre_pixel_buffer[cur+2] += (pixel_buffer[cur - 2] + pixel_buffer[up_cur+2]) / 2;
						pixel_buffer[cur+2] = r;
						a = pre_pixel_buffer[cur+3] += (pixel_buffer[cur - 1] + pixel_buffer[up_cur+3]) / 2;
						pixel_buffer[cur+3] = a;
						//cur ++;
					//	up_cur ++;
						/*if(cur >= 0x10a8f0)
						{
				//			cout<<"df";
						}
						b += pre_pixel_buffer[cur];
						g += pre_pixel_buffer[cur+1];
						r += pre_pixel_buffer[cur+2];
						pixel_buffer[cur] = b;
						pixel_buffer[cur+1] = g;
						pixel_buffer[cur+2] = r;*/
					}
					
				}
					//ofstream fout("test5x.tmp",ios::binary);
					//fout.write((char *)pixel_buffer,cbgh.width * cbgh.height * (cbgh.color_depth / 8));
					return ;
			}
			//24 bits.
			
			pixel_buffer[0] = pre_pixel_buffer[0] + 0;
			pixel_buffer[1] = pre_pixel_buffer[1] + 0;
			pixel_buffer[2] = pre_pixel_buffer[2] + 0;
			
			unsigned char b = 0;
			unsigned char g = 0;
			unsigned char r = 0;
			unsigned long long cur = 0;
			for(long long i = 1; i< cbgh.width; i++)
			{
		//		unsigned int avg = 0;
				cur = i * (cbgh.color_depth / 8);
				b = pre_pixel_buffer[cur] += pixel_buffer[cur - 3];
				pixel_buffer[cur] = b;
			//	cur ++;
				g = pre_pixel_buffer[cur+1] += pixel_buffer[cur - 2];
				pixel_buffer[cur+1] = g;
				//cur ++;
				r = pre_pixel_buffer[cur+2] += pixel_buffer[cur - 1];
				pixel_buffer[cur+2] = r;
				//cur ++;
			}
			b = 0;
			g = 0;
			r = 0;
			cur = 0;
			
			for(long long i = 1; i < cbgh.height; i++)
			{
				
				cur = i * cbgh.width * (cbgh.color_depth / 8);
				unsigned int up_cur = (i -1) * cbgh.width * (cbgh.color_depth / 8);
				b = pre_pixel_buffer[cur] += pixel_buffer[up_cur];
				pixel_buffer[cur] = b;
				g = pre_pixel_buffer[cur+1] += pixel_buffer[up_cur + 1];
				pixel_buffer[cur+1] = g;
				r = pre_pixel_buffer[cur+2] += pixel_buffer[up_cur + 2];
				pixel_buffer[cur+2] = r;
				for(long long j = 1; j < cbgh.width; j++)
				{
					
					cur = i * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
					unsigned int up_cur = (i -1) * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
					b = pre_pixel_buffer[cur] += (pixel_buffer[cur - 3] + pixel_buffer[up_cur]) / 2;
					pixel_buffer[cur] = b;
					//cur ++;
				//	up_cur ++;
					g = pre_pixel_buffer[cur+1] += (pixel_buffer[cur - 2] + pixel_buffer[up_cur+1]) / 2;
					pixel_buffer[cur+1] = g;
					//cur ++;
					//up_cur ++;
					r = pre_pixel_buffer[cur+2] += (pixel_buffer[cur - 1] + pixel_buffer[up_cur+2]) / 2;
					pixel_buffer[cur+2] = r;
					//cur ++;
				//	up_cur ++;
					/*if(cur >= 0x10a8f0)
					{
			//			cout<<"df";
					}
					b += pre_pixel_buffer[cur];
					g += pre_pixel_buffer[cur+1];
					r += pre_pixel_buffer[cur+2];
					pixel_buffer[cur] = b;
					pixel_buffer[cur+1] = g;
					pixel_buffer[cur+2] = r;*/
				}
			}
		//	ofstream fout("test5x.tmp",ios::binary);
		//	fout.write((char *)pixel_buffer,cbgh.width * cbgh.height * (cbgh.color_depth / 8));
		}
		unsigned char* decompress_zero_compressed(unsigned char *decode_buffer)
		{
			unsigned char *pre_pixel_buffer = new unsigned char[cbgh.width * cbgh.height *(cbgh.color_depth / 8)];
			memset(pre_pixel_buffer, 0, sizeof(pre_pixel_buffer));
			unsigned int decompress_cur = 0;
			unsigned int decode_cur = 0;
			bool is_put0 = 0;
			for(;;)
			{
			//	unsigned int temp_decode_cur = decode_cur;
				unsigned int bits = 0;
				unsigned int copy_bytes_num = 0;
				unsigned char get_byte = 0;
				unsigned int tep2 = 0; //must unsigned int.(4byte)
				
				do
				{
					bool ok = 0;
					
					/*if(decrypt_byte_cur >= cbgh.encrypt_length)
					{
						cout<<endl;
						cout<<"Error! The current decrypt byte is out of the decrypt byte length. Maybe the huffman leaf nodes weight data are error!";
					}*/
					get_byte = decode_buffer[decode_cur];
					decode_cur++;
					if(get_byte >= 0x80)
					{
						get_byte -= 0x80;
						ok = 1;
					}
					tep2 = (unsigned char)get_byte;
					tep2 <<= bits;
					copy_bytes_num |= tep2 ;
					if(ok)
					{
						get_byte +=0x80;
					}
					bits += 7;
				}while(get_byte >= 0x80);
				if(!is_put0)
				{
					for(int i=decompress_cur, j=decode_cur; i<decompress_cur+copy_bytes_num, j<decode_cur+copy_bytes_num; i++, j++)
					{
						pre_pixel_buffer[i] = decode_buffer[j];
					}
					decode_cur += copy_bytes_num;
			//		cout<<decode_cur<<endl;
					if(decode_cur >= cbgh.zero_comprlen)
					{
						if(decode_cur > cbgh.zero_comprlen)
						{
							cout<<"bad";
						}
						break;
					}
					is_put0 = 1;
				}
				else
				{
					for(int i = decompress_cur; i< decompress_cur + copy_bytes_num; i++)
					{
						pre_pixel_buffer[i] = 0x00;
					}
					is_put0 = 0;
				}
				decompress_cur += copy_bytes_num;
			/*	if(decompress_cur == 2764800)
				{
					cout<<decode_cur;
						}	*/
				if(decode_cur >= cbgh.zero_comprlen)
					{
						if(decode_cur > cbgh.zero_comprlen)
						{
							cout<<"bad";
						}
						break;
					}	
			}
		//	ofstream fout("testx2.tmp",ios::binary);
		//	for(int i=0;i<code_size;i++)
		//	fout.write((char *)pre_pixel_buffer,cbgh.width * cbgh.height *(cbgh.color_depth / 8));
			//cout<<decompress_cur;
			return pre_pixel_buffer;
		}
		int root_location = 0;
		int leaf_node_max_location = 0;
		unsigned char* decode_huffman_coding()
		{
			//0x30 is cbg header size.
		//	k=0;
			unsigned int code_size = cbgh.filesize - cbgh.encrypt_length - 0x30;
			char *temp_code_buffer = new char[code_size];

			memset(temp_code_buffer, 0, sizeof(code_size));

			fin.read(temp_code_buffer, code_size);

			unsigned char *code_buffer = new unsigned char[code_size];
			memset(code_buffer, 0, sizeof(code_size));

			for(long long i=0; i<code_size; i++)
			{
				code_buffer[i] = (unsigned char)temp_code_buffer[i];
			}
			unsigned char *decode_buffer = new unsigned char[cbgh.zero_comprlen];
			memset(decode_buffer, 0, sizeof(cbgh.zero_comprlen));
			delete[] temp_code_buffer;
			int i = 0,j = 0,decodenum = 0;
			bool ok = 0;
			int now_node_location = root_location;
			for(;;)
			{
				for(; j < 8; j++)
				{
					bool bit = get_bit(i,code_buffer);
					if(bit == 0)
					{
						now_node_location = node[now_node_location].Lchild;
					}
					if(bit == 1)
					{
						now_node_location = node[now_node_location].Rchild;
					}
					if(now_node_location <= leaf_node_max_location)
					{
						j++;
						ok = 1;
						break;
					}
				}
				if(ok == 1)
				{
					decode_buffer[k2] = now_node_location;
					k2++;
					if(k2 >= cbgh.zero_comprlen)
					{
						break;

					}
					now_node_location = root_location;
					decodenum++;
					ok = 0;
				}
				if(j >= 8)
				{
					if(j!=8)
					{
						cout<<"bad!!";
					}
					i++;
					j=0;
					if(k2 >= cbgh.zero_comprlen)
					{
						break;

					}
				}
			}
			delete[] code_buffer;
		//	ofstream fout("testx.tmp",ios::binary);
		//	for(int i=0;i<code_size;i++)
		//	fout.write((char *)decode_buffer,k2);
			return decode_buffer;
		}
		bool get_bit(int i,unsigned char *code_buffer)
		{
			if(code_buffer[i] < 0x80)
			{
				code_buffer[i]<<=1;
				return 0;
			}
			else
			{
				code_buffer[i]<<=1;
				return 1;
			}

		}
		void create_huffman_tree()
		{

			int starttag = 0;
			int j = 0;
		//	sort(leaf_nodes_weight,leaf_nodes_weight+256);
			for(short i =0 ; i< 256;i++)
			{
				if(!leaf_nodes_weight[i])
				{
					node[i].parents = 1;
					starttag++;
					//break;
				}
			}
			int nodenum = 256 ;
			int nodenum2 = 511 - starttag;
	//		nodenum = nodenum * 2 -1;

			//unsigned int **Lchild = new unsigned int

			for(int i = 0; i < 256; i++)
			{
				node[i].weight = leaf_nodes_weight [i];
			}
			for(int i = 0;i < nodenum2-nodenum; i++)
			{
				int min = 0x7fffffff;
				int pointer = 0;
				int tempweight = 0;
				for(int j = 0; j< nodenum2 ; j++)
				{
					if(node[j].parents == -1)
					{
						if(node[j].weight < min)
						{
							min = node[j].weight;
							pointer = j;
						}
					}
				}
				node[256+i].Lchild = pointer;
				tempweight += min;
			//	node[pointer].used = true;
				node[pointer].parents = 256+i;
				min = 0x7fffffff;
				pointer = 0;
				for(int j = 0; j< nodenum2 ; j++)
				{
					if(node[j].parents == -1)
					{
						if(node[j].weight < min)
						{
							min = node[j].weight;
							pointer = j;
						}
					}
				}
				node[256+i].Rchild = pointer;
				tempweight += min;
				node[256+i].weight = tempweight;
			//	node[pointer].used = true;
				node[pointer].parents = 256+i;
				root_location = nodenum2 - 1;
				leaf_node_max_location = 255;
			}
			//debug point 3
			/*cout<<"\n      ID    Lchild    Rchild    Parents    weight"<<endl;
			for(int i=0;i<nodenum2;i++)
			{
				cout<<"node: "<<i+1<<"        "<<node[i].Lchild+1<<"        "<<node[i].Rchild+1<<"       "<<node[i].parents+1<<"      "<<node[i].weight<<endl;
			}*/
		//	system("pause");
		}
		void load_huffman_leaf_nodes_weight(unsigned char *decrypt_buffer)
		{
		//	ofstream fout("test2.out");
		//	fout.write((char *)decrypt_buffer,*decrypt_buffer);
			long long int size=0;
			unsigned int decrypt_byte_cur = 0;
			for(short i =0; i< 256; i++)
			{
				unsigned int bits = 0;
				unsigned int present_weight = 0;
				unsigned char decrypt_byte = 0;
				unsigned int tep2 = 0;
				unsigned char tep;
				do
				{
					bool ok = 0;
					if(decrypt_byte_cur >= cbgh.encrypt_length)
					{
						cout<<endl;
						cout<<"Error! The current decrypt byte is out of the decrypt byte length. Maybe the huffman leaf nodes weight data are error!";
					}
					decrypt_byte = decrypt_buffer[decrypt_byte_cur];
					decrypt_byte_cur++;
					if(decrypt_byte >= 0x80)
					{
						decrypt_byte -= 0x80;
						ok = 1;
					}
					tep2 = (unsigned char)decrypt_byte;
					//decrypt_byte = decrypt_byte & 0x7f;
					tep2 <<= bits;
				//	memset(&present_weight+(bits/7),tep2+(bits/7),8);
					//tep2 <<= 8*(bits/7);
				//	decrypt_byte = decrypt_byte << bits;
					present_weight |= tep2 ;
					if(ok)
					{
						decrypt_byte +=0x80;
					}
					bits += 7;
				}while(decrypt_byte >= 0x80);

				leaf_nodes_weight[i] = present_weight;
		//		fout<<present_weight<<" ";
				size +=present_weight;
			}
			 return;
		}

		void decrypt(unsigned char *encrypt_buffer,unsigned char *decrypt_buffer)
		{
			unsigned int target_reduce_num;
			unsigned int calc_reduce_num_part_a, calc_reduce_num_part_b;
			unsigned int present_key = cbgh.key;

			for(long long i=0; i<cbgh.encrypt_length;i++)
			{
				calc_reduce_num_part_a = (present_key & 0x0000ffff) * 20021;
				calc_reduce_num_part_b = ((present_key >> 16) & 0x0000ffff) * 20021;

				target_reduce_num = (present_key * 346 + calc_reduce_num_part_b) + ((calc_reduce_num_part_a >>16) & 0x0000ffff);

				present_key = (target_reduce_num << 16) + (calc_reduce_num_part_a & 0x0000ffff) + 1;

				decrypt_buffer[i] = encrypt_buffer[i] - (char)target_reduce_num;

				//debug point 2
				//printf("%02X %02X %X ",encrypt_byte,decrypt_buffer[i],present_key);
				/*if(i>=2)
				{
					exit(0);
				}*/
			}

		}
		void decrypt_check(unsigned char *encrypt_buffer)
		{
			decrypt_check_present_key = cbgh.key;
			for(unsigned int i=0; i<cbgh.encrypt_length; i++)
			{
				unsigned char calc_temp = 0;
				calc_temp = encrypt_buffer[i] - get_decrypt_check_reduce_num();
				check_sum_result += calc_temp;
				check_xor_result ^= calc_temp;
				//debug point 3
				//printf("%02X+%02X ",check_sum_result, check_xor_result);
			}
		}
		unsigned char get_decrypt_check_reduce_num()
		{
			unsigned int calc_reduce_num_part_a = 0, calc_reduce_num_part_b = 0;

			calc_reduce_num_part_a = (decrypt_check_present_key & 0x0000ffff) *20021;

			calc_reduce_num_part_b = (decrypt_check_tag[1] << 24) | (decrypt_check_tag[0] << 16) | (decrypt_check_present_key >> 16);

			calc_reduce_num_part_b = calc_reduce_num_part_b * 20021 + decrypt_check_present_key * 346;

			calc_reduce_num_part_b = (calc_reduce_num_part_b + (calc_reduce_num_part_a >> 16)) & 0x0000ffff;

			decrypt_check_present_key = (calc_reduce_num_part_b << 16) + (calc_reduce_num_part_a & 0x0000ffff) + 1;

			calc_reduce_num_part_b = calc_reduce_num_part_b & 0x00007ffff;

			return calc_reduce_num_part_b;
		}
};

int main()
{

	char temp[0x30];
	unsigned char temp2[0x30];
	fin.read(temp,0x30);
	for(int i=0;i<0x30;i++)
	{
		temp2[i] = (unsigned char)temp[i];
	}
	for(int i=0;i<16;i++)
	{
		cbgh.fileheader[i] = temp2[i];
	}
	cbgh.width = temp2[17];
	cbgh.width <<= 8;
	cbgh.width += temp2[16];

	cbgh.height = temp2[19];
	cbgh.height <<= 8;
	cbgh.height += temp2[18];

	cbgh.color_depth = temp2[23];
	cbgh.color_depth <<= 8;
	cbgh.color_depth += temp2[22];
	cbgh.color_depth <<= 8;
	cbgh.color_depth += temp2[21];
	cbgh.color_depth <<= 8;
	cbgh.color_depth += temp2[20];

	cbgh.zero_comprlen = temp2[35];
	cbgh.zero_comprlen <<= 8;
	cbgh.zero_comprlen += temp2[34];
	cbgh.zero_comprlen <<= 8;
	cbgh.zero_comprlen += temp2[33];
	cbgh.zero_comprlen <<= 8;
	cbgh.zero_comprlen += temp2[32];

	cbgh.key = temp2[39];
	cbgh.key <<= 8;
	cbgh.key += temp2[38];
	cbgh.key <<= 8;
	cbgh.key += temp2[37];
	cbgh.key <<= 8;
	cbgh.key += temp2[36];

	cbgh.encrypt_length = temp2[43];
	cbgh.encrypt_length <<= 8;
	cbgh.encrypt_length += temp2[42];
	cbgh.encrypt_length <<= 8;
	cbgh.encrypt_length += temp2[41];
	cbgh.encrypt_length <<= 8;
	cbgh.encrypt_length += temp2[40];

	cbgh.sum_check = temp2[44];
	cbgh.xor_check = temp2[45];

	cbgh.cbg_version = temp2[46];
//	cbgh.cbg_version <<= 8;
//	cbgh.cbg_version += temp2[47];
	//fin>>cbgh.fileheader;

	/*fin>>cbgh.width;
	fin>>cbgh.height;
	fin>>cbgh.color_depth;
	fin>>cbgh.not_use_8byte_a;
	fin>>cbgh.not_use_8byte_b;
	fin>>cbgh.zero_comprlen;
	fin>>cbgh.key;
	fin>>cbgh.encode_length;
	fin>>cbgh.sum_check;
	fin>>cbgh.xor_check;
	fin>>cbgh.cbg_version;*/

	//Get the file size.
	//First, Now the file is only read the header part. So we need to save the pointer.
	int tempfseek;
	tempfseek = 0x30;

//	cout<<hex<<fin.get();
	//When the pointer is at the end of this file, The pointer is equal to the file size.
	fin.seekg(0,ios::end);
	cbgh.filesize = fin.tellg();

	//After get the file size, We need to restore the pointer.
	fin.seekg(tempfseek,ios::beg);

	//Check the file header(magic bytes).
	if(memcmp(cbgh.fileheader,"CompressedBG___",16))
	{
		cout<<"Not a cbg file.Exit."<<endl;
		exit(0);
	}

	//Check the cbg version. But the most of cbg images will not this version.
	if(cbgh.color_depth==16&&cbgh.cbg_version==2)
	{
		cout<<"Not support cbg version.Please use other decoder!"<<endl;
		exit(0);
	}

	//count the color channel number
	//Mostly, If the image support the RGB channels the number will be 3. If the image support the RGBA channels the number will be 4.
	unsigned long long int pixel_size=0;
	unsigned short int color_channel_num=0;

	color_channel_num=cbgh.color_depth / 8;

	//count the image all pixel size. This count way(width*height*color channel num) is apply to all images.Not only cbg images.
	pixel_size = cbgh.width * cbgh.height * color_channel_num;

	//Allocate memory for pixel buffer.The buffer size is equal to pixel size.
	unsigned char *pixel_buffer = new unsigned char[pixel_size];
	memset(pixel_buffer, 0, sizeof(pixel_buffer));

	//Work for the temp debug point 2.
	//pixel_buffer[999]='Y';

	//debug point 1
	cout<<"File header data:"<<endl;
	cout<<"File header "<<cbgh.fileheader<<endl;
	cout<<"Width "<<cbgh.width<<endl;
	cout<<"Height "<<cbgh.height<<endl;
	cout<<"Color_depth "<<cbgh.color_depth<<endl;
	cout<<"Zero_comprlen "<<cbgh.zero_comprlen<<endl;
	cout<<hex<<"Key 0x"<<cbgh.key<<endl;
	cout<<dec<<"Encode Length "<<cbgh.encrypt_length<<endl;
	printf("sum_check 0x%02x\nxor_check 0x%02x\n",cbgh.sum_check,cbgh.xor_check);
	cout<<dec<<"cbg_version "<<cbgh.cbg_version<<endl;
	cout<<"File Size "<<cbgh.filesize<<endl<<endl;

	//Call the main decode function.
	cbg_codec codec;
	codec.decoder(pixel_buffer,pixel_size);

	//temp debug point 1
	//cout<<color_channel_num<<" "<<pixel_size<<endl;



	system("pause");
}
