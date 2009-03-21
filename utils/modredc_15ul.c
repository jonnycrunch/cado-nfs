#include <stdio.h>
#include "modredc_15ul.h"

#if defined(__GNUC__) && (__GNUC__ >= 4 || __GNUC__ >= 3 && __GNUC_MINOR__ >= 4)
/* Opteron prefers LOOKUP_TRAILING_ZEROS 1, 
   Core2 prefers LOOKUP_TRAILING_ZEROS 0 */
#ifndef LOOKUP_TRAILING_ZEROS
#define LOOKUP_TRAILING_ZEROS 1
#endif
#define ctzl(x) __builtin_ctzl(x)
#define clzl(x) __builtin_clzl(x)
#else
/* If we have no ctzl(), we always use the table lookup */
#define LOOKUP_TRAILING_ZEROS 1
#endif

#if LOOKUP_TRAILING_ZEROS
static const unsigned char trailing_zeros[256] = 
  {8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
   5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
   6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
   5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
   7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
   5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
   6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
   5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0};
#endif


void
modredc15ul_div3 (residueredc15ul_t r, const residueredc15ul_t a, 
		  const modulusredc15ul_t m)
{
  residueredc15ul_t t;
  unsigned long a3 = (a[1] % 256UL + a[1] / 256UL + 
		      a[0] % 256UL + a[0] / 256UL) % 3UL;
  const unsigned long m3 = (m[0].m[0] % 256UL + m[0].m[0] / 256UL +
			    m[0].m[1] % 256UL + m[0].m[1] / 256UL) % 3UL;

  ASSERT(m3 != 0UL);

  modredc15ul_init_noset0 (t, m);
  t[1] = a[1];
  t[0] = a[0];

  /* Make t[1]:t[0] divisible by 3 */
  if (a3 != 0UL)
    {
      if (a3 + m3 == 3UL) /* Hence a3 == 1, m3 == 2 or a3 == 2, m3 == 1 */
	{
	  ularith_add_2ul_2ul (&(t[0]), &(t[1]), m[0].m[0], m[0].m[1]);
	}
      else /* a3 == 1, m3 == 1 or a3 == 2, m3 == 2 */
	{
	  ularith_add_2ul_2ul (&(t[0]), &(t[1]), m[0].m[0], m[0].m[1]);
	  ularith_add_2ul_2ul (&(t[0]), &(t[1]), m[0].m[0], m[0].m[1]);
	}
      
      /* Now t[1]:t[0] is divisible by 3 */
      ASSERT_EXPENSIVE ((t[0] % 3UL + t[1] % 3UL) % 3UL == 0UL);
    }
  
  /* a = a1 * 2^w + a0, 3|a
     Let a = a' * 3 * 2^w + a'', a'' < 3 * 2^w. 
     3 | a'', a'' / 3 < 2^w
     So a / 3 = a' * w + a'' / 3
     a' = trunc(a1 / 3)
     a'' = a0 * 3^{-1} (mod 2^w)
     Hence we get the correct result with one one-word multiplication
     and one one-word truncating division by a small constant.
  */
  
  t[1] = t[1] / 3UL;
  if (sizeof (unsigned long) == 4)
    t[0] *= 0xaaaaaaabUL; /* 1/3 (mod 2^32) */
  else
    t[0] *= 0xaaaaaaaaaaaaaaabUL; /* 1/3 (mod 2^64) */

#ifdef WANT_ASSERT_EXPENSIVE
  modredc15ul_sub (r, a, t, m);
  modredc15ul_sub (r, r, t, m);
  modredc15ul_sub (r, r, t, m);
  ASSERT_EXPENSIVE (modredc15ul_is0 (r, m));
#endif
  r[0] = t[0];
  r[1] = t[1];
  modredc15ul_clear (t, m);
}


void
modredc15ul_div7 (residueredc15ul_t r, const residueredc15ul_t a, 
		  const modulusredc15ul_t m)
{
  const unsigned long w_mod_7 = (sizeof (unsigned long) == 4) ? 4UL : 2UL;
  const unsigned long a7 = ((a[1] % 7UL) * w_mod_7 + a[0] % 7UL) % 7UL;
  residueredc15ul_t t;
  
  modredc15ul_init_noset0 (t, m);
  t[1] = a[1];
  t[0] = a[0];
  
  /* Make t[1]:t[0] divisible by 7 */
  if (a7 != 0UL)
    {
      const unsigned long m7 = ((m[0].m[1] % 7UL) * w_mod_7 + m[0].m[0] % 7UL) % 7UL;
      ASSERT(m7 != 0UL);
      
       /* inv7[i] = -1/i (mod 7) */
      const unsigned long inv7[7] = {0,6,3,2,5,4,1};
      unsigned long k;
      /* We want a+km == 0 (mod 7), so k = -a*m^{-1} (mod 7) */
      k = (inv7[m7] * a7) % 7UL;
      ularith_mul_ul_ul_2ul (&(t[0]), &(t[1]), m[0].m[0], k);
      t[1] += m[0].m[1] * k;
      ularith_add_2ul_2ul (&(t[0]), &(t[1]), a[0], a[1]);
      
      /* Now t[1]:t[0] is divisible by 7 */
      ASSERT_EXPENSIVE (((t[1] % 7UL) * w_mod_7 + t[0] % 7UL) % 7UL == 0UL);
    }
  
  t[1] = t[1] / 7UL;
  if (sizeof (unsigned long) == 4)
    t[0] *= 0xb6db6db7UL; /* 1/7 (mod 2^32) */
  else
    t[0] *= 0x6db6db6db6db6db7UL; /* 1/7 (mod 2^64) */

#ifdef WANT_ASSERT_EXPENSIVE
  modredc15ul_sub (r, a, t, m);
  modredc15ul_sub (r, r, t, m);
  modredc15ul_sub (r, r, t, m);
  modredc15ul_sub (r, r, t, m);
  modredc15ul_sub (r, r, t, m);
  modredc15ul_sub (r, r, t, m);
  modredc15ul_sub (r, r, t, m);
  ASSERT_EXPENSIVE (modredc15ul_is0 (r, m));
#endif
  r[0] = t[0];
  r[1] = t[1];
  modredc15ul_clear (t, m);
}


void
modredc15ul_div13 (residueredc15ul_t r, const residueredc15ul_t a, 
		   const modulusredc15ul_t m)
{
  const unsigned long w_mod_13 = (sizeof (unsigned long) == 4) ? 9UL : 3UL;
  const unsigned long a13 = ((a[1] % 13UL) * w_mod_13 + a[0] % 13UL) % 13UL;
  /* inv13[i] = -1/i (mod 13) */
  const unsigned long inv13[13] = {0, 12, 6, 4, 3, 5, 2, 11, 8, 10, 9, 7, 1}; 
  unsigned long m13 = ((m[0].m[1] % 13UL) * w_mod_13 + m[0].m[0] % 13UL) % 13UL;
  residueredc15ul_t t;
  
  ASSERT(m13 != 0UL);
  
  modredc15ul_init_noset0 (t, m);
  
  t[1] = a[1];
  t[0] = a[0];
  /* Make t[1]:t[0] divisible by 13 */
  if (a13 != 0UL)
    {
      /* We want a+km == 0 (mod 13), so k = -a*m^{-1} (mod 13) */
      m13 = (inv13[m13] * a13) % 13UL;
      ularith_mul_ul_ul_2ul (&(t[0]), &(t[1]), m[0].m[0], m13);
      t[1] += m[0].m[1] * m13;
      ularith_add_2ul_2ul (&(t[0]), &(t[1]), a[0], a[1]);
      
      /* Now t[1]:t[0] is divisible by 13 */
      ASSERT_EXPENSIVE (((t[1] % 13UL) * w_mod_13 + t[0] % 13UL) % 13UL == 0UL);
    }
  
  t[1] = t[1] / 13UL;
  if (sizeof (unsigned long) == 4)
    t[0] *= 0xc4ec4ec5; /* 1/13 (mod 2^32) */
  else
    t[0] *= 0x4ec4ec4ec4ec4ec5; /* 1/13 (mod 2^64) */

  r[0] = t[0];
  r[1] = t[1];
  modredc15ul_clear (t, m);
}


void
modredc15ul_gcd (unsigned long *r, const residueredc15ul_t A, 
		 const modulusredc15ul_t m)
{
  unsigned long a[2], b[2];
#ifdef LOOKUP_TRAILING_ZEROS
  int sh;
#endif

  /* Since we do REDC arithmetic, we must have m odd */
  ASSERT_EXPENSIVE (m[0].m[0] & 1UL);

  if (A[0] == 0UL && A[1] == 0UL)
    {
      r[0] = m[0].m[0];
      r[1] = m[0].m[1];
      return;
    }

  a[0] = A[0];
  a[1] = A[1];
  b[0] = m[0].m[0];
  b[1] = m[0].m[1];

  while (b[1] != 0UL || b[0] != 0UL)
    {
      /* Make b odd */
#ifdef LOOKUP_TRAILING_ZEROS
      do {
	sh = trailing_zeros [(unsigned char) b[0]];
	ularith_shrd (&(b[0]), b[1], sh);
	*(long *) &(b[1]) >>= sh;
      } while (sh == 8);
#else
      if (b[0] == 0UL) /* ctzl does not like zero input */
	{
	  b[0] = b[1];
	  b[1] = 0UL;
	}
      sh = ctzl (b[0]);
      ularith_shrd (&(b[0]), b[1], sh);
      *(long *) &(b[1]) >>= sh;
#endif
      
      /* Try to make the low two bits of a[0] zero */
      ASSERT_EXPENSIVE (b[0] % 2UL == 1UL);
      if ((a[0] ^ b[0]) & 2UL)
	ularith_add_2ul_2ul (&(a[0]), &(a[1]), b[0], b[1]);
      else
	ularith_sub_2ul_2ul (&(a[0]), &(a[1]), b[0], b[1]);

      if (a[0] == 0UL && a[1] == 0UL)
	{
	  if ((long) b[1] < 0)
	    {
	      b[1] = -b[1];
	      if (b[0] != 0UL)
		b[1]--;
	      b[0] = -b[0];
	    }
	  r[0] = b[0];
	  r[1] = b[1];
	  return;
	}

#ifdef LOOKUP_TRAILING_ZEROS
      do {
	sh = trailing_zeros [(unsigned char) a[0]];
	ularith_shrd (&(a[0]), a[1], sh);
	*(long *) &(a[1]) >>= sh;
      } while (sh == 8);
#else
      if (a[0] == 0UL) /* ctzl does not like zero input */
	{
	  a[0] = a[1];
	  a[1] = 0UL;
	}
      sh = ctzl (a[0]);
      ularith_shrd (&(a[0]), a[1], sh);
      *(long *) &(a[1]) >>= sh;
#endif
      ASSERT_EXPENSIVE (a[0] & 1);

      if ((a[0] ^ b[0]) & 2)
	ularith_add_2ul_2ul (&(b[0]), &(b[1]), a[0], a[1]);
      else
	ularith_sub_2ul_2ul (&(b[0]), &(b[1]), a[0], a[1]);
    }

  if ((long) a[1] < 0)
    {
      a[1] = -a[1];
      if (a[0] != 0UL)
	a[1]--;
      a[0] = -a[0];
    }
  r[0] = a[0];
  r[1] = a[1];

  return;
}


/* Compute r = b^e. Here, e is an unsigned long */
void
modredc15ul_pow_ul (residueredc15ul_t r, const residueredc15ul_t b, 
		    const unsigned long e, const modulusredc15ul_t m)
{
  unsigned long mask;
  residueredc15ul_t t;
#ifndef NDEBUG
  unsigned long e1 = e;
#endif
  
  if (e == 0UL)
    {
      modredc15ul_set1 (r, m);
      return;
    }

  /* Assume t = 1 here for the invariant.
     r^mask * b^e is invariant, and is the result we want */

  /* Find highest set bit in e. */
  mask = (1UL << (LONG_BIT - 1)) >> ularith_clz (e);
  /* r = 1, so r^(mask/2) * b^e = r^mask * b^e  */

  /* Exponentiate */

  modredc15ul_init (t, m);
  modredc15ul_set (t, b, m);       /* (r*b)^mask * b^(e-mask) = r^mask * b^e */
#ifndef NDEBUG
  e1 -= mask;
#endif

  while (mask > 1UL)
    {
      modredc15ul_mul (t, t, t, m);
      mask >>= 1;            /* (r^2)^(mask/2) * b^e = r^mask * b^e */
      if (e & mask)
        {
	  modredc15ul_mul (t, t, b, m);
#ifndef NDEBUG
          e1 -= mask;
#endif
        }
    }
  ASSERT (e1 == 0UL && mask == 1UL);
  /* Now e = 0, mask = 1, and r^mask * b^0 = r^mask is the result we want */
  modredc15ul_set (r, t, m);
  modredc15ul_clear (t, m);
}


/* Compute r = 2^e. Here, e is an unsigned long */
void
modredc15ul_2pow_ul (residueredc15ul_t r, const unsigned long e, 
		     const modulusredc15ul_t m)
{
  unsigned long mask;
  residueredc15ul_t t;
#ifndef NDEBUG
  unsigned long e1 = e;
#endif

  if (e == 0UL)
    {
      modredc15ul_set1 (r, m);
      return;
    }

  mask = (1UL << (LONG_BIT - 1)) >> ularith_clz (e);
  ASSERT (e & mask);

  modredc15ul_init (t, m);
  modredc15ul_set1 (t, m);
  modredc15ul_add (t, t, t, m); /* t = 2 */
#ifndef NDEBUG
  e1 -= mask;
#endif

  while (mask > 1UL)
    {
      modredc15ul_mul (t, t, t, m);
      mask >>= 1;
      if (e & mask)
        {
	  modredc15ul_add (t, t, t, m);
#ifndef NDEBUG
          e1 -= mask;
#endif
        }
    }
  ASSERT (e1 == 0UL && mask == 1UL);
  modredc15ul_set (r, t, m);
  modredc15ul_clear (t, m);
}


/* Compute r = b^e. Here e is a multiple precision integer 
   sum_{i=0}^{e_nrwords-1} e[i] * (machine word base)^i */
void
modredc15ul_pow_mp (residueredc15ul_t r, const residueredc15ul_t b, 
		    const unsigned long *e, const int e_nrwords, 
		    const modulusredc15ul_t m)
{
  unsigned long mask;
  residueredc15ul_t t;
  int i = e_nrwords - 1;

  if (e_nrwords == 0 || e[i] == 0UL)
    {
      modredc15ul_set1 (r, m);
      return;
    }

  /* Find highest set bit in e[i]. */
  mask = (1UL << (LONG_BIT - 1)) >> ularith_clz (e[i]);
  /* t = 1, so t^(mask/2) * b^e = t^mask * b^e  */

  /* Exponentiate */

  modredc15ul_init (t, m);
  modredc15ul_set (t, b, m);       /* (r*b)^mask * b^(e-mask) = r^mask * b^e */
  mask >>= 1;

  for ( ; i >= 0; i--)
    {
      while (mask > 0UL)
        {
          modredc15ul_mul (t, t, t, m);
          if (e[i] & mask)
            modredc15ul_mul (t, t, b, m);
          mask >>= 1;            /* (r^2)^(mask/2) * b^e = r^mask * b^e */
        }
      mask = ~0UL - (~0UL >> 1);
    }
  modredc15ul_set (r, t, m);
  modredc15ul_clear (t, m);
}


/* Compute r = 2^e. Here e is a multiple precision integer 
   sum_{i=0}^{e_nrwords-1} e[i] * (machine word base)^i */
void
modredc15ul_2pow_mp (residueredc15ul_t r, const unsigned long *e, 
		     const int e_nrwords, const modulusredc15ul_t m)
{
  unsigned long mask;
  residueredc15ul_t t;
  int i = e_nrwords - 1;

  if (e_nrwords == 0 || e[i] == 0UL)
    {
      modredc15ul_set1 (r, m);
      return;
    }

  ASSERT (e[e_nrwords - 1] != 0);

  /* Find highest set bit in e[i]. */
  mask = (1UL << (LONG_BIT - 1)) >> ularith_clz (e[i]);
  /* t = 1, so t^(mask/2) * b^e = t^mask * b^e  */
  
  /* Exponentiate */

  modredc15ul_init (t, m);
  modredc15ul_set1 (t, m);
  modredc15ul_add (t, t, t, m); /* t = 2 */
  /* (t*2)^mask * b^(e-mask) = t^mask * b^e */
  mask >>= 1;

  for ( ; i >= 0; i--)
    {
      while (mask > 0UL)
        {
          modredc15ul_mul (t, t, t, m);
          if (e[i] & mask)
            modredc15ul_add (t, t, t, m);
          mask >>= 1;            /* (r^2)^(mask/2) * b^e = r^mask * b^e */
        }
      mask = ~0UL - (~0UL >> 1);
    }
  modredc15ul_set (r, t, m);
  modredc15ul_clear (t, m);
}


/* Compute r = V_e(b), where V_e(x) is the Chebyshev polynomial defined by
   V_e(x + 1/x) = x^e + 1/x^e. Here e is an unsigned long. */

void
modredc15ul_V_ul (residueredc15ul_t r, const residueredc15ul_t b, 
		  const unsigned long e, const modulusredc15ul_t m)
{
  unsigned long mask;
  residueredc15ul_t t, t1, two;

  if (e == 0UL)
    {
      modredc15ul_set1 (r, m);
      modredc15ul_add (r, r, r, m);
      return;
    }
  
  /* Find highest set bit in e. */
  mask = (1UL << (LONG_BIT - 1)) >> ularith_clz (e);
  /* r = 1, so r^(mask/2) * b^e = r^mask * b^e  */

  /* Exponentiate */
  
  modredc15ul_init_noset0 (t, m);
  modredc15ul_init_noset0 (t1, m);
  modredc15ul_init_noset0 (two, m);
  modredc15ul_set1 (two, m);
  modredc15ul_add (two, two, two, m);
  modredc15ul_set (t, b, m);        /* t = b = V_1 (b) */
  modredc15ul_mul (t1, b, b, m);
  modredc15ul_sub (t1, t1, two, m);  /* r1 = b^2 - 2 = V_2 (b) */
  mask >>= 1;

  /* Here t = V_j (b) and t1 = V_{j+1} (b) for j = 1 */

  while (mask > 0UL)
    {
      if (e & mask)
        {
          /* j -> 2*j+1. Compute V_{2j+1} and V_{2j+2} */
          modredc15ul_mul (t, t, t1, m);
          modredc15ul_sub (t, t, b, m); /* V_j * V_{j+1} - V_1 = V_{2j+1} */
          modredc15ul_mul (t1, t1, t1, m);
          modredc15ul_sub (t1, t1, two, m); /* (V_{j+1})^2 - 2 = V_{2j+2} */
        }
      else
        {
          /* j -> 2*j. Compute V_{2j} and V_{2j+1} */
          modredc15ul_mul (t1, t1, t, m);
          modredc15ul_sub (t1, t1, b, m); /* V_j * V_{j+1} - V_1 = V_{2j+1}*/
          modredc15ul_mul (t, t, t, m);
          modredc15ul_sub (t, t, two, m);
        }
      mask >>= 1;
    }

  modredc15ul_set (r, t, m);
  modredc15ul_clear (t, m);
  modredc15ul_clear (t1, m);
  modredc15ul_clear (two, m);
}


/* Compute r = V_e(b), where V_e(x) is the Chebyshev polynomial defined by
   V_e(x + 1/x) = x^e + 1/x^e. Here e is a multiple precision integer 
   sum_{i=0}^{e_nrwords} e[i] * (machine word base)^i */

void
modredc15ul_V_mp (residueredc15ul_t r, const residueredc15ul_t b, 
		  const unsigned long *e, const int e_nrwords, 
		  const modulusredc15ul_t m)
{
  unsigned long mask;
  int i = e_nrwords - 1;
  residueredc15ul_t t, t1, two;

  if (e_nrwords == 0 || e[i] == 0UL)
    {
      modredc15ul_set1 (r, m);
      modredc15ul_add (r, r, r, m);
      return;
    }

  /* Find highest set bit in e. */
  mask = (1UL << (LONG_BIT - 1)) >> ularith_clz (e[i]);
  /* t = 1, so r^(mask/2) * b^e = t^mask * b^e  */

  /* Exponentiate */

  modredc15ul_init_noset0 (t, m);
  modredc15ul_init_noset0 (t1, m);
  modredc15ul_init_noset0 (two, m);
  modredc15ul_set1 (two, m);
  modredc15ul_add (two, two, two, m);
  modredc15ul_set (t, b, m);         /* t = b = V_1 (b) */
  modredc15ul_mul (t1, b, b, m);
  modredc15ul_sub (t1, t1, two, m);  /* t1 = b^2 - 2 = V_2 (b) */
  mask >>= 1;

  /* Here t = V_j (b) and t1 = V_{j+1} (b) for j = 1 */

  for ( ; i >= 0; i--)
    {
      while (mask > 0UL)
        {
          if (e[i] & mask)
	    {
	      /* j -> 2*j+1. Compute V_{2j+1} and V_{2j+2} */
	      modredc15ul_mul (t, t, t1, m);
	      modredc15ul_sub (t, t, b, m); /* V_j * V_{j+1} - V_1 = V_{2j+1} */
	      modredc15ul_mul (t1, t1, t1, m);
	      modredc15ul_sub (t1, t1, two, m); /* (V_{j+1})^2 - 2 = V_{2j+2} */
	    }
	  else
	    {
	      /* j -> 2*j. Compute V_{2j} and V_{2j+1} */
	      modredc15ul_mul (t1, t1, t, m);
	      modredc15ul_sub (t1, t1, b, m); /* V_j * V_{j+1} - V_1 = V_{2j+1}*/
	      modredc15ul_mul (t, t, t, m);
	      modredc15ul_sub (t, t, two, m);
	    }
          mask >>= 1;
        }
      mask = ~0UL - (~0UL >> 1);
    }
  
  modredc15ul_set (r, t, m);
  modredc15ul_clear (t, m);
  modredc15ul_clear (t1, m);
  modredc15ul_clear (two, m);
}


/* Returns 1 if m is a strong probable prime wrt base b, 0 otherwise.
   We assume m is odd. */
int
modredc15ul_sprp (const residueredc15ul_t b, const modulusredc15ul_t m)
{
  residueredc15ul_t r, minusone;
  int i = 0, po2 = 0;
  modintredc15ul_t mm1;

  modredc15ul_getmod_uls (mm1, m);

  if (modredc15ul_intequal_ul (mm1, 1UL))
    return 0;

  /* Let m-1 = 2^l * k, k odd. Set mm1 = k, po2 = l */
  mm1[0]--; /* No borrow since m is odd */
  if (mm1[0] == 0UL)
    {
      mm1[0] = mm1[1];
      mm1[1] = 0UL;
      po2 += LONG_BIT;
    }
  ASSERT (mm1[0] != 0UL);
  i = ularith_clz (mm1[0]);
  modredc15ul_intshr (mm1, mm1, i);
  po2 += i;

  modredc15ul_init_noset0 (r, m);
  modredc15ul_init_noset0 (minusone, m);
  modredc15ul_set1 (minusone, m);
  modredc15ul_neg (minusone, minusone, m);

  /* Exponentiate */
  if (mm1[1] > 0UL)
    modredc15ul_pow_mp (r, b, mm1, 2UL, m);
  else
    modredc15ul_pow_ul (r, b, mm1[0], m);

  /* If m is prime, then b^mm1 might be == 1 or == -1 (mod m) here */
  if (modredc15ul_is1 (r, m) || modredc15ul_equal (r, minusone, m))
    i = 1;
  else
    {
      /* If m is a prime, then one of b^(2*mm1), b^(2^2*mm1), ..., 
	 b^(2^(po2 - 1)*mm1)  must be == -1 (mod m) */
      for ( ; po2 > 1; po2--)
	{
	  modredc15ul_mul (r, r, r, m);
	  if (modredc15ul_equal (r, minusone, m))
	    {
	      i = 1;
	      break;
	    }
	}
    }

  modredc15ul_clear (r, m);
  modredc15ul_clear (minusone, m);
  return i;
}


/* Returns 1 if m is a strong probable prime wrt base 2, 0 otherwise. 
   We assume m is odd. */
int
modredc15ul_sprp2 (const modulusredc15ul_t m)
{
  residueredc15ul_t r, minusone;
  int i = 0, po2 = 0;
  modintredc15ul_t mm1;

  modredc15ul_getmod_uls (mm1, m);

  /* Let m-1 = 2^l * k, k odd. Set mm1 = k, po2 = l */
  mm1[0]--; /* No borrow since m is odd */

  if (mm1[0] == 0UL)
    {
      mm1[0] = mm1[1];
      mm1[1] = 0UL;
      po2 += LONG_BIT;
    }
  ASSERT (mm1[0] != 0UL);
  i = ularith_clz (mm1[0]);
  modredc15ul_intshr (mm1, mm1, i);
  po2 += i;

  modredc15ul_init_noset0 (r, m);
  modredc15ul_init_noset0 (minusone, m);
  modredc15ul_set1 (minusone, m);
  modredc15ul_neg (minusone, minusone, m);

  /* Exponentiate */
  if (mm1[1] > 0UL)
    modredc15ul_2pow_mp (r, mm1, 2UL, m);
  else
    modredc15ul_2pow_ul (r, mm1[0], m);

  /* If m is prime, then b^mm1 might be == 1 or == -1 (mod m) here */
  if (modredc15ul_is1 (r, m) || modredc15ul_equal (r, minusone, m))
    i = 1;
  else
    {
      /* If m is a prime, then one of b^(2*mm1), b^(2^2*mm1), ..., 
	 b^(2^(po2 - 1)*mm1)  must be == -1 (mod m) */
      for ( ; po2 > 1; po2--)
	{
	  modredc15ul_mul (r, r, r, m);
	  if (modredc15ul_equal (r, minusone, m))
	    {
	      i = 1;
	      break;
	    }
	}
    }

  modredc15ul_clear (r, m);
  modredc15ul_clear (minusone, m);
  return i;
}


int
modredc15ul_inv (residueredc15ul_t r, const residueredc15ul_t A, 
		 const modulusredc15ul_t m) 
{
  modintredc15ul_t a, b, u, v;
  int t, lsh;
#ifdef WANT_ASSERT_EXPENSIVE
  residueredc15ul_t tmp;
  
  modredc15ul_init_noset0 (tmp, m);
  modredc15ul_set (tmp, A, m);
#endif

  ASSERT_EXPENSIVE (modredc15ul_intcmp (A, m[0].m) < 0);
  ASSERT_EXPENSIVE (m[0].m[0] & 1UL);

  if (A[0] == 0UL && A[1] == 0UL)
    return 0;

  b[0] = m[0].m[0];
  b[1] = m[0].m[1];

  /* Let A = x*2^{2w}, so we want the Montgomery representation of 1/x, 
     which is 2^{2w}/x. We start by getting a = x */ 
  modredc15ul_get_uls (a, A, m);

  /* We simply set a = x/2^{2w} and t=0. The result before correction 
     will be 2^(2w+t)/x so we have to divide by t, which may be >64, 
     so we may have to do one or more full and a variable width REDC. */
  /* TODO: If b[1] > 1, we could skip one of the two REDC */
  modredc15ul_frommontgomery (a, a, m);
  /* Now a = x/2^w */
  t = 0;

  modredc15ul_intset_ul (u, 1UL);
  modredc15ul_intset_ul (v, 0UL);

  /* make a odd */
#if LOOKUP_TRAILING_ZEROS
  do {
    lsh = trailing_zeros [(unsigned char) a[0]];
    modredc15ul_intshr (a, a, lsh);
    t += lsh;
  } while (lsh == 8);
#else
  if (a[0] == 0UL)
    {
      /* x86 bsf gives undefined result for zero input */
      a[0] = a[1];
      a[1] = 0UL;
      t += LONG_BIT;
    }
  ASSERT_EXPENSIVE (a[0] != 0UL);
  lsh = ctzl(a[0]);
  modredc15ul_intshr (a, a, lsh);
  t += lsh;
#endif

  // Here a and b are odd, and a < b
  do {
    /* Here, a and b are odd, 0 < a < b, u is odd and v is even */
    ASSERT_EXPENSIVE (modredc15ul_intcmp (a, b) < 0);
    ASSERT_EXPENSIVE ((a[0] & 1UL) == 1UL);
    ASSERT_EXPENSIVE ((b[0] & 1UL) == 1UL);
    ASSERT_EXPENSIVE ((u[0] & 1UL) == 1UL);
    ASSERT_EXPENSIVE ((v[0] & 1UL) == 0UL);
    
    do {
      ularith_sub_2ul_2ul (&(b[0]), &(b[1]), a[0], a[1]);
      ularith_add_2ul_2ul (&(v[0]), &(v[1]), u[0], u[1]);
#if LOOKUP_TRAILING_ZEROS
      do {
	lsh = trailing_zeros [(unsigned char) b[0]];
#else
      if (b[0] == 0UL)
	{
	  b[0] = b[1]; /* b[0] can be odd now, so lsh might be 0 below! */
	  b[1] = 0UL;
	  ASSERT_EXPENSIVE (u[1] == 0UL);
	  u[1] = u[0]; /* Shift left u by LONG_BIT */
	  u[0] = 0UL;
	  t += LONG_BIT;
	}
      lsh = ctzl(b[0]);
#endif
      ASSERT_EXPENSIVE ((b[0] & ((1UL << lsh) - 1UL)) == 0UL);
      modredc15ul_intshr (b, b, lsh);
      t += lsh;
      modredc15ul_intshl (u, u, lsh);
#if LOOKUP_TRAILING_ZEROS
      } while (lsh == 8);
#endif
    } while (modredc15ul_intcmp (a, b) < 0); /* ~50% branch taken :( */
    
    /* Here, a and b are odd, 0 < b =< a, u is even and v is odd */
    ASSERT_EXPENSIVE ((a[0] & 1UL) == 1UL);
    ASSERT_EXPENSIVE ((b[0] & 1UL) == 1UL);
    ASSERT_EXPENSIVE ((u[0] & 1UL) == 0UL);
    ASSERT_EXPENSIVE ((v[0] & 1UL) == 1UL);
    
    if (modredc15ul_intequal (a, b))
      break;
    ASSERT_EXPENSIVE (modredc15ul_intcmp (a, b) > 0);
    
    /* Here, a and b are odd, 0 < b < a, u is even and v is odd */
    do {
      ularith_sub_2ul_2ul (&(a[0]), &(a[1]), b[0], b[1]);
      ularith_add_2ul_2ul (&(u[0]), &(u[1]), v[0], v[1]);
      
      if (a[0] == 0UL)
	{
	  a[0] = a[1];
	  a[1] = 0UL;
	  v[1] = v[0]; /* Shift left u by LONG_BIT */
	  v[0] = 0UL;
	  t += LONG_BIT;
	}
#if LOOKUP_TRAILING_ZEROS
      do {
	lsh = trailing_zeros [(unsigned char) a[0]];
#else
	lsh = ctzl(a[0]);
#endif
        ASSERT_EXPENSIVE ((a[0] & ((1UL << lsh) - 1UL)) == 0UL);
	modredc15ul_intshr (a, a, lsh);
	t += lsh;
	modredc15ul_intshl (v, v, lsh);
#if LOOKUP_TRAILING_ZEROS
      } while (lsh == 8);
#endif

    } while (modredc15ul_intcmp (a, b) > 0); /* about 50% branch taken :( */
    /* Here, a and b are odd, 0 < a =< b, u is odd and v is even */
  } while (a[0] != b[0] || a[1] != b[1]);
  
  if (a[1] != 0UL || a[0] != 1UL) /* Non-trivial GCD */
    return 0;

  ASSERT (t >= 0);

  /* Here, u = 2^w * 2^t / x. We want 2^w / x. */

  /* Here, the inverse of a is u/2^t mod b. To do the division by t,
     we use a variable-width REDC. We want to add a multiple of m to u
     so that the low t bits of the sum are 0 and we can right-shift by t
     with impunity. */
  while (t >= LONG_BIT)
    {
      /* Do full REDC */
      unsigned long s[4], k;
      k = u[0] * m[0].invm;
      ularith_mul_ul_ul_2ul (&(s[0]), &(s[1]), k, m[0].m[0]);
      if (s[0] != 0UL)
	s[1]++;
      s[2] = 0UL;
      ularith_add_ul_2ul (&(s[1]), &(s[2]), u[1]); /* s[2] <= 1 */
      ularith_mul_ul_ul_2ul (&(s[0]), &(s[3]), k, m[0].m[1]); /* s[3]<2^(w/2) */
      ularith_add_2ul_2ul (&(s[1]), &(s[2]), s[0], s[3]);
      u[0] = s[1];
      u[1] = s[2];
      t -= LONG_BIT;
    }

  if (t > 0)
    {
      unsigned long s[5], k;
      k = ((u[0] * m[0].invm) & ((1UL << t) - 1UL)); /* tlow <= 2^t-1 */
      ularith_mul_ul_ul_2ul (&(s[0]), &(s[1]), k, m[0].m[0]); 
      /* s[1]:s[0] <= (2^w-1)*(2^t-1) <= (2^w-1)*(2^(w-1)-1) */
      ularith_add_2ul_2ul (&(s[0]), &(s[1]), u[0], u[1]);
      /* s[1]:s[0] <= (2^w-1)*(2^(w-1)-1) + (m-1) < 2^(2w) */
      /* s[0] == 0 (mod 2^t) */
      ASSERT_EXPENSIVE ((s[0] & ((1UL << t) - 1UL)) == 0UL);
      s[2] = 0;
      ularith_mul_ul_ul_2ul (&(s[3]), &(s[4]), k, m[0].m[1]);
      ularith_add_2ul_2ul (&(s[1]), &(s[2]), s[3], s[4]);

      /* Now shift s[2]:s[1]:s[0] right by t */
      ularith_shrd (&(s[0]), s[1], t);
      ularith_shrd (&(s[1]), s[2], t);

      u[0] = s[0];
      u[1] = s[1];
    }

#ifdef WANT_ASSERT_EXPENSIVE
  modredc15ul_mul (tmp, tmp, u, m);
  if (!modredc15ul_is1 (tmp, m))
    {
      modintredc15ul_t tmpi;
      modredc15ul_get_uls (tmpi, tmp, m);
      fprintf (stderr, "Error, Mod(1/(%lu + 2^%d * %lu), %lu + 2^%d * %lu) == "
               "%lu + 2^%d * %lu\n",
               A[0], LONG_BIT, A[1], m[0].m[0], LONG_BIT, m[0].m[1],
               tmpi[0], LONG_BIT, tmpi[1]);
      ASSERT_EXPENSIVE (modredc15ul_intcmp_ul (tmpi, 1UL) == 0);
    }
  modredc15ul_clear (tmp, m);
#endif

  r[0] = u[0];
  r[1] = u[1];
  return 1;
}


int
modredc15ul_jacobi (const residueredc15ul_t a_par, 
		    const modulusredc15ul_t m_par)
{
  modintredc15ul_t a, m, s;
  int t = 1;

  modredc15ul_get_uls (a, a_par, m_par);
  modredc15ul_getmod_uls (m, m_par);
  
  while (!modredc15ul_intequal_ul (a, 0UL))
  {
    while (a[0] % 2UL == 0UL) /* TODO speedup */
    {
      modredc15ul_intshr (a, a, 1);
      if (m[0] % 8UL == 3UL || m[0] % 8UL == 5UL)
        t = -t;
    }
    modredc15ul_intset (s, a); /* swap a and m */
    modredc15ul_intset (a, m);
    modredc15ul_intset (m, s);
    if (a[0] % 4UL == 3UL && m[0] % 4UL == 3UL)
      t = -t;
    
    /* m is odd here */
    if (modredc15ul_intcmp (a, m) >= 0)
      {
        if (a[1] == 0UL)
          {
            a[0] %= m[0];
          }
        else
          {
            /* FIXME, slow and stupid */
            modintredc15ul_t t;
            modredc15ul_intset (t, m);
            while (modredc15ul_intcmp (t, a) < 0)
              modredc15ul_intshl (t, t, 1);
            while (modredc15ul_intcmp (a, m) >= 0)
              {
		ularith_sub_2ul_2ul_ge (&(a[0]), &(a[1]), t[0], t[1]);
                modredc15ul_intshr (t, t, 1);
              }
          }
      }
  }
  if (m[1] != 0UL || m[0] != 1UL)
    t = 0;
  
#ifdef MODTRACE
  printf ("kronecker(%lu, %lu) == %d\n", 
          modredc15ul_get_ul (a_par, m_par), modredc15ul_getmod_ul (m_par), t);
#endif
  return t;
}
