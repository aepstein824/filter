#ifndef THREEBAND_SPLITTER_H
#define THREEBAND_SPLITTER_H

extern const int splitterOrder;
extern const int dcCutterOrder;
extern const double dcCutterHz;
extern const int avcOrder;
extern const double avcHz;
extern const int levelOrder;
extern const double levelsHz;

typedef struct iirfilter iirfilter_t;

typedef struct ThreebandSplitter_t
{
  double lowMidCutoff; //hz
  double midHighCutoff; //hz
  
  int sampleRate;
  
  iirfilter_t *dcCutter;
  iirfilter_t *avcFilter;
  iirfilter_t *lowFilter, *midFilter, *highFilter;
  iirfilter_t *levelFilters[3];
  double avc;
  double levels[3];
} ThreebandSplitter;

ThreebandSplitter *createSplitter (double lm, double mh);
void destroySplitter (ThreebandSplitter *t);

//sampling rate should be set before calling
void calculateFilters (ThreebandSplitter *t);


#endif
