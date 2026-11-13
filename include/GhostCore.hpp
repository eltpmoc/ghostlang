#pragma once
// GhostCore.hpp - runtime do núcleo da linguagem GhostLang (coleções, erros,
// result, utilidades).
//
//   list<int> xs = [3, 1, 2];        -> GhostList<int>   (std::vector com
//   métodos da linguagem) map<string, int> m = {"a": 1};   ->
//   GhostMap<std::string, int> (std::map ordenado) set<int> s = [3, 1, 3]; ->
//   GhostSet<int> (std::set: ordenado e sem repetidos) result<double> r =
//   ok(2.5);      -> GhostResult<double> foreach (x in xs) { }  foreach (k, v
//   in m) { }  foreach (i in range(0, 10)) { } try { throw "falhou"; } catch
//   (e) { print(e); } finally { ... }
//
// Índices de lista são verificados (erro capturável com try/catch) e aceitam
// negativos: xs[-1].
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "GhostNumText.hpp"

// ---------------------------------------------------------------- texto de
// qualquer valor
namespace ghost_detail {
template <class T, class = void> struct streamable : std::false_type {};
template <class T>
struct streamable<T, std::void_t<decltype(std::declval<std::ostream &>()
                                          << std::declval<const T &>())>>
    : std::true_type {};

inline void repr(std::ostream &os, const std::string &s, bool quoted) {
  if (quoted)
    os << '"' << s << '"';
  else
    os << s;
}
inline void repr(std::ostream &os, const char *s, bool quoted) {
  repr(os, std::string(s), quoted);
}
inline void repr(std::ostream &os, bool b, bool) {
  os << (b ? "true" : "false");
}
inline void repr(std::ostream &os, double v, bool) { os << ghost_num_text(v); }
inline void repr(std::ostream &os, float v, bool) { os << ghost_num_text(v); }
template <class T> void repr(std::ostream &os, const T &v, bool) { os << v; }
} // namespace ghost_detail

// ---------------------------------------------------------------- list<T>
template <class T> class GhostList : public std::vector<T> {
public:
  using std::vector<T>::vector;
  GhostList() = default;
  GhostList(const std::vector<T> &v) : std::vector<T>(v) {}
  GhostList(std::vector<T> &&v) : std::vector<T>(std::move(v)) {}
  // list<double> xs = list_de_int;  (conversão elemento a elemento)
  template <class U, class = std::enable_if_t<!std::is_same<U, T>::value &&
                                              std::is_convertible<U, T>::value>>
  GhostList(const GhostList<U> &o) : std::vector<T>(o.begin(), o.end()) {}

  // xs[i] com verificação de limites; xs[-1] é o último
  T &operator[](long long i) { return this->at(index(i)); }
  const T &operator[](long long i) const { return this->at(index(i)); }

  int length() const { return static_cast<int>(this->size()); }
  bool empty() const { return std::vector<T>::empty(); }
  void push(const T &v) { this->push_back(v); }
  void append(const T &v) { this->push_back(v); }
  T pop() {
    if (this->std::vector<T>::empty())
      throw std::out_of_range("list.pop: lista vazia");
    T v = this->back();
    this->pop_back();
    return v;
  }
  void insert_at(long long i, const T &v) {
    const long long n = static_cast<long long>(this->size());
    if (i < 0)
      i += n;
    if (i < 0 || i > n)
      throw std::out_of_range("list.insert_at: indice " + std::to_string(i) +
                              " fora da lista de " + std::to_string(n));
    this->insert(this->begin() + i, v);
  }
  T remove_at(long long i) {
    const std::size_t k = index(i);
    T v = (*static_cast<std::vector<T> *>(this))[k];
    this->erase(this->begin() + static_cast<long long>(k));
    return v;
  }
  bool contains(const T &v) const {
    return std::find(this->begin(), this->end(), v) != this->end();
  }
  int index_of(const T &v) const {
    auto it = std::find(this->begin(), this->end(), v);
    return it == this->end() ? -1 : static_cast<int>(it - this->begin());
  }
  GhostList<T> &sort() {
    std::sort(this->begin(), this->end());
    return *this;
  }
  GhostList<T> &reverse() {
    std::reverse(this->begin(), this->end());
    return *this;
  }
  GhostList<T> sorted() const {
    GhostList<T> c(*this);
    std::sort(c.begin(), c.end());
    return c;
  }
  GhostList<T> slice(long long from, long long to, long long step = 1) const {
    if (step == 0)
      throw std::invalid_argument("o passo do fatiamento nao pode ser zero");
    const long long n = static_cast<long long>(this->size());
    GhostList<T> res;
    if (step > 0) {
      if (from < 0)
        from += n;
      if (to < 0)
        to += n;
      from = std::max(0LL, std::min(from, n));
      to = std::max(0LL, std::min(to, n));
      for (long long i = from; i < to; i += step)
        res.push_back((*this)[i]);
    } else {
      if (from < 0)
        from += n;
      if (to < 0)
        to += n;
      from = std::max(-1LL, std::min(from, n - 1));
      to = std::max(-1LL, std::min(to, n - 1));
      for (long long i = from; i > to; i += step)
        res.push_back((*this)[i]);
    }
    return res;
  }
  // Operações vetoriais em lote
  GhostList<T> vec_add(const GhostList<T> &o) const {
    if (this->size() != o.size())
      throw std::runtime_error("vec_add: vetores com tamanhos diferentes (" +
                               std::to_string(this->size()) +
                               " != " + std::to_string(o.size()) + ")");
    GhostList<T> r;
    r.reserve(this->size());
    for (std::size_t i = 0; i < this->size(); ++i)
      r.push_back((*this)[i] + o[i]);
    return r;
  }
  GhostList<T> vec_sub(const GhostList<T> &o) const {
    if (this->size() != o.size())
      throw std::runtime_error("vec_sub: vetores com tamanhos diferentes (" +
                               std::to_string(this->size()) +
                               " != " + std::to_string(o.size()) + ")");
    GhostList<T> r;
    r.reserve(this->size());
    for (std::size_t i = 0; i < this->size(); ++i)
      r.push_back((*this)[i] - o[i]);
    return r;
  }
  GhostList<T> vec_mul(const GhostList<T> &o) const {
    if (this->size() != o.size())
      throw std::runtime_error("vec_mul: vetores com tamanhos diferentes (" +
                               std::to_string(this->size()) +
                               " != " + std::to_string(o.size()) + ")");
    GhostList<T> r;
    r.reserve(this->size());
    for (std::size_t i = 0; i < this->size(); ++i)
      r.push_back((*this)[i] * o[i]);
    return r;
  }
  GhostList<double> vec_div(const GhostList<T> &o) const {
    if (this->size() != o.size())
      throw std::runtime_error("vec_div: vetores com tamanhos diferentes (" +
                               std::to_string(this->size()) +
                               " != " + std::to_string(o.size()) + ")");
    GhostList<double> r;
    r.reserve(this->size());
    for (std::size_t i = 0; i < this->size(); ++i) {
      if (o[i] == 0)
        throw std::runtime_error("divisao por zero");
      r.push_back(static_cast<double>((*this)[i]) / static_cast<double>(o[i]));
    }
    return r;
  }
  GhostList<double> vec_scale(double k) const {
    GhostList<double> r;
    r.reserve(this->size());
    for (std::size_t i = 0; i < this->size(); ++i)
      r.push_back(static_cast<double>((*this)[i]) * k);
    return r;
  }
  double dot(const GhostList<T> &o) const {
    if (this->size() != o.size())
      throw std::runtime_error("dot: vetores com tamanhos diferentes (" +
                               std::to_string(this->size()) +
                               " != " + std::to_string(o.size()) + ")");
    double s = 0.0;
    for (std::size_t i = 0; i < this->size(); ++i)
      s += static_cast<double>((*this)[i]) * static_cast<double>(o[i]);
    return s;
  }
  double norm() const {
    double s = 0.0;
    for (std::size_t i = 0; i < this->size(); ++i)
      s += static_cast<double>((*this)[i]) * static_cast<double>((*this)[i]);
    return std::sqrt(s);
  }
  T first() const { return (*this)[0]; }
  T last() const { return (*this)[-1]; }
  T sum() const { return std::accumulate(this->begin(), this->end(), T{}); }
  T min() const {
    if (this->std::vector<T>::empty())
      throw std::out_of_range("list.min: lista vazia");
    return *std::min_element(this->begin(), this->end());
  }
  T max() const {
    if (this->std::vector<T>::empty())
      throw std::out_of_range("list.max: lista vazia");
    return *std::max_element(this->begin(), this->end());
  }
  std::string join(const std::string &sep) const {
    std::ostringstream os;
    for (std::size_t k = 0; k < this->size(); ++k) {
      if (k)
        os << sep;
      ghost_detail::repr(os, (*static_cast<const std::vector<T> *>(this))[k],
                         false);
    }
    return os.str();
  }
  template <class F>
  auto map(F&& f) const {
    using R = std::decay_t<decltype(std::declval<F>()(std::declval<T>()))>;
    GhostList<R> res;
    res.reserve(this->size());
    for (std::size_t i = 0; i < this->size(); ++i) {
      res.push_back(f((*this)[i]));
    }
    return res;
  }
  template <class F>
  GhostList<T> filter(F&& f) const {
    GhostList<T> res;
    for (std::size_t i = 0; i < this->size(); ++i) {
      if (f((*this)[i])) {
        res.push_back((*this)[i]);
      }
    }
    return res;
  }
  template <class F, class Acc>
  Acc reduce(F&& f, Acc init) const {
    Acc acc = init;
    for (std::size_t i = 0; i < this->size(); ++i) {
      acc = f(acc, (*this)[i]);
    }
    return acc;
  }
  template <class F>
  void for_each(F&& f) const {
    for (std::size_t i = 0; i < this->size(); ++i) {
      f((*this)[i]);
    }
  }
  GhostList<T> operator+(const GhostList<T> &o) const {
    GhostList<T> c(*this);
    c.insert(c.end(), o.begin(), o.end());
    return c;
  }

private:
  std::size_t index(long long i) const {
    const long long n = static_cast<long long>(this->size());
    const long long k = i < 0 ? i + n : i;
    if (k < 0 || k >= n)
      throw std::out_of_range("indice " + std::to_string(i) +
                              " fora da lista de " + std::to_string(n) +
                              " elemento(s)");
    return static_cast<std::size_t>(k);
  }
};

template <class T>
std::ostream &operator<<(std::ostream &os, const GhostList<T> &xs) {
  os << '[';
  for (std::size_t k = 0; k < xs.size(); ++k) {
    if (k)
      os << ", ";
    ghost_detail::repr(os, static_cast<const std::vector<T> &>(xs)[k], true);
  }
  return os << ']';
}

// [a, b, c] em expressões
template <class... A>
GhostList<std::common_type_t<std::decay_t<A>...>> ghost_list(A &&...a) {
  return GhostList<std::common_type_t<std::decay_t<A>...>>{
      std::forward<A>(a)...};
}

inline GhostList<int> range(int end) {
  GhostList<int> r;
  for (int i = 0; i < end; ++i)
    r.push_back(i);
  return r;
}
inline GhostList<int> range(int start, int end, int step = 1) {
  if (step == 0)
    throw std::invalid_argument("range: passo zero");
  GhostList<int> r;
  if (step > 0)
    for (int i = start; i < end; i += step)
      r.push_back(i);
  else
    for (int i = start; i > end; i += step)
      r.push_back(i);
  return r;
}

// ---------------------------------------------------------------- fatiamento
// [start:end:step]
struct GhostSliceArg {
  bool has_start = false;
  long long start = 0;
  bool has_end = false;
  long long end = 0;
  bool has_step = false;
  long long step = 1;
};

template <class T>
inline GhostList<T> ghost_slice(const GhostList<T> &xs, GhostSliceArg arg) {
  const long long n = static_cast<long long>(xs.size());
  const long long step = arg.has_step ? arg.step : 1;
  if (step == 0)
    throw std::invalid_argument("o passo do fatiamento nao pode ser zero");
  GhostList<T> res;
  if (step > 0) {
    long long s = 0;
    if (arg.has_start) {
      s = arg.start;
      if (s < 0)
        s += n;
      s = std::max(0LL, std::min(s, n));
    }
    long long e = n;
    if (arg.has_end) {
      e = arg.end;
      if (e < 0)
        e += n;
      e = std::max(0LL, std::min(e, n));
    }
    for (long long i = s; i < e; i += step)
      res.push_back(xs[i]);
  } else {
    long long s = n - 1;
    if (arg.has_start) {
      s = arg.start;
      if (s < 0)
        s += n;
      s = std::max(-1LL, std::min(s, n - 1));
    }
    long long e = -1;
    if (arg.has_end) {
      e = arg.end;
      if (e < 0)
        e += n;
      e = std::max(-1LL, std::min(e, n - 1));
    }
    for (long long i = s; i > e; i += step)
      res.push_back(xs[i]);
  }
  return res;
}

inline std::string ghost_slice(const std::string &str, GhostSliceArg arg) {
  const long long n = static_cast<long long>(str.size());
  const long long step = arg.has_step ? arg.step : 1;
  if (step == 0)
    throw std::invalid_argument("o passo do fatiamento nao pode ser zero");
  std::string res;
  if (step > 0) {
    long long s = 0;
    if (arg.has_start) {
      s = arg.start;
      if (s < 0)
        s += n;
      s = std::max(0LL, std::min(s, n));
    }
    long long e = n;
    if (arg.has_end) {
      e = arg.end;
      if (e < 0)
        e += n;
      e = std::max(0LL, std::min(e, n));
    }
    for (long long i = s; i < e; i += step)
      res += str[static_cast<size_t>(i)];
  } else {
    long long s = n - 1;
    if (arg.has_start) {
      s = arg.start;
      if (s < 0)
        s += n;
      s = std::max(-1LL, std::min(s, n - 1));
    }
    long long e = -1;
    if (arg.has_end) {
      e = arg.end;
      if (e < 0)
        e += n;
      e = std::max(-1LL, std::min(e, n - 1));
    }
    for (long long i = s; i > e; i += step)
      res += str[static_cast<size_t>(i)];
  }
  return res;
}

// ---------------------------------------------------------------- operações
// vetoriais e matriciais em lote Listas de tipos numéricos diferentes usam o
// tipo comum (int com double vira double), como no REPL: dot([1.5, 2], [2, 2])
// == 7.
template <class A, class B>
inline GhostList<std::common_type_t<A, B>> vec_add(const GhostList<A> &a,
                                                   const GhostList<B> &b) {
  using C = std::common_type_t<A, B>;
  return GhostList<C>(a).vec_add(GhostList<C>(b));
}
template <class A, class B>
inline GhostList<std::common_type_t<A, B>> vec_sub(const GhostList<A> &a,
                                                   const GhostList<B> &b) {
  using C = std::common_type_t<A, B>;
  return GhostList<C>(a).vec_sub(GhostList<C>(b));
}
template <class A, class B>
inline GhostList<std::common_type_t<A, B>> vec_mul(const GhostList<A> &a,
                                                   const GhostList<B> &b) {
  using C = std::common_type_t<A, B>;
  return GhostList<C>(a).vec_mul(GhostList<C>(b));
}
template <class A, class B>
inline GhostList<double> vec_div(const GhostList<A> &a, const GhostList<B> &b) {
  using C = std::common_type_t<A, B>;
  return GhostList<C>(a).vec_div(GhostList<C>(b));
}
template <class T>
inline GhostList<double> vec_scale(const GhostList<T> &a, double k) {
  return a.vec_scale(k);
}
template <class A, class B>
inline double dot(const GhostList<A> &a, const GhostList<B> &b) {
  using C = std::common_type_t<A, B>;
  return GhostList<C>(a).dot(GhostList<C>(b));
}
template <class T> inline double norm(const GhostList<T> &a) {
  return a.norm();
}

template <class A, class B,
          class = std::enable_if_t<!std::is_same<A, B>::value>>
inline GhostList<std::common_type_t<A, B>>
mat_mul(const GhostList<A> &A_, const GhostList<B> &B_, int rA, int cA, int cB);

template <class T>
inline GhostList<T> mat_mul(const GhostList<T> &A, const GhostList<T> &B,
                            int rA, int cA, int cB) {
  if (rA <= 0 || cA <= 0 || cB <= 0)
    throw std::invalid_argument("mat_mul: dimensoes precisam ser positivas");
  if (static_cast<size_t>(rA * cA) != A.size())
    throw std::invalid_argument("mat_mul: tamanho de A nao bate com rA * cA");
  if (static_cast<size_t>(cA * cB) != B.size())
    throw std::invalid_argument("mat_mul: tamanho de B nao bate com cA * cB");
  GhostList<T> C;
  C.resize(static_cast<size_t>(rA * cB), T{});
  for (int i = 0; i < rA; ++i) {
    for (int k = 0; k < cA; ++k) {
      T aik = A[static_cast<size_t>(i * cA + k)];
      for (int j = 0; j < cB; ++j) {
        C[static_cast<size_t>(i * cB + j)] +=
            aik * B[static_cast<size_t>(k * cB + j)];
      }
    }
  }
  return C;
}

template <class A, class B, class>
inline GhostList<std::common_type_t<A, B>> mat_mul(const GhostList<A> &A_,
                                                   const GhostList<B> &B_,
                                                   int rA, int cA, int cB) {
  using C = std::common_type_t<A, B>;
  return mat_mul(GhostList<C>(A_), GhostList<C>(B_), rA, cA, cB);
}

template <class T>
inline GhostList<T> mat_transpose(const GhostList<T> &A, int r, int c) {
  if (r <= 0 || c <= 0)
    throw std::invalid_argument(
        "mat_transpose: dimensoes precisam ser positivas");
  if (static_cast<size_t>(r * c) != A.size())
    throw std::invalid_argument(
        "mat_transpose: tamanho de A nao bate com r * c");
  GhostList<T> C;
  C.resize(static_cast<size_t>(r * c), T{});
  for (int i = 0; i < r; ++i) {
    for (int j = 0; j < c; ++j) {
      C[static_cast<size_t>(j * r + i)] = A[static_cast<size_t>(i * c + j)];
    }
  }
  return C;
}

template <class T>
inline GhostList<GhostList<T>> mat2d_mul(const GhostList<GhostList<T>> &A,
                                         const GhostList<GhostList<T>> &B) {
  if (A.empty() || B.empty())
    throw std::invalid_argument("mat2d_mul: matrizes nao podem ser vazias");
  const size_t rA = A.size();
  const size_t cA = A[0].size();
  const size_t rB = B.size();
  const size_t cB = B[0].size();
  if (cA != rB)
    throw std::invalid_argument("mat2d_mul: colunas de A (" +
                                std::to_string(cA) + ") != linhas de B (" +
                                std::to_string(rB) + ")");
  GhostList<GhostList<T>> C;
  C.resize(rA);
  for (size_t i = 0; i < rA; ++i) {
    C[i].resize(cB, T{});
    for (size_t k = 0; k < cA; ++k) {
      T aik = A[i][k];
      for (size_t j = 0; j < cB; ++j) {
        C[i][j] += aik * B[k][j];
      }
    }
  }
  return C;
}

// ---------------------------------------------------------------- map<K, V>
template <class K, class V> class GhostMap : public std::map<K, V> {
public:
  using std::map<K, V>::map;
  GhostMap() = default;

  // m[k] cria a chave se não existir (como em C++/Python dict com default)
  int length() const { return static_cast<int>(this->size()); }
  bool has(const K &k) const { return this->find(k) != this->end(); }
  bool contains(const K &k) const { return has(k); }
  V get(const K &k) const {
    auto it = this->find(k);
    if (it == this->end()) {
      std::ostringstream os;
      os << "chave ";
      ghost_detail::repr(os, k, true);
      os << " nao existe no mapa";
      throw std::out_of_range(os.str());
    }
    return it->second;
  }
  V get(const K &k, const V &fallback) const {
    auto it = this->find(k);
    return it == this->end() ? fallback : it->second;
  }
  void set(const K &k, const V &v) { (*this)[k] = v; }
  bool remove(const K &k) { return this->erase(k) > 0; }
  GhostList<K> keys() const {
    GhostList<K> r;
    for (const auto &kv : *this)
      r.push_back(kv.first);
    return r;
  }
  GhostList<V> values() const {
    GhostList<V> r;
    for (const auto &kv : *this)
      r.push_back(kv.second);
    return r;
  }
};

template <class K, class V>
std::ostream &operator<<(std::ostream &os, const GhostMap<K, V> &m) {
  os << '{';
  bool first = true;
  for (const auto &kv : m) {
    if (!first)
      os << ", ";
    first = false;
    ghost_detail::repr(os, kv.first, true);
    os << ": ";
    ghost_detail::repr(os, kv.second, true);
  }
  return os << '}';
}

// ---------------------------------------------------------------- set<T>
// Conjunto ordenado sem repetidos. set<int> s = [3, 1, 3];  ->  {1, 3}
// s.union(o) vira s.union_(o) no C++ (union é palavra reservada).
template <class T> class GhostSet : public std::set<T> {
public:
  using std::set<T>::set;
  GhostSet() = default;
  // set<int> s = lista;  set<double> d = conjunto_de_int;
  template <class U> GhostSet(const GhostList<U> &xs) {
    for (const auto &x : xs)
      this->insert(static_cast<T>(x));
  }
  template <class U, class = std::enable_if_t<!std::is_same<U, T>::value &&
                                              std::is_convertible<U, T>::value>>
  GhostSet(const GhostSet<U> &o) {
    for (const auto &x : o)
      this->insert(static_cast<T>(x));
  }

  int length() const { return static_cast<int>(this->size()); }
  bool empty() const { return std::set<T>::empty(); }
  bool add(const T &v) {
    return this->insert(v).second;
  } // true se o valor era novo
  bool remove(const T &v) { return this->erase(v) > 0; } // true se existia
  bool contains(const T &v) const { return this->find(v) != this->end(); }
  bool has(const T &v) const { return contains(v); }
  GhostSet<T> union_(const GhostSet<T> &o) const {
    GhostSet<T> r(*this);
    r.insert(o.begin(), o.end());
    return r;
  }
  GhostSet<T> intersection(const GhostSet<T> &o) const {
    GhostSet<T> r;
    for (const auto &x : *this)
      if (o.contains(x))
        r.insert(x);
    return r;
  }
  GhostSet<T> difference(const GhostSet<T> &o) const {
    GhostSet<T> r;
    for (const auto &x : *this)
      if (!o.contains(x))
        r.insert(x);
    return r;
  }
  bool is_subset(const GhostSet<T> &o) const {
    for (const auto &x : *this)
      if (!o.contains(x))
        return false;
    return true;
  }
  bool is_superset(const GhostSet<T> &o) const { return o.is_subset(*this); }
  GhostList<T> to_list() const {
    return GhostList<T>(this->begin(), this->end());
  }
  T min() const {
    if (std::set<T>::empty())
      throw std::out_of_range("set.min: conjunto vazio");
    return *this->begin();
  }
  T max() const {
    if (std::set<T>::empty())
      throw std::out_of_range("set.max: conjunto vazio");
    return *this->rbegin();
  }
};

template <class T>
std::ostream &operator<<(std::ostream &os, const GhostSet<T> &s) {
  os << '{';
  bool first = true;
  for (const auto &x : s) {
    if (!first)
      os << ", ";
    first = false;
    ghost_detail::repr(os, x, true);
  }
  return os << '}';
}

// set(lista): conjunto com os elementos da lista (em GhostLang: set(xs))
template <class T> GhostSet<T> ghost_to_set(const GhostList<T> &xs) {
  return GhostSet<T>(xs.begin(), xs.end());
}
template <class T> GhostSet<T> ghost_to_set(const GhostSet<T> &s) { return s; }

// ---------------------------------------------------------------- [expr for x
// in coleção if cond] ghost_iter: o que a compreensão percorre. Lista/conjunto:
// a própria coleção (sem cópia se for variável); mapa: as chaves; texto: as
// letras (como o foreach do REPL).
namespace ghost_detail {
template <class T> struct is_ghost_map : std::false_type {};
template <class K, class V>
struct is_ghost_map<GhostMap<K, V>> : std::true_type {};
} // namespace ghost_detail

template <class T> decltype(auto) ghost_iter(T &&c) {
  using D = std::decay_t<T>;
  if constexpr (ghost_detail::is_ghost_map<D>::value) {
    return c.keys();
  } else if constexpr (std::is_same_v<D, std::string> ||
                       std::is_same_v<D, const char *> ||
                       std::is_same_v<D, char *>) {
    const std::string s(c);
    GhostList<std::string> r;
    for (char ch : s)
      r.push_back(std::string(1, ch));
    return r;
  } else if constexpr (std::is_lvalue_reference_v<T>) {
    return static_cast<T>(c);
  } else {
    return D(std::move(c));
  }
}
template <class T> decltype(auto) ghost_iter_kv(T &&c) {
  using D = std::decay_t<T>;
  static_assert(ghost_detail::is_ghost_map<D>::value,
                "compreensao com (chave, valor) exige um map<K, V>");
  if constexpr (std::is_lvalue_reference_v<T>) {
    return static_cast<T>(c);
  } else {
    return D(std::move(c));
  }
}
template <class T, class U> void ghost_collect(GhostList<T> &out, U &&v) {
  out.push_back(static_cast<T>(std::forward<U>(v)));
}
template <class T, class U> void ghost_collect(GhostSet<T> &out, U &&v) {
  out.insert(static_cast<T>(std::forward<U>(v)));
}
template <class K, class V, class K2, class V2>
void ghost_collect(GhostMap<K, V> &out, K2 &&k, V2 &&v) {
  out[static_cast<K>(std::forward<K2>(k))] =
      static_cast<V>(std::forward<V2>(v));
}

// ---------------------------------------------------------------- len(x)
inline int len(const std::string &s) { return static_cast<int>(s.size()); }
inline int len(const char *s) {
  return static_cast<int>(std::string(s).size());
}
template <class T> int len(const std::vector<T> &v) {
  return static_cast<int>(v.size());
}
template <class K, class V> int len(const std::map<K, V> &m) {
  return static_cast<int>(m.size());
}
template <class T> int len(const std::set<T> &s) {
  return static_cast<int>(s.size());
}

// ---------------------------------------------------------------- erros
// Mensagem de qualquer exceção (std::exception, string, const char*) para catch
// (e).
inline std::string ghost_exception_message(std::exception_ptr p) {
  try {
    if (p)
      std::rethrow_exception(p);
  } catch (const std::exception &e) {
    return e.what();
  } catch (const std::string &s) {
    return s;
  } catch (const char *s) {
    return s;
  } catch (...) {
    return "erro desconhecido";
  }
  return "erro desconhecido";
}

// throw expr;  ->  throw ghost_error(expr)
class GhostError : public std::runtime_error {
public:
  explicit GhostError(const std::string &msg) : std::runtime_error(msg) {}
};
template <class T> GhostError ghost_error(const T &v) {
  std::ostringstream os;
  ghost_detail::repr(os, v, false);
  return GhostError(os.str());
}

// finally { ... }: roda ao sair do bloco (normal, return, break ou exceção)
class GhostDefer {
public:
  explicit GhostDefer(std::function<void()> f) : f_(std::move(f)) {}
  GhostDefer(const GhostDefer &) = delete;
  GhostDefer &operator=(const GhostDefer &) = delete;
  ~GhostDefer() {
    if (f_)
      f_();
  }

private:
  std::function<void()> f_;
};

// assert(cond) / assert(cond, "mensagem")
inline void ghost_assert(bool cond, const std::string &msg = "assert falhou") {
  if (!cond)
    throw GhostError(msg);
}

// ---------------------------------------------------------------- result<T>
// Erros recuperáveis sem exceção:
//   function result<double> dividir(double a, double b) { if (b == 0) { return
//   err("divisao por zero"); } return ok(a / b); } result<double> r =
//   dividir(1, 0); if (r.is_err()) { print(r.error()); }   double v =
//   r.value_or(0);
template <class T> struct GhostOk {
  T value;
};
struct GhostErr {
  std::string message;
};
template <class T> GhostOk<std::decay_t<T>> ok(T &&v) {
  return {std::forward<T>(v)};
}
inline GhostOk<bool> ok() { return {true}; }
template <class T> GhostErr err(const T &msg) {
  std::ostringstream os;
  ghost_detail::repr(os, msg, false);
  return {os.str()};
}

template <class T> class GhostResult {
public:
  GhostResult() : ok_(false), error_("result sem valor") {}
  template <class U>
  GhostResult(const GhostOk<U> &o)
      : ok_(true), value_(static_cast<T>(o.value)) {}
  GhostResult(const GhostErr &e) : ok_(false), error_(e.message) {}

  bool is_ok() const { return ok_; }
  bool is_err() const { return !ok_; }
  const T &value() const {
    if (!ok_)
      throw GhostError(error_);
    return value_;
  }
  T value_or(const T &fallback) const { return ok_ ? value_ : fallback; }
  std::string error() const { return ok_ ? std::string() : error_; }
  explicit operator bool() const { return ok_; }

private:
  bool ok_;
  T value_{};
  std::string error_;
};

// r? (operador de result): só aceita result<T>; o resto vira erro de compilação
// "ghost_try_arg"
template <class T> GhostResult<T> ghost_try_arg(GhostResult<T> r) { return r; }

template <class T>
std::ostream &operator<<(std::ostream &os, const GhostResult<T> &r) {
  if (r.is_ok()) {
    os << "ok(";
    ghost_detail::repr(os, r.value(), true);
    return os << ")";
  }
  return os << "err(\"" << r.error() << "\")";
}

// ----------------------------------------------------------------
// fn<Ret(Args...)> e lambdas Valor de função com semântica de valor (cópia
// duplica o estado capturado, como em [=]() mutable).
template <class Sig> class GhostFn;
template <class F> class GhostPolyFn;

namespace ghost_detail {
template <class T> struct is_ghost_fn : std::false_type {};
template <class R, class... A>
struct is_ghost_fn<GhostFn<R(A...)>> : std::true_type {};
template <class T> struct is_ghost_poly_fn : std::false_type {};
template <class F> struct is_ghost_poly_fn<GhostPolyFn<F>> : std::true_type {};

template <class From, class To>
struct fn_ret_conv
    : std::bool_constant<
          std::is_void_v<To> ||
          (std::is_arithmetic_v<From> && std::is_arithmetic_v<To>
               ? (std::is_same_v<From, To> ||
                  (std::is_floating_point_v<To> && std::is_integral_v<From> &&
                   !std::is_same_v<From, bool>) ||
                  (std::is_same_v<To, double> && std::is_same_v<From, float>))
               : std::is_convertible_v<From, To>)> {};

template <class F, class R, class... A> struct callable_ret_ok {
  static constexpr bool value =
      fn_ret_conv<std::decay_t<std::invoke_result_t<F &, A...>>, R>::value;
};

template <class F, class R, class... A>
constexpr bool is_callable_for_fn_v =
    std::conditional_t<std::is_invocable_r_v<R, F &, A...>,
                       callable_ret_ok<F, R, A...>, std::false_type>::value;
} // namespace ghost_detail

template <class R, class... A> class GhostFn<R(A...)> {
public:
  GhostFn() = default;
  GhostFn(const GhostFn &) = default;
  GhostFn(GhostFn &&) = default;
  GhostFn &operator=(const GhostFn &) = default;
  GhostFn &operator=(GhostFn &&) = default;

  template <
      class F, class D = std::decay_t<F>,
      class = std::enable_if_t<!std::is_same_v<D, GhostFn> &&
                               !ghost_detail::is_ghost_fn<D>::value &&
                               ghost_detail::is_callable_for_fn_v<D, R, A...>>>
  GhostFn(F &&f) : fn_(std::forward<F>(f)) {}

  template <class R2, class... A2,
            class = std::enable_if_t<!std::is_same_v<R(A...), R2(A2...)> &&
                                     std::is_same_v<void(A...), void(A2...)> &&
                                     ghost_detail::fn_ret_conv<R2, R>::value>>
  GhostFn(const GhostFn<R2(A2...)> &o) {
    if (o.has_value()) {
      GhostFn<R2(A2...)> copy = o;
      fn_ = [copy](A... a) mutable -> R {
        if constexpr (std::is_void_v<R>) {
          copy(std::forward<A>(a)...);
        } else {
          return static_cast<R>(copy(std::forward<A>(a)...));
        }
      };
    }
  }

  bool has_value() const { return static_cast<bool>(fn_); }
  explicit operator bool() const { return static_cast<bool>(fn_); }

  R operator()(A... a) const {
    if (!fn_)
      throw GhostError(
          "fn sem funcao: guarde uma funcao ou lambda antes de chamar");
    return fn_(std::forward<A>(a)...);
  }

private:
  mutable std::function<R(A...)> fn_;
};

template <class Sig>
std::ostream &operator<<(std::ostream &os, const GhostFn<Sig> &) {
  return os << "<fn>";
}

template <class F> class GhostPolyFn {
public:
  GhostPolyFn(F f) : fn_(std::move(f)) {}
  GhostPolyFn(const GhostPolyFn &) = default;
  GhostPolyFn(GhostPolyFn &&) = default;
  GhostPolyFn &operator=(const GhostPolyFn &) = default;
  GhostPolyFn &operator=(GhostPolyFn &&) = default;

  bool has_value() const { return true; }
  explicit operator bool() const { return true; }

  template <class... A>
  auto operator()(A &&...a) const
      -> decltype(std::declval<F &>()(std::forward<A>(a)...)) {
    return fn_(std::forward<A>(a)...);
  }

private:
  mutable F fn_;
};

template <class F>
std::ostream &operator<<(std::ostream &os, const GhostPolyFn<F> &) {
  return os << "<fn>";
}

template <class F> GhostPolyFn<std::decay_t<F>> ghost_make_poly_fn(F &&f) {
  return GhostPolyFn<std::decay_t<F>>(std::forward<F>(f));
}

template <class... A, class F> auto ghost_make_fn(F &&f) {
  using R = std::invoke_result_t<std::decay_t<F> &, A...>;
  return GhostFn<R(A...)>(std::forward<F>(f));
}

template <class R, class... A> GhostFn<R(A...)> ghost_fn_val(R (*fp)(A...)) {
  return GhostFn<R(A...)>(fp);
}
template <class T, class = std::enable_if_t<!std::is_function_v<
                       std::remove_pointer_t<std::decay_t<T>>>>>
decltype(auto) ghost_fn_val(T &&v) {
  return std::forward<T>(v);
}

// ---------------------------------------------------------------- conversões
// str(x): texto de qualquer valor (números, bool, listas, mapas, result,
// objetos com <<)
template <class T> std::string str(const T &v) {
  std::ostringstream os;
  ghost_detail::repr(os, v, false);
  return os.str();
}
template <class T> std::string str(const GhostList<T> &v) {
  std::ostringstream os;
  os << v;
  return os.str();
}
template <class T> std::string str(const GhostSet<T> &v) {
  std::ostringstream os;
  os << v;
  return os.str();
}
template <class K, class V> std::string str(const GhostMap<K, V> &v) {
  std::ostringstream os;
  os << v;
  return os.str();
}

// ---------------------------------------------------------------- "texto" +
// coleção / result
template <class T>
std::string operator+(const GhostList<T> &v, const std::string &s) {
  std::ostringstream os;
  os << v << s;
  return os.str();
}
template <class T>
std::string operator+(const std::string &s, const GhostList<T> &v) {
  std::ostringstream os;
  os << s << v;
  return os.str();
}
template <class K, class V>
std::string operator+(const std::string &s, const GhostMap<K, V> &m) {
  std::ostringstream os;
  os << s << m;
  return os.str();
}
template <class K, class V>
std::string operator+(const GhostMap<K, V> &m, const std::string &s) {
  std::ostringstream os;
  os << m << s;
  return os.str();
}
template <class T>
std::string operator+(const std::string &s, const GhostSet<T> &v) {
  std::ostringstream os;
  os << s << v;
  return os.str();
}
template <class T>
std::string operator+(const GhostSet<T> &v, const std::string &s) {
  std::ostringstream os;
  os << v << s;
  return os.str();
}
template <class T>
std::string operator+(const std::string &s, const GhostResult<T> &r) {
  std::ostringstream os;
  os << s << r;
  return os.str();
}
template <class T>
std::string operator+(const GhostResult<T> &r, const std::string &s) {
  std::ostringstream os;
  os << r << s;
  return os.str();
}
template <class Sig>
std::string operator+(const std::string &s, const GhostFn<Sig> &f) {
  std::ostringstream os;
  os << s << f;
  return os.str();
}
template <class Sig>
std::string operator+(const GhostFn<Sig> &f, const std::string &s) {
  std::ostringstream os;
  os << f << s;
  return os.str();
}
template <class F>
std::string operator+(const std::string &s, const GhostPolyFn<F> &f) {
  std::ostringstream os;
  os << s << f;
  return os.str();
}
template <class F>
std::string operator+(const GhostPolyFn<F> &f, const std::string &s) {
  std::ostringstream os;
  os << f << s;
  return os.str();
}
inline std::string operator+(const std::string &s, bool b) {
  return s + (b ? "true" : "false");
}
inline std::string operator+(bool b, const std::string &s) {
  return (b ? "true" : "false") + s;
}

// ---------------------------------------------------------------- ghost_pow
// (potencia **) int ** int é inteiro (ghost_ipow, o mesmo do REPL): x ** 2 % 4
// compila e 2 ** 10 == 1024 exato. Com um lado real, é std::pow: 2.0 ** -1 ==
// 0.5.
template <typename A, typename B>
inline auto ghost_pow(const A &a, const B &b) {
  if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
    return ghost_ipow(static_cast<long long>(a), static_cast<long long>(b));
  } else {
    return std::pow(static_cast<double>(a), static_cast<double>(b));
  }
}

// ---------------------------------------------------------------- ghost_nth_root
inline double ghost_nth_root(double x, int n) {
  if (n <= 0) {
    throw std::invalid_argument("indice da raiz deve ser um numero natural maior que zero (n >= 1)");
  }
  if (n == 1) return x;
  if (n == 2) return std::sqrt(x);
  if (n == 3) return std::cbrt(x);
  if (x == 0.0) return 0.0;
  if (x < 0.0) {
    if (n % 2 != 0) {
      return -std::pow(-x, 1.0 / static_cast<double>(n));
    } else {
      throw std::domain_error("raiz de indice par com radicando negativo no corpo real: utilize o tipo complex nativo (ex.: csqrt, raiz_n_complexa ou raiz_n(complex))");
    }
  }
  return std::pow(x, 1.0 / static_cast<double>(n));
}

// ---------------------------------------------------------------- ghost_nth_root_complex
inline std::complex<double> ghost_nth_root_complex(const std::complex<double>& z, int n) {
  if (n <= 0) {
    throw std::invalid_argument("indice da raiz deve ser um numero natural maior que zero (n >= 1)");
  }
  if (n == 1) return z;
  if (n == 2) return std::sqrt(z);
  double r = std::abs(z);
  double theta = std::arg(z);
  double r_root = std::pow(r, 1.0 / static_cast<double>(n));
  double theta_root = theta / static_cast<double>(n);
  return std::polar(r_root, theta_root);
}

inline std::complex<double> ghost_nth_root_complex(double x, int n) {
  return ghost_nth_root_complex(std::complex<double>(x, 0.0), n);
}

inline std::complex<double> ghost_nth_root(const std::complex<double>& z, int n) {
  return ghost_nth_root_complex(z, n);
}

inline std::complex<double> ghost_csqrt(const std::complex<double>& z) {
  return std::sqrt(z);
}

inline std::complex<double> ghost_csqrt(double x) {
  return std::sqrt(std::complex<double>(x, 0.0));
}

// ---------------------------------------------------------------- ghost_bool
// (conversao truthy de bool(x))
inline bool ghost_bool(bool b) { return b; }
inline bool ghost_bool(int n) { return n != 0; }
inline bool ghost_bool(long long n) { return n != 0; }
inline bool ghost_bool(double d) { return d != 0.0; }
inline bool ghost_bool(float f) { return f != 0.0f; }
inline bool ghost_bool(const std::string &s) { return !s.empty(); }
inline bool ghost_bool(const char *s) { return s != nullptr && *s != '\0'; }

template <typename T> bool ghost_bool(const GhostList<T> &l) {
  return !l.empty();
}

template <typename K, typename V> bool ghost_bool(const GhostMap<K, V> &m) {
  return !m.empty();
}

template <typename T> bool ghost_bool(const GhostSet<T> &s) {
  return !s.empty();
}

template <typename T> bool ghost_bool(const GhostResult<T> &r) {
  return r.is_ok();
}

template <typename T>
auto ghost_bool(const T &val) -> decltype(static_cast<bool>(val)) {
  return static_cast<bool>(val);
}

#include "GhostAutograd.hpp"
#include "GhostDB.hpp"
#include "GhostDataset.hpp"
#include "GhostDensityMatrix.hpp"
#include "GhostGUI.hpp"
#include "GhostNDTensor.hpp"
#include "GhostQHardware.hpp"

using namespace GhostAI;
using namespace GhostData;
using namespace GhostQuantum;
using namespace GhostAlgebra;
