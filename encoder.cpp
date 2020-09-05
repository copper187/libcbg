//libcbgx-cbg1 (cbgx/Part I). Powered by copper187
//build with mingw-gcc.



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

mutex m;

ifstream fin("testfile.bmp",ios::binary);
ofstream fout("testfiel.cbg",ios::binary); //The testfile for decoder is "testfile.cbg", So :)




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
	unsigned int filesize;
	unsigned int huffman_comprlen;
};

struct huffman_nodes
{
	int Lchild = -1;
	int Rchild = -1;
	int parents = -1;
	int weight = 0x7fffffff;
	bool L_or_R_child = 0;
//	bool used = 0;
};

cbgheader cbgh;
huffman_nodes node[511];
long long each_byte_num[256];
int root_location;
bool *buffer_pointer[3];
int buffer_size[3];
bool coding_complete[3]={0,0,0};
long long num[3];
unsigned char decrypt_check_tag[2]={0,0};
unsigned int decrypt_check_present_key = 0;
unsigned char check_sum_result = 0;
unsigned char check_xor_result = 0;


queue<unsigned char> q; //q is buffer for not zero bytes.
queue<unsigned char> q2;//q2 is buffer for compressed zero bytes data.
vector<unsigned char> v;//v is buffer for compressed zero bytes data.For calc each byte nums and huffman coding.
queue<unsigned char> q3;//v2 is buffer for encrypted huffman leaf nodes weight. 
//Why I don't use new? Because I have no way to know the not zero bytes size and compressed zero bytes data size. 
//So I cannot allocate static size memory.I need a dynamic memory container such as C++ STL queue and vector.
		
void coding_thread(int ID,int start,int end)
{
	int coding_cur = 0;
	int root_node = root_location;
	//v2 is huffman coding data buffer.
	bool *v2 = new bool[(cbgh.zero_comprlen/2) * 8];
	//But why I use new to allocate huffman coding data buffer? Because I use multithreads to huffman coding.If I use vector,I will be can't cross border to write memory.
	//For more, You should read more information about C++ STL vector.
	for(int i = start;i < end; i++)
	{
		
		unsigned char present_byte = v[i];
		unsigned int present_node = v[i];
		unsigned int start_cur = coding_cur;
		for(;;)
		{
			if(present_node == root_node)
			{
				bool *temp_huffman_buffer = new bool[coding_cur - start_cur + 1];
				for(int k = start_cur , j = 0; k < coding_cur, j < coding_cur - start_cur; k++, j++)
				{
					temp_huffman_buffer[j] = v2[k];
				}
				for(int k = coding_cur - 1, j = 0; k >= start_cur, j < coding_cur - start_cur; k--, j++)
				{
					v2[k] = temp_huffman_buffer[j];
				}
				delete[] temp_huffman_buffer;
				num[ID-1]++;
				break;
			}
			if(node[present_node].L_or_R_child == 0)
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
	buffer_pointer[ID-1] = v2;
	buffer_size[ID-1] = coding_cur ;
	coding_complete[ID-1] = 1;
	return;
}

class cbg_codec
{
	public:
		
		
		
		void encoder(unsigned char *raw_pixel_buffer)
		{
			cout<<"Transform the pixels......";//<<endl;
			unsigned char *transed_pixel_buffer = new unsigned char[cbgh.height * cbgh.width * (cbgh.color_depth/8)];
			trans_pixels(raw_pixel_buffer, transed_pixel_buffer);
			delete[] raw_pixel_buffer;
			cout<<"complete."<<endl;
			
			cout<<"Compress the zero bytes......";
			compress_zero_bytes(transed_pixel_buffer);
			cbgh.zero_comprlen = q2.size();
			cout<<"complete."<<endl;
			
		//	ofstream fout2("test8x.tmp",ios::binary);
			while(!q2.empty())
			{
				char c;
				c = q2.front();
				//cout<<c<<endl;
		//	fout2.put(c);
				v.push_back(q2.front());
				q2.pop();
				//cout<<q2.size();
			}
			cout<<"Build huffman tree......";//<<endl;
			build_huffman_tree();
			cout<<"complete."<<endl;
			
			cout<<"Encode the huffman coding......";//<<endl;
			unsigned char *v3  = encode_huffman_coding();
			cout<<"complete."<<endl;
			
			cout<<"Encrypt huffman leaf nodes weight......";
			shift7_leaf_nodes();
			/*ofstream fout13("test13x.tmp",ios::binary);
			while(!q3.empty())
			{
				fout13.put(q3.front());
				q3.pop();
			}*/
			int y=q3.size();
			unsigned char *encrypt_buffer = encrypt_leaf_nodes();
			//ofstream fout13("test14x.tmp",ios::binary);
			//fout13.write((char *)encrypt_buffer,y);
			cout<<"complete."<<endl;
			
			cout<<"calc decrypt check xor and sum......";
			calc_xor_and_sum(encrypt_buffer,y);
			cout<<"complete."<<endl;
			
			cout<<"write cbg......";
			write_cbg(encrypt_buffer,v3,y);
			cout<<"complete."<<endl;
			
			cout<<"Encode complete."<<endl;
		}
		void write_cbg(unsigned char *encrypt_buffer,unsigned char *v3,unsigned int y)
		{
			char encoder_information[] = "bylibcbg";
			
			fout.write(encoder_information,8);
			unsigned int temp1 = cbgh.zero_comprlen;
			for(int i = 0;i<4;i++)
			{
				char t = temp1;
				fout.put(t);
				temp1>>=8;
			}
			char key[] = "cbg1";
			fout.write(key,4);
			unsigned int temp2 = y;
			for(int i = 0;i<4;i++)
			{
				char t = temp2;
				fout.put(t);
				temp2>>=8;
			}
			fout.put((char)cbgh.sum_check);
			fout.put((char)cbgh.xor_check);
			fout.put(0x01);
			fout.put(0x00);
			fout.write((char *)encrypt_buffer,y);
			fout.write((char *)v3,cbgh.huffman_comprlen);
		}
		void calc_xor_and_sum(unsigned char *encrypt_buffer,int y)
		{
				decrypt_check_present_key = 0x31676263;
				for(unsigned int i=0; i<y; i++)
				{
					unsigned char calc_temp = 0;
					calc_temp = encrypt_buffer[i] - get_decrypt_check_reduce_num();
					check_sum_result += calc_temp;
					check_xor_result ^= calc_temp;
					//debug point 3
					//printf("%02X+%02X ",check_sum_result, check_xor_result);
				}
				cbgh.xor_check = check_xor_result;
				cbgh.sum_check = check_sum_result;
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
		unsigned char *encrypt_leaf_nodes()
		{
			unsigned int target_reduce_num;
			unsigned int calc_reduce_num_part_a, calc_reduce_num_part_b;
			unsigned int present_key = 0x31676263;
			unsigned char *encrypt_buffer = new unsigned char[q3.size()];
			int y=q3.size();
			for(long long i=0; i<y;i++)
			{
				calc_reduce_num_part_a = (present_key & 0x0000ffff) * 20021;
				calc_reduce_num_part_b = ((present_key >> 16) & 0x0000ffff) * 20021;

				target_reduce_num = (present_key * 346 + calc_reduce_num_part_b) + ((calc_reduce_num_part_a >>16) & 0x0000ffff);

				present_key = (target_reduce_num << 16) + (calc_reduce_num_part_a & 0x0000ffff) + 1;

				encrypt_buffer[i] = q3.front() + (char)target_reduce_num;
				q3.pop();
				
				//debug point 2
				//printf("%02X %02X %X ",encrypt_byte,decrypt_buffer[i],present_key);
				/*if(i>=2)
				{
					exit(0);
				}*/
			}
			return encrypt_buffer;
		}
		void shift7_leaf_nodes()
		{
			for(int i = 0; i < 256; i++)
			{
				unsigned int j = each_byte_num[i];
				unsigned int b;
				unsigned char c;
				char c2;
				b=j<<24;
				b>>=24;
				if(j<0x80)
				{
					c=b;
					c2=c;
					q3.push(c2);
					continue;
				}
				b=b|0x80;
				c=b;
				c2=c;
				q3.push(c2);
				
			//	q2.push(c2);
			//	printf("%2X ",c2);
				j>>=7;
				b=j<<24;
				b>>=24;
				if(j<0x80)
				{
					c=b;
					c2=c;
					q3.push(c2);
					continue;
				}
				b=b|0x80;	
				c=b;
				c2=c;
				q3.push(c2);
				
			//	q2.push(c2);
			//	printf("%2X ",c2);
				j>>=7;
				b=j<<24;
				b>>=24;
				if(j<0x80)
				{
					c=b;
					c2=c;
					q3.push(c2);
					continue;
				}
				b=b|0x80;	
				c=b;
				c2=c;
				q3.push(c2);
				
				j>>=7;
				b=j<<24;
				b>>=24;
				if(j<0x80)
				{
					c=b;
					c2=c;
					q3.push(c2);
					continue;
				}
				b=b|0x80;	
				c=b;
				c2=c;
				q3.push(c2);
				
			//	q2.push(c2);
			//	printf("%2X ",c2);
			//q3.push(0x00);
			//	q2.push(0x00);
			//	printf("0x00 ");
			}
		}
		unsigned char *encode_huffman_coding()
		{
		//	v[0] = 0x01; 
			int thread1_start = 0;
			int thread1_end = (cbgh.zero_comprlen / 3) ;
			int thread2_start = cbgh.zero_comprlen / 3;
			int thread2_end = (cbgh.zero_comprlen / 3) * 2 ;
			int thread3_start = (cbgh.zero_comprlen / 3) * 2;
			int thread3_end = cbgh.zero_comprlen ;
			thread t[3];
			t[0] = thread(coding_thread,1,thread1_start,thread1_end);
			t[1] = thread(coding_thread,2,thread2_start,thread2_end);
			t[2] = thread(coding_thread,3,thread3_start,thread3_end);
		//	coding_thread(1,thread1_start,thread1_end);
		//	coding_thread(2,thread2_start,thread2_end);
		//	coding_thread(3,thread3_start,thread3_end);
			t[0].detach();
			t[1].detach();
			t[2].detach();
			
			//Wait all thread coding complete.
			for(;;)
			{
				if(coding_complete[0] == 1 && coding_complete[1] == 1 && coding_complete[2] == 1)
				{
					break;
				}
			}
			unsigned char *v3 = new unsigned char[(buffer_size[0] + buffer_size[1] + buffer_size[2]) / 8 +100];
			//v3 is huffman coding data buffer.Trans to bytes.
			int k = 0;
			int m = 0;
		/*	ofstream fout2("test9x.tmp",ios::binary);
			ofstream fout3("test12x.tmp",ios::binary);
			for(int i=0; i<buffer_size[0];++i)
			{
				if(buffer_pointer[2][i])
				{
					fout3<<"1";
				}
				else
				{
					fout3<<"0";
				}
			}*/
			for(int i=0;i<3;i++)
			{
				for(int j=0; j<buffer_size[i];j++)
				{
					if(buffer_pointer[i][j]==true)
					{
					//	cout<<"1";
						v3[k] += 1;
						//v3[k]<<=1;
						m++;
						if(m >= 8)
						{
							//printf("%2X ",v3[k]);
					//		fout2.put(v3[k]);
							k++;
							m=0;
						}
						v3[k]<<=1;
					}
					else
					{
					//	cout<<"0";
						v3[k] += 0;
						
						m++;
						if(m >= 8)
						{
							//printf("%2X ",v3[k]);
					//		fout2.put(v3[k]);
							k++;
							m=0;
						}
						v3[k]<<=1;
					}
				}
			}
			v3[k]<<=(7-m);
		//	fout2.put(v3[k]);
			cbgh.huffman_comprlen = k;
			return v3;
			
		}
		
		void build_huffman_tree()
		{
			//calc each byte nums
			for(int i = 0;i < v.size();i++)
			{
				each_byte_num[v[i]]++;
			}
		//	ofstream fout2("test10x.tmp");
		/*	for(int i = 0;i < 256;i++)
			{
				cout<<each_byte_num[i]<<" ";
				fout2<<each_byte_num[i]<<" ";
			}*/
			int starttag = 0;
			int j = 0;
		//	sort(leaf_nodes_weight,leaf_nodes_weight+256);
			for(short i =0 ; i< 256;i++)
			{
				if(!each_byte_num[i])
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
				node[i].weight = each_byte_num [i];
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
				node[pointer].L_or_R_child = 0; 
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
				node[pointer].L_or_R_child = 1; 
				tempweight += min;
				node[256+i].weight = tempweight;
			//	node[pointer].used = true;
				node[pointer].parents = 256+i;
				root_location = nodenum2 - 1;
				//leaf_node_max_location = 255;
			}
			/*cout<<"      ID    Lchild    Rchild    Parents    weight     L_or_R_child"<<endl;
			for(int i=0;i<nodenum2;i++)
			{
				cout<<"node: "<<i+1<<"        "<<node[i].Lchild+1<<"        "<<node[i].Rchild+1<<"       "<<node[i].parents+1<<"      "<<node[i].weight<<"      ";
				if(node[i].L_or_R_child)
				{
					cout<<"R"<<endl;
				}
				else
				{
					cout<<"L"<<endl;
				}
				
			}*/
		}			
		void compress_zero_bytes(unsigned char *transed_pixel_buffer)
		{
			bool is_comp_zero = false;
			int cur = 0;
			
			for(;;)
			{
				if(is_comp_zero == false)
				{
					for(int i = cur,j = 0;;i++,j++)
					{
						if(i == cbgh.height * cbgh.width * (cbgh.color_depth/8))
						{
							return;
						}
						if(transed_pixel_buffer[i] == 0x00)
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
					for(int i = cur ,j = 0;;i++,j++)
					{
						if(i == cbgh.height * cbgh.width * (cbgh.color_depth/8))
						{
							return;
						}
						if(transed_pixel_buffer[i] != 0x00)
						{
							cur = i;
							calc_shift7_num(j);
							//push_not_zero_bytes(j);
							is_comp_zero = false;
							break;
						}
					}
				}
				if(cur == cbgh.height * cbgh.width * (cbgh.color_depth/8))
				{
					return;
				}
			}
		}
		void push_not_zero_bytes(int j)
		{
			for(int i = 0;i<j;i++)
			{
				unsigned char c = q.front();
				q2.push(c);
				q.pop();
			}
			if(!q.empty())
			{
				cout<<"bad!!"<<endl;
			}
		}
		void calc_shift7_num(int j)
		{
			unsigned int b;
			unsigned char c;
			char c2;
			b=j<<24;
			b>>=24;
			if(j<0x80)
			{
				c=b;
				c2=c;
				q2.push(c2);
				return;
			}
			b=b|0x80;
			c=b;
			c2=c;
			q2.push(c2);
			
		//	q2.push(c2);
		//	printf("%2X ",c2);
			j>>=7;
			b=j<<24;
			b>>=24;
			if(j<0x80)
			{
				c=b;
				c2=c;
				q2.push(c2);
				return;
			}
			b=b|0x80;	
			c=b;
			c2=c;
			q2.push(c2);
			
		//	q2.push(c2);
		//	printf("%2X ",c2);
			j>>=7;
			b=j<<24;
			b>>=24;
			if(j<0x80)
			{
				c=b;
				c2=c;
				q2.push(c2);
				return;
			}
			b=b|0x80;	
			c=b;
			c2=c;
			q2.push(c2);
			
			j>>=7;
			b=j<<24;
			b>>=24;
			if(j<0x80)
			{
				c=b;
				c2=c;
				q2.push(c2);
				return;
			}
			b=b|0x80;	
			c=b;
			c2=c;
			q2.push(c2);
			
		//	q2.push(c2);
		//	printf("%2X ",c2);
		//q3.push(0x00);
		//	q2.push(0x00);
		//	printf("0x00 ");
		}
		void trans_pixels(unsigned char *raw_pixel_buffer,unsigned char *transed_pixel_buffer)
		{
			if(cbgh.color_depth/8 == 4)
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
				for(long long i = 1; i< cbgh.width; i++)
				{
					cur = i * (cbgh.color_depth / 8);
					b = raw_pixel_buffer[cur] - raw_pixel_buffer[cur - 4];
					transed_pixel_buffer[cur] = b;

					g = raw_pixel_buffer[cur+1] - raw_pixel_buffer[cur - 3];
					transed_pixel_buffer[cur+1] = g;
					
					r = raw_pixel_buffer[cur+2] - raw_pixel_buffer[cur - 2];
					transed_pixel_buffer[cur+2] = r;
					
					a = raw_pixel_buffer[cur+3] - raw_pixel_buffer[cur - 1];
					transed_pixel_buffer[cur+3] = a;
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
					
					b = raw_pixel_buffer[cur] - raw_pixel_buffer[up_cur];
					transed_pixel_buffer[cur] = b;
					
					g = raw_pixel_buffer[cur+1] - raw_pixel_buffer[up_cur + 1];
					transed_pixel_buffer[cur+1] = g;
					
					r = raw_pixel_buffer[cur+2] - raw_pixel_buffer[up_cur + 2];
					transed_pixel_buffer[cur+2] = r;
					
					a = raw_pixel_buffer[cur+3] - raw_pixel_buffer[up_cur + 3];
					transed_pixel_buffer[cur+3] = a;
					for(long long j = 1; j < cbgh.width; j++)
					{
						
						cur = i * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
						unsigned int up_cur = (i -1) * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
						b = raw_pixel_buffer[cur] - (raw_pixel_buffer[cur - 4] + raw_pixel_buffer[up_cur]) / 2;
						transed_pixel_buffer[cur] = b;
						
						g = raw_pixel_buffer[cur+1] - (raw_pixel_buffer[cur - 3] + raw_pixel_buffer[up_cur+1]) / 2;
						transed_pixel_buffer[cur+1] = g;

						r = raw_pixel_buffer[cur+2] - (raw_pixel_buffer[cur - 2] + raw_pixel_buffer[up_cur+2]) / 2;
						transed_pixel_buffer[cur+2] = r;
						
						a = raw_pixel_buffer[cur+3] - (raw_pixel_buffer[cur - 1] + raw_pixel_buffer[up_cur+3]) / 2;
						transed_pixel_buffer[cur+3] = a;
					}
				}
				//ofstream fout("test7x.tmp",ios::binary);
			//	fout.write((char *)transed_pixel_buffer,cbgh.width * cbgh.height * (cbgh.color_depth / 8));
				return ;
			}
			
			//24 bits.
			transed_pixel_buffer[0] = raw_pixel_buffer[0] + 0;
			transed_pixel_buffer[1] = raw_pixel_buffer[1] + 0;
			transed_pixel_buffer[2] = raw_pixel_buffer[2] + 0;
			//transed_pixel_buffer[3] = raw_pixel_buffer[3] + 0;
			
			unsigned char b = 0;
			unsigned char g = 0;
			unsigned char r = 0;
			//unsigned char a = 0;
			unsigned long long cur = 0;
			for(long long i = 1; i< cbgh.width; i++)
			{
				cur = i * (cbgh.color_depth / 8);
				b = raw_pixel_buffer[cur] - raw_pixel_buffer[cur - 3];
				transed_pixel_buffer[cur] = b;

				g = raw_pixel_buffer[cur+1] - raw_pixel_buffer[cur - 2];
				transed_pixel_buffer[cur+1] = g;
				
				r = raw_pixel_buffer[cur+2] - raw_pixel_buffer[cur - 1];
				transed_pixel_buffer[cur+2] = r;
				
				/*a = raw_pixel_buffer[cur+3] -= raw_pixel_buffer[cur - 1];
				transed_pixel_buffer[cur+3] = a;*/
			}
			b = 0;
			g = 0;
			r = 0;
			//a = 0;
			cur = 0;
			
			for(long long i = 1; i < cbgh.height; i++)
			{
				
				cur = i * cbgh.width * (cbgh.color_depth / 8);
				unsigned int up_cur = (i -1) * cbgh.width * (cbgh.color_depth / 8);
				
				b = raw_pixel_buffer[cur] - raw_pixel_buffer[up_cur];
				transed_pixel_buffer[cur] = b;
				
				g = raw_pixel_buffer[cur+1] - raw_pixel_buffer[up_cur + 1];
				transed_pixel_buffer[cur+1] = g;
				
				r = raw_pixel_buffer[cur+2] - raw_pixel_buffer[up_cur + 2];
				transed_pixel_buffer[cur+2] = r;
				
				/*a = pre_pixel_buffer[cur+3] -= raw_pixel_buffer[up_cur + 3];
				transed_pixel_buffer[cur+3] = a;*/
				for(long long j = 1; j < cbgh.width; j++)
				{
					
					cur = i * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
					unsigned int up_cur = (i -1) * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8);
					b = raw_pixel_buffer[cur] - (raw_pixel_buffer[cur - 3] + raw_pixel_buffer[up_cur]) / 2;
					transed_pixel_buffer[cur] = b;
					
					g = raw_pixel_buffer[cur+1] - (raw_pixel_buffer[cur - 2] + raw_pixel_buffer[up_cur+1]) / 2;
					transed_pixel_buffer[cur+1] = g;

					r = raw_pixel_buffer[cur+2] - (raw_pixel_buffer[cur - 1] + raw_pixel_buffer[up_cur+2]) / 2;
					transed_pixel_buffer[cur+2] = r;
					
					/*a = raw_pixel_buffer[cur+3] -= (raw_pixel_buffer[cur - 1] + raw_pixel_buffer[up_cur+3]) / 2;
					transed_pixel_buffer[cur+3] = a;*/
				}
			}
				//ofstream fout("test5x.tmp",ios::binary);
				//fout.write((char *)pixel_buffer,cbgh.width * cbgh.height * (cbgh.color_depth / 8));
				//return ;
		//	ofstream fout("test7x.tmp",ios::binary);
			//fout.write((char *)transed_pixel_buffer,cbgh.width * cbgh.height * (cbgh.color_depth / 8));
		}
};

int main()
{
	char fileheader[] = "CompressedBG___"; //CompressedBG___
	fout.write(fileheader,15);
	fout.put(0x00);
	char temp=0;
	//The cbg image ONLY support 0xffff (2bytes) max width and height!!
	//fin.get(temp);
	
	for(int i=0;i<2;i++)
	{
		fin.seekg(0x12+i);
		temp=fin.get();
		//write the image width
		fout.put(temp);
		unsigned int temp2 = (unsigned char)temp;
		temp2 <<= 8*i;
		cbgh.width += temp2;
	}
	if(fin.get())
	{
		cout<<"Error. The cbg image only support 0xffff (2bytes) max width and height!!"<<endl;
		getchar();
		exit(0);
	}
	for(int i=0;i<2;i++)
	{
		fin.seekg(0x16+i);
		temp=fin.get();
		//write the image height
		fout.put(temp);
		unsigned int temp2 = (unsigned char)temp;
		temp2 <<= 8*i;
		cbgh.height += temp2;
	}
	if(fin.get())
	{
		cout<<"Error. The cbg image only support 0xffff (2bytes) max width and height!!"<<endl;
		getchar();
		exit(0);
	}
	cout<<cbgh.width<<" "<<cbgh.height<<endl;
	fin.seekg(0x1c);
	temp=fin.get();
	//write the image color depth
	fout.put(temp);
	fout.put(0x00);
	fout.put(0x00);
	fout.put(0x00);
	unsigned int temp2 = (unsigned char)temp;
	cbgh.color_depth = temp2;
	cout<<cbgh.color_depth<<endl;
	fin.seekg(0x36);
	unsigned char *raw_pixel_buffer = new unsigned char[cbgh.height * cbgh.width * (cbgh.color_depth/8)];
	for(long long i = cbgh.height - 1; i >=0; i--)
	{
		for(long long j = 0; j<cbgh.width; j++)
		{
			for(short k = 0; k< cbgh.color_depth/8;k++)
			{
				unsigned int cur = i * cbgh.width * (cbgh.color_depth / 8) + j * (cbgh.color_depth / 8) + k;	
				raw_pixel_buffer[cur] = (unsigned char)fin.get();
			}
		}
	}
	//ofstream fout("test6x.tmp",ios::binary);
	//fout.write((char *)raw_pixel_buffer,cbgh.width * cbgh.height * (cbgh.color_depth / 8));
	
	//call main encode function
	cbg_codec encode;
	encode.encoder(raw_pixel_buffer);
}
