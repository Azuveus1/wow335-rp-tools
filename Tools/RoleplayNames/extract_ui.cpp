#include <cstdint>
#include <cstdio>
#include <vector>
#include "libmpq/mpq.h"
int main(int argc,char** argv) {
 if(argc!=3)return 2;mpq_archive_s* a=nullptr;
 if(libmpq__archive_open(&a,argv[1],0))return 3;
 uint32_t n;libmpq__off_t size,read;
 if(libmpq__file_number(a,argv[2],&n)){libmpq__archive_close(a);return 4;}
 libmpq__file_size_unpacked(a,n,&size);std::vector<uint8_t> data(size);
 int result=libmpq__file_read(a,n,data.data(),size,&read);
 if(!result)fwrite(data.data(),1,read,stdout);libmpq__archive_close(a);return result?5:0;
}
