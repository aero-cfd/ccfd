/*
 * exactRiemann.c
 *
 * Created: Sun 29 Mar 2020 12:35:17 PM CEST
 * Author : hhh
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "equation.h"

double G[9];
double tol = 1e-6;
int nIter = 1000;

/*
 * helper function
 */
void preFun(double *f, double *fd, double p, double rhok, double pk, double ck)
{
	if (p < pk) {
		double pRat = p / pk;
		*f = G[3] * ck * (pow(pRat, G[0]) - 1.0);
		*fd = (1.0 / (rhok * ck)) * pow(pRat, -G[1]);
	} else {
		double ak = G[4] / rhok;
		double bk = G[5] * pk;
		double qrt = sqrt(ak / (bk + p));
		*f = (p - pk) * qrt;
		*fd = (1.0 - 0.5 * (p - pk) / (bk + p)) * qrt;
	}
}

/*
 * calculate the exact solution to the Riemann problem
 */
void exactRiemann(double rhol, double rhor, double *rho,
		  double ul, double ur, double *u,
		  double pl, double pr, double *p,
		  double al, double ar, double s)
{
	/* fill G matrix */
	G[0] = (gamma - 1.0) / (2.0 * gamma);
	G[1] = (gamma + 1.0) / (2.0 * gamma);
	G[2] = 2.0 * gamma / (gamma - 1.0);
	G[3] = 2.0 / (gamma - 1.0);
	G[4] = 2.0 / (gamma + 1.0);
	G[5] = (gamma - 1.0) / (gamma + 1.0);
	G[6] = 0.5 * (gamma - 1.0);
	G[7] = 1.0 / gamma;
	G[8] = gamma - 1.0;

	double du = ur - ul;
	double duCrit = G[3] * (al + ar) - du;
	if (duCrit < 0.0) {
		printf("| ERROR: Vacuum is generated by given data:\n");
		printf("| rho_l = %g\n", rhol);
		printf("| rho_r = %g\n", rhor);
		printf("| u_l   = %g\n", ul);
		printf("| u_r   = %g\n", ur);
		printf("| p_l   = %g\n", pl);
		printf("| p_r   = %g\n", pr);
		printf("| a_l   = %g\n", al);
		printf("| a_r   = %g\n", ar);
		printf("| du    = %g\n", du);
		printf("| G[3]  = %g\n", G[3]);
		printf("| al+ar = %g\n", al + ar);
		exit(1);
	}

	/* starting pressure */
	double qMax = 2.0;
	double pv = 0.5 * (pl + pr)
		- 0.125 * (ur - ul) * (rhol + rhor) * (al + ar);
	double pMin = fmin(pl, pr);
	double pMax = fmax(pl, pr);
	double qRat = pMax / pMin;
	if ((qRat < qMax) && (pMin < pv) && (pv < pMax)) {
		*p = fmax(tol, pv);
	} else {
		if (pv < pMin) {
			double pnu = al + ar - G[6] * (ur - ul);
			double pde = al / pow(pl, G[0]) + ar / pow(pr, G[0]);
			*p = pow(pnu / pde, G[2]);
		} else {
			double gel = sqrt((G[4] / rhol) / (G[5] * pl + fmax(tol, pv)));
			double ger = sqrt((G[4] / rhor) / (G[5] * pr + fmax(tol, pv)));
			*p = (gel * pl + ger * pr - (ur - ul)) / (gel + ger);
			*p = fmax(tol, *p);
		}
	}

	/* iteration */
	double p0 = *p;
	double cha = 2.0 * tol;
	int KK = 0;
	double fr, fl, frd, fld;
	while ((cha > tol) && (KK < nIter)) {
		preFun(&fl, &fld, *p, rhol, pl, al);
		preFun(&fr, &frd, *p, rhor, pr, ar);

		*p -= (fl + fr + du) / (fld + frd);
		cha = 2.0 * fabs((*p - p0) / (*p + p0));

		if (*p < 0.0) {
			*p = tol;
		}

		p0 = *p;

		KK++;
	}

	if (KK > nIter) {
		printf("| WARNING: Divergence in Newton-Raphson Scheme\n");
	}

	*u = 0.5 * (ul + ur + fr - fl);

	double pm = *p;
	double um = *u;

	/* sample */
	if (s < um) {
		if (pm < pl) {
			if (s < ul - al) {
				*rho = rhol;
				*u = ul;
				*p = pl;
			} else {
				double cml = al * pow(pm / pl, G[0]);
				double stl = um - cml;
				if (s > stl) {
					*rho = rhol * pow(pm / pl, G[7]);
					*u = um;
					*p = pm;
				} else {
					*u = G[4] * (al + G[6] * ul + s);
					double c = G[4] * (al + G[6] * (ul - s));
					*rho = rhol * pow(c / al, G[3]);
					*p = pl * pow(c / al, G[2]);
				}
			}
		} else {
			double pml = pm / pl;
			double sl = ul - al * sqrt(G[1] * pml + G[0]);
			if (s < sl) {
				*rho = rhol;
				*u = ul;
				*p = pl;
			} else {
				*rho = rhol * (pml + G[5]) / (pml * G[5] + 1.0);
				*u = um;
				*p = pm;
			}
		}
	} else {
		if (pm > pr) {
			double pmr = pm / pr;
			double sr = ur + ar * sqrt(G[1] * pmr + G[0]);
			if (s > sr) {
				*rho = rhor;
				*u = ur;
				*p = pr;
			} else {
				*rho = rhor * (pmr + G[5]) / (pmr * G[5] + 1.0);
				*u = um;
				*p = pm;
			}
		} else {
			double shr = ur + ar;
			if (s > shr) {
				*rho = rhor;
				*u = ur;
				*p = pr;
			} else {
				double cmr = ar * pow(pm / pr, G[0]);
				double str = um + cmr;
				if (s < str) {
					*rho = rhor * pow(pm / pr, G[7]);
					*u = um;
					*p = pm;
				} else {
					*u = G[4] * (-ar + G[6] * ur + s);
					double c = G[4] * (ar - G[6] * (ur - s));
					*rho = rhor * pow(c / ar, G[3]);
					*p = pr * pow(c / ar, G[2]);
				}
			}
		}
	}
}