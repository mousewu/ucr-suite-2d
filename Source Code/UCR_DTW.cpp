/***********************************************************************/
/************************* DISCLAIMER **********************************/
/***********************************************************************/
/** This UCR Suite software is copyright protected � 2012 by          **/
/** Thanawin Rakthanmanon, Bilson Campana, Abdullah Mueen,            **/
/** Gustavo Batista and Eamonn Keogh.                                 **/
/**                                                                   **/
/** Unless stated otherwise, all software is provided free of charge. **/
/** As well, all software is provided on an "as is" basis without     **/
/** warranty of any kind, express or implied. Under no circumstances  **/
/** and under no legal theory, whether in tort, contract,or otherwise,**/
/** shall Thanawin Rakthanmanon, Bilson Campana, Abdullah Mueen,      **/
/** Gustavo Batista, or Eamonn Keogh be liable to you or to any other **/
/** person for any indirect, special, incidental, or consequential    **/
/** damages of any character including, without limitation, damages   **/
/** for loss of goodwill, work stoppage, computer failure or          **/
/** malfunction, or for any and all other damages or losses.          **/
/**                                                                   **/
/** If you do not agree with these terms, then you you are advised to **/
/** not use this software.                                            **/
/***********************************************************************/
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <time.h>
#include <iostream>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))
#define dist(x,y) ((x-y)*(x-y))

#define INF 1e20       //Pseudo Infitinte number for this code

using namespace std;

/// Data structure for sorting the query
typedef struct Index
    {   double value;
        int    index;
    } Index;

/// Data structure (circular array) for finding minimum and maximum for LB_Keogh envolop
struct deque
{   int *dq;
    int size,capacity;
    int f,r;
};


/// Sorting function for the query, sort by abs(z_norm(q[i])) from high to low
int comp(const void *a, const void* b)
{   Index* x = (Index*)a;
    Index* y = (Index*)b;
    return abs(y->value) - abs(x->value);   // high to low
}

/// Initial the queue at the begining step of envelop calculation
void init(deque *d, int capacity)
{
    d->capacity = capacity;
    d->size = 0;
    d->dq = (int *) malloc(sizeof(int)*d->capacity);
    d->f = 0;
    d->r = d->capacity-1;
}

/// Destroy the queue
void destroy(deque *d)
{
    free(d->dq);
}

/// Insert to the queue at the back
void push_back(struct deque *d, int v)
{
    d->dq[d->r] = v;
    d->r--;
    if (d->r < 0)
        d->r = d->capacity-1;
    d->size++;
}

/// Delete the current (front) element from queue
void pop_front(struct deque *d)
{
    d->f--;
    if (d->f < 0)
        d->f = d->capacity-1;
    d->size--;
}

/// Delete the last element from queue
void pop_back(struct deque *d)
{
    d->r = (d->r+1)%d->capacity;
    d->size--;
}

/// Get the value at the current position of the circular queue
int front(struct deque *d)
{
    int aux = d->f - 1;

    if (aux < 0)
        aux = d->capacity-1;
    return d->dq[aux];
}

/// Get the value at the last position of the circular queueint back(struct deque *d)
int back(struct deque *d)
{
    int aux = (d->r+1)%d->capacity;
    return d->dq[aux];
}

/// Check whether or not the queue is empty
int empty(struct deque *d)
{
    return d->size == 0;
}

/// Finding the envelop of min and max value for LB_Keogh
/// Implementation idea is intoruduced by Danial Lemire in his paper
/// "Faster Retrieval with a Two-Pass Dynamic-Time-Warping Lower Bound", Pattern Recognition 42(9), 2009.
void lower_upper_lemire(double *t, int len, int r, double *l, double *u)
{
    struct deque du, dl;

    init(&du, 2*r+2);
    init(&dl, 2*r+2);

    push_back(&du, 0);
    push_back(&dl, 0);

    for (int i = 1; i < len; i++)
    {
        if (i > r)
        {
            u[i-r-1] = t[front(&du)];
            l[i-r-1] = t[front(&dl)];
        }
        if (t[i] > t[i-1])
        {
            pop_back(&du);
            while (!empty(&du) && t[i] > t[back(&du)])
                pop_back(&du);
        }
        else
        {
            pop_back(&dl);
            while (!empty(&dl) && t[i] < t[back(&dl)])
                pop_back(&dl);
        }
        push_back(&du, i);
        push_back(&dl, i);
        if (i == 2 * r + 1 + front(&du))
            pop_front(&du);
        else if (i == 2 * r + 1 + front(&dl))
            pop_front(&dl);
    }
    for (int i = len; i < len+r+1; i++)
    {
        u[i-r-1] = t[front(&du)];
        l[i-r-1] = t[front(&dl)];
        if (i-front(&du) >= 2 * r + 1)
            pop_front(&du);
        if (i-front(&dl) >= 2 * r + 1)
            pop_front(&dl);
    }
    destroy(&du);
    destroy(&dl);
}

/// Calculate quick lower bound
/// Usually, LB_Kim take time O(m) for finding top,bottom,fist and last.
/// However, because of z-normalization the top and bottom cannot give siginifant benefits.
/// And using the first and last points can be computed in constant time.
/// The prunning power of LB_Kim is non-trivial, especially when the query is not long, say in length 128.
double lb_kim_hierarchy(double *t, double *q, int j, int len, double mean, double std, double bsf = INF)
{
    /// 1 point at front and back
    double d, lb;
    double x0 = (t[j] - mean) / std;
    double y0 = (t[(len-1+j)] - mean) / std;
    lb = dist(x0,q[0]) + dist(y0,q[len-1]);
    if (lb >= bsf)   return lb;

    /// 2 points at front
    double x1 = (t[(j+1)] - mean) / std;
    d = min(dist(x1,q[0]), dist(x0,q[1]));
    d = min(d, dist(x1,q[1]));
    lb += d;
    if (lb >= bsf)   return lb;

    /// 2 points at back
    double y1 = (t[(len-2+j)] - mean) / std;
    d = min(dist(y1,q[len-1]), dist(y0, q[len-2]) );
    d = min(d, dist(y1,q[len-2]));
    lb += d;
    if (lb >= bsf)   return lb;

    /// 3 points at front
    double x2 = (t[(j+2)] - mean) / std;
    d = min(dist(x0,q[2]), dist(x1, q[2]));
    d = min(d, dist(x2,q[2]));
    d = min(d, dist(x2,q[1]));
    d = min(d, dist(x2,q[0]));
    lb += d;
    if (lb >= bsf)   return lb;

    /// 3 points at back
    double y2 = (t[(len-3+j)] - mean) / std;
    d = min(dist(y0,q[len-3]), dist(y1, q[len-3]));
    d = min(d, dist(y2,q[len-3]));
    d = min(d, dist(y2,q[len-2]));
    d = min(d, dist(y2,q[len-1]));
    lb += d;

    return lb;
}

/// LB_Keogh 1: Create Envelop for the query
/// Note that because the query is known, envelop can be created once at the begenining.
///
/// Variable Explanation,
/// order : sorted indices for the query.
/// uo, lo: upper and lower envelops for the query, which already sorted.
/// t     : a circular array keeping the current data.
/// j     : index of the starting location in t
/// cb    : (output) current bound at each position. It will be used later for early abandoning in DTW.
double lb_keogh_cumulative(int* order, double *t, double *uo, double *lo, double *cb, int j, int len, double mean, double std, double best_so_far = INF)
{
    double lb = 0;
    double x, d;

    for (int i = 0; i < len && lb < best_so_far; i++)
    {
        x = (t[(order[i]+j)] - mean) / std;
        d = 0;
        if (x > uo[i])
            d = dist(x,uo[i]);
        else if(x < lo[i])
            d = dist(x,lo[i]);
        lb += d;
        cb[order[i]] = d;
    }
    return lb;
}

/// LB_Keogh 2: Create Envelop for the data
/// Note that the envelops have been created (in main function) when each data point has been read.
///
/// Variable Explanation,
/// tz: Z-normalized data
/// qo: sorted query
/// cb: (output) current bound at each position. Used later for early abandoning in DTW.
/// l,u: lower and upper envelop of the current data
double lb_keogh_data_cumulative(int* order, double *tz, double *qo, double *cb, double *l, double *u, int len, double mean, double std, double best_so_far = INF)
{
    double lb = 0;
    double uu,ll,d;

    for (int i = 0; i < len && lb < best_so_far; i++)
    {
        uu = (u[order[i]]-mean)/std;
        ll = (l[order[i]]-mean)/std;
        d = 0;
        if (qo[i] > uu)
            d = dist(qo[i], uu);
        else
        {   if(qo[i] < ll)
            d = dist(qo[i], ll);
        }
        lb += d;
        cb[order[i]] = d;
    }
    return lb;
}

struct dtw_distance {
  double d1, d2;
};

dtw_distance dtw(double* A1, double* A2, double* B1, double* B2,
           double* cb1, double* cb2, int m, int r,
           double bsf1 = INF, double bsf2 = INF) {
  double *cost1;
  double *cost2;
  double *cost_prev1;
  double *cost_prev2;
  double *cost_tmp1;
  double *cost_tmp2;

  int i, j, k;
  double x1, x2, y1, y2, z1, z2, min_cost1, min_cost2;

  cost1 = (double*)malloc(sizeof(double)*(2*r+1));
  cost2 = (double*)malloc(sizeof(double)*(2*r+1));
  cost_prev1 = (double*)malloc(sizeof(double)*(2*r+1));
  cost_prev2 = (double*)malloc(sizeof(double)*(2*r+1));
  for(k=0; k<2*r+1; k++) {
    cost1[k] = INF;
    cost2[k] = INF;
    cost_prev1[k] = INF;
    cost_prev2[k] = INF;
  }

  for(i = 0; i < m; ++i) {
    k = max(0, r - i);
    min_cost1 = INF;
    min_cost2 = INF;
    
    for(j = max(0, i - r); j <= min(m - 1, i + r); ++j, ++k) {
      if(i == 0 && j == 0) {
        cost1[k] = dist(A1[0], B1[0]);
        cost2[k] = dist(A2[0], B2[0]);
        min_cost1 = cost1[k];
        min_cost2 = cost2[k];
        continue;
      }

      if(j - 1 < 0 || k - 1 < 0) {
        y1 = INF;
        y2 = INF;
      } else {
        y1 = cost1[k - 1];
        y2 = cost2[k - 1];
      }

      if(i - 1 < 0 || k + 1 > 2 * r) {
        x1 = INF;
        x2 = INF;
      } else {
        x1 = cost_prev1[k + 1];
        x2 = cost_prev2[k + 1];
      }

      if(i - 1 < 0 || k + 1 > 2 * r) {
        z1 = INF;
        z2 = INF;
      } else {
        z1 = cost_prev1[k];
        z2 = cost_prev2[k];
      }

      cost1[k] = min(min(x1,y1), z1) + dist(A1[i], B1[j]);
      cost2[k] = min(min(x2,y2), z2) + dist(A2[i], B2[j]);

      if(cost1[k] < min_cost1 && cost2[k] < min_cost2) {
        min_cost1 = cost1[k];
        min_cost2 = cost2[k];
      }
    }
    if(i + r < m - 1
       && min_cost1 + cb1[i+r+1] >= bsf1
       && min_cost2 + cb2[i+r+1] >= bsf2) {
      free(cost1);
      free(cost2);
      free(cost_prev1);
      free(cost_prev2);
      dtw_distance d;
      d.d1 = min_cost1;
      d.d2 = min_cost2;
      return d;
    }    
    
    cost_tmp1 = cost1;
    cost_tmp2 = cost2;
    cost1 = cost_prev1;
    cost2 = cost_prev2;
    cost_prev1 = cost_tmp1;
    cost_prev2 = cost_tmp2;
  }
  --k;
  double final_dtw1 = cost_prev1[k];
  double final_dtw2 = cost_prev2[k];
  free(cost1);
  free(cost2);
  free(cost_prev1);
  free(cost_prev2);
  dtw_distance d;
  d.d1 = final_dtw1;
  d.d2 = final_dtw2;
  return d;
}

/// Calculate Dynamic Time Wrapping distance
/// A,B: data and query, respectively
/// cb : cummulative bound used for early abandoning
/// r  : size of Sakoe-Chiba warpping band
double dtw(double* A, double* B, double *cb, int m, int r, double bsf = INF)
{

    double *cost;
    double *cost_prev;
    double *cost_tmp;
    int i,j,k;
    double x,y,z,min_cost;

    /// Instead of using matrix of size O(m^2) or O(mr), we will reuse two array of size O(r).
    cost = (double*)malloc(sizeof(double)*(2*r+1));
    for(k=0; k<2*r+1; k++)    cost[k]=INF;

    cost_prev = (double*)malloc(sizeof(double)*(2*r+1));
    for(k=0; k<2*r+1; k++)    cost_prev[k]=INF;

    for (i=0; i<m; i++)
    {
        k = max(0,r-i);
        min_cost = INF;

        for(j=max(0,i-r); j<=min(m-1,i+r); j++, k++)
        {
            /// Initialize all row and column
            if ((i==0)&&(j==0))
            {
                cost[k]=dist(A[0],B[0]);
                min_cost = cost[k];
                continue;
            }

            if ((j-1<0)||(k-1<0))
              y = INF;
            else
              y = cost[k-1];
            if ((i-1<0)||(k+1>2*r))
              x = INF;
            else
              x = cost_prev[k+1];
            if ((i-1<0)||(j-1<0))
              z = INF;
            else
              z = cost_prev[k];

            /// Classic DTW calculation
            cost[k] = min( min( x, y) , z) + dist(A[i],B[j]);

            /// Find minimum cost in row for early abandoning (possibly to use column instead of row).
            if (cost[k] < min_cost)
            {   min_cost = cost[k];
            }
        }

        /// We can abandon early if the current cummulative distace with lower bound together are larger than bsf
        if (i+r < m-1 && min_cost + cb[i+r+1] >= bsf)
        {   free(cost);
            free(cost_prev);
            return min_cost + cb[i+r+1];
        }

        /// Move current array to previous array.
        cost_tmp = cost;
        cost = cost_prev;
        cost_prev = cost_tmp;
    }
    k--;

    /// the DTW distance is in the last cell in the matrix of size O(m^2) or at the middle of our array.
    double final_dtw = cost_prev[k];
    free(cost);
    free(cost_prev);
    return final_dtw;
}

/// Print function for debugging
void printArray(double *x, int len)
{   for(int i=0; i<len; i++)
        printf(" %6.2lf\n",x[i]);
    printf("\n");
}

/// If expected error happens, teminated the program.
void error(int id)
{
    if(id==1)
        printf("ERROR : Memory can't be allocated!!!\n\n");
    else if ( id == 2 )
        printf("ERROR : File not Found!!!\n\n");
    else if ( id == 3 )
        printf("ERROR : Can't create Output File!!!\n\n");
    else if ( id == 4 )
    {
        printf("ERROR : Invalid Number of Arguments!!!\n");
        printf("Command Usage:  UCR_DTW.exe  data-file  query-file   m   R\n\n");
        printf("For example  :  UCR_DTW.exe  data.txt   query.txt   128  0.05\n");
    }
    exit(1);
}

/// Main Function
int main(  int argc , char *argv[] )
{
    FILE *fp;            /// data file pointer
    FILE *qp;            /// query file pointer
    double bsf, bsfA;          /// best-so-far
    double *t, *tA, *q, *qA;       /// data array and query array
    int *order, *orderA;          ///new order of the query
    double *u, *l, *qo, *uo, *lo,*tz,*cb, *cb1, *cb2,*u_d, *l_d;
    double *uA, *lA, *qoA, *uoA, *loA, *tzA, *cbA, *cb1A, *cb2A, *u_dA, *l_dA;


    double d;
    double dA;
    long long i , j;
    double ex , ex2 , mean, std;
    double exA, ex2A, meanA, stdA;
    int m=-1, r=-1;
    long long loc = 0;
    double t1,t2;
    int kim = 0,keogh = 0, keogh2 = 0;
    double dist=0, lb_kim=0, lb_k=0, lb_k2=0;
    double distA = 0, lb_kimA = 0, lb_kA = 0, lb_k2A = 0;
    double *buffer, *u_buff, *l_buff;
    double *bufferA, *u_buffA, *l_buffA;
    Index *Q_tmp, *QA_tmp;

    /// For every EPOCH points, all cummulative values, such as ex (sum), ex2 (sum square), will be restarted for reducing the floating point error.
    int EPOCH = 100000;

    /// If not enough input, display an error.
    if (argc<=3)
        error(4);

    /// read size of the query
    if (argc>3)
        m = atol(argv[3]);

    /// read warping windows
    if (argc>4)
    {   double R = atof(argv[4]);
        if (R<=1)
            r = floor(R*m);
        else
            r = floor(R);
    }

    fp = fopen(argv[1],"r");
    if( fp == NULL )
        error(2);

    qp = fopen(argv[2],"r");
    if( qp == NULL )
        error(2);

    /// start the clock
    t1 = clock();


    /// malloc everything here
    q = (double *)malloc(sizeof(double)*m);
    if( q == NULL )
        error(1);

    qA = (double *)malloc(sizeof(double)*m);
    if( qA == NULL )
        error(1);
    
    qo = (double *)malloc(sizeof(double)*m);
    if( qo == NULL )
        error(1);
    uo = (double *)malloc(sizeof(double)*m);
    if( uo == NULL )
        error(1);
    lo = (double *)malloc(sizeof(double)*m);
    if( lo == NULL )
        error(1);

    qoA = (double *)malloc(sizeof(double)*m);
    if( qo == NULL )
        error(1);
    uoA = (double *)malloc(sizeof(double)*m);
    if( uo == NULL )
        error(1);
    loA = (double *)malloc(sizeof(double)*m);
    if( lo == NULL )
        error(1);

    order = (int *)malloc(sizeof(int)*m);
    if( order == NULL )
        error(1);

    orderA = (int *)malloc(sizeof(int)*m);
    if( orderA == NULL )
        error(1);

    Q_tmp = (Index *)malloc(sizeof(Index)*m);
    if( Q_tmp == NULL )
        error(1);

    QA_tmp = (Index *)malloc(sizeof(Index)*m);
    if( QA_tmp == NULL )
        error(1);

    u = (double *)malloc(sizeof(double)*m);
    if( u == NULL )
        error(1);
    uA = (double *)malloc(sizeof(double)*m);
    if( uA == NULL )
        error(1);

    l = (double *)malloc(sizeof(double)*m);
    if( l == NULL )
        error(1);

    lA = (double *)malloc(sizeof(double)*m);
    if( lA == NULL )
        error(1);

    cb = (double *)malloc(sizeof(double)*m);
    if( cb == NULL )
        error(1);

    cb1 = (double *)malloc(sizeof(double)*m);
    if( cb1 == NULL )
        error(1);

    cb2 = (double *)malloc(sizeof(double)*m);
    if( cb2 == NULL )
        error(1);

    cbA = (double *)malloc(sizeof(double)*m);
    if( cb == NULL )
        error(1);

    cb1A = (double *)malloc(sizeof(double)*m);
    if( cb1 == NULL )
        error(1);

    cb2A = (double *)malloc(sizeof(double)*m);
    if( cb2 == NULL )
        error(1);
    
    u_d = (double *)malloc(sizeof(double)*m);
    if( u == NULL )
        error(1);

    l_d = (double *)malloc(sizeof(double)*m);
    if( l == NULL )
        error(1);

    t = (double *)malloc(sizeof(double)*m*2);
    if( t == NULL )
        error(1);

    tA = (double *)malloc(sizeof(double)*m*2);
    if( tA == NULL )
        error(1);

    tz = (double *)malloc(sizeof(double)*m);
    if( tz == NULL )
        error(1);

    tzA = (double *)malloc(sizeof(double)*m);
    if( tzA == NULL )
        error(1);

    buffer = (double *)malloc(sizeof(double)*EPOCH);
    if( buffer == NULL )
        error(1);

    u_buff = (double *)malloc(sizeof(double)*EPOCH);
    if( u_buff == NULL )
        error(1);

    l_buff = (double *)malloc(sizeof(double)*EPOCH);
    if( l_buff == NULL )
        error(1);
    
    bufferA = (double *)malloc(sizeof(double)*EPOCH);
    if( bufferA == NULL )
        error(1);

    u_buffA = (double *)malloc(sizeof(double)*EPOCH);
    if( u_buffA == NULL )
        error(1);
    
    l_buffA = (double *)malloc(sizeof(double)*EPOCH);
    if( l_buffA == NULL )
        error(1);


    /// Read query file
    bsfA = bsf = INF;
    
    i = 0;
    j = 0;
    ex = ex2 = 0;
    exA = ex2A = 0;

    while(fscanf(qp,"%lf %lf",&d,&dA) != EOF && i < m)
    {
        ex += d;
        ex2 += d*d;
        
        exA += dA;
        ex2A += dA*dA;
        q[i] = d;
        qA[i] = dA;
        i++;
    }
    fclose(qp);

    /// Do z-normalize the query, keep in same array, q
    mean = ex/m;
    std = ex2/m;
    std = sqrt(std-mean*mean);

    meanA = exA/m;
    stdA = ex2A/m;
    std = sqrt(std-mean*mean);
    for( i = 0 ; i < m ; i++ ) {
         q[i] = (q[i] - mean)/std;
         qA[i] = (qA[i] - meanA)/stdA;
    }

    /// Create envelop of the query: lower envelop, l, and upper envelop, u
    //    lower_upper_lemire(
    lower_upper_lemire(q, m, r, l, u);
    lower_upper_lemire(qA, m, r, lA, uA);

    /// Sort the query one time by abs(z-norm(q[i]))
    for( i = 0; i<m; i++) {
      QA_tmp[i].value = qA[i];
      QA_tmp[i].index = i;
      Q_tmp[i].value = q[i];
      Q_tmp[i].index = i;
    }
    qsort(Q_tmp, m, sizeof(Index),comp);
    qsort(QA_tmp, m, sizeof(Index),comp);

    /// also create another arrays for keeping sorted envelop
    for( i=0; i<m; i++) {
      int o = Q_tmp[i].index;
      int oA = QA_tmp[i].index;

      order[i] = o;
      orderA[i] = oA;

      qo[i] = q[o];
      qoA[i] = qA[oA];
      
      uo[i] = u[o];
      uoA[i] = uA[oA];
      
      lo[i] = l[o];
      loA[i] = lA[oA];
    }
    free(Q_tmp);
    free(QA_tmp);

    
    /// Initial the cummulative lower bound
    for( i=0; i<m; i++) {
      cb[i]=0;
      cbA[i]=0;
      
      cb1[i]=0;
      cb1A[i] = 0;
        
      cb2[i]=0;
      cb2A[i]=0;
    }

    i = 0;          /// current index of the data in current chunk of size EPOCH
    j = 0;          /// the starting index of the data in the circular array, t
    ex = ex2 = 0;
    exA = ex2A = 0;
    bool done = false;
    int it=0, ep=0, k=0;
    long long I;    /// the starting index of the data in current chunk of size EPOCH
    while(!done) {
      /// Read first m-1 points
      ep=0;
      if (it==0){
        for(k=0; k<m-1; k++) {
          if (fscanf(fp,"%lf\t%lf", &d, &dA) != EOF) {
            buffer[k] = d;
            bufferA[k] = dA;
          }
        }
      } else {
        for(k=0; k<m-1; k++) {
          buffer[k] = buffer[EPOCH-m+1+k];
          bufferA[k] = buffer[EPOCH-m+1+k];
        }
      }

      /// Read buffer of size EPOCH or when all data has been read.
      ep=m-1;
      while(ep<EPOCH) {
        if (fscanf(fp,"%lf\t%lf", &d, &dA) == EOF) {
          break;
        }
        buffer[ep] = d;
        bufferA[ep] = dA;
        ep++;
      }
      
      /// Data are read in chunk of size EPOCH.
      /// When there is nothing to read, the loop is end.
      if (ep<=m-1) {
        done = true;
      } else {
        lower_upper_lemire(buffer, ep, r, l_buff, u_buff);
        lower_upper_lemire(bufferA, ep, r, l_buffA, u_buffA);

        /// Just for printing a dot for approximate a million point. Not much accurate.
        if (it%(1000000/(EPOCH-m+1))==0) {
          //          fprintf(stderr,".");
        }

        /// Do main task here..
        ex=0; ex2=0;
        exA = 0; ex2A = 0;
        for(i=0; i<ep; i++) {
          /// A bunch of data has been read and pick one of them at a time to use
          d = buffer[i];
          dA = bufferA[i];
          
          /// Calcualte sum and sum square
          ex += d;
          exA += dA;
          
          ex2 += d*d;
          ex2A += dA*dA;
          
          /// t is a circular array for keeping current data
          t[i%m] = d;
          tA[i%m] = dA;
          /// Double the size for avoiding using modulo "%" operator
          t[(i%m)+m] = d;
          tA[(i%m)+m] = dA;
          
          /// Start the task when there are more than m-1 points in the current chunk
          if( i >= m-1 ) {
            mean = ex/m;
            meanA = exA/m;
            std = ex2/m;
            stdA = ex2A/m;
            std = sqrt(std-mean*mean);
            stdA = sqrt(stdA - mean*mean);
            
            /// compute the start location of the data in the current circular array, t
            j = (i+1)%m;
            /// the start location of the data in the current chunk
            I = i-(m-1);
            
            /// Use a constant lower bound to prune the obvious subsequence
            /// Compute both at once.
            lb_kim = lb_kim_hierarchy(t, q, j, m, mean, std, bsf);
            lb_kimA = lb_kim_hierarchy(tA, qA, j, m, meanA, stdA, bsfA);

            if (lb_kim < bsf && lb_kimA < bsfA) {
              /// Use a linear time lower bound to prune;
              /// z_normalization of t will be computed on the fly.
              /// uo, lo are envelop of the query.
              lb_k = lb_keogh_cumulative(order, t, uo, lo, cb1, j, m, mean, std, bsf);
              if(lb_k < bsf) {
                lb_kA = lb_keogh_cumulative(orderA, tA, uoA, loA, cb1A, j, m, meanA, stdA, bsfA);
              } else {
                lb_kA = INF;
              }
              if (lb_k < bsf && lb_kA < bsfA) {
                /// Take another linear time to compute z_normalization of t.
                /// Note that for better optimization, this can merge to the previous function.
                for(k=0;k<m;k++) {
                  tz[k] = (t[(k+j)] - mean)/std;
                  tzA[k] = (tA[k+j] - meanA)/stdA;
                }
                  
                /// Use another lb_keogh to prune
                /// qo is the sorted query. tz is unsorted z_normalized data.
                /// l_buff, u_buff are big envelop for all data in this chunk
                lb_k2 = lb_keogh_data_cumulative(order, tz, qo, cb2, l_buff+I, u_buff+I, m, mean, std, bsf);
                if(lb_k2 < bsf) {
                  lb_k2A = lb_keogh_data_cumulative(orderA, tzA, qoA, cb2A, l_buffA+I, u_buffA+I, m, meanA, stdA, bsfA);
                } else {
                  lb_k2A = INF;
                }
                if (lb_k2 < bsf && lb_k2A < bsf) {
                  /// Choose better lower bound between lb_keogh and lb_keogh2
                  /// to be used in early abandoning DTW
                  /// Note that cb and cb2 will be cumulative summed here.
                  if (lb_k > lb_k2) {
                    cb[m-1]=cb1[m-1];
                    cbA[m-1] = cb1A[m-1];
                    for(k=m-2; k>=0; k--) {
                      cb[k] = cb[k+1]+cb1[k];
                      cbA[k] = cbA[k+1]+cb1A[k];
                    }
                  } else {
                    cb[m-1]=cb2[m-1];
                    cbA[m-1]=cb2A[m-1];
                    for(k=m-2; k>=0; k--) {
                      cb[k] = cb[k+1]+cb2[k];
                      cbA[k] = cbA[k+1]+cb2A[k];
                    }
                  }
                  /// Compute DTW and early abandoning if possible
                  double dist = dtw(tz, q, cb, m, r, bsf);
                  double distA = INF;
                  if(dist < bsf) {
                    distA = dtw(tzA, qA, cbA, m, r, bsfA);
                  }
                  if( dist < bsf && distA < bsfA ) {
                    /// Update bsf
                    /// loc is the real starting location of the nearest neighbor in the file
                    bsf = dist;
                    bsfA = distA;
                    loc = (it)*(EPOCH-m+1) + i-m+1;
                  }
                } else
                  keogh2++;
              } else
                keogh++;
            } else
              kim++;
            
            /// Reduce obsolute points from sum and sum square
            ex -= t[j];
            ex2 -= t[j]*t[j];
          }
        }
        
        /// If the size of last chunk is less then EPOCH, then no more data and terminate.
        if (ep<EPOCH) {
          done=true;
        } else {
          it++;
        }
      }
    }
               
    i = (it)*(EPOCH-m+1) + ep;
    fclose(fp);

    free(q);
    free(u);
    free(l);
    free(uo);
    free(lo);
    free(qo);
    free(cb);
    free(cb1);
    free(cb2);
    free(tz);
    free(t);
    free(l_d);
    free(u_d);
    free(l_buff);
    free(u_buff);

    t2 = clock();
    /*    printf("\n");

    /// Note that loc and i are long long.
    cout << "Location : " << loc << endl;
    cout << "Distance(1) : " << sqrt(bsf) << endl;
    cout << "Distance(2) : " << sqrt(bsfA) << endl; 
    cout << "Data Scanned : " << i << endl;
    cout << "Total Execution Time : " << (t2-t1)/CLOCKS_PER_SEC << " sec" << endl;

    /// printf is just easier for formating ;)
    printf("\n");
    printf("Pruned by LB_Kim    : %6.7f%%\n", ((double) kim / i)*100);
    printf("Pruned by LB_Keogh  : %6.7f%%\n", ((double) keogh / i)*100);
    printf("Pruned by LB_Keogh2 : %6.7f%%\n", ((double) keogh2 / i)*100);
    printf("DTW Calculation     : %6.7f%%\n", 100-(((double)kim+keogh+keogh2)/i*100));
    */
    double kimp = ((double) kim / i)*100;
    double keop = ((double) keogh / i)*100;
    double keo2p = ((double) keogh2 / i)*100;
    double dtwp  = 100-(((double)kim+keogh+keogh2)/i*100);
    cout << kimp << "," << keop << "," << keo2p << "," << dtwp << "," << (t2-t1)/CLOCKS_PER_SEC << endl;    
    return 0;
}
