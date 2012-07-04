#ifndef POLYNOMIAL_INCLUDE
#define POLYNOMIAL_INCLUDE

#include <complex.h>

#define POLYNOMIAL_MAX_TERMS 20

typedef struct polynomial
{
  double complex terms[POLYNOMIAL_MAX_TERMS];
} poly_t;

void initpoly (poly_t *poly);
void polyappend (poly_t *poly, double complex coeff, int exp);
void polyadd (poly_t *a, poly_t *b, poly_t *sum);
void polymul (poly_t *a, poly_t *b, poly_t *prod);
void display (poly_t *poly);

#endif //POLYNOMIAL_INCLUDE
