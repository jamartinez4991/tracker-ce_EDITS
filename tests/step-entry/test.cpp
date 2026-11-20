#include "../../core/source/rtQuantize.h"
#include <cstdio>
// Mirrors stepEntryDelta() + stepEntryMicromoveFor(): walk 24 entries per division, check step, micromove.
int main(){ int fails=0;
 for(int div=0;div<5;div++){ int32_t micro=0; int step=0; printf("%s:",rtq::kDivisionLabel[div]);
  for(int i=0;i<24;i++){
    int32_t exact=i*rtq::kIntervalMicro[div];   // ideal absolute microstep position of entry i
    int expStep=exact/48, expFrac=exact%48;
    int expMove = expFrac<=0 ? -1 : (expFrac*100+23)/47;
    if(i>0){ micro=rtq::nextEntryPosition(micro,div,step); step=micro/48; }
    int frac=micro%48; int mv = frac<=0 ? -1 : (frac*100+23)/47;
    if(step!=expStep||mv!=expMove){printf(" FAIL i=%d step %d/%d mv %d/%d\n",i,step,expStep,mv,expMove);fails++;}
    if(i<7) printf(" %d.%d",step,mv<0?0:mv);
  } printf("\n"); }
 printf(fails?"FAILED\n":"ALL OK\n"); return fails; }
