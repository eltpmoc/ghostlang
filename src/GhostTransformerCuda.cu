#include "GhostTransformerCuda.hpp"
#include <cuda_runtime.h>
#include <stdexcept>
#include <string>
namespace {
void check(cudaError_t e) { if(e!=cudaSuccess) throw std::runtime_error(std::string("Transformer CUDA: ")+cudaGetErrorString(e)); }
__global__ void matrix(const float* a,const float* b,float* out,int m,int n,int k,bool ta,bool tb,float scale,bool accum) {
    __shared__ float aa[16][16],bb[16][16];
    int i=blockIdx.y*16+threadIdx.y,j=blockIdx.x*16+threadIdx.x,z=blockIdx.z;
    a+=z*m*k; b+=z*k*n; out+=z*m*n;
    float sum=0;
    for(int base=0;base<k;base+=16) {
        int p=base+threadIdx.x,q=base+threadIdx.y;
        aa[threadIdx.y][threadIdx.x]=(i<m&&p<k)?(ta?a[p*m+i]:a[i*k+p]):0;
        bb[threadIdx.y][threadIdx.x]=(j<n&&q<k)?(tb?b[j*k+q]:b[q*n+j]):0;
        __syncthreads();
        for(int h=0;h<16;++h) sum+=aa[threadIdx.y][h]*bb[h][threadIdx.x];
        __syncthreads();
    }
        if(i<m&&j<n) { if(accum) out[i*n+j]+=sum*scale; else out[i*n+j]=sum*scale; }
}
struct Qubit { float r, i, t; };
__global__ void matrix_qubit(const Qubit* a,const Qubit* b,Qubit* out,int m,int n,int k,bool ta,bool tb,float scale,bool accum) {
    __shared__ Qubit aa[16][16],bb[16][16];
    int i=blockIdx.y*16+threadIdx.y,j=blockIdx.x*16+threadIdx.x,z=blockIdx.z;
    a+=z*m*k; b+=z*k*n; out+=z*m*n;
    float sum_r=0, sum_i=0, max_t=0;
    for(int base=0;base<k;base+=16) {
        int p=base+threadIdx.x,q=base+threadIdx.y;
        Qubit zeroQ = {0,0,0};
        aa[threadIdx.y][threadIdx.x]=(i<m&&p<k)?(ta?a[p*m+i]:a[i*k+p]):zeroQ;
        bb[threadIdx.y][threadIdx.x]=(j<n&&q<k)?(tb?b[j*k+q]:b[q*n+j]):zeroQ;
        __syncthreads();
        for(int h=0;h<16;++h) {
            Qubit qa = aa[threadIdx.y][h];
            Qubit qb = bb[h][threadIdx.x];
            sum_r += qa.r * qb.r - qa.i * qb.i;
            sum_i += qa.r * qb.i + qa.i * qb.r;
            float inter_t = qa.t * qb.t;
            if (inter_t > max_t) max_t = inter_t;
        }
        __syncthreads();
    }
    if(i<m&&j<n) { 
        if(accum) {
            out[i*n+j].r += sum_r*scale;
            out[i*n+j].i += sum_i*scale;
            out[i*n+j].t = max_t == 0.0f ? 1.0f : fmaxf(out[i*n+j].t, max_t);
        } else {
            out[i*n+j].r = sum_r*scale;
            out[i*n+j].i = sum_i*scale;
            out[i*n+j].t = max_t == 0.0f ? 1.0f : max_t;
        }
    }
}

__global__ void matrix_qubit_backward(const Qubit* a,const Qubit* b,Qubit* out,int m,int n,int k,bool ta,bool tb,float scale,bool accum, bool conj_a, bool conj_b) {
    __shared__ Qubit aa[16][16],bb[16][16];
    int i=blockIdx.y*16+threadIdx.y,j=blockIdx.x*16+threadIdx.x,z=blockIdx.z;
    a+=z*m*k; b+=z*k*n; out+=z*m*n;
    float sum_r=0, sum_i=0, max_t=0;
    for(int base=0;base<k;base+=16) {
        int p=base+threadIdx.x,q=base+threadIdx.y;
        Qubit zeroQ = {0,0,0};
        aa[threadIdx.y][threadIdx.x]=(i<m&&p<k)?(ta?a[p*m+i]:a[i*k+p]):zeroQ;
        bb[threadIdx.y][threadIdx.x]=(j<n&&q<k)?(tb?b[j*k+q]:b[q*n+j]):zeroQ;
        __syncthreads();
        for(int h=0;h<16;++h) {
            Qubit qa = aa[threadIdx.y][h];
            if (conj_a) qa.i = -qa.i; // Conjugado
            Qubit qb = bb[h][threadIdx.x];
            if (conj_b) qb.i = -qb.i; // Conjugado
            sum_r += qa.r * qb.r - qa.i * qb.i;
            sum_i += qa.r * qb.i + qa.i * qb.r;
            float inter_t = qa.t * qb.t;
            if (inter_t > max_t) max_t = inter_t;
        }
        __syncthreads();
    }
    if(i<m&&j<n) { 
        if(accum) {
            out[i*n+j].r += sum_r*scale;
            out[i*n+j].i += sum_i*scale;
            out[i*n+j].t = max_t == 0.0f ? 1.0f : fmaxf(out[i*n+j].t, max_t);
        } else {
            out[i*n+j].r = sum_r*scale;
            out[i*n+j].i = sum_i*scale;
            out[i*n+j].t = max_t == 0.0f ? 1.0f : max_t;
        }
    }
}
__global__ void plus(const float* a,const float* b,float* out,int n,bool backward) {
    int i=blockIdx.x*blockDim.x+threadIdx.x;
    if(i<n) { if(backward) out[i]+=a[i]; else out[i]=a[i]+b[i]; }
}
__global__ void normalize(const float* x,const float* dy,float* out,int rows,int cols,bool backward) {
    int r=blockIdx.x*blockDim.x+threadIdx.x; if(r>=rows) return;
    float mean=0,var=0,g=0,gx=0;
    for(int j=0;j<cols;++j) mean+=x[r*cols+j]; mean/=cols;
    for(int j=0;j<cols;++j) { float z=x[r*cols+j]-mean; var+=z*z; }
    float inv=rsqrtf(var/cols+1e-5f);
    if(backward) for(int j=0;j<cols;++j) { g+=dy[r*cols+j]; gx+=dy[r*cols+j]*(x[r*cols+j]-mean)*inv; }
    for(int j=0;j<cols;++j) {
        int i=r*cols+j; float z=(x[i]-mean)*inv;
        if(backward) out[i]+=inv*(dy[i]-g/cols-z*gx/cols); else out[i]=z;
    }
}
__global__ void activation(const float* x,const float* dy,float* out,int n,bool backward) {
    int i=blockIdx.x*blockDim.x+threadIdx.x; if(i>=n) return;
    float v=x[i],t=tanhf(0.7978845608f*(v+0.044715f*v*v*v));
    if(backward) out[i]+=dy[i]*(0.5f*(1+t)+0.5f*v*(1-t*t)*0.7978845608f*(1+0.134145f*v*v));
    else out[i]=0.5f*v*(1+t);
}
__global__ void rearrange(const float* x,float* out,int batch,int time,int width,int nh,bool inverse,bool accum) {
    int i=blockIdx.x*blockDim.x+threadIdx.x; if(i>=batch*time*width) return;
    int dh=width/nh,b=i/(time*width),t=i/width%time,h=i%width/dh,k=i%dh;
    int j=((b*nh+h)*time+t)*dh+k,dst=inverse?i:j,src=inverse?j:i;
    if(accum) out[dst]+=x[src]; else out[dst]=x[src];
}

__global__ void add_qubit_kernel(const Qubit* a, const Qubit* b, Qubit* out, int n, bool backward) {
    int i = blockIdx.x * blockDim.x + threadIdx.x; 
    if(i >= n) return;
    if(backward) {
        out[i].r += a[i].r;
        out[i].i += a[i].i;
    } else {
        out[i].r = a[i].r + b[i].r;
        out[i].i = a[i].i + b[i].i;
        out[i].t = fmaxf(a[i].t, b[i].t);
    }
}

__global__ void normalize_qubit_kernel(const Qubit* x, const Qubit* dy, Qubit* out, int rows, int cols, bool backward) {
    int r = blockIdx.x * blockDim.x + threadIdx.x; 
    if(r >= rows) return;
    float mean_r = 0, var_r = 0, g_r = 0, gx_r = 0;
    float mean_i = 0, var_i = 0, g_i = 0, gx_i = 0;
    for(int j=0; j<cols; ++j) { mean_r += x[r*cols + j].r; mean_i += x[r*cols + j].i; }
    mean_r /= cols; mean_i /= cols;
    for(int j=0; j<cols; ++j) { 
        float z_r = x[r*cols + j].r - mean_r; var_r += z_r * z_r; 
        float z_i = x[r*cols + j].i - mean_i; var_i += z_i * z_i;
    }
    float inv_r = rsqrtf(var_r / cols + 1e-5f);
    float inv_i = rsqrtf(var_i / cols + 1e-5f);
    
    if(backward) {
        for(int j=0; j<cols; ++j) { 
            g_r += dy[r*cols + j].r; gx_r += dy[r*cols + j].r * (x[r*cols + j].r - mean_r) * inv_r; 
            g_i += dy[r*cols + j].i; gx_i += dy[r*cols + j].i * (x[r*cols + j].i - mean_i) * inv_i;
        }
    }
    
    for(int j=0; j<cols; ++j) {
        int i = r*cols + j; 
        float z_r = (x[i].r - mean_r) * inv_r;
        float z_i = (x[i].i - mean_i) * inv_i;
        if(backward) {
            out[i].r += inv_r * (dy[i].r - g_r/cols - z_r * gx_r/cols);
            out[i].i += inv_i * (dy[i].i - g_i/cols - z_i * gx_i/cols);
            out[i].t = 0;
        } else {
            out[i].r = z_r;
            out[i].i = z_i;
            out[i].t = x[i].t;
        }
    }
}

__global__ void gelu_qubit_kernel(const Qubit* x, const Qubit* dy, Qubit* out, int n, bool backward) {
    int i = blockIdx.x * blockDim.x + threadIdx.x; 
    if(i >= n) return;
    float vr = x[i].r, vi = x[i].i;
    float tr = tanhf(0.7978845608f * (vr + 0.044715f * vr * vr * vr));
    float ti = tanhf(0.7978845608f * (vi + 0.044715f * vi * vi * vi));
    if(backward) {
        out[i].r += dy[i].r * (0.5f * (1 + tr) + 0.5f * vr * (1 - tr * tr) * 0.7978845608f * (1 + 0.134145f * vr * vr));
        out[i].i += dy[i].i * (0.5f * (1 + ti) + 0.5f * vi * (1 - ti * ti) * 0.7978845608f * (1 + 0.134145f * vi * vi));
        out[i].t = 0;
    } else {
        out[i].r = 0.5f * vr * (1 + tr);
        out[i].i = 0.5f * vi * (1 + ti);
        out[i].t = x[i].t;
    }
}

__global__ void rearrange_qubit(const Qubit* x, Qubit* out, int batch, int time, int width, int nh, bool inverse, bool accum) {
    int i = blockIdx.x * blockDim.x + threadIdx.x; 
    if (i >= batch * time * width) return;
    int dh = width / nh;
    int b = i / (time * width);
    int t = (i / width) % time;
    int h = (i % width) / dh;
    int k = i % dh;
    int j = ((b * nh + h) * time + t) * dh + k;
    int dst = inverse ? i : j;
    int src = inverse ? j : i;
    if (accum) {
        out[dst].r += x[src].r;
        out[dst].i += x[src].i;
        out[dst].t = fmaxf(out[dst].t, x[src].t);
    } else {
        out[dst] = x[src];
    }
}
__global__ void embed(const float* ids,const float* token,const float* position,float* out,float* dt,float* dp,int rows,int time,int width,bool backward) {
    int i=blockIdx.x*blockDim.x+threadIdx.x; if(i>=rows*width) return;
    int r=i/width,j=i%width,t=int(ids[r])*width+j,p=(r%time)*width+j;
    if(backward) { atomicAdd(dt+t,out[i]); atomicAdd(dp+p,out[i]); } else out[i]=token[t]+position[p];
}
__global__ void attention(const float* x,const float* dy,float* out,int batches,int time,bool backward) {
    int r=blockIdx.x*blockDim.x+threadIdx.x; if(r>=batches*time) return;
    int last=r%time;
    if(backward) {
        float dot=0; for(int j=0;j<=last;++j) dot+=dy[r*time+j]*x[r*time+j];
        for(int j=0;j<=last;++j) out[r*time+j]+=x[r*time+j]*(dy[r*time+j]-dot);
    } else {
        float maximum=-INFINITY,sum=0;
        for(int j=0;j<=last;++j) maximum=fmaxf(maximum,x[r*time+j]);
        for(int j=0;j<=last;++j) { out[r*time+j]=expf(x[r*time+j]-maximum); sum+=out[r*time+j]; }
        for(int j=0;j<time;++j) out[r*time+j]=j<=last?out[r*time+j]/sum:0;
    }
}

__global__ void softmax_qubit_kernel(const Qubit* x, Qubit* out, int batches, int time, float scale) {
    int r = blockIdx.x * blockDim.x + threadIdx.x; 
    if(r >= batches) return;
    int last = r % time; // Causal mask
    
    float maximum = -INFINITY;
    for(int j=0; j<=last; ++j) maximum = fmaxf(maximum, x[r*time + j].r * scale);
    
    float sum = 0;
    for(int j=0; j<=last; ++j) sum += expf(x[r*time + j].r * scale - maximum);
    
    for(int j=0; j<time; ++j) {
        if (j <= last) {
            float w = expf(x[r*time + j].r * scale - maximum) / sum;
            out[r*time + j].r = w;
            out[r*time + j].i = 0.0f;
            out[r*time + j].t = x[r*time + j].t;
        } else {
            out[r*time + j].r = 0.0f;
            out[r*time + j].i = 0.0f;
            out[r*time + j].t = 0.0f;
        }
    }
}

__global__ void softmax_qubit_backward_kernel(const Qubit* out, const Qubit* dy, Qubit* dx, int batches, int time, float scale) {
    int r = blockIdx.x * blockDim.x + threadIdx.x; 
    if(r >= batches) return;
    int last = r % time;
    
    float dot = 0;
    for(int j=0; j<=last; ++j) dot += dy[r*time + j].r * out[r*time + j].r;
    
    for(int j=0; j<=last; ++j) {
        dx[r*time + j].r += out[r*time + j].r * (dy[r*time + j].r - dot) * scale;
        // The imaginary part derivative is 0 since we only applied softmax on real part
    }
}
__global__ void objective(const float* x,const float* targets,float* grad,float* losses,int rows,int vocab,int labeled) {
    int r=blockIdx.x*blockDim.x+threadIdx.x; if(r>=rows) return;
    int target=int(targets[r]); losses[r]=0;
    if(target<0) { for(int j=0;j<vocab;++j) grad[r*vocab+j]=0; return; }
    float maximum=-INFINITY,sum=0;
    for(int j=0;j<vocab;++j) maximum=fmaxf(maximum,x[r*vocab+j]);
    for(int j=0;j<vocab;++j) sum+=expf(x[r*vocab+j]-maximum);
    losses[r]=(logf(sum)+maximum-x[r*vocab+target])/labeled;
    for(int j=0;j<vocab;++j) grad[r*vocab+j]=(expf(x[r*vocab+j]-maximum)/sum-(j==target?1:0))/labeled;
}
__global__ void optimizer(float* w,float* m,float* v,const float* grad,int n,float rate,float c1,float c2) {
    int i=blockIdx.x*blockDim.x+threadIdx.x; if(i>=n) return;
    float g=fminf(1.0f,fmaxf(-1.0f,grad[i]));
    m[i]=0.9f*m[i]+0.1f*g; v[i]=0.999f*v[i]+0.001f*g*g;
    w[i]-=rate*(m[i]/c1/(sqrtf(v[i]/c2)+1e-8f)+0.01f*w[i]);
}

__global__ void optimizer_qubit(Qubit* w, Qubit* m_q, Qubit* v_q, const Qubit* grad, int n, float rate, float c1, float c2) {
    int i=blockIdx.x*blockDim.x+threadIdx.x; if(i>=n) return;
    float gr=fminf(1.0f,fmaxf(-1.0f,grad[i].r));
    m_q[i].r = 0.9f*m_q[i].r + 0.1f*gr;
    v_q[i].r = 0.999f*v_q[i].r + 0.001f*gr*gr;
    w[i].r -= rate*(m_q[i].r/c1/(sqrtf(v_q[i].r/c2)+1e-8f) + 0.01f*w[i].r);
    
    float gi=fminf(1.0f,fmaxf(-1.0f,grad[i].i));
    m_q[i].i = 0.9f*m_q[i].i + 0.1f*gi;
    v_q[i].i = 0.999f*v_q[i].i + 0.001f*gi*gi;
    w[i].i -= rate*(m_q[i].i/c1/(sqrtf(v_q[i].i/c2)+1e-8f) + 0.01f*w[i].i);
    
    float grad_mag = sqrtf(gr*gr + gi*gi);
    w[i].t = fmaxf(0.01f, w[i].t - rate * grad_mag); 
}
__global__ void embed_qubit_kernel(const int* tokens, const Qubit* token_emb, const Qubit* pos_emb, Qubit* out, int time, int width) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= time * width) return;
    int t = i / width;
    int d = i % width;
    int token = tokens[t];
    Qubit te = token_emb[token * width + d];
    Qubit pe = pos_emb[t * width + d];
    out[i].r = te.r + pe.r;
    out[i].i = te.i + pe.i;
    out[i].t = fmaxf(te.t, pe.t);
}

__global__ void logits_qubit_kernel(const Qubit* out_state, const Qubit* output_weights, float* logits, int time, int width, int vocab) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= time * vocab) return;
    int t = i / vocab;
    int v = i % vocab;
    float sum_r = 0, sum_i = 0;
    for(int d=0; d<width; ++d) {
        Qubit os = out_state[t * width + d];
        Qubit ow = output_weights[v * width + d];
        sum_r += os.r * ow.r - os.i * ow.i;
        sum_i += os.r * ow.i + os.i * ow.r;
    }
    logits[i] = sqrtf(sum_r * sum_r + sum_i * sum_i);
}

__global__ void cross_entropy_qubit_kernel(const Qubit* logits_qubit, const int* targets, Qubit* grad_logits, float* loss, int time, int vocab) {
    int t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t >= time) return;
    
    float max_mag = -1e9;
    for(int v=0; v<vocab; ++v) {
        Qubit q = logits_qubit[t * vocab + v];
        float mag = sqrtf(q.r * q.r + q.i * q.i);
        if (mag > max_mag) max_mag = mag;
    }
    
    float sum_exp = 0;
    for(int v=0; v<vocab; ++v) {
        Qubit q = logits_qubit[t * vocab + v];
        float mag = sqrtf(q.r * q.r + q.i * q.i);
        sum_exp += expf(mag - max_mag);
    }
    
    int target = targets[t];
    float batch_loss = 0;
    
    for(int v=0; v<vocab; ++v) {
        Qubit q = logits_qubit[t * vocab + v];
        float mag = sqrtf(q.r * q.r + q.i * q.i);
        float prob = expf(mag - max_mag) / sum_exp;
        
        float grad_mag = prob;
        if (v == target) {
            grad_mag -= 1.0f;
            batch_loss = -logf(prob + 1e-9f);
        }
        
        float dmag_dr = (mag > 1e-9f) ? (q.r / mag) : 0.0f;
        float dmag_di = (mag > 1e-9f) ? (q.i / mag) : 0.0f;
        
        grad_logits[t * vocab + v].r = grad_mag * dmag_dr;
        grad_logits[t * vocab + v].i = grad_mag * dmag_di;
        grad_logits[t * vocab + v].t = 0.0f;
    }
    
    atomicAdd(loss, batch_loss / time);
}


int blocks(int n) { return (n+127)/128; }
}
namespace ghost_tr_cuda {
float* allocate(bool,size_t n) { float* p=nullptr; check(cudaMalloc(&p,n*sizeof(float))); check(cudaMemset(p,0,n*sizeof(float))); return p; }
int* allocate_int(bool,size_t n) { int* p=nullptr; check(cudaMalloc(&p,n*sizeof(int))); check(cudaMemset(p,0,n*sizeof(int))); return p; }
void release(bool,float* p) { cudaFree(p); }
void release_int(bool,int* p) { cudaFree(p); }
void upload(bool,float* to,const float* from,size_t n) { check(cudaMemcpy(to,from,n*sizeof(float),cudaMemcpyHostToDevice)); }
void upload_int(bool,int* to,const int* from,size_t n) { check(cudaMemcpy(to,from,n*sizeof(int),cudaMemcpyHostToDevice)); }
void download(bool,float* to,const float* from,size_t n) { check(cudaMemcpy(to,from,n*sizeof(float),cudaMemcpyDeviceToHost)); }
void zero(bool,float* p,size_t n) { check(cudaMemset(p,0,n*sizeof(float))); }
void gemm(bool,const float* a,const float* b,float* out,int batches,int m,int n,int k,bool ta,bool tb,float scale,bool accum) {
    matrix<<<dim3((n+15)/16,(m+15)/16,batches),dim3(16,16)>>>(a,b,out,m,n,k,ta,tb,scale,accum); check(cudaGetLastError());
}
void gemm_qubit(bool,const float* a,const float* b,float* out,int batches,int m,int n,int k,bool ta,bool tb,float scale,bool accum) {
    matrix_qubit<<<dim3((n+15)/16,(m+15)/16,batches),dim3(16,16)>>>((const Qubit*)a,(const Qubit*)b,(Qubit*)out,m,n,k,ta,tb,scale,accum); check(cudaGetLastError());
}
void gemm_qubit_backward(bool,const float* a,const float* b,float* out,int batches,int m,int n,int k,bool ta,bool tb,float scale,bool accum, bool conj_a, bool conj_b) {
    matrix_qubit_backward<<<dim3((n+15)/16,(m+15)/16,batches),dim3(16,16)>>>((const Qubit*)a,(const Qubit*)b,(Qubit*)out,m,n,k,ta,tb,scale,accum,conj_a,conj_b); check(cudaGetLastError());
}
void add(bool,const float* a,const float* b,float* out,int n,bool backward) { plus<<<blocks(n),128>>>(a,b,out,n,backward); check(cudaGetLastError()); }
void norm(bool,const float* x,const float* dy,float* out,int rows,int cols,bool backward) { normalize<<<blocks(rows),128>>>(x,dy,out,rows,cols,backward); check(cudaGetLastError()); }
void gelu(bool,const float* x,const float* dy,float* out,int n,bool backward) { activation<<<blocks(n),128>>>(x,dy,out,n,backward); check(cudaGetLastError()); }

void add_qubit(bool,const float* a,const float* b,float* out,int n,bool backward) { add_qubit_kernel<<<blocks(n),128>>>((const Qubit*)a,(const Qubit*)b,(Qubit*)out,n,backward); check(cudaGetLastError()); }
void norm_qubit(bool,const float* x,const float* dy,float* out,int rows,int cols,bool backward) { normalize_qubit_kernel<<<blocks(rows),128>>>((const Qubit*)x,(const Qubit*)dy,(Qubit*)out,rows,cols,backward); check(cudaGetLastError()); }
void gelu_qubit(bool,const float* x,const float* dy,float* out,int n,bool backward) { gelu_qubit_kernel<<<blocks(n),128>>>((const Qubit*)x,(const Qubit*)dy,(Qubit*)out,n,backward); check(cudaGetLastError()); }
void heads(bool,const float* x,float* out,int batch,int time,int width,int nh,bool inv,bool acc) { rearrange<<<blocks(batch*time*width),128>>>(x,out,batch,time,width,nh,inv,acc); check(cudaGetLastError()); }
void heads_qubit(bool,const float* x,float* out,int batch,int time,int width,int nh,bool inv,bool acc) {
    rearrange_qubit<<<blocks(batch*time*width),128>>>((const Qubit*)x,(Qubit*)out,batch,time,width,nh,inv,acc); check(cudaGetLastError()); 
}
void embedding(bool,const float* ids,const float* token,const float* position,float* out,float* dt,float* dp,int rows,int time,int width,bool back) { embed<<<blocks(rows*width),128>>>(ids,token,position,out,dt,dp,rows,time,width,back); check(cudaGetLastError()); }
void embedding_qubit(bool, const int* tokens, const float* token_emb, const float* pos_emb, float* out, int time, int width) {
    embed_qubit_kernel<<<blocks(time*width),128>>>((const int*)tokens, (const Qubit*)token_emb, (const Qubit*)pos_emb, (Qubit*)out, time, width);
    check(cudaGetLastError());
}
void logits_qubit(bool, const float* out_state, const float* output_weights, float* logits, int time, int width, int vocab) {
    logits_qubit_kernel<<<blocks(time*vocab),128>>>((const Qubit*)out_state, (const Qubit*)output_weights, logits, time, width, vocab);
    check(cudaGetLastError());
}
void cross_entropy_qubit(bool, const float* logits_qubit, const int* targets, float* grad_logits, float* loss, int time, int vocab) {
    cross_entropy_qubit_kernel<<<blocks(time),128>>>((const Qubit*)logits_qubit, (const int*)targets, (Qubit*)grad_logits, loss, time, vocab);
    check(cudaGetLastError());
}
void softmax(bool,const float* x,const float* dy,float* out,int batches,int time,bool backward) { attention<<<blocks(batches*time),128>>>(x,dy,out,batches,time,backward); check(cudaGetLastError()); }
void loss(bool,const float* x,const float* targets,float* grad,float* losses,int rows,int vocab,int labeled) { objective<<<blocks(rows),128>>>(x,targets,grad,losses,rows,vocab,labeled); check(cudaGetLastError()); }
void adam(bool,float* w,float* m,float* v,const float* grad,int n,float rate,float c1,float c2) { optimizer<<<blocks(n),128>>>(w,m,v,grad,n,rate,c1,c2); check(cudaGetLastError()); }
void adam_qubit(bool,float* w,float* m,float* v,const float* grad,int n,float rate,float c1,float c2) { optimizer_qubit<<<blocks(n),128>>>((Qubit*)w,(Qubit*)m,(Qubit*)v,(const Qubit*)grad,n,rate,c1,c2); check(cudaGetLastError()); }
void softmax_qubit(bool, const float* x, float* out, int batches, int time, float scale) { softmax_qubit_kernel<<<blocks(batches*time),128>>>((const Qubit*)x, (Qubit*)out, batches, time, scale); check(cudaGetLastError()); }
void softmax_qubit_backward(bool, const float* out, const float* dy, float* dx, int batches, int time, float scale) { softmax_qubit_backward_kernel<<<blocks(batches*time),128>>>((const Qubit*)out, (const Qubit*)dy, (Qubit*)dx, batches, time, scale); check(cudaGetLastError()); }
}
