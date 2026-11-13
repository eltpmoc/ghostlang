#include "GhostTransformerCuda.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <iostream>

#ifndef GHOST_HAS_CUDA

namespace ghost_tr_cuda {

float* allocate(bool, size_t n) {
    return new float[n]();
}

int* allocate_int(bool, size_t n) {
    return new int[n]();
}

void release(bool, float* p) {
    delete[] p;
}

void release_int(bool, int* p) {
    delete[] p;
}

void upload(bool, float* to, const float* from, size_t n) {
    std::memcpy(to, from, n * sizeof(float));
}

void upload_int(bool, int* to, const int* from, size_t n) {
    std::memcpy(to, from, n * sizeof(int));
}

void download(bool, float* to, const float* from, size_t n) {
    std::memcpy(to, from, n * sizeof(float));
}

void zero(bool, float* p, size_t n) {
    std::fill(p, p + n, 0.0f);
}

void gemm(bool, const float* a, const float* b, float* out, int batches, int m, int n, int k, bool ta, bool tb, float scale, bool accumulate) {
    #pragma omp parallel for if(batches * m * n > 4096)
    for (int row = 0; row < batches * m; ++row) {
        int batch = row / m, i = row % m;
        const float* aa = a + batch * m * k;
        const float* bb = b + batch * k * n;
        for (int j = 0; j < n; ++j) {
            float sum = 0.0f;
            for (int p = 0; p < k; ++p) {
                sum += (ta ? aa[p * m + i] : aa[i * k + p]) * (tb ? bb[j * k + p] : bb[p * n + j]);
            }
            if (accumulate) out[row * n + j] += sum * scale;
            else out[row * n + j] = sum * scale;
        }
    }
}

struct QubitCPU {
    float r, i, t;
};

void gemm_qubit(bool, const float* a, const float* b, float* out, int batches, int m, int n, int k, bool ta, bool tb, float scale, bool accumulate) {
    const QubitCPU* qa = reinterpret_cast<const QubitCPU*>(a);
    const QubitCPU* qb = reinterpret_cast<const QubitCPU*>(b);
    QubitCPU* qout = reinterpret_cast<QubitCPU*>(out);

    #pragma omp parallel for if(batches * m * n > 2048)
    for (int row = 0; row < batches * m; ++row) {
        int batch = row / m, i = row % m;
        const QubitCPU* batch_a = qa + batch * m * k;
        const QubitCPU* batch_b = qb + batch * k * n;
        for (int j = 0; j < n; ++j) {
            float sum_r = 0.0f, sum_i = 0.0f, max_t = 0.0f;
            for (int p = 0; p < k; ++p) {
                QubitCPU va = ta ? batch_a[p * m + i] : batch_a[i * k + p];
                QubitCPU vb = tb ? batch_b[j * k + p] : batch_b[p * n + j];
                sum_r += va.r * vb.r - va.i * vb.i;
                sum_i += va.r * vb.i + va.i * vb.r;
                float inter_t = va.t * vb.t;
                if (inter_t > max_t) max_t = inter_t;
            }
            int dst_idx = row * n + j;
            if (accumulate) {
                qout[dst_idx].r += sum_r * scale;
                qout[dst_idx].i += sum_i * scale;
                qout[dst_idx].t = max_t == 0.0f ? 1.0f : std::max(qout[dst_idx].t, max_t);
            } else {
                qout[dst_idx].r = sum_r * scale;
                qout[dst_idx].i = sum_i * scale;
                qout[dst_idx].t = max_t == 0.0f ? 1.0f : max_t;
            }
        }
    }
}

void gemm_qubit_backward(bool, const float* a, const float* b, float* out, int batches, int m, int n, int k, bool ta, bool tb, float scale, bool accumulate, bool conj_a, bool conj_b) {
    const QubitCPU* qa = reinterpret_cast<const QubitCPU*>(a);
    const QubitCPU* qb = reinterpret_cast<const QubitCPU*>(b);
    QubitCPU* qout = reinterpret_cast<QubitCPU*>(out);

    #pragma omp parallel for if(batches * m * n > 2048)
    for (int row = 0; row < batches * m; ++row) {
        int batch = row / m, i = row % m;
        const QubitCPU* batch_a = qa + batch * m * k;
        const QubitCPU* batch_b = qb + batch * k * n;
        for (int j = 0; j < n; ++j) {
            float sum_r = 0.0f, sum_i = 0.0f, max_t = 0.0f;
            for (int p = 0; p < k; ++p) {
                QubitCPU va = ta ? batch_a[p * m + i] : batch_a[i * k + p];
                QubitCPU vb = tb ? batch_b[j * k + p] : batch_b[p * n + j];
                if (conj_a) va.i = -va.i;
                if (conj_b) vb.i = -vb.i;
                sum_r += va.r * vb.r - va.i * vb.i;
                sum_i += va.r * vb.i + va.i * vb.r;
                float inter_t = va.t * vb.t;
                if (inter_t > max_t) max_t = inter_t;
            }
            int dst_idx = row * n + j;
            if (accumulate) {
                qout[dst_idx].r += sum_r * scale;
                qout[dst_idx].i += sum_i * scale;
                qout[dst_idx].t = max_t == 0.0f ? 1.0f : std::max(qout[dst_idx].t, max_t);
            } else {
                qout[dst_idx].r = sum_r * scale;
                qout[dst_idx].i = sum_i * scale;
                qout[dst_idx].t = max_t == 0.0f ? 1.0f : max_t;
            }
        }
    }
}

void add(bool, const float* a, const float* b, float* out, int n, bool backward) {
    for (int i = 0; i < n; ++i) {
        if (backward) out[i] += a[i];
        else out[i] = a[i] + b[i];
    }
}

void norm(bool, const float* x, const float* dy, float* out, int rows, int cols, bool backward) {
    #pragma omp parallel for if(rows > 128)
    for (int r = 0; r < rows; ++r) {
        float mean = 0.0f, var = 0.0f, g = 0.0f, gx = 0.0f;
        for (int j = 0; j < cols; ++j) mean += x[r * cols + j];
        mean /= cols;
        for (int j = 0; j < cols; ++j) {
            float z = x[r * cols + j] - mean;
            var += z * z;
        }
        float inv = 1.0f / std::sqrt(var / cols + 1e-5f);
        if (backward) {
            for (int j = 0; j < cols; ++j) {
                g += dy[r * cols + j];
                gx += dy[r * cols + j] * (x[r * cols + j] - mean) * inv;
            }
        }
        for (int j = 0; j < cols; ++j) {
            int i = r * cols + j;
            float z = (x[i] - mean) * inv;
            if (backward) out[i] += inv * (dy[i] - g / cols - z * gx / cols);
            else out[i] = z;
        }
    }
}

void gelu(bool, const float* x, const float* dy, float* out, int n, bool backward) {
    for (int i = 0; i < n; ++i) {
        float v = x[i];
        float t = std::tanh(0.7978845608f * (v + 0.044715f * v * v * v));
        if (backward) {
            out[i] += dy[i] * (0.5f * (1.0f + t) + 0.5f * v * (1.0f - t * t) * 0.7978845608f * (1.0f + 0.134145f * v * v));
        } else {
            out[i] = 0.5f * v * (1.0f + t);
        }
    }
}

void add_qubit(bool, const float* a, const float* b, float* out, int n, bool backward) {
    for (int i = 0; i < n * 3; ++i) {
        if (backward) out[i] += a[i];
        else out[i] = a[i] + b[i];
    }
}

void norm_qubit(bool, const float* x, const float* dy, float* out, int rows, int cols, bool backward) {
    norm(false, x, dy, out, rows, cols * 3, backward);
}

void gelu_qubit(bool, const float* x, const float* dy, float* out, int n, bool backward) {
    gelu(false, x, dy, out, n * 3, backward);
}

void heads(bool, const float* x, float* out, int batch, int time, int width, int nh, bool inverse, bool accumulate) {
    int dh = width / nh;
    for (int i = 0; i < batch * time * width; ++i) {
        int b = i / (time * width), t = (i / width) % time, h = (i % width) / dh, k = i % dh;
        int j = ((b * nh + h) * time + t) * dh + k;
        int dst = inverse ? i : j, src = inverse ? j : i;
        if (accumulate) out[dst] += x[src];
        else out[dst] = x[src];
    }
}

void heads_qubit(bool, const float* x, float* out, int batch, int time, int width, int heads_count, bool inverse, bool accumulate) {
    const QubitCPU* qx = reinterpret_cast<const QubitCPU*>(x);
    QubitCPU* qout = reinterpret_cast<QubitCPU*>(out);
    int dh = width / heads_count;
    for (int i = 0; i < batch * time * width; ++i) {
        int b = i / (time * width), t = (i / width) % time, h = (i % width) / dh, k = i % dh;
        int j = ((b * heads_count + h) * time + t) * dh + k;
        int dst = inverse ? i : j, src = inverse ? j : i;
        if (accumulate) {
            qout[dst].r += qx[src].r;
            qout[dst].i += qx[src].i;
            qout[dst].t = std::max(qout[dst].t, qx[src].t);
        } else {
            qout[dst] = qx[src];
        }
    }
}

void embedding(bool, const float* ids, const float* token, const float* position, float* out, float* dt, float* dp, int rows, int time, int width, bool backward) {
    for (int r = 0; r < rows; ++r) {
        for (int j = 0; j < width; ++j) {
            int t = int(ids[r]) * width + j;
            int p = (r % time) * width + j;
            int i = r * width + j;
            if (backward) {
                dt[t] += out[i];
                dp[p] += out[i];
            } else {
                out[i] = token[t] + position[p];
            }
        }
    }
}

void embedding_qubit(bool, const int* tokens, const float* token_emb, const float* pos_emb, float* out, int time, int width) {
    const QubitCPU* qt = reinterpret_cast<const QubitCPU*>(token_emb);
    const QubitCPU* qp = reinterpret_cast<const QubitCPU*>(pos_emb);
    QubitCPU* qout = reinterpret_cast<QubitCPU*>(out);

    for (int t = 0; t < time; ++t) {
        int tok = tokens[t];
        for (int w = 0; w < width; ++w) {
            int out_idx = t * width + w;
            int tok_idx = tok * width + w;
            int pos_idx = t * width + w;
            qout[out_idx].r = qt[tok_idx].r + qp[pos_idx].r;
            qout[out_idx].i = qt[tok_idx].i + qp[pos_idx].i;
            qout[out_idx].t = (qt[tok_idx].t + qp[pos_idx].t) * 0.5f;
        }
    }
}

void logits_qubit(bool, const float* out_state, const float* output_weights, float* logits, int time, int width, int vocab) {
    const QubitCPU* qs = reinterpret_cast<const QubitCPU*>(out_state);
    const QubitCPU* qw = reinterpret_cast<const QubitCPU*>(output_weights);

    #pragma omp parallel for
    for (int t = 0; t < time; ++t) {
        for (int v = 0; v < vocab; ++v) {
            float sum_r = 0.0f, sum_i = 0.0f;
            for (int w = 0; w < width; ++w) {
                QubitCPU s = qs[t * width + w];
                QubitCPU ow = qw[w * vocab + v];
                sum_r += s.r * ow.r - s.i * ow.i;
                sum_i += s.r * ow.i + s.i * ow.r;
            }
            logits[t * vocab + v] = std::sqrt(sum_r * sum_r + sum_i * sum_i);
        }
    }
}

void cross_entropy_qubit(bool, const float* logits, const int* targets, float* grad_logits, float* loss, int time, int vocab) {
    float total_loss = 0.0f;
    for (int t = 0; t < time; ++t) {
        int target = targets[t];
        if (target < 0 || target >= vocab) {
            std::fill(grad_logits + t * vocab, grad_logits + (t + 1) * vocab, 0.0f);
            continue;
        }
        float max_l = -1e9f;
        for (int v = 0; v < vocab; ++v) max_l = std::max(max_l, logits[t * vocab + v]);
        float sum_exp = 0.0f;
        for (int v = 0; v < vocab; ++v) sum_exp += std::exp(logits[t * vocab + v] - max_l);
        total_loss += std::log(sum_exp) + max_l - logits[t * vocab + target];
        for (int v = 0; v < vocab; ++v) {
            float p = std::exp(logits[t * vocab + v] - max_l) / sum_exp;
            grad_logits[t * vocab + v] = (p - (v == target ? 1.0f : 0.0f)) / time;
        }
    }
    *loss = total_loss / time;
}

void softmax(bool, const float* x, const float* dy, float* out, int batches, int time, bool backward) {
    for (int r = 0; r < batches * time; ++r) {
        int last = r % time;
        if (backward) {
            float dot = 0.0f;
            for (int j = 0; j <= last; ++j) dot += dy[r * time + j] * x[r * time + j];
            for (int j = 0; j <= last; ++j) out[r * time + j] += x[r * time + j] * (dy[r * time + j] - dot);
        } else {
            float maximum = -1e9f, sum = 0.0f;
            for (int j = 0; j <= last; ++j) maximum = std::max(maximum, x[r * time + j]);
            for (int j = 0; j <= last; ++j) {
                out[r * time + j] = std::exp(x[r * time + j] - maximum);
                sum += out[r * time + j];
            }
            for (int j = 0; j < time; ++j) out[r * time + j] = (j <= last) ? out[r * time + j] / sum : 0.0f;
        }
    }
}

void loss(bool, const float* x, const float* targets, float* grad, float* losses, int rows, int vocab, int labeled) {
    for (int r = 0; r < rows; ++r) {
        int target = int(targets[r]);
        losses[r] = 0.0f;
        if (target < 0) {
            std::fill(grad + r * vocab, grad + (r + 1) * vocab, 0.0f);
            continue;
        }
        float maximum = -1e9f, sum = 0.0f;
        for (int j = 0; j < vocab; ++j) maximum = std::max(maximum, x[r * vocab + j]);
        for (int j = 0; j < vocab; ++j) sum += std::exp(x[r * vocab + j] - maximum);
        losses[r] = (std::log(sum) + maximum - x[r * vocab + target]) / labeled;
        for (int j = 0; j < vocab; ++j) grad[r * vocab + j] = (std::exp(x[r * vocab + j] - maximum) / sum - (j == target ? 1 : 0)) / labeled;
    }
}

void adam(bool, float* w, float* m, float* v, const float* grad, int n, float rate, float c1, float c2) {
    for (int i = 0; i < n; ++i) {
        float g = std::clamp(grad[i], -1.0f, 1.0f);
        m[i] = 0.9f * m[i] + 0.1f * g;
        v[i] = 0.999f * v[i] + 0.001f * g * g;
        w[i] -= rate * (m[i] / c1 / (std::sqrt(v[i] / c2) + 1e-8f) + 0.01f * w[i]);
    }
}

void adam_qubit(bool, float* w, float* m, float* v, const float* grad, int n, float rate, float c1, float c2) {
    adam(false, w, m, v, grad, n * 3, rate, c1, c2);
}

void softmax_qubit(bool, const float* x, float* out, int batches, int time, float scale) {
    const QubitCPU* qx = reinterpret_cast<const QubitCPU*>(x);
    QubitCPU* qout = reinterpret_cast<QubitCPU*>(out);

    for (int r = 0; r < batches; ++r) {
        int last = r % time;
        float maximum = -1e9f;
        for (int j = 0; j <= last; ++j) {
            maximum = std::max(maximum, qx[r * time + j].r * scale);
        }
        float sum = 0.0f;
        for (int j = 0; j <= last; ++j) {
            sum += std::exp(qx[r * time + j].r * scale - maximum);
        }
        for (int j = 0; j < time; ++j) {
            if (j <= last) {
                float w = std::exp(qx[r * time + j].r * scale - maximum) / (sum > 0.0f ? sum : 1.0f);
                qout[r * time + j].r = w;
                qout[r * time + j].i = 0.0f;
                qout[r * time + j].t = qx[r * time + j].t;
            } else {
                qout[r * time + j] = {0.0f, 0.0f, 0.0f};
            }
        }
    }
}

void softmax_qubit_backward(bool, const float* out, const float* dy, float* dx, int batches, int time, float scale) {
    const QubitCPU* qout = reinterpret_cast<const QubitCPU*>(out);
    const QubitCPU* qdy = reinterpret_cast<const QubitCPU*>(dy);
    QubitCPU* qdx = reinterpret_cast<QubitCPU*>(dx);

    for (int r = 0; r < batches; ++r) {
        int last = r % time;
        float dot = 0.0f;
        for (int j = 0; j <= last; ++j) {
            dot += qdy[r * time + j].r * qout[r * time + j].r;
        }
        for (int j = 0; j < time; ++j) {
            if (j <= last) {
                qdx[r * time + j].r = (qdy[r * time + j].r - dot) * qout[r * time + j].r * scale;
                qdx[r * time + j].i = 0.0f;
                qdx[r * time + j].t = 1.0f;
            } else {
                qdx[r * time + j] = {0.0f, 0.0f, 0.0f};
            }
        }
    }
}

} // namespace ghost_tr_cuda

#endif // !GHOST_HAS_CUDA
