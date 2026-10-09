#include <Telemetry/GaugeLayout.h>
#include <Telemetry/SpeedUnits.h>
#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>
using telemetry::gauge::NeedleSmoother;
using telemetry::gauge::steadyIntegerSpeedScaled;
static bool close(float a,float b,float e=.005f){return std::fabs(a-b)<e;}
int main(){
  NeedleSmoother s;
  assert(close(s.observe(15, .016f, .80f),15));
  // A sustained cruise should resist back/forth velocity samples.
  float lo=100,hi=-100; for(int i=0;i<180;i++) {
    float a=15.0f+((i%2)?0.45f:-0.45f);
    float n=s.observe(a,.016f,.80f);
    if(i>70){lo=std::min(lo,n);hi=std::max(hi,n);}
  }
  assert(hi-lo<0.40f);
  // Real braking to zero must animate through intermediate values rather than jump.
  float first=s.observe(0,.016f,.80f);
  assert(first>0.1f && first<15);
  for(int i=0;i<24;i++)s.observe(0,.016f,.80f);
  assert(s.displayed<3.2f && s.displayed>0);
  for(int i=0;i<130;i++)s.observe(0,.016f,.80f);
  assert(close(s.displayed,0));
  // Real acceleration should not be delayed by 0.8sec cruising filter.
  s.reset();s.observe(0,.016f,.80f);
  for(int i=0;i<20;i++)s.observe(14.0f*(i+1)/20.0f,.016f,.80f);
  assert(s.displayed>6.0f);
  std::cout<<"accelerating after 0.32s: "<<s.displayed<<" U/S\n";
  // Disabled smoothing should copy the actual measured value.
  assert(close(s.observe(14,.016f,0),14));
  assert(close(s.observe(0,.016f,0),0));
  // All labels need consistent physical-speed deadband despite unit scaling.
  for(int u=0;u<4;u++) {
    float f=telemetry::speedunits::factor(u,1);
    const int start=static_cast<int>(std::lround(14.0f*f));
    int shown=start;
    // Fluctuation at steady top speed should not strobe adjacent digits.
    for(int i=0;i<120;i++){
      float val=(14.0f+((i&1)?0.12f:-0.12f))*f;
      shown=steadyIntegerSpeedScaled(val,shown,f);
      assert(shown==start);
    }
    int shifted=steadyIntegerSpeedScaled(16.0f*f,shown,f);
    assert(shifted!=shown);
  }
  // Calibration flag is presentation state, never changes the conversion factor.
  assert(close(telemetry::speedunits::convert(14,1,1),50.4f));
  assert(close(telemetry::speedunits::convert(14,2,1),31.317108f,.0005f));
  std::cout<<"ALL UNIT/NEEDLE REGRESSIONS PASSED\n";
}
