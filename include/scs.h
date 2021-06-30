#ifndef SCS_H_GUARD
#define SCS_H_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "aa.h"
#include "glbopts.h"

/* private data structs (that you define) containing any necessary data to solve
 * linear system, etc. this defines the matrix A, only the linear system solver
 * interacts with this struct */
typedef struct SCS_A_DATA_MATRIX ScsMatrix;
/* stores the necessary private workspace, only the linear system solver
 * interacts with this struct */
typedef struct SCS_LIN_SYS_WORK ScsLinSysWork;

typedef struct SCS_PROBLEM_DATA ScsData;
typedef struct SCS_SETTINGS ScsSettings;
typedef struct SCS_SOL_VARS ScsSolution;
typedef struct SCS_INFO ScsInfo;
typedef struct SCS_SCALING ScsScaling;
typedef struct SCS_WORK ScsWork;
typedef struct SCS_RESIDUALS ScsResiduals;
typedef struct SCS_CONE ScsCone;
typedef struct SCS_ACCEL_WORK ScsAccelWork;
typedef struct SCS_CONE_WORK ScsConeWork;

/* struct containing problem data */
struct SCS_PROBLEM_DATA {
  /* these cannot change for multiple runs for the same call to SCS(init) */
  scs_int m, n; /* A has m rows, n cols */
  ScsMatrix *A; /* A is supplied in data format specified by linsys solver */
  ScsMatrix *P; /* P is supplied in data format specified by linsys solver */

  /* these can change for multiple runs for the same call to SCS(init) */
  scs_float *b, *c; /* dense arrays for b (size m), c (size n) */

  ScsSettings *stgs; /* contains solver settings specified by user */
};

/* ScsSettings struct */
struct SCS_SETTINGS {
  /* settings parameters: default suggested input */

  /* these *cannot* change for multiple runs with the same call to SCS(init) */
  scs_int normalize; /* boolean, heuristic data rescaling: 1 */
  scs_float scale;   /* if normalized, rescales by this factor: 5 */
  scs_float rho_x;   /* x equality constraint scaling: 1e-3 */

  /* these can change for multiple runs with the same call to SCS(init) */
  scs_int max_iters;    /* maximum iterations to take: 2500 */
  scs_float eps_abs;    /* absolute convergence tolerance: 1e-4 */
  scs_float eps_rel;    /* relative convergence tolerance: 1e-4 */
  scs_float eps_infeas; /* infeasible convergence tolerance: 1e-5 */
  scs_float alpha;      /* relaxation parameter: 1.8 */
  scs_float time_limit_secs; /* time limit in secs (can be fractional) */
  scs_int verbose;      /* boolean, write out progress: 1 */
  scs_int warm_start;   /* boolean, warm start (put initial guess in ScsSolution
                           struct): 0 */
  scs_int acceleration_lookback;   /* memory for acceleration */
  scs_int acceleration_interval;   /* interval to apply acceleration */
  scs_int adaptive_scaling; /* whether to adaptively update the scale param */
  const char *write_data_filename; /* string, if set will dump raw prob data */
  const char *log_csv_filename; /* string, if set will log solve */
};

/* NB: rows of data matrix A must be specified in this exact order */
struct SCS_CONE {
  scs_int f;          /* number of linear equality constraints */
  scs_int l;          /* length of LP cone */
  scs_float *bu, *bl; /* upper/lower box values, length = bsize - 1 */
  scs_int bsize;      /* length of box cone constraint, including scale t */
  scs_int *q;         /* array of second-order cone constraints */
  scs_int qsize;      /* length of SOC array */
  scs_int *s;         /* array of SD constraints */
  scs_int ssize;      /* length of SD array */
  scs_int ep;         /* number of primal exponential cone triples */
  scs_int ed;         /* number of dual exponential cone triples */
  scs_float *p;       /* array of power cone params, must be \in [-1, 1],
                         negative values are interpreted as specifying the
                         dual cone */
  scs_int psize;      /* number of (primal and dual) power cone triples */
};

/* contains primal-dual solution arrays */
struct SCS_SOL_VARS {
  scs_float *x, *y, *s;
};

/* contains terminating information */
struct SCS_INFO {
  scs_int iter;          /* number of iterations taken */
  char status[64];       /* status string, e.g. 'solved' */
  scs_int status_val;    /* status as scs_int, defined in glbopts.h */
  scs_int scale_updates; /* number of updates to scale */
  scs_float pobj;        /* primal objective */
  scs_float dobj;        /* dual objective */
  scs_float res_pri;     /* primal equality residual */
  scs_float res_dual;    /* dual equality residual */
  scs_float res_infeas;  /* infeasibility cert residual */
  scs_float res_unbdd_a; /* unbounded cert residual */
  scs_float res_unbdd_p; /* unbounded cert residual */
  scs_float gap;         /* relative duality gap */
  scs_float setup_time;  /* time taken for setup phase (milliseconds) */
  scs_float solve_time;  /* time taken for solve phase (milliseconds) */
  scs_float scale;       /* (final) scale parameter */
};

/* contains normalization variables */
struct SCS_SCALING {
  scs_float *D, *E; /* for normalization */
  scs_float primal_scale, dual_scale;
};

/*
 * main library api's:
 * SCS(init): allocates memory etc (e.g., factorize matrix [I A; A^T -I])
 * SCS(solve): can be called many times with different b,c data per init call
 * SCS(finish): cleans up the memory (one per init call)
 */
ScsWork *SCS(init)(const ScsData *d, const ScsCone *k, ScsInfo *info);
scs_int SCS(solve)(ScsWork *w, const ScsData *d, const ScsCone *k,
                   ScsSolution *sol, ScsInfo *info);
void SCS(finish)(ScsWork *w);
/* scs calls SCS(init), SCS(solve), and SCS(finish) */
scs_int scs(const ScsData *d, const ScsCone *k, ScsSolution *sol,
            ScsInfo *info);

const char *SCS(version)(void);
size_t SCS(sizeof_int)(void);
size_t SCS(sizeof_float)(void);

/* the following structs are not exposed to user */

/* workspace for SCS */
struct SCS_WORK {
  /* x_prev = x from previous iteration */
  scs_int time_limit_reached; /* set if the time-limit is reached */
  scs_float *u, *v, *u_t, *v_prev, *rsk;
  scs_float *h; /* h = [c; b] */
  scs_float *g; /* g = (I + M)^{-1} h */
  scs_float *lin_sys_warm_start; /* linear system warm-start (indirect only) */
  scs_float *rho_y_vec; /* vector of rho y parameters (affects cone project) */
  AaWork *accel;          /* struct for acceleration workspace */
  scs_float *b_orig, *c_orig;       /* original b and c vectors */
  scs_float *b_normalized, *c_normalized;    /* normalized b and c vectors */
  scs_int m, n;           /* A has m rows, n cols */
  ScsMatrix *A;           /* (possibly normalized) A matrix */
  ScsMatrix *P;           /* (possibly normalized) P matrix */
  ScsLinSysWork *p;       /* struct populated by linear system solver */
  ScsSettings *stgs;      /* contains solver settings specified by user */
  ScsScaling *scal;       /* contains the re-scaling data */
  ScsConeWork *cone_work; /* workspace for the cone projection step */
  scs_int *cone_boundaries; /* array with boundaries of cones */
  scs_int cone_boundaries_len; /* total length of cones */
  /* normalized and unnormalized residuals */
  ScsResiduals *r_orig, *r_normalized;
  /* track x,y,s as alg progresses, tau *not* divided out */
  ScsSolution *xys_orig, *xys_normalized;
  /* updating scale params workspace */
  scs_float sum_log_scale_factor;
  scs_int last_scale_update_iter, n_log_scale_factor, scale_updates;
  /* aa norm stat */
  scs_float aa_norm;
};

/* to hold residual information, *all are unnormalized* */
struct SCS_RESIDUALS {
  scs_int last_iter;
  scs_float xt_p_x;     /* x' P x  */
  scs_float xt_p_x_tau; /* x'Px * tau^2 *not* divided out */
  scs_float ctx;
  scs_float ctx_tau;  /* tau *not* divided out */
  scs_float bty;
  scs_float bty_tau;  /* tau *not* divided out */
  scs_float pobj; /* primal objective */
  scs_float dobj; /* dual objective */
  scs_float gap; /* pobj - dobj */
  scs_float tau;
  scs_float kap;
  scs_float res_pri;
  scs_float res_dual;
  scs_float res_infeas;
  scs_float res_unbdd_p;
  scs_float res_unbdd_a;
  /* tau NOT divided out */
  scs_float *ax, *ax_s, *px, *aty, *ax_s_btau, *px_aty_ctau;
};

#ifdef __cplusplus
}
#endif
#endif
