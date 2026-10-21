#include "controller_input.h"
#include <xtl.h>

namespace controller_input {
bool read_port(unsigned int port,Snapshot* result) {
    if(!result || port>=4)return false;
    XINPUT_STATE state;
    ZeroMemory(&state,sizeof(state));
    if(XInputGetState(port,&state)!=ERROR_SUCCESS)return false;
    Snapshot snap;
    snap.port=port;
    snap.packet_number=state.dwPacketNumber;
    snap.buttons=state.Gamepad.wButtons;
    snap.left_trigger=state.Gamepad.bLeftTrigger;
    snap.right_trigger=state.Gamepad.bRightTrigger;
    snap.lx=state.Gamepad.sThumbLX;
    snap.ly=state.Gamepad.sThumbLY;
    snap.rx=state.Gamepad.sThumbRX;
    snap.ry=state.Gamepad.sThumbRY;
    *result=snap;
    return true;
}
bool read_first(Snapshot* result) {
    if(!result)return false;
    for(unsigned int i=0;i<4;++i)
        if(read_port(i,result))return true;
    return false;
}
float normalize_axis(short raw,unsigned int deadzone) {
    const int value=(int)raw;
    const int absval=value<0?-value:value;
    if(deadzone>32767u)deadzone=32767u;
    if(absval<=(int)deadzone)return 0.f;
    const int validRange=32767-(int)deadzone;
    if(validRange<=0)return value<0?-1.f:1.f;
    float x=(float)(absval-(int)deadzone)/(float)validRange;
    if(x>1.f)x=1.f;
    return value<0?-x:x;
}
}
