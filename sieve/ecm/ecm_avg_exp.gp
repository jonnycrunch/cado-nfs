ecmtorsion12(n,p) = {
  local (t2, a, A, B, u, c, pnt, nmod2);

  /* If the n parameter is a negative rational, use that as the u value */
  if (n < 0,
    u = Mod(-n,p);
  ,
    /* We want a rational u so that v^2 = u^3-12u is a rational square. 
       u=-2, v=4 satisfies this and together with the torsion point (0,0)
       generates all rational points on the curve (which has rank 1).
       So we get all other (u,v) pairs as multiples of this point on 
       the curve v^2=u^3-12*u. */
    c = ellinit([Mod(0,p),Mod(0,p),Mod(0,p),Mod(-12,p),Mod(0,p)]);
    nmod2 = n % 2;
    pnt = ellpow (c, [Mod(-2,p), Mod(4,p)], (n - nmod2) / 2);
    if (nmod2, pnt = elladd (c, pnt, [0, 0]));
    u = pnt[1];
  );

  if (u == Mod(0, p), 
    print ("u = ", u);
    return(0);
  );
  t2 = (u^2 - Mod(12,p))/(4*u);
  if (t2 == Mod (-3, p) || t2 == Mod(1,p) || t2 == Mod(-1,p),
     print("t2 = ", t2);
     return(0);
  );
  a = (t2 - Mod(1,p))/(t2 + Mod(3,p));
  if (a == Mod (0, p), return(0));
  A = (-3*a^4 - 6*a^2 + Mod(1,p))/(4*a^3);
  if (A == Mod(2, p) || A == Mod(-2, p), 
    print ("A = ", A);
    return(0);
  );
  B = (a^2 - Mod(1,p))^2/(4*a^3);
  if (B == Mod(0, p), 
    print ("B = ", B);
    return(0);
  );
/* print("u = ", u, ", t^2 = ", t2, ", a = ", a, ", A = ", A, ", B = ", B); */
  E = ellinit ([0, B*A, 0, B^2, 0]);
}

ecmsigma(s, p)={
  local (t, u, v, x, w, a, b, A, E, X);
  
  t = Mod(s, p);
  v = 4*t;
  u = t^2 - 5;
  a = (v-u)^3*(3*u+v);
  b = 4*u^3*v;
  if (gcd (b, p) != 1, return(0));
  A = a/b-2;
/*
  if (poldegree(gcd(3*z^2 + 2*A*z + 1, z^3 + A*z^2 + z)) > 0, 
    print("Skipped singular curve sigma = ",s,", A = ",A);
    return(0);
  );
*/
  if (A == Mod(2, p) || A == Mod(-2, p), 
/*    print("Skipped singular curve, sigma = ", s,", A = ", A); */
    return(0);
  );

 x = u^3/v^3;
  w = x^3 + A*x^2 + x;
  if (w == 0, return(0));
  /* print ("sigma = ", s, ", p = ",p,", A = ", A, ", w = ", w); */

  /* Now we use the curve  E : wy^2 = x^3 + A*x^2 + x.
     This curve is equivalent to 
     Y^2 = X^3 + w*A*X^2 + w^2 * X
     with X=w*x, Y=w^2*y 
     The trick is that if w is a quadratic residue, then
     E is isomorphic to y^2 = x^3 + A*x^2 + x
     whereas if w is a non-residue, then E is the twist curve instead.
  */

  /* X = w*x; */
  E = ellinit ([0, w*A, 0, w^2, 0]); 

  return(E);
}

primeexp(p, n) = {
  local (r, i);
  r = n;
  i = 0;
  while (r%p == 0, i++; r/=p);

  return(i);
}

ecm_dist_exp(s,pmin,pmax,r,m) = {
  local(n, p2, p3, p5, p7, p11, c, singular, len, i);
  n = 0; singular = 0; len = 15; svec = Vec(s);
  p2 = listcreate(len); for(i=1,len,listput(p2, 0));
  p3 = listcreate(len); for(i=1,len,listput(p3, 0));
  p5 = listcreate(len); for(i=1,len,listput(p5, 0));
  p7 = listcreate(len); for(i=1,len,listput(p7, 0));
  p11 = listcreate(len); for(i=1,len,listput(p11, 0));
  for (i = 1, length(svec),  
    forprime (p = pmin, pmax,
      if (p % m == r,
        c = ecmsigma (svec[i], p);
        if (c != 0,
          n++;
          o = ellsea(c,p); 
          p2[min (primeexp(2,o)+1, len)]++; 
          p3[min (primeexp(3,o)+1, len)]++; 
          p5[min (primeexp(5,o)+1, len)]++; 
          p7[min (primeexp(7,o)+1, len)]++;
          p11[min (primeexp(11,o)+1, len)]++;
        , singular++);
      );
    );
  );
/*  printp("2: ",precision(1.*p2/n,9),", 3: ",precision(1.*p3/n,9), \
           ", 5: ",precision(1.*p5/n,9),", 7: ",precision(1.*p7/n,9)); */
  return([Vec(p2),Vec(p3),Vec(p5),Vec(p7),Vec(p11), Vec([n,singular])]);
}

ecm_dist_exp_t12(s,pmin,pmax,r,m) = {
  local(n, p2, p3, p5, p7, p11, c, singular, len, svec, i);
  n = 0; singular = 0; len = 15; svec = Vec(s); 
  p2 = listcreate(len); for(i=1,len,listput(p2, 0));
  p3 = listcreate(len); for(i=1,len,listput(p3, 0));
  p5 = listcreate(len); for(i=1,len,listput(p5, 0));
  p7 = listcreate(len); for(i=1,len,listput(p7, 0));
  p11 = listcreate(len); for(i=1,len,listput(p11, 0));
  for (i = 1, length(svec),
    forprime (p = pmin, pmax,
      if ((m == 0 || p % m == r) && 
          !(svec[i] < 0 && p == 10709),
        c = ecmtorsion12 (svec[i], p);
        if (c != 0,
          n++;
          o = ellsea(c,p); 
          p2[min (primeexp(2,o)+1, len)]++; 
          p3[min (primeexp(3,o)+1, len)]++; 
          p5[min (primeexp(5,o)+1, len)]++; 
          p7[min (primeexp(7,o)+1, len)]++;
          p11[min (primeexp(11,o)+1, len)]++;
        , singular++);
      );
    );  /* forprime (p = pmin, ...) */
  ); /* for (i = 1, ...) */
/*  printp("2: ",(1.*p2/n,9),", 3: ",precision(1.*p3/n,9), \
           ", 5: ",precision(1.*p5/n,9),", 7: ",precision(1.*p7/n,9)); */
  return([Vec(p2),Vec(p3),Vec(p5),Vec(p7),Vec(p11),Vec([n,singular])]);
}

ecm_avg_exp(d) = {
  local(n, s, i, j, avg, l);
  l = length(d);
  n = d[l][1]; /* The total number of good curves we tried */
  avg = listcreate(l - 1);
  for(i = 1, l - 1,
    s = 0;
    for (j = 1, length(d[i]),
      s += (j - 1) * d[i][j];
    );
    listput (avg, s / n);
  );
  return(Vec(avg));
}
