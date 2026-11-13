#pragma once
#include <cstddef>
namespace ghost_tr_cuda {
float* allocate(bool gpu, size_t n);
int* allocate_int(bool gpu, size_t n);
void release(bool gpu, float* p);
void release_int(bool gpu, int* p);
void upload(bool gpu,float* to,const float* from,size_t n);
void upload_int(bool gpu,int* to,const int* from,size_t n);
void download(bool gpu,float* to,const float* from,size_t n);
void zero(bool gpu,float* p,size_t n);
void gemm(bool gpu,const float* a,const float* b,float* out,int batches,int m,int n,int k,bool ta,bool tb,float scale,bool accumulate);
void gemm_qubit(bool gpu,const float* a,const float* b,float* out,int batches,int m,int n,int k,bool ta,bool tb,float scale,bool accumulate);
void gemm_qubit_backward(bool gpu,const float* a,const float* b,float* out,int batches,int m,int n,int k,bool ta,bool tb,float scale,bool accumulate,bool conj_a,bool conj_b);
void add(bool gpu,const float* a,const float* b,float* out,int n,bool backward=false);
void norm(bool gpu,const float* x,const float* dy,float* out,int rows,int cols,bool backward);
void gelu(bool gpu,const float* x,const float* dy,float* out,int n,bool backward);
void add_qubit(bool gpu,const float* a,const float* b,float* out,int n,bool backward=false);
void norm_qubit(bool gpu,const float* x,const float* dy,float* out,int rows,int cols,bool backward);
void gelu_qubit(bool gpu,const float* x,const float* dy,float* out,int n,bool backward);
void heads(bool gpu,const float* x,float* out,int batch,int time,int width,int heads,bool inverse,bool accumulate);
void heads_qubit(bool gpu,const float* x,float* out,int batch,int time,int width,int heads,bool inverse,bool accumulate);
void embedding(bool gpu,const float* ids,const float* token,const float* position,float* out,float* dt,float* dp,int rows,int time,int width,bool backward);
void embedding_qubit(bool gpu, const int* tokens, const float* token_emb, const float* pos_emb, float* out, int time, int width);
void logits_qubit(bool gpu, const float* out_state, const float* output_weights, float* logits, int time, int width, int vocab);
void cross_entropy_qubit(bool gpu, const float* logits_qubit, const int* targets, float* grad_logits, float* loss, int time, int vocab);
void softmax(bool gpu,const float* x,const float* dy,float* out,int batches,int time,bool backward);
void loss(bool gpu,const float* x,const float* targets,float* grad,float* losses,int rows,int vocab,int labeled);
void adam(bool gpu,float* w,float* m,float* v,const float* grad,int n,float rate,float correction1,float correction2);
void adam_qubit(bool gpu,float* w,float* m,float* v,const float* grad,int n,float rate,float correction1,float correction2);
void softmax_qubit(bool gpu, const float* x, float* out, int batches, int time, float scale);
void softmax_qubit_backward(bool gpu, const float* out, const float* dy, float* dx, int batches, int time, float scale);
}
