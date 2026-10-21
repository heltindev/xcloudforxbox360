#include "h264_nal.h"

namespace h264 {
namespace {
size_t prefix(const unsigned char* data,size_t size,size_t i) {
    if(i+3<=size && data[i]==0 && data[i+1]==0 && data[i+2]==1)return 3;
    if(i+4<=size && data[i]==0 && data[i+1]==0 &&
       data[i+2]==0 && data[i+3]==1)return 4;
    return 0;
}
}
bool next_nal(const unsigned char* data,size_t size,size_t* cursor,NalUnit* out) {
    if(!data||!cursor||!out||*cursor>=size)return false;
    size_t at=*cursor,start=size,begin=size;
    while(at+3<=size) {
        const size_t n=prefix(data,size,at);
        if(n){start=at;begin=at+n;break;}
        ++at;
    }
    if(start==size||begin>=size){*cursor=size;return false;}
    at=begin;
    while(at+3<=size && !prefix(data,size,at))++at;
    const size_t end=at+3>size?size:at;
    if(end<=begin){*cursor=size;return false;}
    NalUnit unit;
    unit.offset=begin;
    unit.length=end-begin;
    unit.nal_type=data[begin]&31;
    unit.ref_idc=(data[begin]>>5)&3;
    if(unit.nal_type==0 || unit.nal_type>23) { *cursor=end;return false; }
    *out=unit;
    *cursor=end;
    return true;
}
}
