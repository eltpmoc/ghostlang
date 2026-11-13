#pragma once
#include <cstddef>
namespace ghost_tr {
float* allocate(bool gpu, size_t n);
void release(bool gpu, float* p);
void upload(bool gpu,float* to,const float* from,size_t n);
void download(bool gpu,float* to,const float* from,size_t n);
void zero(bool gpu,float* p,size_t n);
void gemm(bool gpu,const float* a,const float* b,float* out,int batches,int m,int n,int k,bool ta,bool tb,float scale,bool accumulate);
void add(bool gpu,const float* a,const float* b,float* out,int n,bool backward=false);
void norm(bool gpu,const float* x,const float* dy,float* out,int rows,int cols,bool backward);
void gelu(bool gpu,const float* x,const float* dy,float* out,int n,bool backward);
void heads(bool gpu,const float* x,float* out,int batch,int time,int width,int heads,bool inverse,bool accumulate);
void embedding(bool gpu,const float* ids,const float* token,const float* position,float* out,float* dt,float* dp,int rows,int time,int width,bool backward);
void softmax(bool gpu,const float* x,const float* dy,float* out,int batches,int time,bool backward);
void loss(bool gpu,const float* x,const float* targets,float* grad,float* losses,int rows,int vocab,int labeled);
void adam(bool gpu,float* w,float* m,float* v,const float* grad,int n,float rate,float correction1,float correction2);
}
