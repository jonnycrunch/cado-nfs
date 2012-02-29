#include <stdlib.h>

#include "latsieve.h"
#include "types.h"


typedef struct {
    int dim;
    ijvec_t *vec;
} Fpbasis_t;


// fill in V with vectors of the form (i*t^k, j*t^k), under the
// conditions:
//    deg(i*t^k) < degI
//    deg(j*t^k) < degJ
int fillin_basis(ijvec_t *V, fbprime_t i, fbprime_t j, int degI, 
        int degJ)
{
    int k = 0;
    fbprime_t ii, jj;
    fbprime_set(ii, i);
    fbprime_set(jj, j);
    while( (fbprime_deg(ii) < degI) && (fbprime_deg(jj) < degJ)) {
        ij_set_fbprime(V[0]->i, ii);
        ij_set_fbprime(V[0]->j, jj);
        k++;
        V++;
        fbprime_shl(ii, ii, 1);
        fbprime_shl(jj, jj, 1);
    }
    return k;
}

void sieveFB(unsigned char *S, factorbase_t FB, int I, int J, 
        qlat_t qlat)
{
    Fpbasis_t bas;
    // allocate more than enough space for the basis: I+J.
    bas.vec = (ijvec_t *) malloc ((I+J)*sizeof(ijvec_t));
    ASSERT_ALWAYS(bas.vec != NULL);

    for (unsigned int ii = 0; ii < FB.size; ++ii) {
        fbideal_t gothp = FB.elts[ii];
        fbprime_t lambda_p;
        int L = gothp.degp;
        //printf("%u %u\n", gothp.p[0], gothp.r[0]);
        
        //******************************************************************
        // compute the p-lattice, in the form of a basis of a Fp-vector space
        // of (i,j)-vectors.

        // compute lambda_p = - (a1 - r*b1) / (a0 - r*b0) mod p.
        {
            fbprime_t t0, t1; // temps
            fbprime_mulmod(t0, gothp.r, qlat->b0, gothp.p);
            fbprime_sub(t0, qlat->a0, t0);
            fbprime_rem(t0, t0, gothp.p);
            if (fbprime_is_zero(t0)) {
                // This is a projective root! Yurk!
                //printf("\n");
                continue;
            }
            fbprime_invmod(t0, t0, gothp.p);
            fbprime_mulmod(t1, gothp.r, qlat->b1, gothp.p);
            fbprime_sub(t1, t1, qlat->a1);
            fbprime_rem(t1, t1, gothp.p);
            fbprime_mulmod(lambda_p, t0, t1, gothp.p);
        }
        //printf("%u\n", lambda_p[0]);

        // The form of the basis is different for small p and for large p.
        if (L < I) {
            // Basis is { (t^k*p, 0) : k in [0..I-L-1] } join
            //          { (lambda*t^k mod p, t^k) : k in [0..J-1] }.
            bas.dim = I+J - L;
            ij_set_fbprime(bas.vec[0]->i, gothp.p);
            ij_set_zero(bas.vec[0]->j);
            for (int jj = 1; jj < I-L; ++jj) {
                ij_shl(bas.vec[jj]->i, bas.vec[jj-1]->i, 1);
                ij_set_zero(bas.vec[jj]->j);
            }
            ij_set_fbprime(bas.vec[I-L]->i, lambda_p);
            ij_set_one(bas.vec[I-L]->j);
            for (int jj = I-L+1; jj < I-L+J; ++jj) {
                ij_shl1mod(bas.vec[jj]->i, bas.vec[jj-1]->i, gothp.p);
                ij_set_ti(bas.vec[jj]->j, jj - (I-L));
            }
        } else {
            int dim = 0;
            // Basis is obtained from an Euclidian algorithm on (p,0)
            // and (lambda, 1). See tex file.
            fbprime_t alpha0, beta0;
            fbprime_t alpha1, beta1;

            fbprime_set(alpha0, gothp.p);
            fbprime_set_zero(beta0);
            fbprime_set(alpha1, lambda_p);
            fbprime_set_one(beta1);

            int ff = fillin_basis(bas.vec+dim, alpha1, beta1, I, J);
            dim += ff;

            while ((fbprime_deg(beta1) < J) && !fbprime_is_zero(alpha1)) {
                fbprime_t q, alpha2, beta2;
                fbprime_divrem(q, alpha2, alpha0, alpha1);
                fbprime_mul(beta2, q, beta1);
                fbprime_sub(beta2, beta0, beta2);
                fbprime_set(alpha0, alpha1);
                fbprime_set(beta0, beta1);
                fbprime_set(alpha1, alpha2);
                fbprime_set(beta1, beta2);
                ff = fillin_basis(bas.vec+dim, alpha1, beta1, 
                        MIN(fbprime_deg(alpha0), I), J);
                dim += ff;
            }
            // Conjecture: the final dimension is I+J-L
            //ASSERT_ALWAYS(dim == I+J-L);
            bas.dim=dim;
        }

        // Print basis
#if 0
        for (int i = 0; i < bas.dim; ++i) {
            printf("vec %d: ", i);
            ij_out(stdout, bas.vec[i]->i);
            printf(" ");
            ij_out(stdout, bas.vec[i]->j);
            printf("\n");
        }
#endif

        //******************************************************************
        // visit this vector space and subtract the contribution of
        // gothp in the corresponding position in S.
        // TODO: quadratic version, specific to char 2. Please improve!
        ASSERT_ALWAYS(FP_CHAR == 2);
        for (unsigned int k = 0; k < (1U<<bas.dim); ++k) {
            // at some point, we might deal with *affine* spaces.
            // In that case, just change the initial value of V.
            ijvec_t V = {{ {0}, {0}}};  
            unsigned int l = k;
            for (int i = 0; i < bas.dim; ++i, l>>= 1)
                if (l & 1U)
                    ijvec_add(V, V, bas.vec[i]);
            /*
            printf("vec %u: ", k);
            ij_out(stdout, V->i);
            printf(" ");
            ij_out(stdout, V->j);
            printf("\n"); */

            ijpos_t pos = ijvec2pos(V, I, J);
#ifdef TRACE_POS
            if (pos == TRACE_POS) {
                fprintf(stderr, "TRACE_POS(%d): ", pos);
                fbprime_out(stderr, gothp.p); fprintf(stderr, " ");
                fbprime_out(stderr, gothp.r); fprintf(stderr, "\n");
                fprintf(stderr, "TRACE_POS(%d): degnorm is now %d\n", pos,
                        S[pos]-gothp.degp);
            }
#endif
            if (pos != 0 && (S[pos] < gothp.degp)) {
                fprintf(stderr, "faulty pos is %u\n", pos);
            }
            ASSERT(pos == 0 || (S[pos] >= gothp.degp)); 
            S[pos] -= gothp.degp;
        }
    }

    free(bas.vec);
}
