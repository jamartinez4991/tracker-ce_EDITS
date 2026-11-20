#include "../../core/source/condChance.h"
#include <cstdio>
#include <cstdlib>
using namespace condChance;
int fails=0;
#define CHECK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
int main(){
  // every value decodes; 0..100 are pure percentages, 101..172 counted, no gaps, no gate tiers
  for(int v=0; v<=kMaxValue; v++){ Decoded d=decode(v);
    if(v<=100){ CHECK(!d.counted); CHECK(d.pct==v); }
    else { CHECK(d.counted); CHECK(d.pct==100); CHECK(d.n>=1&&d.n<=6&&d.m>=1&&d.m<=6);
           CHECK(encodeCounted(d.playFirst,d.n,d.m)==v); CHECK(d.playFirst==(v<kSkipPlayStart)); } }
  // all 72 combos: simulate 4 full cycles, exact sequence for both arrangements, independently
  int combos=0;
  for(int pf=0;pf<2;pf++) for(int n=1;n<=6;n++) for(int m=1;m<=6;m++){
    uint8_t v=encodeCounted(pf==0,n,m); Decoded d=decode(v); uint8_t ph=0; combos++;
    CHECK(d.n==n&&d.m==m&&d.playFirst==(pf==0));
    for(int i=0;i<4*(n+m);i++){ int k=i%(n+m); bool exp = pf==0 ? (k<n) : (k>=n); CHECK(countedPasses(d,ph)==exp); }
    CHECK(ph==0);
    // gate never blocks a counted condition
    for(int r=0;r<100;r++) CHECK(gatePasses(d,r));
  }
  CHECK(combos==72);
  // pure percentage thresholds
  Decoded d=decode(0); CHECK(!gatePasses(d,0)); d=decode(100); CHECK(gatePasses(d,99));
  d=decode(37); CHECK(gatePasses(d,36)); CHECK(!gatePasses(d,37));
  srand(1); for(int pct=10;pct<=90;pct+=40){ Decoded g=decode(pct); int hit=0; for(int i=0;i<100000;i++) hit+=gatePasses(g,rand()%100); printf("%d%% -> %.1f%%\n",pct,hit/1000.0);}
  uint8_t ph=7; Decoded e=decode(encodeCounted(true,1,1)); (void)countedPasses(e,ph); CHECK(ph==1);
  printf(fails?"FAILED %d\n":"ALL OK\n",fails); return fails?1:0; }
