#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace GhostQuantum {

struct QHardwareJob {
  std::string job_id;
  std::string backend_name;
  std::string status; // "QUEUED", "RUNNING", "COMPLETED", "FAILED"
  std::string qasm_circuit;
  int shots = 1024;
  std::map<std::string, int> counts;
};

class GhostQHardware {
public:
  static GhostQHardware &get() {
    static GhostQHardware instance;
    return instance;
  }

  // Provedor e backend só dão nome ao job: ainda não há cliente HTTP para
  // IBM Quantum, AWS Braket ou IonQ, e o token não é usado.
  void configure_provider(const std::string &provider,
                          const std::string &api_token,
                          const std::string &default_backend) {
    m_provider = provider;
    m_token = api_token;
    m_backend = default_backend;
  }

  const std::string &provider() const { return m_provider; }
  const std::string &backend() const { return m_backend; }

  // Execução de um circuito OpenQASM 2.0/3.0. NÃO há envio para hardware real
  // (IBM Quantum, IonQ, Braket): o circuito roda no simulador de vetor de estado
  // local (GhostQReg), com semente fixa por job, e o resumo diz isso.
  // Implementado em src/GhostQHardware.cpp.
  std::string submit_qasm(const std::string &qasm_code,
                          const std::string &backend_override = "",
                          int shots = 1024);

  std::string job_status(const std::string &job_id) {
    auto it = m_jobs.find(job_id);
    if (it != m_jobs.end())
      return it->second.status;
    return "NOT_FOUND";
  }

  int get_count(const std::string &job_id, const std::string &bitstring) {
    auto it = m_jobs.find(job_id);
    if (it != m_jobs.end()) {
      auto c_it = it->second.counts.find(bitstring);
      if (c_it != it->second.counts.end())
        return c_it->second;
    }
    return 0;
  }

  std::string result_summary(const std::string &job_id) {
    auto it = m_jobs.find(job_id);
    if (it == m_jobs.end())
      return "Job not found";
    std::ostringstream ss;
    ss << "SIMULACAO LOCAL (nada enviado a " << it->second.backend_name
       << ") shots=" << it->second.shots << " {";
    bool first = true;
    for (const auto &kv : it->second.counts) {
      if (!first)
        ss << ", ";
      ss << "\"" << kv.first << "\": " << kv.second;
      first = false;
    }
    ss << "}";
    return ss.str();
  }

private:
  GhostQHardware() : m_provider("ibm"), m_backend("ibm_brisbane") {}
  std::string m_provider;
  std::string m_token;
  std::string m_backend;
  int m_job_counter = 1000;
  std::map<std::string, QHardwareJob> m_jobs;
};

} // namespace GhostQuantum

// Sobrecargas C++ para compilador GhostLang e REPL (suporta std::string e const char*)
inline void ghost_qhw_configure(const std::string &provider, const std::string &api_token,
                                const std::string &backend) {
  GhostQuantum::GhostQHardware::get().configure_provider(provider, api_token, backend);
}
inline void ghost_qhw_configure(const char *provider, const char *api_token,
                                const char *backend) {
  GhostQuantum::GhostQHardware::get().configure_provider(
      provider ? provider : "ibm", api_token ? api_token : "",
      backend ? backend : "ibm_brisbane");
}

inline std::string ghost_qhw_submit(const std::string &qasm, const std::string &backend, int shots) {
  return GhostQuantum::GhostQHardware::get().submit_qasm(qasm, backend, shots);
}
inline const char *ghost_qhw_submit(const char *qasm, const char *backend, int shots) {
  static thread_local std::string buf;
  buf = GhostQuantum::GhostQHardware::get().submit_qasm(
      qasm ? qasm : "", backend ? backend : "", shots);
  return buf.c_str();
}

inline std::string ghost_qhw_status(const std::string &job_id) {
  return GhostQuantum::GhostQHardware::get().job_status(job_id);
}
inline const char *ghost_qhw_status(const char *job_id) {
  static thread_local std::string buf;
  buf = GhostQuantum::GhostQHardware::get().job_status(job_id ? job_id : "");
  return buf.c_str();
}

inline int ghost_qhw_get_count(const std::string &job_id, const std::string &bitstring) {
  return GhostQuantum::GhostQHardware::get().get_count(job_id, bitstring);
}
inline int ghost_qhw_get_count(const char *job_id, const char *bitstring) {
  return GhostQuantum::GhostQHardware::get().get_count(
      job_id ? job_id : "", bitstring ? bitstring : "");
}

inline std::string ghost_qhw_summary(const std::string &job_id) {
  return GhostQuantum::GhostQHardware::get().result_summary(job_id);
}
inline const char *ghost_qhw_summary(const char *job_id) {
  static thread_local std::string buf;
  buf = GhostQuantum::GhostQHardware::get().result_summary(job_id ? job_id : "");
  return buf.c_str();
}
