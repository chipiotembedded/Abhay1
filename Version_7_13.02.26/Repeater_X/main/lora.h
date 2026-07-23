#ifndef _LORA_H_
#define _LORA_H_

int  lora_init(void);
void lora_set_frequency(long frequency);
void lora_set_tx_power(int level);
void lora_set_sync_word(int sw);
void lora_enable_crc(void);
void lora_disable_crc(void);
void lora_explicit_header_mode(void);
void lora_implicit_header_mode(int size);
int lora_receive(void);
int  lora_receive_packet(uint8_t *buf, int size);
void lora_send_packet(uint8_t *buf, int size);
int  lora_received(void);
int  lora_packet_rssi(void);
float lora_packet_snr(void);
void lora_close(void);
void lora_set_spreading_factor(int sf);
void lora_set_bandwidth(long sbw);
void lora_set_coding_rate(int denominator);

#endif