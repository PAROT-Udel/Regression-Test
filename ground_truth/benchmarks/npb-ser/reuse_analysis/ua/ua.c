/*
Copyright (C) 1991-2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it andor
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http:www.gnu.org/licenses/>.
*/
/*
This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.
*/
/* We do support the IEC 559 math functionality, real and complex.  */
/*
wchar_t uses ISOIEC 10646 (2nd ed., published 2011-03-15) /
   Unicode 6.0.
*/
/* We do not support C11 <threads.h>.  */
/* ------------------------------------------------------------------------- */
/*                                                                          */
/*  This benchmark is a serial C version of the NPB UA code. This C         */
/*  version is developed by the Center for Manycore Programming at Seoul    */
/*  National University and derived from the serial Fortran versions in     */
/*  "NPB3.3-SER" developed by NAS.                                          */
/*                                                                          */
/*  Permission to use, copy, distribute and modify this software for any    */
/*  purpose with or without fee is hereby granted. This software is         */
/*  provided "as is" without express or implied warranty.                   */
/*                                                                          */
/*  Information on NPB 3.3, including the technical report, the original    */
/*  specifications, source code, results and information on how to submit   */
/*  new results, is available at:                                           */
/*                                                                          */
/*           http:www.nas.nasa.govSoftware/NPB/                          */
/*                                                                          */
/*  Send comments or suggestions for this C version to cmp@aces.snu.ac.kr   */
/*                                                                          */
/*          Center for Manycore Programming                                 */
/*          School of Computer Science and Engineering                      */
/*          Seoul National University                                       */
/*          Seoul 151-744, Korea                                            */
/*                                                                          */
/*          E-mail:  cmp@aces.snu.ac.kr                                     */
/*                                                                          */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* Authors: Sangmin Seo, Jungwon Kim, Jun Lee, Jeongho Nah, Gangwon Jo,     */
/*          and Jaejin Lee                                                  */
/* ------------------------------------------------------------------------- */
/* program ua */
#include "header.h"
#include "print_results.h"
#include "timers.h"
#include <math.h>
#include <omp.h>
#include <stdio.h>
/* commonusrdati */
int fre, niter, nmxh;
/* commonusrdatr */
double alpha, dlmin, dtime;
/* commondimn */
int nelt, ntot, nmor, nvertex;
/* commonbench1 */
double x0, _y0, z0, time;
/* double arrays associated with collocation points */
/* commoncolldp */
double ta1[250][5][5][5];
double ta2[250][5][5][5];
double trhs[250][5][5][5];
double t[250][5][5][5];
double tmult[250][5][5][5];
double dpcelm[250][5][5][5];
double pdiff[250][5][5][5];
double pdiffp[250][5][5][5];
/* double arays associated with mortar points */
/* commonmortdp */
double umor[11600];
double mormult[11600];
double tmort[11600];
double tmmor[11600];
double rmor[11600];
double dpcmor[11600];
double pmorx[11600];
double ppmor[11600];
/* integer arrays associated with element faces */
/* commonfacein */
int idmo[250][6][2][2][5][5];
int idel[250][6][5][5];
int sje[250][6][2][2];
int sje_new[250][6][2][2];
int ijel[250][6][2];
int ijel_new[250][6][2];
int cbc[250][6];
int cbc_new[250][6];
/* integer array associated with vertices */
/* commonvin */
int vassign[250][8];
int emo[(8 * 250)][8][2];
int nemo[(8 * 250)];
/* integer array associated with element edges */
/* commonedgein */
int diagn[250][12][2];
/* integer arrays associated with elements */
/* commoneltin */
int tree[250];
int treenew[250];
int mt_to_id[250];
int mt_to_id_old[250];
int id_to_mt[250];
int newc[250];
int newi[250];
int newe[250];
int ref_front_id[250];
int ich[250];
int size_e[250];
int front[250];
int action[250];
/* logical arrays associated with vertices */
/* commonvlg */
logical ifpcmor[(8 * 250)];
/* logical arrays associated with edge */
/* commonedgelg */
logical eassign[250][12];
logical ncon_edge[250][12];
logical if_1_edge[250][12];
/* logical arrays associated with elements */
/* commonfacelg */
logical skip[250];
logical ifcoa[250];
logical ifcoa_id[250];
/* logical arrays associated with element faces */
/* commonmasonl */
logical fassign[250][6];
logical edgevis[250][6][4];
/* small arrays */
/* commontransr */
double qbnew[2][5][(5 - 2)];
double bqnew[2][(5 - 2)][(5 - 2)];
/* commonpcr */
double pcmor_nc1[4][2][2][5][5];
double pcmor_nc2[4][2][2][5][5];
double pcmor_nc0[4][2][2][5][5];
double pcmor_c[4][5][5];
double tcpre[5][5];
double pcmor_cor[4][8];
/* gauss-labotto and gauss points */
/* commongauss */
double zgm1[5];
/* weights */
/* commonwxyz */
double wxm1[5];
double w3m1[5][5][5];
/* coordinate of element vertices */
/* commoncoord */
double xc[250][8];
double yc[250][8];
double zc[250][8];
double xc_new[250][8];
double yc_new[250][8];
double zc_new[250][8];
/* drdx, dx/dr  and Jacobian */
/* commongiso */
double jacm1_s[4][5][5][5];
double rxm1_s[4][5][5][5];
double xrm1_s[4][5][5][5];
/* mass matrices (diagonal) */
/* commonmass */
double bm1_s[4][5][5][5];
/* dertivative matrices ddr */
/* commondxyz */
double dxm1[5][5];
double dxtm1[5][5];
double wdtdr[5][5];
/* interpolation operators */
/* commonixyz */
double ixm31[((5 * 2) - 1)][5];
double ixtm31[5][((5 * 2) - 1)];
double ixmc1[5][5];
double ixtmc1[5][5];
double ixmc2[5][5];
double ixtmc2[5][5];
double map2[5];
double map4[5];
/* collocation location within an element */
/* commonxfracs */
double xfrac[5];
/* used in laplacian operator */
/* commongmfact */
double g1m1_s[4][5][5][5];
double g4m1_s[4][5][5][5];
double g5m1_s[4][5][5][5];
double g6m1_s[4][5][5][5];
/* We store some tables of useful topological constants */
/* These constants are intialized in a block data 'top_constants' */
/* commontop_consts */
int f_e_ef[6][4];
int e_c[8][3];
int local_corner[6][8];
int cal_nnb[8][3];
int oplc[4];
int cal_iijj[4][2];
int cal_intempx[6][4];
int c_f[6][4];
int le_arr[3][2][4];
int jjface[6];
int e_face2[6][4];
int op[4];
int localedgenumber[12][6];
int edgenumber[6][4];
int f_c[8][3];
int e1v1[6][6];
int e2v1[6][6];
int e1v2[6][6];
int e2v2[6][6];
int children[6][4];
int iijj[4][2];
int v_end[2];
int face_l1[3];
int face_l2[3];
int face_ld[3];
/* Timer parameters */
/* commontiming */
logical timeron;
int main(int argc, char *argv[]) {
  int step, ie, iside, i, j, k;
  double mflops, tmax, nelt_tot = 0.0;
  char Class;
  logical ifmortar = false, verified;
  double t2, trecs[(10 + 1)];
  char *t_names[(10 + 1)];
  /* --------------------------------------------------------------------- */
  /* Read input file (if it exists), else take */
  /* defaults from parameters */
  /* --------------------------------------------------------------------- */
  FILE *fp;
  if ((fp = fopen("timer.flag", "r")) != ((void *)0)) {
    timeron = true;
    t_names[1] = "total";
    t_names[2] = "init";
    t_names[3] = "convect";
    t_names[4] = "transfb_c";
    t_names[5] = "diffusion";
    t_names[6] = "transf";
    t_names[7] = "transfb";
    t_names[8] = "adaptation";
    t_names[9] = "transf+b";
    t_names[10] = "add2";
    fclose(fp);
  } else {
    timeron = false;
  }
  printf("\n\n NAS Parallel Benchmarks (NPB3.3-SER-C) - UA Benchmark\n\n");
  if ((fp = fopen("inputua.data", "r")) != ((void *)0)) {
    int result;
    printf(" Reading from input file inputua.data\n");
    result = fscanf(fp, "%d", &fre);
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%d", &niter);
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%d", &nmxh);
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%lf", &alpha);
    Class = 'U';
    fclose(fp);
  } else {
    printf(" No input file inputua.data. Using compiled defaults\n");
    fre = 5;
    niter = 50;
    nmxh = 10;
    alpha = 0.04;
    Class = 'S';
  }
  dlmin = pow(0.5, 4);
  dtime = (0.04 * dlmin);
  printf(" Levels of refinement: %8d\n", 4);
  printf(" Adaptation frequency: %8d\n", fre);
  printf(" Time steps:           %8d    dt: %15.6E\n", niter, dtime);
  printf(" CG iterations:        %8d\n", nmxh);
  printf(" Heat source radius:   %8.4f\n\n", alpha);
  top_constants();
#pragma cetus private(i)
#pragma loop name main #0
  for (i = 1; i <= 10; i++) {
    timer_clear(i);
  }
  if (timeron) {
    timer_start(2);
  }
  /* set up initial mesh (single element) and solution (all zero) */
  create_initial_grid();
  r_init((double *)ta1, ntot, 0.0);
  nr_init((int *)sje, (4 * 6) * nelt, -1);
  /* compute tables of coefficients and weights       */
  coef();
  geom1();
  /* compute the discrete laplacian operators */
  setdef();
  /* prepare for the preconditioner */
  setpcmo_pre();
  /* refine initial mesh and do some preliminary work */
  time = 0.0;
  mortar();
  prepwork();
  adaptation(&ifmortar, 0);
  if (timeron) {
    timer_stop(2);
  }
  timer_clear(1);
  time = 0.0;
#pragma cetus private(i, ie, iside, j, k, step)
#pragma loop name main #1
  for (step = 0; step <= niter; step++) {
    if (step == 1) {
      /* reset the solution and start the timer, keep track of total no elms */
      r_init((double *)ta1, ntot, 0.0);
      time = 0.0;
      nelt_tot = 0.0;
#pragma cetus private(i)
#pragma loop name main #1 #0
      for (i = 1; i <= 10; i++) {
        if (i != 2) {
          timer_clear(i);
        }
      }
      timer_start(1);
    }
    /* advance the convection step  */
    convect(ifmortar);
    if (timeron) {
      timer_start(9);
    }
    /* prepare the intital guess for cg */
    transf(tmort, (double *)ta1);
/* compute residual for diffusion term based on intital guess */
/* compute the left hand side of equation, lapacian t */
#pragma cetus private(ie)
#pragma loop name main #1 #1
    for (ie = 0; ie < nelt; ie++) {
      laplacian(ta2[ie], ta1[ie], size_e[ie]);
    }
/* compute the residual  */
#pragma cetus private(i, j, k)
#pragma loop name main #1 #2
#pragma cetus parallel
#pragma omp parallel for private(i, j, k)
    for (ie = 0; ie < nelt; ie++) {
#pragma cetus private(i, j, k)
#pragma loop name main #1 #2 #0
      for (k = 0; k < 5; k++) {
#pragma cetus private(i, j)
#pragma loop name main #1 #2 #0 #0
        for (j = 0; j < 5; j++) {
#pragma cetus private(i)
#pragma loop name main #1 #2 #0 #0 #0
          for (i = 0; i < 5; i++) {
            trhs[ie][k][j][i] = (trhs[ie][k][j][i] - ta2[ie][k][j][i]);
          }
        }
      }
    }
    /* get the residual on mortar  */
    transfb(rmor, (double *)trhs);
    if (timeron) {
      timer_stop(9);
    }
/* apply boundary condition: zero out the residual on domain boundaries */
/* apply boundary conidtion to trhs */
#pragma cetus private(ie, iside)
#pragma loop name main #1 #3
    for (ie = 0; ie < nelt; ie++) {
#pragma cetus private(iside)
#pragma loop name main #1 #3 #0
      for (iside = 0; iside < 6; iside++) {
        if (cbc[ie][iside] == 0) {
          facev(trhs[ie], iside, 0.0);
        }
      }
    }
    /* apply boundary condition to rmor */
    col2(rmor, tmmor, nmor);
    /* call the conjugate gradient iterative solver */
    diffusion(ifmortar);
    /* add convection and diffusion */
    if (timeron) {
      timer_start(10);
    }
    add2((double *)ta1, (double *)t, ntot);
    if (timeron) {
      timer_stop(10);
    }
    /* perform mesh adaptation */
    time = (time + dtime);
    if ((step != 0) && (((step / fre) * fre) == step)) {
      if (step != niter) {
        adaptation(&ifmortar, step);
      }
    } else {
      ifmortar = false;
    }
    nelt_tot = (nelt_tot + ((double)nelt));
  }
  timer_stop(1);
  tmax = timer_read(1);
  verify(&Class, &verified);
  /* compute millions of collocation points advanced per second. */
  /* diffusion: nmxh advancements, convection: 1 advancement */
  mflops = ((nelt_tot * ((double)(((5 * 5) * 5) * (nmxh + 1)))) /
            (tmax * 1000000.0));
  print_results("UA", Class, 4, 0, 0, niter, tmax, mflops,
                "    coll. point advanced", verified, "3.3.1", "09 Sep 2026",
                "gcc", "$(CC)", "-lm", "-I../common",
                "-g -Wall -O3 -fopenmp -mcmodel=large",
                "-O3 -fopenmp -mcmodel=large", "(none)");
  /* --------------------------------------------------------------------- */
  /* More timers */
  /* --------------------------------------------------------------------- */
  if (timeron) {
#pragma cetus private(i)
#pragma loop name main #2
    for (i = 1; i <= 10; i++) {
      trecs[i] = timer_read(i);
    }
    if (tmax == 0.0) {
      tmax = 1.0;
    }
    printf("  SECTION     Time (secs)\n");
#pragma cetus private(i, t2)
#pragma loop name main #3
    for (i = 1; i <= 10; i++) {
      printf("  %-10s:%9.3f  (%6.2f%%)\n", t_names[i], trecs[i],
             (trecs[i] * 100.0) / tmax);
      if (i == 4) {
        t2 = (trecs[3] - trecs[4]);
        printf("    --> %11s:%9.3f  (%6.2f%%)\n", "sub-convect", t2,
               (t2 * 100.0) / tmax);
      } else {
        if (i == 7) {
          t2 = ((trecs[5] - trecs[6]) - trecs[7]);
          printf("    --> %11s:%9.3f  (%6.2f%%)\n", "sub-diffuse", t2,
                 (t2 * 100.0) / tmax);
        }
      }
    }
  }
  return 0;
}
