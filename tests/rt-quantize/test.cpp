#include "../../core/source/rtQuantize.h"
#include <cstdio>
using namespace rtq;
int fails=0;
#define CHECK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
int main(){
  // intervals
  CHECK(interval(0)==384&&interval(1)==192&&interval(2)==128&&interval(3)==96&&interval(4)==64&&interval(5)==48);
  // all intervals divide the measure
  for(int d=0;d<6;d++) CHECK(768%interval(d)==0);
  // nearest grid, ties forward
  CHECK(nearestGrid(0,48)==0); CHECK(nearestGrid(23,48)==0); CHECK(nearestGrid(24,48)==48); CHECK(nearestGrid(47,48)==48);
  CHECK(nearestGrid(191,192)==192); CHECK(nearestGrid(95,192)==0); CHECK(nearestGrid(96,192)==192);
  CHECK(nearestGrid(63,128)==0); CHECK(nearestGrid(64,128)==128);
  CHECK(nearestGrid(31,64)==0); CHECK(nearestGrid(32,64)==64);
  // across measure boundaries (absolute)
  CHECK(nearestGrid(768+10,192)==768); CHECK(nearestGrid(768*2-10,192)==768*2);
  CHECK(nearestGrid(768+100,128)==768+128);
  // strength
  for(int P=0;P<1536;P++) for(int d=0;d<6;d++){
    CHECK(quantize(P,d,0)==P);
    CHECK(quantize(P,d,100)==nearestGrid(P,interval(d)));
    int q50=quantize(P,d,50); int pq=nearestGrid(P,interval(d));
    CHECK((q50>=P&&q50<=pq)||(q50<=P&&q50>=pq));
    int prev=P; int dir = pq>=P?1:-1;
    for(int s=0;s<=100;s++){ int r=quantize(P,d,s); CHECK((r-prev)*dir>=0); prev=r; }
  }
  CHECK(quantize(10,5,50)==10-(10)/2); // 48 grid, P=10 -> Pq=0 delta -10 -> -5
  CHECK(quantize(21,5,50)==21-11); // -21*0.5=-10.5 -> away from zero -> -11
  CHECK(quantize(27,5,50)==27+11); // +21*0.5=10.5 -> +11
  // micromove mapping round trip
  CHECK(toMicromove(0)==0); CHECK(toMicromove(47)==100);
  for(int m=0;m<48;m++){ int mm=toMicromove(m); CHECK(mm>=0&&mm<=100); }
  // step entry
  CHECK(nextEntryPosition(0,5,0)==48);
  // quarter triplet from step 0: 128 (step2), 256 (step5), 384 (step8), 512(step10), 640 (13), 768(16)
  int32_t p=0; int steps[6]; for(int i=0;i<6;i++){ p=nextEntryPosition(p,2,p/48); steps[i]=p/48; }
  CHECK(steps[0]==2&&steps[1]==5&&steps[2]==8&&steps[3]==10&&steps[4]==13&&steps[5]==16);
  // eighth triplet: 64 (1), 128(2), 192(4)
  p=0; p=nextEntryPosition(p,4,0); CHECK(p/48==1); p=nextEntryPosition(p,4,p/48); CHECK(p/48==2); p=nextEntryPosition(p,4,p/48); CHECK(p/48==4);
  // half note: 8 steps
  p=0; for(int i=1;i<=4;i++){ p=nextEntryPosition(p,0,p/48); CHECK(p/48==i*8);} 
  // manual cursor move to step 3 with quarter: next grid 192 -> step 4
  CHECK(nextEntryPosition(3*48,1,3)/48==4);
  printf(fails?"FAILED %d\n":"ALL OK\n",fails); return fails?1:0; }
