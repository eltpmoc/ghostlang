#include "GhostDevice.hpp"
#include <cuda_runtime.h>
#include <cstdio>
using ghost::PackedG;
namespace {
__global__ void probe(double* value) { value[0]=7.0; }
__global__ void product(const PackedG* a,const PackedG* b,PackedG* out,int r,int k,int c) {
    __shared__ PackedG aa[16][16],bb[16][16];
    int tx=threadIdx.x,ty=threadIdx.y,row=blockIdx.y*16+ty,col=blockIdx.x*16+tx;
    PackedG sum{0,0,0};
    for (int tile=0;tile<(k+15)/16;++tile) {
        int ak=tile*16+tx,bk=tile*16+ty;
        aa[ty][tx]=(row<r && ak<k)?a[static_cast<size_t>(row)*k+ak]:PackedG{0,0,0};
        bb[ty][tx]=(bk<k && col<c)?b[static_cast<size_t>(bk)*c+col]:PackedG{0,0,0};
        __syncthreads();
        for (int t=0;t<16 && tile*16+t<k;++t) {
            PackedG av=aa[ty][t],bv=bb[t][tx];
            sum.x+=av.x*bv.x-av.y*bv.y;
            sum.y+=av.x*bv.y+av.y*bv.x;
            sum.p+=av.x*bv.p+bv.x*av.p;
        }
        __syncthreads();
    }
    if (row<r && col<c) out[static_cast<size_t>(row)*c+col]=sum;
}
struct Workspace {
    PackedG *a=nullptr,*b=nullptr,*c=nullptr;
    size_t na=0,nb=0,nc=0;
    ~Workspace() { cudaFree(a);cudaFree(b);cudaFree(c); }
    cudaError_t reserve(PackedG*& ptr,size_t& capacity,size_t count) {
        if (count<=capacity) return cudaSuccess;
        cudaFree(ptr);ptr=nullptr;capacity=0;
        auto status=cudaMalloc(&ptr,count*sizeof(PackedG));
        if (status==cudaSuccess) capacity=count;
        return status;
    }
};
thread_local Workspace workspace;
}
extern "C" int ghost_cuda_probe(char* info,int capacity) {
    int count=0; auto status=cudaGetDeviceCount(&count);
    if (status!=cudaSuccess || count==0) {
        std::snprintf(info,capacity,"%s",status==cudaSuccess?"nenhuma GPU visivel":cudaGetErrorString(status)); return 0;
    }
    status=cudaSetDevice(0);
    double *ptr=nullptr,value=0;
    if (status==cudaSuccess) status=cudaMalloc(&ptr,sizeof(double));
    if (status==cudaSuccess) { probe<<<1,1>>>(ptr); status=cudaGetLastError(); }
    if (status==cudaSuccess) status=cudaMemcpy(&value,ptr,sizeof(double),cudaMemcpyDeviceToHost);
    cudaFree(ptr);
    if (status!=cudaSuccess || value!=7) {
        std::snprintf(info,capacity,"%s",status==cudaSuccess?"autoteste incorreto":cudaGetErrorString(status));return 0;
    }
    cudaDeviceProp properties{}; status=cudaGetDeviceProperties(&properties,0);
    if (status!=cudaSuccess) { std::snprintf(info,capacity,"%s",cudaGetErrorString(status));return 0; }
    std::snprintf(info,capacity,"%s; autoteste OK",properties.name); return 1;
}
extern "C" int ghost_cuda_matmul(const PackedG* a,const PackedG* b,PackedG* out,int r,int k,int c,char* error,int capacity) {
    auto fail=[&](cudaError_t status) { if (status==cudaSuccess) return false;std::snprintf(error,capacity,"%s",cudaGetErrorString(status));return true; };
    size_t na=static_cast<size_t>(r)*k,nb=static_cast<size_t>(k)*c,nc=static_cast<size_t>(r)*c;
    if (fail(cudaSetDevice(0)) || fail(workspace.reserve(workspace.a,workspace.na,na)) ||
        fail(workspace.reserve(workspace.b,workspace.nb,nb)) || fail(workspace.reserve(workspace.c,workspace.nc,nc))) return 0;
    if (fail(cudaMemcpy(workspace.a,a,na*sizeof(PackedG),cudaMemcpyHostToDevice)) ||
        fail(cudaMemcpy(workspace.b,b,nb*sizeof(PackedG),cudaMemcpyHostToDevice))) return 0;
    product<<<dim3((c+15)/16,(r+15)/16),dim3(16,16)>>>(workspace.a,workspace.b,workspace.c,r,k,c);
    return !fail(cudaGetLastError()) && !fail(cudaMemcpy(out,workspace.c,nc*sizeof(PackedG),cudaMemcpyDeviceToHost));
}
