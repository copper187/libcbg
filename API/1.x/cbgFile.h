#ifndef CBGFILE_H
#define CBGFILE_H

#include <fstream>
#include "cbg.h"

int cbg_writefile(char* filename, unsigned char* buffer, unsigned long long buffersize)
{
	std::ofstream fout(filename, ios::binary);
	fout.write((char*)buffer, buffersize);
	return 0;
}
#endif
