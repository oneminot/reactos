#ifndef __TCP_HELPER_H__
#define __TCP_HELPER_H__

#include "../lwip_check.h"
#include "lwip/arch.h"
#include "lwip/tcp.h"

/* counters used for test_tcp_counters_* callback functions */
struct test_tcp_counters {
  u32_t recv_calls;
  u32_t recved_bytes;
  u32_t recv_calls_after_close;
  u32_t recved_bytes_after_close;
  u32_t close_calls;
  u32_t err_calls;
  err_t last_err;
  char* expected_data;
  u32_t expected_data_len;
};

/* Helper functions */
void tcp_remove_all(void);

struct pbuf* tcp_create_segment(struct ip_addr* src_ip, struct ip_addr* dst_ip,
                   u16_t src_port, u16_t dst_port, void* data, size_t data_len,
                   u32_t seqno, u32_t ackno, u8_t headerflags);
struct pbuf* tcp_create_rx_segment(struct tcp_pcb* pcb, void* data, size_t data_len,
                   u32_t seqno_offset, u32_t ackno_offset, u8_t headerflags);
void tcp_set_state(struct tcp_pcb* pcb, enum tcp_state state, struct ip_addr* local_ip,
                   struct ip_addr* remote_ip, u16_t local_port, u16_t remote_port);
void test_tcp_counters_err(void* arg, err_t err);
err_t test_tcp_counters_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err);

struct tcp_pcb* test_tcp_new_counters_pcb(struct test_tcp_counters* counters);

#endif
