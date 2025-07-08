// Copyright GNU GPLv3 (c) 2023-2025 MoneroOcean <support@moneroocean.stream>

#pragma once

#include "async-worker.h"
#include "ctpl-stl.h" // used for randomx threads
#include "crypto/common/VirtualMemory.h"
#include "crypto/cn/CnHash.h"
#include "crypto/randomx/randomx.h"
#include "consts.h"

typedef void (*cn_any_hash_fun)();
typedef void (*cn_gpu_hash_fun)(
  const uint8_t* input, unsigned input_size, uint8_t* output,
  void* Lpads, void* Spads, unsigned batch, const std::string& dev_str
);
static_assert(
  sizeof(cn_any_hash_fun) == sizeof(xmrig::cn_hash_fun) &&
  sizeof(cn_any_hash_fun) == sizeof(cn_gpu_hash_fun),
  "Compute function pointers differ in size!"
);
union FN {
  cn_any_hash_fun any;
  xmrig::cn_hash_fun cpu;
  cn_gpu_hash_fun    gpu;
};
enum DEV { CPU, RX_CPU, GPU };

class Core: public AsyncWorker {
  const unsigned HASHRATE_COUNTER_INTERVAL = 10; // iterations to skip to update/check hashrate
  // store pointer to send messages back easier
  const AsyncProgressQueueWorker<char>::ExecutionProgress* m_progress;
  FN m_fn;
  DEV m_dev;
  xmrig::VirtualMemory *m_lpads, *m_rx_cache_mem, *m_rx_dataset_mem;
  void* m_spads;
  struct cryptonight_ctx** m_ctx;
  uint8_t *m_input, *m_output;
  unsigned m_job_ref, m_height, m_batch, m_mem_size, m_input_len, m_nonce_step, m_nonce_offset;
  uint32_t m_nonce; // next nonce that will be used in an input
  uint64_t m_target, m_timestamp, m_hash_count;
  std::string m_algo_str, m_dev_str, m_seed_hex, m_input_hex, m_pool_id, m_worker_id, m_job_id;
  bool m_is_rx_jit, m_is_nicehash;
  randomx_cache*   m_rx_cache;
  randomx_dataset* m_rx_dataset;
  ctpl::thread_pool* m_thread_pool;
  randomx_vm** m_vm;
  std::mutex m_mutex_hashrate;

  inline uint32_t* get_nonce(uint8_t* const input, const unsigned batch = 0) {
    return reinterpret_cast<uint32_t*>(input + (batch * m_input_len) + m_nonce_offset);
  }
  inline uint32_t* get_nonce(const unsigned batch) {
    return get_nonce(m_input, batch);
  }
  inline const uint64_t* get_result(const uint8_t* const output, const unsigned batch = 0) const {
    return reinterpret_cast<const uint64_t*>(output + (batch * HASH_LEN) + 24);
  }
  inline const uint64_t* get_result(const unsigned batch) const {
    return get_result(m_output, batch);
  }

  char* hash_bin2hex(const uint8_t* const output, char* hash, const unsigned batch = 0) const;
  char* hash_bin2hex(char* const hash, const unsigned batch) const;
  void send_msg(const std::string key, const MessageValues& values);
  void send_msg(
    const std::string& topic, const std::string& key = std::string(),
    const std::string& value = std::string()
  );
  void send_error(const std::string& str);
  void send_result(const uint32_t nonce, const uint8_t* const output);
  void send_last_nonce(const uint32_t nonce, const std::string& pool_id);
  void free_memory(
    const bool is_batch_changed    = true,
    const bool is_mem_size_changed = true,
    const bool is_free_cn          = true,
    const bool is_free_rx          = true
  );
  void set_fn(cn_any_hash_fun fn);
  void set_job(
    const bool is_set_nonce, const bool is_no_same_input, const MessageValues& v,
    std::function<void(void)> fn_extra_setup = [](){}
  );
  void get_algo_params(const MessageValues& v);
  bool process_message(const std::string& type, const MessageValues& v);

  static bool hex2bin(const char* in, unsigned int len, unsigned char* out);
  static std::vector<std::string> tokenize(const std::string& str, const char delim);

  public:

  Core(
    Nan::Callback* const data, Nan::Callback* const complete,
    Nan::Callback* const error_callback,  const v8::Local<v8::Object>& options
  ) : AsyncWorker(data, complete, error_callback), m_progress(nullptr),
      m_lpads(nullptr), m_rx_cache_mem(nullptr), m_rx_dataset_mem(nullptr),
      m_spads(nullptr), m_ctx(nullptr), m_input(nullptr), m_output(nullptr),
      m_job_ref(0), m_height(0), m_batch(0), m_mem_size(0), m_input_len(0),
      m_nonce_step(1), m_nonce_offset(39), m_nonce(0), m_target(0),
      m_timestamp(0), m_hash_count(0),
      m_is_rx_jit(true), m_is_nicehash(true), m_rx_cache(nullptr), m_rx_dataset(nullptr),
      m_thread_pool(nullptr), m_vm(nullptr)
  {
    m_fn.any = nullptr;
  }

  void Execute(const AsyncProgressQueueWorker<char>::ExecutionProgress& progress);
};
