#include "ThreebandSplitter.h"
#include "iirfilter.h"

const int    splitterOrder = 2;
const int    dcCutterOrder = 2;
const double dcCutterHz = 1.;
const int    avcOrder = 2;
const double avcHz = .1;
const int    levelOrder = 4;
const double levelsHz = 7;

ThreebandSplitter *createSplitter (double lm, double mh)
{
  ThreebandSplitter *t = malloc (sizeof(ThreebandSplitter));
  t -> lowMidCutoff = lm;
  t -> midHighCutoff = mh;
  return t;
}

void calculateFilters (ThreebandSplitter *t)
{
  double lm = t -> lowMidCutoff;
  double mh = t -> midHighCutoff;
  double sr = t -> sampleRate;
  t -> dcCutter = create_iirfilter (dcCutterOrder, TYPE_HIGH,
				    bilinearDigitalFreq (dcCutterHz, sr), 0.);
  t -> avcFilter = create_iirfilter (avcOrder, TYPE_LOW,
				     bilinearDigitalFreq (avcHz, sr), 0.);
  t -> lowFilter = create_iirfilter (splitterOrder, TYPE_LOW,
				     bilinearDigitalFreq (lm, sr), 0.);
  t -> midFilter = create_iirfilter (splitterOrder, TYPE_BAND,
				     bilinearDigitalFreq (lm, sr),
				     bilinearDigitalFreq (mh, sr));
  t -> highFilter = create_iirfilter (splitterOrder, TYPE_HIGH,
				      bilinearDigitalFreq (mh, sr), 0.0);
  for (int i = 0; i < 3; i++)
    {
      t ->levelFilters [i] = create_iirfilter (levelOrder, TYPE_LOW,
						 bilinearDigitalFreq (levelsHz, sr), 0.0);
    }  
}

void destroySplitter (ThreebandSplitter *t)
{
  /*destroy_iirfilter (dccutter);
  destroy_iirfilter (avcfilter);
  destroy_iirfilter (lowfilter);
  destroy_iirfilter (midfilter);
  destroy_iirfilter (highfilter);
  for (int i = 0; i < 3; i++)
    {
      destroy_iirfilter (brightnessFilters [i]);
      }*/
}
