#include "cado.h"
#include <stdio.h>
#include "portability.h"
#include "utils.h"

/* Init polynomial rel and set it to a - b*x */
void
mpz_poly_init_set_ab (mpz_poly_ptr rel, int64_t a, uint64_t b)
{
  mpz_poly_init(rel, 1);
  mpz_poly_setcoeff_int64(rel, 0, a);
  mpz_poly_setcoeff_int64(rel, 1, -b);
}

/* compute the SM */
void
compute_sm (mpz_poly_t SM, mpz_poly_srcptr num, const mpz_poly_t F, const mpz_t ell,
            const mpz_t smexp, const mpz_t ell2, const mpz_t invl2)
{
  mpz_poly_power_mod_f_mod_mpz_barrett(SM, num, F, smexp, ell2, invl2);
  mpz_poly_sub_ui(SM, SM, 1);

  for(int j = 0; j <= SM->deg; j++)
  {
    ASSERT_ALWAYS(mpz_divisible_p(SM->coeff[j], ell));
    mpz_divexact(SM->coeff[j], SM->coeff[j], ell);
    ASSERT_ALWAYS(mpz_cmp(ell, SM->coeff[j])>0);
  }
}

void compute_sm_splitchunks(mpz_poly_ptr dst, mpz_poly_srcptr u, sm_side_info_srcptr sm)
{
    /* now for a split-chunk compute_sm */

    if (sm->nsm == 0)
        return;

    int n = sm->f0->deg;
    mpz_poly_realloc(dst, n);
    mpz_poly_factor_list_srcptr fac = sm->fac;

    mpz_poly_t temp;
    mpz_poly_init(temp, n);

    /* we can afford calling malloc() here */
    mpz_poly_t * chunks = malloc(fac->size * sizeof(mpz_poly_t));
    for(int j = 0 ; j < fac->size ; j++) {
        mpz_poly_srcptr g = fac->factors[j]->f;
        mpz_poly_init(chunks[j], g->deg-1);
    }

    for(int j = 0, s = 0 ; j < fac->size ; j++) {
        /* simply call the usual function here */
        mpz_poly_srcptr g = fac->factors[j]->f;
        compute_sm (chunks[j],
                u,
                g,
                sm->ell,
                sm->exponents[j],
                sm->ell2,
                sm->invl2);
        for(int k = 0 ; k < g->deg ; k++, s++) {
            mpz_swap(temp->coeff[s], chunks[j]->coeff[k]);
        }
    }
    temp->deg = n - 1;

    /* now apply the change of basis matrix */
    for(int s = 0 ; s < n ; s++) {
        mpz_set_ui(dst->coeff[s], 0);
        for(int k = 0 ; k < n ; k++) {
            mpz_addmul(dst->coeff[s],
                    temp->coeff[k],
                    sm->matrix[k * n + s]);
        }
        mpz_mod(dst->coeff[s], dst->coeff[s], sm->ell);
    }

    dst->deg = n - 1;

    for(int j = 0 ; j < fac->size ; j++) {
        mpz_poly_clear(chunks[j]);
    }
    free(chunks);
    mpz_poly_clear(temp);
}

/* Assume nSM > 0 */
void
print_sm (FILE *f, mpz_poly_t SM, int nSM, int d)
{
  if (nSM == 0)
    return;
  d--;
  if (d > SM->deg)
    fprintf(f, "0");
  else
    gmp_fprintf(f, "%Zu", SM->coeff[d]);
  for(int j = 1; j < nSM; j++)
  {
    d--;
    if (d > SM->deg)
      fprintf(f, " 0");
    else
      gmp_fprintf(f, " %Zu", SM->coeff[d]);
  }
  //fprintf(f, "\n");
}

/* Compute the Shirokauer maps of a single pair (a,b). 
   SM must be allocated and is viewed as a polynomial of degree F->deg. */
void
sm_single_rel(mpz_poly_ptr *SM, int64_t a, uint64_t b, mpz_poly_ptr *F,
              const mpz_ptr *eps, const mpz_t ell, const mpz_t ell2,
              const mpz_t invl2)
{
  mpz_poly_t rel;

  mpz_poly_init_set_ab(rel, a, b);
  for (int s = 0; s < 2; s++) {
    if(F[s] != NULL)
      compute_sm (SM[s], rel, F[s], ell, eps[s], ell2, invl2);
  }
  mpz_poly_clear(rel);
}

void
sm_relset_init (sm_relset_t r, int *d)
{
  for (int side = 0; side < 2; side++) {
    mpz_poly_init (r->num[side], d[side]);
    mpz_poly_init (r->denom[side], d[side]);
  }
}

void
sm_relset_clear (sm_relset_t r)
{
  for (int side = 0; side < 2; side++) {
    mpz_poly_clear (r->num[side]);
    mpz_poly_clear (r->denom[side]);
  }
}

/* Given an array of index of rows and an array of abpolys,
 * construct the polynomial that corresponds to the relation-set, i.e. 
 *          rel = prod(abpoly[rk]^ek)
 * where rk is the index of a row, ek its exponent 
 * and abpoly[rk] = bk*x - ak.
 * rel is built as a fraction (sm_relset_t) and should be initialized.
 */
void
sm_build_one_relset (sm_relset_ptr rel, uint64_t *r, int64_t *e, int len,
                 mpz_poly_t * abpolys, mpz_poly_ptr *F, const mpz_t ell2)
{
  mpz_t ee;
  mpz_init(ee);  
  mpz_poly_t tmp[2];
  for (int side = 0; side < 2; side++) {
    if (F[side] == NULL) continue;
    mpz_poly_init(tmp[side], F[side]->deg);
  }

  /* Set the initial fraction to 1 / 1 */
  for (int side = 0; side < 2; side++) {
    mpz_poly_set_zero (rel->num[side]);
    mpz_poly_setcoeff_si(rel->num[side], 0, 1); 
    mpz_poly_set_zero (rel->denom[side]);
    mpz_poly_setcoeff_si(rel->denom[side], 0, 1); 
  }

  for(int k = 0 ; k < len ; k++)
  {
    /* Should never happen! */
    ASSERT_ALWAYS(e[k] != 0);

    if (e[k] > 0)
    {
      mpz_set_si(ee, e[k]);
      /* TODO: mpz_poly_long_power_mod_f_mod_mpz */
      for (int s = 0; s < 2; ++s) {
        if (F[s] == NULL) continue;
        mpz_poly_power_mod_f_mod_mpz (tmp[s], abpolys[r[k]], F[s], ee, ell2);
        mpz_poly_mul_mod_f_mod_mpz (rel->num[s], rel->num[s], tmp[s], F[s],
            ell2, NULL);
      }
    }
    else
    {
      mpz_set_si(ee, -e[k]);
      /* TODO: mpz_poly_long_power_mod_f_mod_mpz */
      for (int s = 0; s < 2; ++s) {
        if (F[s] == NULL) continue;
        mpz_poly_power_mod_f_mod_mpz(tmp[s], abpolys[r[k]], F[s], ee, ell2);
        mpz_poly_mul_mod_f_mod_mpz(rel->denom[s], rel->denom[s], tmp[s], F[s],
            ell2, NULL);
      }
    }
  }
  for (int s = 0; s < 2; ++s) {
    if (F[s] == NULL) continue;
    mpz_poly_cleandeg(rel->num[s], F[s]->deg);
    mpz_poly_cleandeg(rel->denom[s], F[s]->deg);
    mpz_poly_clear(tmp[s]);
  }

  mpz_clear (ee);
}

static int compute_unit_rank(mpz_poly_srcptr f)
{
    int r1 = mpz_poly_number_of_real_roots(f);
    ASSERT_ALWAYS((f->deg - r1) % 2 == 0);
    int r2 = (f->deg - r1) / 2;
    int unitrank = r1 + r2 - 1;
    return unitrank;
}

void compute_change_of_basis_matrix(mpz_t * matrix, mpz_poly_srcptr f, mpz_poly_factor_list_srcptr fac, mpz_srcptr ell)
{
    /* now compute the change of basis matrix. This is given simpy
     * as the CRT matrix from the piecewise representation modulo
     * the factors of f to the full representation.
     *
     * We thus have full_repr(x) = \sum multiplier_g * repr_g(x)
     *
     * Where multiplier_g is a multiple of f/g, such that
     * f/g*multiplier_g is one modulo f.
     *
     * Note that the computation of this matrix is ok to do mod ell
     * only ; going to ell^2 is unnecessary.
     */

    for(int i = 0, s = 0 ; i < fac->size ; i++) {
        mpz_poly_t d, a, b, h;
        mpz_poly_srcptr g = fac->factors[i]->f;
        mpz_poly_init(h, f->deg - g->deg);
        mpz_poly_init(d, 0);
        mpz_poly_init(a, f->deg - g->deg - 1);
        mpz_poly_init(b, g->deg - 1);

        /* compute h = product of other factors */
        mpz_poly_divexact(h, f, g, ell);

        /* now invert h modulo g */

        /* say a*g + b*h = 1 */
        mpz_poly_xgcd_mpz(d, g, h, a, b, ell);

        /* we now have the complete cofactor */
        mpz_poly_mul_mod_f_mod_mpz(h, b, h, f, ell, NULL);
        for(int j = 0 ; j < g->deg ; j++, s++) {
            /* store into the matrix the coefficients of x^j*h
             * modulo f */
            for(int k = 0 ; k < f->deg ; k++) {
                if (k <= h->deg)
                    mpz_set(matrix[s * f->deg + k], h->coeff[k]);
            }
            mpz_poly_mul_xi(h, h, 1);
            mpz_poly_mod_f_mod_mpz(h, f, ell, NULL);
        }
        mpz_poly_clear(b);
        mpz_poly_clear(a);
        mpz_poly_clear(d);
        mpz_poly_clear(h);
    }
}

void sm_side_info_print(FILE * out, sm_side_info_srcptr sm)
{
    /* check sm->matrix, not sm->nsm, because the user might have set
     * sm->nsm forcibly to any value, including 0 */
    if (sm->matrix == NULL) {
        fprintf(out, "# unit rank is 0, no SMs to compute\n");
        return;
    }
    fprintf(out, "# lifted factors of f modulo ell^2\n");
    for(int i = 0 ; i < sm->fac->size ; i++) {
        fprintf(out, "# ");
        mpz_poly_fprintf(out, sm->fac->factors[i]->f);
    }
    fprintf(out, "# change of basis matrix to OK/ell*OK from piecewise representation\n");
    for(int i = 0 ; i < sm->f->deg ; i++) {
        fprintf(out, "# ");
        for(int j = 0 ; j < sm->f->deg ; j++) {
            gmp_fprintf(out, " %Zd", sm->matrix[i * sm->f->deg + j]);
        }
        fprintf(out, "\n");
    }

}


void sm_side_info_init(sm_side_info_ptr sm, mpz_poly_srcptr f0, mpz_srcptr ell)
{
    memset(sm, 0, sizeof(*sm));

    sm->nsm = compute_unit_rank(f0);

    if (sm->nsm == 0)
        return;

    /* initialize all fields */
    mpz_init_set(sm->ell, ell);
    mpz_init(sm->ell2);
    mpz_mul(sm->ell2, sm->ell, sm->ell);
    mpz_init(sm->invl2);

    mpz_poly_init(sm->f, -1);
    sm->f0 = f0;
    mpz_poly_makemonic_mod_mpz(sm->f, f0, sm->ell2);
    mpz_poly_factor_list_init(sm->fac);

    mpz_init(sm->exponent);
    sm->matrix = malloc(sm->f->deg * sm->f->deg * sizeof(mpz_t));
    for(int i = 0 ; i < sm->f->deg ; i++)
        for(int j = 0 ; j < sm->f->deg ; j++)
            mpz_init_set_ui(sm->matrix[i * sm->f->deg + j], 0);

    /* note that sm->exponents is initialized at the very end of this
     * function */

    /* compute the lifted factors of f modulo ell^2 */
    {
        /* polynomial factorization is Las Vegas type */
        gmp_randstate_t rstate;
        gmp_randinit_default(rstate);
        mpz_poly_factor_and_lift_padically(sm->fac, sm->f, sm->ell, 2, rstate);
        gmp_randclear(rstate);
    }

    /* compute this, for fun. */
    compute_change_of_basis_matrix(sm->matrix, sm->f, sm->fac, ell);

    /* also compute the lcm of the ell^i-1 */
    sm->exponents = malloc(sm->fac->size * sizeof(mpz_t));
    mpz_set_ui(sm->exponent, 1);
    for(int i = 0 ; i < sm->fac->size ; i++) {
        mpz_init(sm->exponents[i]);
        mpz_pow_ui(sm->exponents[i], sm->ell, sm->fac->factors[i]->f->deg);
        mpz_sub_ui(sm->exponents[i], sm->exponents[i], 1);
        mpz_lcm(sm->exponent, sm->exponent, sm->exponents[i]);
    }

    barrett_init(sm->invl2, sm->ell2);
}

void sm_side_info_clear(sm_side_info_ptr sm)
{
    /* check sm->matrix, not sm->nsm, because the user might have set
     * sm->nsm forcibly to any value, including 0 */
    if (sm->matrix == NULL)
        return;

    for(int i = 0 ; i < sm->f->deg ; i++)
        for(int j = 0 ; j < sm->f->deg ; j++)
            mpz_clear(sm->matrix[i * sm->f->deg + j]);
    free(sm->matrix);

    mpz_clear(sm->exponent);

    for(int i = 0 ; i < sm->fac->size ; i++) {
        mpz_clear(sm->exponents[i]);
    }
    free(sm->exponents);

    mpz_poly_factor_list_clear(sm->fac);
    mpz_poly_clear(sm->f);

    mpz_clear(sm->invl2);
    mpz_clear(sm->ell2);
    mpz_clear(sm->ell);
}

