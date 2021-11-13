#include "soundchip.h"
#include <string.h>

#ifndef M_PI
#define M_PI 3.141592653589793238
#endif

#define minval(a,b) (((a)<(b))?(a):(b))
#define maxval(a,b) (((a)>(b))?(a):(b))

void soundchip::NextSample(short* l, short* r) {
  for (int i=0; i<8; i++) {
    if (chan[i].vol==0 && !chan[i].flags.swvol) {fns[i]=0; continue;}
    if (chan[i].flags.pcm) {
      ns[i]=pcm[chan[i].pcmpos];
    } else switch (chan[i].flags.shape) {
      case 0:
        ns[i]=(((cycle[i]>>15)&127)>chan[i].duty)*127;
        break;
      case 1:
        ns[i]=cycle[i]>>14;
        break;
      case 2:
        ns[i]=SCsine[(cycle[i]>>14)&255];
        break;
      case 3:
        ns[i]=SCtriangle[(cycle[i]>>14)&255];
        break;
      case 4: case 5:
        ns[i]=(lfsr[i]&1)*127;
        break;
      case 6:
        ns[i]=((((cycle[i]>>15)&127)>chan[i].duty)*127)^(short)SCsine[(cycle[i]>>14)&255];
        break;
      case 7:
        ns[i]=((((cycle[i]>>15)&127)>chan[i].duty)*127)^(short)SCtriangle[(cycle[i]>>14)&255];
        break;
    }
    
    if (chan[i].flags.pcm) {
      pcmdec[i]+=chan[i].freq;
      if (pcmdec[i]>32767) {
        pcmdec[i]-=32767;
        if (chan[i].pcmpos<chan[i].pcmbnd) {
          chan[i].pcmpos++;
          chan[i].wc++;
          if (chan[i].pcmpos==chan[i].pcmbnd) {
            if (chan[i].flags.pcmloop) {
              chan[i].pcmpos=chan[i].pcmrst;
            }
          }
          chan[i].pcmpos&=(SOUNDCHIP_PCM_SIZE-1);
        } else if (chan[i].flags.pcmloop) {
          chan[i].pcmpos=chan[i].pcmrst;
        }
      }
    } else {
      ocycle[i]=cycle[i];
      if (chan[i].flags.shape==5) {
        switch ((chan[i].duty>>4)&3) {
          case 0:
            cycle[i]+=chan[i].freq*1-(chan[i].freq>>3);
            break;
          case 1:
            cycle[i]+=chan[i].freq*2-(chan[i].freq>>3);
            break;
          case 2:
            cycle[i]+=chan[i].freq*4-(chan[i].freq>>3);
            break;
          case 3:
            cycle[i]+=chan[i].freq*8-(chan[i].freq>>3);
            break;
        }
      } else {
        cycle[i]+=chan[i].freq;
      }
      if ((cycle[i]&0xf80000)!=(ocycle[i]&0xf80000)) {
        if (chan[i].flags.shape==4) {
          lfsr[i]=(lfsr[i]>>1|(((lfsr[i]) ^ (lfsr[i] >> 2) ^ (lfsr[i] >> 3) ^ (lfsr[i] >> 5) ) & 1)<<31);
        } else {
          switch ((chan[i].duty>>4)&3) {
            case 0:
              lfsr[i]=(lfsr[i]>>1|(((lfsr[i] >> 3) ^ (lfsr[i] >> 4) ) & 1)<<5);
              break;
            case 1:
              lfsr[i]=(lfsr[i]>>1|(((lfsr[i] >> 2) ^ (lfsr[i] >> 3) ) & 1)<<5);
              break;
            case 2:
              lfsr[i]=(lfsr[i]>>1|(((lfsr[i]) ^ (lfsr[i] >> 2) ^ (lfsr[i] >> 3) ) & 1)<<5);
              break;
            case 3:
              lfsr[i]=(lfsr[i]>>1|(((lfsr[i]) ^ (lfsr[i] >> 2) ^ (lfsr[i] >> 3) ^ (lfsr[i] >> 5) ) & 1)<<5);
              break;
          }
          if ((lfsr[i]&63)==0) {
            lfsr[i]=0xaaaa;
          }
        }
      }
      if (chan[i].flags.restim) {
        if (--rcycle[i]<=0) {
          cycle[i]=0;
          rcycle[i]=chan[i].restimer;
          lfsr[i]=0xaaaa;
        }
      }
    }
    //ns[i]=(char)((short)ns[i]*(short)vol[i]/256);
    fns[i]=ns[i]*chan[i].vol*2;
    if (chan[i].flags.fmode!=0) {
      int ff=chan[i].cutoff;
      nslow[i]=nslow[i]+(((ff)*nsband[i])>>16);
      nshigh[i]=fns[i]-nslow[i]-(((256-chan[i].reson)*nsband[i])>>8);
      nsband[i]=(((ff)*nshigh[i])>>16)+nsband[i];
      fns[i]=(((chan[i].flags.fmode&1)?(nslow[i]):(0))+((chan[i].flags.fmode&2)?(nshigh[i]):(0))+((chan[i].flags.fmode&4)?(nsband[i]):(0)));
    }
    nsL[i]=(fns[i]*SCpantabL[(unsigned char)chan[i].pan])>>8;
    nsR[i]=(fns[i]*SCpantabR[(unsigned char)chan[i].pan])>>8;
    oldfreq[i]=chan[i].freq;
    oldflags[i]=chan[i].flags.flags;
    if (chan[i].flags.swvol) {
      if (--swvolt[i]<=0) {
        swvolt[i]=chan[i].swvol.speed;
        if (chan[i].swvol.dir) {
          chan[i].vol+=chan[i].swvol.amt;
          if (chan[i].vol>chan[i].swvol.bound && !chan[i].swvol.loop) {
            chan[i].vol=chan[i].swvol.bound;
          }
          if (chan[i].vol&0x80) {
            if (chan[i].swvol.loop) {
              if (chan[i].swvol.loopi) {
                chan[i].swvol.dir=!chan[i].swvol.dir;
                chan[i].vol=0xff-chan[i].vol;
              } else {
                chan[i].vol&=~0x80;
              }
            } else {
              chan[i].vol=0x7f;
            }
          }
        } else {
          chan[i].vol-=chan[i].swvol.amt;
          if (chan[i].vol&0x80) {
            if (chan[i].swvol.loop) {
              if (chan[i].swvol.loopi) {
                chan[i].swvol.dir=!chan[i].swvol.dir;
                chan[i].vol=-chan[i].vol;
              } else {
                chan[i].vol&=~0x80;
              }
            } else {
              chan[i].vol=0x0;
            }
          }
          if (chan[i].vol<chan[i].swvol.bound && !chan[i].swvol.loop) {
            chan[i].vol=chan[i].swvol.bound;
          }
        }
      }
    }
    if (chan[i].flags.swfreq) {
      if (--swfreqt[i]<=0) {
        swfreqt[i]=chan[i].swfreq.speed;
        if (chan[i].swfreq.dir) {
          if (chan[i].freq>(0xffff-chan[i].swfreq.amt)) {
            chan[i].freq=0xffff;
          } else {
            chan[i].freq=(chan[i].freq*(0x80+chan[i].swfreq.amt))>>7;
            if ((chan[i].freq>>8)>chan[i].swfreq.bound) {
              chan[i].freq=chan[i].swfreq.bound<<8;
            }
          }
        } else {
          if (chan[i].freq<chan[i].swfreq.amt) {
            chan[i].freq=0;
          } else {
            chan[i].freq=(chan[i].freq*(0xff-chan[i].swfreq.amt))>>8;
            if ((chan[i].freq>>8)<chan[i].swfreq.bound) {
              chan[i].freq=chan[i].swfreq.bound<<8;
            }
          }
        }
      }
    }
    if (chan[i].flags.swcut) {
      if (--swcutt[i]<=0) {
        swcutt[i]=chan[i].swcut.speed;
        if (chan[i].swcut.dir) {
          if (chan[i].cutoff>(0xffff-chan[i].swcut.amt)) {
            chan[i].cutoff=0xffff;
          } else {
            chan[i].cutoff+=chan[i].swcut.amt;
            if ((chan[i].cutoff>>8)>chan[i].swcut.bound) {
              chan[i].cutoff=chan[i].swcut.bound<<8;
            }
          }
        } else {
          if (chan[i].cutoff<chan[i].swcut.amt) {
            chan[i].cutoff=0;
          } else {
            chan[i].cutoff=((2048-(unsigned int)chan[i].swcut.amt)*(unsigned int)chan[i].cutoff)>>11;
            if ((chan[i].cutoff>>8)<chan[i].swcut.bound) {
              chan[i].cutoff=chan[i].swcut.bound<<8;
            }
          }
        }
      }
    }
    if (chan[i].flags.resosc) {
      cycle[i]=0;
      rcycle[i]=chan[i].restimer;
      ocycle[i]=0;
      chan[i].flags.resosc=0;
    }
  }
  tnsL=(nsL[0]+nsL[1]+nsL[2]+nsL[3]+nsL[4]+nsL[5]+nsL[6]+nsL[7])>>2;///256;
  tnsR=(nsR[0]+nsR[1]+nsR[2]+nsR[3]+nsR[4]+nsR[5]+nsR[6]+nsR[7])>>2;///256;
  
  *l=minval(32767,maxval(-32767,tnsL));//(2047*(pnsL+tnsL-ppsL))>>11;
  *r=minval(32767,maxval(-32767,tnsR));//(2047*(pnsR+tnsR-ppsR))>>11;
}

void soundchip::Init() {
  Reset();
  memset(pcm,0,SOUNDCHIP_PCM_SIZE);
  for (int i=0; i<256; i++) {
    SCsine[i]=sin((i/128.0f)*M_PI)*127;
    SCtriangle[i]=(i>127)?(255-i):(i);
    SCpantabL[i]=127;
    SCpantabR[i]=127;
  }
  for (int i=0; i<128; i++) {
    SCpantabL[i]=127-i;
    SCpantabR[128+i]=i-1;
  }
  SCpantabR[128]=0;
}

void soundchip::Reset() {
  for (int i=0; i<8; i++) {
    cycle[i]=0;
    resetfreq[i]=0;
    //duty[i]=64;
    //pan[i]=0;
    //cut[i]=0;
    //res[i]=0;
    //flags[i]=0;
    //postvol[i]=0;
    voldcycles[i]=0;
    volicycles[i]=0;
    fscycles[i]=0;
    sweep[i]=0;
    ns[i]=0;
    swvolt[i]=1;
    swfreqt[i]=1;
    swcutt[i]=1;
    lfsr[i]=0xaaaa;
  }
  memset(chan,0,sizeof(channel)*8);
}

soundchip::soundchip() {
  Init();
}
