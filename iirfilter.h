#ifndef IIRFILTER_INCLUDE
#define IIRFILTER_INCLUDE true

#include "ringbuffer.h"
#include "polynomial.h"

#define TYPE_LOW 0
#define TYPE_HIGH 1
#define TYPE_BAND 2
#define TYPE_STOP 3

#define IIR_ORDER_MIN 1
#define IIR_ORDER_MAX 10

extern const double PI;

typedef struct iirfilter
{
  /**
   * refers to one of the FILTER_ defines.
   */
  int type;
  /**
   * refers to the order of the denominator polynomial in R
   *  for band and stop filters, this doesn't necessarily
   *  match the number of poles
   */
  int order;
  /**
   * the cutoff in Hz of the filter
   * in band and stop filters, this is the lower cutoff
   */
  double cutoff;
  /** the higher cutoff in band and stop filters */
  double cutoff2;
  /**the numerator polynomial in R */
  poly_t num;
  /** denominator polynomial in R */
  poly_t den;
  /** in history */
  ringbuffer_t *in_history;
  /** output history */
  ringbuffer_t *out_history;
} iirfilter_t;

iirfilter_t *create_iirfilter (int order, unsigned int type, double cutoff, double cutoff2);
void destroy_iirfilter (iirfilter_t *victim);

double bilinearDigitalFreq (double freq, int samplingRate);

int clamp_order (int order, int min, int max);

//must use analog cutoff, because this code doesn't know about jack sampling rates
void create_butterworth_low (int order, double cutoff, poly_t *num, poly_t *den);
void create_butterworth_high (int order, double cutoff, poly_t *num, poly_t *den);
void create_butterworth_band (int order, double lowcut, double highcut, poly_t *num, poly_t *den);
void create_butterworth_stop (int order, double lowcut, double highcut, poly_t *num, poly_t *den);

double next_sample (iirfilter_t *system, double insample);

#endif //IIRILTER_INCLUDE
