#include "GhostTransformerOps.hpp"
#ifdef GHOST_HAS_CUDA
#include "GhostTransformerCuda.hpp"
#define GPU(name, ...) if (gpu) { ghost_tr_cuda::name(true, __VA_ARGS__); return; }
#else
#define GPU(name, ...) if (gpu) throw std::runtime_error("Transformer CUDA nao compilado.");
#endif
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace ghost_tr {
float* allocate(bool gpu,size_t n) {
#ifdef GHOST_HAS_CUDA
    if (gpu) return ghost_tr_cuda::allocate(true,n);
#endif
    if (gpu) throw std::runtime_error("Transformer CUDA nao compilado.");
    return new float[n]();
}
void release(bool gpu,float* p) { GPU(release,p); delete[] p; }
void upload(bool gpu,float* to,const float* from,size_t n) { GPU(upload,to,from,n); std::memcpy(to,from,n*sizeof(float)); }
void download(bool gpu,float* to,const float* from,size_t n) { GPU(download,to,from,n); std::memcpy(to,from,n*sizeof(float)); }
void zero(bool gpu,float* p,size_t n) { GPU(zero,p,n); std::fill(p,p+n,0.0f); }
void gemm(bool gpu,const float* a,const float* b,float* out,int batches,int m,int n,int k,bool ta,bool tb,float scale,bool accumulate) {
    GPU(gemm,a,b,out,batches,m,n,k,ta,tb,scale,accumulate);
    #pragma omp parallel for if(batches*m*n>4096)
    for(int row=0;row<batches*m;++row) {
        int batch=row/m,i=row%m;
        const float* aa=a+batch*m*k; const float* bb=b+batch*k*n;
        for(int j=0;j<n;++j) {
            float sum=0;
            for(int p=0;p<k;++p) sum+=(ta?aa[p*m+i]:aa[i*k+p])*(tb?bb[j*k+p]:bb[p*n+j]);
            if(accumulate) out[row*n+j]+=sum*scale; else out[row*n+j]=sum*scale;
        }
    }
}
void add(bool gpu,const float* a,const float* b,float* out,int n,bool backward) {
    GPU(add,a,b,out,n,backward);
    for(int i=0;i<n;++i) { if(backward) out[i]+=a[i]; else out[i]=a[i]+b[i]; }
}
void norm(bool gpu,const float* x,const float* dy,float* out,int rows,int cols,bool backward) {
    GPU(norm,x,dy,out,rows,cols,backward);
    #pragma omp parallel for if(rows>128)
    for(int r=0;r<rows;++r) {
        float mean=0,var=0,g=0,gx=0;
        for(int j=0;j<cols;++j) mean+=x[r*cols+j]; mean/=cols;
        for(int j=0;j<cols;++j) { float z=x[r*cols+j]-mean; var+=z*z; }
        float inv=1/std::sqrt(var/cols+1e-5f);
        if(backward) for(int j=0;j<cols;++j) { g+=dy[r*cols+j]; gx+=dy[r*cols+j]*(x[r*cols+j]-mean)*inv; }
        for(int j=0;j<cols;++j) {
            int i=r*cols+j; float z=(x[i]-mean)*inv;
            if(backward) out[i]+=inv*(dy[i]-g/cols-z*gx/cols); else out[i]=z;
        }
    }
}
void gelu(bool gpu,const float* x,const float* dy,float* out,int n,bool backward) {
    GPU(gelu,x,dy,out,n,backward);
    for(int i=0;i<n;++i) {
        float v=x[i],t=std::tanh(0.7978845608f*(v+0.044715f*v*v*v));
        if(backward) out[i]+=dy[i]*(0.5f*(1+t)+0.5f*v*(1-t*t)*0.7978845608f*(1+0.134145f*v*v));
        else out[i]=0.5f*v*(1+t);
    }
}
void heads(bool gpu,const float* x,float* out,int batch,int time,int width,int nh,bool inverse,bool accumulate) {
    GPU(heads,x,out,batch,time,width,nh,inverse,accumulate);
    int dh=width/nh;
    for(int i=0;i<batch*time*width;++i) {
        int b=i/(time*width),t=i/width%time,h=i%width/dh,k=i%dh;
        int j=((b*nh+h)*time+t)*dh+k;
        int dst=inverse?i:j,src=inverse?j:i;
        if(accumulate) out[dst]+=x[src]; else out[dst]=x[src];
    }
}
void embedding(bool gpu,const float* ids,const float* token,const float* position,float* out,float* dt,float* dp,int rows,int time,int width,bool backward) {
    GPU(embedding,ids,token,position,out,dt,dp,rows,time,width,backward);
    for(int r=0;r<rows;++r) for(int j=0;j<width;++j) {
        int t=int(ids[r])*width+j,p=(r%time)*width+j,i=r*width+j;
        if(backward) { dt[t]+=out[i]; dp[p]+=out[i]; } else out[i]=token[t]+position[p];
    }
}
void softmax(bool gpu,const float* x,const float* dy,float* out,int batches,int time,bool backward) {
    GPU(softmax,x,dy,out,batches,time,backward);
    for(int r=0;r<batches*time;++r) {
        int last=r%time;
        if(backward) {
            float dot=0; for(int j=0;j<=last;++j) dot+=dy[r*time+j]*x[r*time+j];
            for(int j=0;j<=last;++j) out[r*time+j]+=x[r*time+j]*(dy[r*time+j]-dot);
        } else {
            float maximum=-INFINITY,sum=0;
            for(int j=0;j<=last;++j) maximum=std::max(maximum,x[r*time+j]);
            for(int j=0;j<=last;++j) { out[r*time+j]=std::exp(x[r*time+j]-maximum); sum+=out[r*time+j]; }
            for(int j=0;j<time;++j) out[r*time+j]=j<=last?out[r*time+j]/sum:0;
        }
    }
}
void loss(bool gpu,const float* x,const float* targets,float* grad,float* losses,int rows,int vocab,int labeled) {
    GPU(loss,x,targets,grad,losses,rows,vocab,labeled);
    for(int r=0;r<rows;++r) {
        int target=int(targets[r]); losses[r]=0;
        if(target<0) { std::fill(grad+r*vocab,grad+(r+1)*vocab,0.0f); continue; }
        float maximum=-INFINITY,sum=0;
        for(int j=0;j<vocab;++j) maximum=std::max(maximum,x[r*vocab+j]);
        for(int j=0;j<vocab;++j) sum+=std::exp(x[r*vocab+j]-maximum);
        losses[r]=(std::log(sum)+maximum-x[r*vocab+target])/labeled;
        for(int j=0;j<vocab;++j) grad[r*vocab+j]=(std::exp(x[r*vocab+j]-maximum)/sum-(j==target?1:0))/labeled;
    }
}
void adam(bool gpu,float* w,float* m,float* v,const float* grad,int n,float rate,float c1,float c2) {
    GPU(adam,w,m,v,grad,n,rate,c1,c2);
    for(int i=0;i<n;++i) {
        float g=std::clamp(grad[i],-1.0f,1.0f);
        m[i]=0.9f*m[i]+0.1f*g; v[i]=0.999f*v[i]+0.001f*g*g;
        w[i]-=rate*(m[i]/c1/(std::sqrt(v[i]/c2)+1e-8f)+0.01f*w[i]);
    }
}
}
