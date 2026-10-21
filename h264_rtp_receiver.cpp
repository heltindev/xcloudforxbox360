#include "h264_rtp_receiver.h"
#include "rtp_packet.h"
#include <string.h>

namespace h264_rtp {
namespace {
const unsigned char START_CODE[4]={0,0,0,1};
unsigned short read_be16(const unsigned char* p) {
    return (unsigned short)(((unsigned int)p[0]<<8)|p[1]);
}
}
Receiver::Receiver() { reset(); }
void Receiver::reset() {
    active_=false;
    fragmented_=false;
    need_idr_=true;
    has_idr_=false;
    have_seq_=false;
    have_ssrc_=false;
    next_seq_=0;
    ssrc_=0;
    timestamp_=0;
    used_=0;
    memset(&stats_,0,sizeof(stats_));
}
bool Receiver::append(const unsigned char* bytes,size_t len) {
    if(!bytes || len>ACCESS_UNIT_CAPACITY-used_) return false;
    if(len)memcpy(data_+used_,bytes,len);
    used_+=len;
    return true;
}
bool Receiver::append_nal(const unsigned char* bytes,size_t len) {
    if(len<1 || used_>ACCESS_UNIT_CAPACITY-4 ||
       len>ACCESS_UNIT_CAPACITY-used_-4) return false;
    const unsigned int type=bytes[0]&31u;
    if(type==0 || type>=24u)return false;
    if(type==5)has_idr_=true;
    return append(START_CODE,4) && append(bytes,len);
}
void Receiver::abandon_frame() {
    if(active_ && used_) ++stats_.dropped;
    active_=false;
    fragmented_=false;
    has_idr_=false;
    used_=0;
    need_idr_=true;
}
PushResult Receiver::push(const unsigned char* data,size_t len,Frame* done) {
    if(done) {
        done->annex_b=NULL;
        done->size=0;
        done->rtp_timestamp=0;
        done->contains_idr=false;
    }
    if(!done || !data) {
        ++stats_.invalid;
        return PACKET_INVALID;
    }
    rtp::Packet pkt;
    if(!rtp::parse(data,len,&pkt)) {
        ++stats_.invalid;
        return PACKET_INVALID;
    }
    if(pkt.payload_size==0) return WAITING; // RTP padding-only probe.
    const unsigned char* payload=data+pkt.payload_offset;
    const size_t bytes=pkt.payload_size;

    if(have_ssrc_ && pkt.ssrc!=ssrc_) {
        abandon_frame();
        have_seq_=false;
    }
    ssrc_=pkt.ssrc;
    have_ssrc_=true;
    if(have_seq_ && pkt.sequence!=next_seq_) {
        // Reordering and NACK are not implemented in this first receiver.
        // Never emit a prediction frame after a missing packet.
        abandon_frame();
    }
    next_seq_=(unsigned short)(pkt.sequence+1);
    have_seq_=true;

    if(active_ && pkt.timestamp!=timestamp_) {
        // Missing RTP marker, premature frame switch or lost last packet.
        abandon_frame();
    }
    if(!active_) {
        active_=true;
        fragmented_=false;
        timestamp_=pkt.timestamp;
        used_=0;
        has_idr_=false;
    }

    bool ok=true;
    const unsigned int kind=payload[0]&31u;
    if(kind>=1 && kind<=23) {
        if(fragmented_) ok=false;
        else ok=append_nal(payload,bytes);
    } else if(kind==24) { // STAP-A: length-prefixed NAL units
        if(fragmented_) ok=false;
        size_t cursor=1;
        unsigned int count=0;
        while(ok && cursor<bytes) {
            if(bytes-cursor<2) {ok=false;break;}
            const size_t segment=read_be16(payload+cursor);
            cursor+=2;
            if(!segment || segment>bytes-cursor) {ok=false;break;}
            ok=append_nal(payload+cursor,segment);
            cursor+=segment;
            ++count;
        }
        if(count==0)ok=false;
    } else if(kind==28) { // FU-A fragments
        if(bytes<3) ok=false;
        else {
            const unsigned char fu_header=payload[1];
            const bool first=(fu_header&0x80)!=0;
            const bool last=(fu_header&0x40)!=0;
            const unsigned int nal_type=fu_header&31u;
            if(nal_type<1 || nal_type>23 || (first && last))ok=false;
            else if(first) {
                if(fragmented_)ok=false;
                else {
                    const unsigned char reconstructed=
                        (unsigned char)((payload[0]&0xe0)|nal_type);
                    if(nal_type==5)has_idr_=true;
                    ok=append(START_CODE,4) &&
                       append(&reconstructed,1) &&
                       append(payload+2,bytes-2);
                    fragmented_=ok;
                }
            } else if(!fragmented_)ok=false;
            else {
                ok=append(payload+2,bytes-2);
                if(ok && last)fragmented_=false;
            }
        }
    } else ok=false; // Unsupported packetization types.

    if(!ok) {
        ++stats_.invalid;
        abandon_frame();
        return PACKET_INVALID;
    }
    ++stats_.accepted;
    if(!pkt.marker)return WAITING;
    if(fragmented_ || used_==0) {
        abandon_frame();
        return NEED_KEYFRAME;
    }
    if(need_idr_ && !has_idr_) {
        // Ignore SPS/PPS-only and predicted video until a complete IDR.
        abandon_frame();
        return NEED_KEYFRAME;
    }
    need_idr_=false;
    done->annex_b=data_;
    done->size=used_;
    done->rtp_timestamp=timestamp_;
    done->contains_idr=has_idr_;
    ++stats_.completed;
    active_=false;
    has_idr_=false;
    used_=0;
    return ACCESS_UNIT;
}
}
