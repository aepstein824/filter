#include "iirfilter.h"
#include "common.h"


iirfilter_t *create_iirfilter (int order, unsigned int type, double cutoff, double cutoff2)
{
  iirfilter_t *newfilter = malloc (sizeof (iirfilter_t));
  if (type == TYPE_BAND || type == TYPE_STOP)
    {
      order -= (order % 2);
    }
  
  order = clamp_order (order, IIR_ORDER_MIN, IIR_ORDER_MAX);
  newfilter -> order = order;
  newfilter -> type = type;
  newfilter -> cutoff = cutoff;
  newfilter -> cutoff2 = cutoff2;
  //order+1 is number of coefficients in orderth degree poly
  newfilter -> in_history = buffer_create (order + 1);
  newfilter -> out_history = buffer_create (order + 1);
  switch (type)
    {
    case TYPE_HIGH:
      create_butterworth_high (order, cutoff, &(newfilter -> num), &(newfilter -> den));
      break;
    case TYPE_BAND:
      create_butterworth_band (order / 2, cutoff, cutoff2, &(newfilter -> num), &(newfilter -> den));
      break;
    case TYPE_STOP:
      create_butterworth_stop (order / 2, cutoff, cutoff2, &(newfilter -> num), &(newfilter -> den));
      break;
    case TYPE_LOW:
    default:
      create_butterworth_low (order, cutoff, &(newfilter -> num), &(newfilter -> den));
      break;
    }
  return newfilter;
}

void destroy_iirfilter (iirfilter_t *system)
{
  free (system -> in_history);
  free (system -> out_history);
  free (system);
}

int clamp_order (int order, int min, int max)
{
  if (order > max) order = max;
  if (order < min) order = min;
  return order;
}

//polynomials should be pre-initialized to zero
//ct cutoff
void create_butterworth_low (int n, double cutoff, poly_t *num, poly_t *den)
{
  polyappend ( num, 1, 0 );
  polyappend ( den, 1, 0 );

  for (int i = 0; i < (2*n); i++){
    double complex pole = cexp ( I * ( PI/2 + PI/(2*n) + PI*i/n ) );
    //only want poles on left side of jw axis
    if ( creal (pole) < 0 )
      {
	poly_t pole_poly;
	poly_t temp;
	//add (w + wr) to numerator
	initpoly (&pole_poly);
	polyappend ( &pole_poly, cutoff, 1 );
      	polyappend ( &pole_poly, cutoff, 0 );
	temp = *num;
	polymul ( &pole_poly, &temp, num );
	
	//add (1 - p*w  - (p*w + 1) * r ) to denominator
	initpoly (&pole_poly);
	polyappend ( &pole_poly, -1 - pole * cutoff, 1 );
      	polyappend ( &pole_poly, 1 - pole * cutoff, 0 );
	temp = *den;
	polymul ( &pole_poly, &temp, den );
      }
  }
}

//polynomials should be pre-initialized to zero
//ct cutoff
void create_butterworth_high (int n, double cutoff, poly_t *num, poly_t *den)
{
  polyappend ( num, 1, 0 );
  polyappend ( den, 1, 0 );

  for (int i = 0; i < (2*n); i++){
    double complex pole = cexp ( I * ( PI/2 + PI/(2*n) + PI*i/n ) );
    //only want poles on left side of jw axis
    if ( creal (pole) < 0 )
      {
	poly_t pole_poly;
	poly_t temp;
	//add (w + wr) to numerator
	initpoly (&pole_poly);
	polyappend ( &pole_poly, 1, 1 );
      	polyappend ( &pole_poly, -1, 0 );
		temp = *num;
	polymul ( &pole_poly, &temp, num );
	
	//add (1 - p*w  - (p*w + 1) * r ) to denominator
	initpoly (&pole_poly);
	polyappend ( &pole_poly, cutoff + pole, 1 );
      	polyappend ( &pole_poly, cutoff - pole, 0 );
	temp = *den;
	polymul ( &pole_poly, &temp, den );
      }
  }
}


//polynomials should be pre-initialized to zero
//ct cutoffs
void create_butterworth_band (int n, double lowcut, double highcut, poly_t *num, poly_t *den)
{
  polyappend ( num, 1, 0 );
  polyappend ( den, 1, 0 );

  double prod = lowcut * highcut;
  double diff = highcut - lowcut;

  for (int i = 0; i < (2*n); i++){
    double complex pole = cexp ( I * ( PI/2 + PI/(2*n) + PI*i/n ) );
    //only want poles on left side of jw axis
    if ( creal (pole) < 0 )
      {
	poly_t pole_poly;
	poly_t temp;
	//add (w + wr) to numerator
	initpoly (&pole_poly);
	polyappend ( &pole_poly, -1 * diff, 2 );
      	polyappend ( &pole_poly, diff, 0 );
	temp = *num;
	polymul ( &pole_poly, &temp, num );
	
	//add (1 - p*w  - (p*w + 1) * r ) to denominator
	initpoly (&pole_poly);
	polyappend ( &pole_poly, 1 + pole * diff + prod, 2 );
	polyappend ( &pole_poly, 2 * prod - 2, 1 );
      	polyappend ( &pole_poly, 1 - pole * diff + prod, 0 );
	temp = *den;
	polymul ( &pole_poly, &temp, den );
      }
  }
}

//polynomials should be pre-initialized to zero
//ct cutoffs
void create_butterworth_stop (int n, double lowcut, double highcut, poly_t *num, poly_t *den)
{
  polyappend ( num, 1, 0 );
  polyappend ( den, 1, 0 );

  double prod = lowcut * highcut;
  double diff = highcut - lowcut;

  for (int i = 0; i < (2*n); i++){
    double complex pole = cexp ( I * ( PI/2 + PI/(2*n) + PI*i/n ) );
    //only want poles on left side of jw axis
    if ( creal (pole) < 0 )
      {
	poly_t pole_poly;
	poly_t temp;
	//add (w + wr) to numerator
	initpoly (&pole_poly);
	polyappend ( &pole_poly, prod + 1, 2 );
      	polyappend ( &pole_poly, 2 * prod - 2, 1 );
	polyappend ( &pole_poly, prod + 1, 0 );
	temp = *num;
	polymul ( &pole_poly, &temp, num );
	
	//add (1 - p*w  - (p*w + 1) * r ) to denominator
	initpoly (&pole_poly);
	polyappend ( &pole_poly, -1 * pole - diff - pole * prod , 2 );
	polyappend ( &pole_poly, 2 * pole - 2 * pole * prod, 1 );
      	polyappend ( &pole_poly, diff - pole - pole * prod, 0 );
	temp = *den;
	polymul ( &pole_poly, &temp, den );
      }
  }
}

double next_sample (iirfilter_t *system, double insample)
{
  double next = 0;

  
  buffer_append_first (system -> in_history, insample);
  
  for ( int i = 0; i <= system -> order; i++ )
    {
      next += creal ( system -> num . terms[i] ) * buffer_peek ( system -> in_history, i );
    }
  
  for ( int i = 1; i <= system -> order; i++ )
    {
      // i - 1 takes into account that the y[n] isn't in the buffer because we're calculating it
      next -= creal ( system -> den . terms[i] ) * buffer_peek ( system -> out_history, i - 1 );
    }
  next = next / system -> den . terms[0];

  buffer_append_first (system -> out_history, next);
  
  return next;
}
