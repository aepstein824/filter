#include "polynomial.h"
#include <stdio.h>

void initpoly (poly_t *poly)
{
  poly -> maxExp = 0;
  for (int i = 0; i < POLYNOMIAL_MAX_TERMS; i++)
    {
      poly->terms[i] = 0;
    }
}

void polyappend (poly_t *poly, double complex coeff, int exp)
{
  if (exp > poly -> maxExp)
    poly -> maxExp = exp;
  poly -> terms[exp] = coeff;
}

void polyadd (poly_t *a, poly_t *b, poly_t *sum)
{
  initpoly (sum);
  sum -> maxExp = a -> maxExp > b -> maxExp ? a -> maxExp : b -> maxExp;
  for (int i = 0; i < POLYNOMIAL_MAX_TERMS; i++)
    {
      sum->terms[i] = a->terms[i] + b->terms[i];
    }
}

void polymul (poly_t *a, poly_t *b, poly_t *prod)
{
  initpoly (prod);
  prod -> maxExp = a -> maxExp + b -> maxExp;
  if (prod -> maxExp > POLYNOMIAL_MAX_TERMS - 1)
    {
      prod -> maxExp = POLYNOMIAL_MAX_TERMS - 1;
    }

  for (int i = 0; i < POLYNOMIAL_MAX_TERMS; i++)
    {
      prod->terms[i] = 0;
      for (int j = 0; j <= i; j++)
	{
	  prod->terms[i] += a->terms[j] * b->terms[i - j];
	}
    }
}

void display (poly_t *poly)
{
  printf ("|");
  for (int i = 0; i < POLYNOMIAL_MAX_TERMS; i++)
    {
      if (poly->terms[i] == 0)
	{
	  //break;
	}
      printf ("%e|", creal (poly->terms[i]));
    }
  printf ("\n");
}

//int main ()
int test( )
{
  poly_t p1, p2, p3 ;

  initpoly ( &p1 ) ;
  initpoly ( &p2 ) ;
  initpoly ( &p3 ) ;

  polyappend ( &p1, 1., 4 ) ;
  polyappend ( &p1, 2., 3 ) ;
  polyappend ( &p1, 2., 2 ) ;
  polyappend ( &p1, 2., 0 ) ;

  polyappend ( &p2, 2., 3 ) ;
  polyappend ( &p2, 3., 2 ) ;
  polyappend ( &p2, 4., 0 ) ;

  polymul ( &p1, &p2, &p3 ) ;

  printf ( "\nFirst polynomial:\n" ) ;
  display ( &p1 ) ;

  printf ( "\n\nSecond polynomial:\n" ) ;
  display ( &p2 ) ;

  printf ( "\n\nResultant polynomial:\n" ) ;
  display ( &p3 ) ;

  return 0;
}

