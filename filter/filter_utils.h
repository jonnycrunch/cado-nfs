#ifndef FILTER_UTILS_H_
#define FILTER_UTILS_H_

#define NB_PRIMES_OPT 31

typedef struct {
  index_t h;
  p_r_values_t p;
  exponent_t e;
} prime_t;

typedef struct {
  index_t id;
  int32_t e; //or exponent_t ??
} ideal_merge_t;

typedef struct {
  index_t nrels;
  index_t nprimes;
  double W; //weight of the active part of the matrix
} info_mat_t;

#define DEBUG 1

#define CA_DUP1 314159265358979323UL
#define CB_DUP1 271828182845904523UL

#define CA_DUP2 271828182845904523UL
#define CB_DUP2 577215664901532889UL

#include <time.h>
#include <pthread.h>

#include "filter_io.h"
#include "filter_memalloc.h"


index_t insert_rel_in_table_no_e (buf_rel_t *, index_t, uint8_t, index_t **,
                                  weight_t *);
index_t insert_rel_in_table_with_e (buf_rel_t *, index_t, uint8_t,
                                    ideal_merge_t **, weight_t *);
void print_relation (FILE *, buf_rel_t *);
uint32_t insert_relation_in_dup_hashtable (uint32_t *, unsigned long, buf_rel_t*,
                                           double *, unsigned int *);

void compute_index_rel (renumber_t, buf_rel_t *);

#endif /* FILTER_UTILS_H_ */