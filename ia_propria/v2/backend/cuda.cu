#include "layout.hpp"
#include <cuda_runtime.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {
std::string error;
bool check(cudaError_t status) {
    if (status==cudaSuccess) return true;
    error=cudaGetErrorString(status); return false;
}
struct Memory {
    Layout s;
    float *w=nullptr,*m=nullptr,*v=nullptr,*grad=nullptr,*x=nullptr,*h=nullptr,*p=nullptr;
    float *dz=nullptr,*dh=nullptr,*dx=nullptr,*loss=nullptr,*norm=nullptr;
    int *ids=nullptr,*targets=nullptr;
    void release() {
        for (float** ptr:{&w,&m,&v,&grad,&x,&h,&p,&dz,&dh,&dx,&loss,&norm}) { cudaFree(*ptr); *ptr=nullptr; }
        cudaFree(ids); ids=nullptr; cudaFree(targets); targets=nullptr;
    }
    ~Memory() { release(); }
} memory;
__global__ void probe(float* out) { out[0]=7.0f; }
__global__ void gather(Layout s,const float* w,const int* ids,float* x,int batch) {
    int i=blockIdx.x*blockDim.x+threadIdx.x;
    if (i<batch*s.width) x[i]=w[ids[i/s.embedding]*s.embedding+i%s.embedding];
}
__global__ void hidden(Layout s,const float* w,const float* x,float* h,int batch) {
    int i=blockIdx.x*blockDim.x+threadIdx.x;
    if (i>=batch*s.hidden) return;
    int b=i/s.hidden,r=i%s.hidden; float sum=w[s.b1+r];
    for (int k=0;k<s.width;++k) sum+=w[s.w1+r*s.width+k]*x[b*s.width+k];
    h[i]=tanhf(sum);
}
__global__ void logits(Layout s,const float* w,const float* h,float* p,int batch) {
    int i=blockIdx.x*blockDim.x+threadIdx.x;
    if (i>=batch*s.vocab) return;
    int b=i/s.vocab,r=i%s.vocab; float sum=w[s.b2+r];
    for (int k=0;k<s.hidden;++k) sum+=w[s.w2+r*s.hidden+k]*h[b*s.hidden+k];
    p[i]=sum;
}
__global__ void softmax(Layout s,float* p,float* dz,const int* targets,float* loss,int batch,bool labeled) {
    __shared__ float work[256];
    int b=blockIdx.x,t=threadIdx.x;
    float maximum=-INFINITY;
    for (int r=t;r<s.vocab;r+=256) maximum=fmaxf(maximum,p[b*s.vocab+r]);
    work[t]=maximum; __syncthreads();
    for (int stride=128;stride;stride/=2) { if (t<stride) work[t]=fmaxf(work[t],work[t+stride]); __syncthreads(); }
    maximum=work[0]; __syncthreads();
    float sum=0;
    for (int r=t;r<s.vocab;r+=256) { float value=expf(p[b*s.vocab+r]-maximum); p[b*s.vocab+r]=value; sum+=value; }
    work[t]=sum; __syncthreads();
    for (int stride=128;stride;stride/=2) { if (t<stride) work[t]+=work[t+stride]; __syncthreads(); }
    sum=work[0];
    for (int r=t;r<s.vocab;r+=256) {
        float value=p[b*s.vocab+r]/sum; p[b*s.vocab+r]=value;
        dz[b*s.vocab+r]=labeled?(value-(r==targets[b]?1.0f:0.0f))/batch:0;
    }
    __syncthreads();
    if (t==0) loss[b]=labeled?-logf(fmaxf(p[b*s.vocab+targets[b]],1e-30f)):0;
}
__global__ void hidden_delta(Layout s,const float* w,const float* h,const float* dz,float* dh,int batch) {
    int i=blockIdx.x*blockDim.x+threadIdx.x;
    if (i>=batch*s.hidden) return;
    int b=i/s.hidden,k=i%s.hidden; float sum=0;
    for (int r=0;r<s.vocab;++r) sum+=w[s.w2+r*s.hidden+k]*dz[b*s.vocab+r];
    dh[i]=sum*(1-h[i]*h[i]);
}
__global__ void input_delta(Layout s,const float* w,const float* dh,float* dx,int batch) {
    int i=blockIdx.x*blockDim.x+threadIdx.x;
    if (i>=batch*s.width) return;
    int b=i/s.width,k=i%s.width; float sum=0;
    for (int r=0;r<s.hidden;++r) sum+=w[s.w1+r*s.width+k]*dh[b*s.hidden+r];
    dx[i]=sum;
}
__global__ void dense_gradient(Layout s,const float* x,const float* h,const float* dz,const float* dh,
                               float* gradient,int batch) {
    int i=s.w1+blockIdx.x*blockDim.x+threadIdx.x;
    if (i>=s.count) return;
    float sum=0;
    if (i<s.b1) {
        int r=(i-s.w1)/s.width,k=(i-s.w1)%s.width;
        for (int b=0;b<batch;++b) sum+=dh[b*s.hidden+r]*x[b*s.width+k];
    } else if (i<s.w2) {
        for (int b=0;b<batch;++b) sum+=dh[b*s.hidden+i-s.b1];
    } else if (i<s.b2) {
        int r=(i-s.w2)/s.hidden,k=(i-s.w2)%s.hidden;
        for (int b=0;b<batch;++b) sum+=dz[b*s.vocab+r]*h[b*s.hidden+k];
    } else {
        for (int b=0;b<batch;++b) sum+=dz[b*s.vocab+i-s.b2];
    }
    gradient[i]=sum;
}
__global__ void embedding_gradient(Layout s,const int* ids,const float* dx,float* gradient,int batch) {
    int i=blockIdx.x*blockDim.x+threadIdx.x;
    if (i<batch*s.width) atomicAdd(&gradient[ids[i/s.embedding]*s.embedding+i%s.embedding],dx[i]);
}
__global__ void gradient_norm(const float* gradient,float* norm,int count) {
    __shared__ double parts[256];
    int t=threadIdx.x; double sum=0;
    for (int i=t;i<count;i+=256) sum+=double(gradient[i])*gradient[i];
    parts[t]=sum; __syncthreads();
    for (int stride=128;stride;stride/=2) { if (t<stride) parts[t]+=parts[t+stride]; __syncthreads(); }
    if (t==0) norm[0]=static_cast<float>(parts[0]);
}
__global__ void update(float* w,float* m,float* v,const float* gradient,const float* norm,int count,
                        float rate,float c1,float c2) {
    int i=blockIdx.x*blockDim.x+threadIdx.x;
    if (i>=count) return;
    float scale=1.0f/fmaxf(1.0f,sqrtf(norm[0])),g=gradient[i]*scale;
    m[i]=0.9f*m[i]+0.1f*g; v[i]=0.999f*v[i]+0.001f*g*g;
    w[i]-=rate*(m[i]/c1/(sqrtf(v[i]/c2)+1e-8f)+0.0001f*w[i]);
}
bool allocate(float** ptr,size_t count) { return check(cudaMalloc(ptr,count*sizeof(float))); }
int blocks(int count) { return (count+255)/256; }
}

extern "C" const char* v2_cuda_error() { return error.c_str(); }
extern "C" int v2_cuda_init(char* info,int capacity) {
    int count=0;
    if (!check(cudaGetDeviceCount(&count))) return 0;
    if (!count) { error="Nenhuma GPU CUDA visivel"; return 0; }
    if (!check(cudaSetDevice(0))) return 0;
    cudaDeviceProp properties{};
    if (!check(cudaGetDeviceProperties(&properties,0))) return 0;
    float* result=nullptr;
    if (!allocate(&result,1)) return 0;
    probe<<<1,1>>>(result); float actual=0;
    bool ok=check(cudaGetLastError())&&check(cudaMemcpy(&actual,result,sizeof(float),cudaMemcpyDeviceToHost));
    cudaFree(result);
    if (!ok) return 0;
    if (actual!=7) { error="Autoteste CUDA incorreto"; return 0; }
    std::snprintf(info,capacity,"%s",properties.name); return 1;
}
extern "C" int v2_cuda_setup(Layout s,const float* w,const float* m,const float* v) {
    memory.release(); memory.s=s;
    if (!allocate(&memory.w,s.count)||!allocate(&memory.m,s.count)||!allocate(&memory.v,s.count)||
        !allocate(&memory.grad,s.count)||!allocate(&memory.x,s.batch*s.width)||!allocate(&memory.h,s.batch*s.hidden)||
        !allocate(&memory.p,s.batch*s.vocab)||!allocate(&memory.dz,s.batch*s.vocab)||!allocate(&memory.dh,s.batch*s.hidden)||
        !allocate(&memory.dx,s.batch*s.width)||!allocate(&memory.loss,s.batch)||!allocate(&memory.norm,1)||
        !check(cudaMalloc(&memory.ids,s.batch*s.context*sizeof(int)))||!check(cudaMalloc(&memory.targets,s.batch*sizeof(int)))) return 0;
    return check(cudaMemcpy(memory.w,w,s.count*sizeof(float),cudaMemcpyHostToDevice))&&
           check(cudaMemcpy(memory.m,m,s.count*sizeof(float),cudaMemcpyHostToDevice))&&
           check(cudaMemcpy(memory.v,v,s.count*sizeof(float),cudaMemcpyHostToDevice));
}
extern "C" int v2_cuda_step(const int* input,const int* targets,int batch,int training,float rate,uint64_t step,
                            float* probabilities,float* loss,float* gradients) {
    Layout s=memory.s;
    if (batch<1||batch>s.batch) { error="Tamanho de lote invalido"; return 0; }
    if (!check(cudaMemcpy(memory.ids,input,batch*s.context*sizeof(int),cudaMemcpyHostToDevice))) return 0;
    if (targets && !check(cudaMemcpy(memory.targets,targets,batch*sizeof(int),cudaMemcpyHostToDevice))) return 0;
    gather<<<blocks(batch*s.width),256>>>(s,memory.w,memory.ids,memory.x,batch);
    hidden<<<blocks(batch*s.hidden),256>>>(s,memory.w,memory.x,memory.h,batch);
    logits<<<blocks(batch*s.vocab),256>>>(s,memory.w,memory.h,memory.p,batch);
    softmax<<<batch,256>>>(s,memory.p,memory.dz,memory.targets,memory.loss,batch,targets!=nullptr);
    if (training) {
        if (!check(cudaMemset(memory.grad,0,s.w1*sizeof(float)))) return 0;
        hidden_delta<<<blocks(batch*s.hidden),256>>>(s,memory.w,memory.h,memory.dz,memory.dh,batch);
        input_delta<<<blocks(batch*s.width),256>>>(s,memory.w,memory.dh,memory.dx,batch);
        dense_gradient<<<blocks(s.count-s.w1),256>>>(s,memory.x,memory.h,memory.dz,memory.dh,memory.grad,batch);
        embedding_gradient<<<blocks(batch*s.width),256>>>(s,memory.ids,memory.dx,memory.grad,batch);
        gradient_norm<<<1,256>>>(memory.grad,memory.norm,s.count);
        float norm=0;
        if (!check(cudaMemcpy(&norm,memory.norm,sizeof(float),cudaMemcpyDeviceToHost))) return 0;
        if (!std::isfinite(norm)) { error="Gradiente CUDA nao finito"; return 0; }
        if (gradients && !check(cudaMemcpy(gradients,memory.grad,s.count*sizeof(float),cudaMemcpyDeviceToHost))) return 0;
        update<<<blocks(s.count),256>>>(memory.w,memory.m,memory.v,memory.grad,memory.norm,s.count,rate,
                                       1-std::pow(0.9,step),1-std::pow(0.999,step));
    }
    if (!check(cudaGetLastError())) return 0;
    std::vector<float> losses(batch);
    if (!check(cudaMemcpy(losses.data(),memory.loss,batch*sizeof(float),cudaMemcpyDeviceToHost))||
        !check(cudaMemcpy(probabilities,memory.p,batch*s.vocab*sizeof(float),cudaMemcpyDeviceToHost))) return 0;
    double sum=0; for (float value:losses) sum+=value; *loss=sum/batch; return 1;
}
extern "C" int v2_cuda_download(float* w,float* m,float* v) {
    int count=memory.s.count;
    return check(cudaMemcpy(w,memory.w,count*sizeof(float),cudaMemcpyDeviceToHost))&&
           check(cudaMemcpy(m,memory.m,count*sizeof(float),cudaMemcpyDeviceToHost))&&
           check(cudaMemcpy(v,memory.v,count*sizeof(float),cudaMemcpyDeviceToHost));
}
