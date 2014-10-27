#include "fm.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

fm_t*
fm_create (void)
{
  fm_t* t = malloc(sizeof(*t));
  assert (t != NULL);
  t->len_method = 4;
  t->len_proba = 1;
  t->len_time = 1;
  t->method = calloc (t->len_method, sizeof(unsigned long));
  assert (t->method != NULL);
  t->proba = calloc (t->len_proba, sizeof(double));
  assert (t->proba != NULL);
  t->time = calloc (t->len_time, sizeof(double));
  assert (t->time != NULL);
  t->len_p_min = 0;
  return t;
}


void
fm_free (fm_t* t)
{
  free (t->time);
  free (t->proba);
  free (t->method);

  free (t);
}


unsigned long*
fm_get_method(fm_t* t)
{
  return t->method;
}

double*
fm_get_proba(fm_t* t)
{
  return t->proba;
}

double*
fm_get_time(fm_t* t)
{
  return t->time;
}

int
fm_get_len_method(fm_t* t)
{
  return t->len_method;
}

int
fm_get_len_proba(fm_t* t)
{
  return t->len_proba;
}

int
fm_get_len_time(fm_t* t)
{
  return t->len_time;
}

int
fm_get_len_p_min(fm_t* t)
{
  return t->len_p_min;
}


void
fm_set_method(fm_t* t, unsigned long* value, int len)
{
  if (len != t->len_method)
    {//realloc
      t->method = realloc(t->method, len  *(sizeof(unsigned long)));
      assert (t->method!=NULL);
      t->len_method = len;
    }

  for (int i=0; i < t->len_method; i++)
    t->method[i] = value[i];
}

void
fm_set_proba(fm_t* t, double* value, int len, int len_p_min)
{
  if (len==0)
    return ;
  t->len_p_min = len_p_min;
  if (len != t->len_proba)
    {//realloc
      t->proba = realloc(t->proba, len  *(sizeof(double)));
      assert (t->proba!=NULL);
      t->len_proba = len;
    }
  
  for (int i=0; i < t->len_proba; i++)
    t->proba[i] = value[i];
}


void
fm_set_time(fm_t* t, double* value, int len)
{
  if (len==0)
    return ;

  if (len != t->len_time)
    {//realloc
      t->time = realloc(t->time, len  *(sizeof(double)));
      assert (t->time!=NULL);
      t->len_time = len;
    }

  for (int i = 0; i < t->len_time; i++)
    t->time[i] =  value[i];
}




fm_t*
fm_copy (fm_t* t)
{
  fm_t* cop = fm_create ();
  fm_set_method (cop, t->method, t->len_method);
  fm_set_proba (cop, t->proba, t->len_proba, t->len_p_min);
  fm_set_time (cop, t->time, t->len_time);
  return cop;
}


void
fm_put_zero (fm_t* t)
{
  t->method[2] = 0;//B1
  t->method[3] = 0;//B2
  for (int i=0; i < t->len_proba; i++)
    t->proba[i] = 0;
  for (int i=0; i < t->len_time; i++)
    t->time[i] = 0;
}

bool
fm_is_zero (fm_t* t)
{
  return (t->method[2] == 0 && t->method[3]==0);
}

int
fm_is_equal (fm_t* c1, fm_t* c2)
{
  int len = c1->len_method;
  for (int i = 0; i < len; i++)
    if (c1->method[i] != c2->method[i])
      return false;
  return true;
}

int
fm_print (fm_t* t)
{
  return fm_fprint (stdout, t);
}


int
fm_fprint (FILE* file, fm_t* elem)
{
  if (file == NULL)
    return -1;

  unsigned long *method = fm_get_method(elem);
  int len_method = fm_get_len_method(elem);
  for (int i = 0; i < len_method; i++)
  	fprintf (file, "%lu ", method[i]);
  fputs ("| ", file);

  fprintf (file, "%d ", elem->len_p_min);
  double *proba = fm_get_proba(elem);
  int len_proba = fm_get_len_proba(elem);
  for (int i = 0; i < len_proba; i++)
  	fprintf (file, "%lf ", proba[i]);
  fputs ("| ", file);

  double *time = fm_get_time(elem);
  int len_time = fm_get_len_time(elem);
  for (int i = 0; i < len_time; i++)
  	fprintf (file, "%lf ", time[i]);
      
  fputs ("|\n", file);
  return 0;
}



/* /\* gcc -std=c99 -Wall fm.c -o fm *\/ */
/* int */
/* main() */
/* { */
/*   fm_t* t = fm_create(); */
  
/*   unsigned long elem[4]; */
/*   for (int i=0; i < 4; i++) */
/*     { */
/*       elem[i] = i; */
/*     } */
/*   fm_set_method (t, elem, 4); */

/*   double* elem2 = malloc (sizeof(double)*10); */

/*   for (int i=0; i < 10; i++) */
/*     { */
/*       elem2[i] = i*10; */
/*     } */
/*   fm_set_proba (t, elem2, 10); */
/*   free (elem2); */

/*   double elem3[10]; */
/*   for (int i=0; i < 10; i++) */
/*     { */
/*       elem3[i] = i*100; */
/*     } */

/*   fm_set_time (t, elem3, 10); */
  
/*   fm_print (t); */

/*   double elem4[15]; */
/*   for (int i=0; i < 15; i++) */
/*     { */
/*       elem4[i] = i*100; */
/*     } */

/*   fm_set_time (t, elem4, 15); */
/*   //fm_set_proba (t, elem4, 15); */

/*   fm_print (t); */

/*   fm_free (t); */

/*   exit(1); */
/* } */
