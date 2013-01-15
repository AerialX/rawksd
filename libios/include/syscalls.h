#pragma once

#include "gctypes.h"
#include "ipc.h"

/* misc IOS info that has nowhere else to live
 *
 * maximum number of threads: 61
 * maximum number of queues: 256
 * maximum number of timers: 256
 * maximum number of resources (device names): 32
 * maximum number of heaps:   16
 * PIDs:
 *  0 - kernel (/dev/aes, /dev/sha)
 *  1 - ES/eTicket (/dev/es)
 *  2 - FS (/dev/flash, /dev/boot2, /)
 *  3 - DI (/dev/di)
 *  4 - OH0 (/dev/usb/oh0)
 *  5 - OH1 (/dev/usb/oh1)
 *  6 - EHCI (/dev/usb/ehc?)
 *  7 - SDI (/dev/sdio)
 *  8 - USBETH (/dev/net/usbeth/top)
 *  9 - NET (/dev/net/ip/top, /dev/net/ip/bottom)
 * 10 - WD (/dev/listen, /dev/net/wd/command)
 * 11 - WL (/dev/wl)
 * 12 - KD (/dev/net/kd/request, /dev/net/kd/time)
 * 13 - NCD (/dev/net/ncd/manage, /dev/net/wd/top)
 * 14 - STM (/dev/stm/immediate, /dev/stm/eventhook)
 * 15 - PPCBOOT (manages the powerpc, has extra allotted FDs)
 * 16 - SSL (/dev/net/ssl)
 * 17 - USB (/dev/usb/hid, maybe more in later IOSes)
 * 18 - P2P (?)
 * 19 - WFS (?)
 */

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

/*  0 */ int       os_thread_create(u32 (*entry)(void* _arg), void* arg, void* stack_top, u32 stacksize, u32 priority, u32 detached);
/*  1    int       os_thread_join(int thread_id, int *return_code);  srsly, don't use this. */
/*  2    os_thread_cancel */
/*  3 */ int       os_get_thread_id(void);
/*  4 */ int       os_get_process_id(void);
/*  5 */ int       os_thread_continue(int id);
/*  6 */ int       os_thread_stop(int id);
/*  7 */ void      os_thread_yield();
/*  8 */ int       os_thread_get_priority(int thread);
/*  9 */ int       os_thread_set_priority(int thread, u32 priority);
/*  A */ osqueue_t os_message_queue_create(void* ptr, u32 n_msgs);
/*  B */ s32       os_message_queue_destroy(osqueue_t queue);
/*  C */ s32       os_message_queue_send(osqueue_t queue, u32 message, u32 flags);
/*  D */ s32       os_message_queue_send_now(osqueue_t queue, u32 message, u32 flags);
/*  E */ s32       os_message_queue_receive(osqueue_t queue, u32* message, u32 flags);
/*  F    os_register_event_handler(int device, osqueue_t queue, u32 message) */
/* 10    os_unregister_event_handler */
/* 11 */ ostimer_t os_create_timer(s32 time_us, s32 repeat_time_us, osqueue_t message_queue, u32 message);
/* 12 */ s32       os_restart_timer(ostimer_t timer_id, s32 time_us, s32 repeat_time_us);
/* 13 */ s32       os_stop_timer(ostimer_t timer_id);
/* 14 */ s32       os_destroy_timer(ostimer_t time_id);
/* 15 */ u32       os_time_now(); // just returns HW_TIMER?
/* 16 */ u32       os_heap_create(void* ptr, u32 size);
/* 17 */ u32       os_heap_destroy(u32 heap);
/* 18 */ void*     os_heap_alloc(u32 heap, u32 size);
/* 19 */ void*     os_heap_alloc_aligned(int heap, int size, int align);
/* 1A */ void      os_heap_free(u32 heap, void* ptr);
/* 1B */ u32       os_device_register(const char* devicename, osqueue_t queuehandle);
/* 1C */ s32       os_open(const char* device, s32 mode);
/* 1D */ s32       os_close(s32 fd);
/* 1E */ s32       os_read(s32 fd, void* buffer, s32 length);
/* 1F */ s32       os_write(s32 fd, const void* buffer, s32 length);
/* 20 */ s32       os_seek(s32 fd, s32 where, s32 whence);
/* 21 */ s32       os_ioctl(s32 fd, s32 request, const void* buffer_in, s32 bytes_in, void* buffer_io, s32 bytes_io);
/* 22 */ s32       os_ioctlv(s32 fd, s32 request, s32 count_in, s32 count_out, const ioctlv* vector);
/* 23 */ s32       os_open_async(const char* device, s32 mode, osqueue_t cb, ipcmessage* cb_data);
/* 24 */ s32       os_close_async(s32 fd, osqueue_t cb, ipcmessage* cb_data);
/* 25 */ s32       os_read_async(s32 fd, void* buffer, s32 length, osqueue_t cb, ipcmessage* cb_data);
/* 26 */ s32       os_write_async(s32 fd, const void* buffer, s32 length, osqueue_t cb, ipcmessage* cb_data);
/* 27 */ s32       os_seek_async(s32 fd, s32 where, s32 whence, osqueue_t cb, ipcmessage *cb_data);
/* 28 */ s32       os_ioctl_async(s32 fd, s32 request, const void* buffer_in, s32 bytes_in, void* buffer_io, s32 bytes_io, osqueue_t cb, ipcmessage* cb_data);
/* 29 */ s32       os_ioctlv_async(s32 fd, s32 request, s32 count_in, s32 count_out, const ioctlv* vector, osqueue_t cb, ipcmessage* cb_data);
/* 2A */ void      os_message_queue_ack(const ipcmessage* message, s32 result);
/* 2B    os_set_uid(u32 pid, u32 uid) */
/* 2C    os_get_uid */
/* 2D    os_set_gid */
/* 2E    os_get_gid */
/* 2F    os_cc_ahbMemFlush(u32 device) */
/* 30    os_ahbMemFlush(u32 device) */
/* 31    os_software_IRQ_31 */
/* 32    os_software_IRQ_18 */
/* 33    os_software_IRQ_7_or_8(int which) */
/* 34    os_software_IRQ(int device) */
/* 35    os_access_iobuf_pool (empty?) */
/* 36    os_alloc_iobuf */
/* 37    os_free_iobuf */
/* 38    os_iobuf_log_header_info */
/* 39    os_iobuf_log_buffer_info */
/* 3A    os_extend_iobuf */
/* 3B    os_push_iobuf */
/* 3C    os_pull_iobuf */
/* 3D    os_verify_iobuf */
/* 3E    os_syscall_3E ?? */
/* 3F */ void      os_sync_before_read(const void* ptr, u32 size);
/* 40 */ void      os_sync_after_write(const void* ptr, u32 size);
/* 41    os_ppc_boot(const char* path) */
/* 42    os_ios_boot(const char* path, u32 flag, u32 version) */
/* 43    os_syscall_43 ?? */
/* 44    os_di_reset_assert */
/* 45    os_di_reset_deassert */
/* 46    os_di_reset_check */
/* 47    os_get_some_flags(u32 *r0, u16 *r1) */
/* 48    os_get_other_flags (might be empty) */
/* 49    os_get_boot_vector */
/* 4A    os_get_hollywood_revision */
/* 4B */ void      os_debug_print(u32 flags);
/* 4C    os_set_kernel_version(u32 version) */
/* 4D    u32 os_get_kernel_version() */
/* 4E    os_do_gpio_something */
/* 4F    void* os_virt_to_phys(void* ptr) */
/* 50 */ void      os_set_dvd_video(u32 unknown);
/* 51    os_check_some_di_flag */
/* 52    os_EXI_something */
/* 53    os_syscall_53 */
/* 54    os_set_ahbprot(int enable) */
/* 55    os_get_bc_flag */
/* 56 */ int       os_poke_gpios(u32 reg, u32 value);
/* 57    os_syscall_57 */
/* 58    os_set_debug_gpios */
/* 59    os_load_PPC */
/* 5A    os_load_module(const char* path) */
 // enum usage {crypto, sig, ecc_pair, info}, enum type {aes, hmac, cert_2048, cert_4096, ecc, console_id, seeprom_counter}
/* 5B */ int       os_create_key(int *keyid_out, u32 usage, u32 type);
/* 5C */ int       os_destroy_key(int key_id);
/* 5D */ int       os_init_key(int key_id, u32 zero, u32 decrypt_key_id, u32 one, u32 zero2, void *iv, const void *cipher_title_key);
/* 5E    os_syscall_5E */
/* 5F */ int       os_set_sig_info(const u8 *data, /*optional*/const int *tag_data, int keyid); // data length should match key type
/* 60 */ int       os_get_sig_info(u8 *data, /*optional*/int *tag_data, int keyid);
/* 61 */ int       os_calc_ecdh_shared(int self_key_id, int sender_public_key_id, int dest_shared_key_id);
/* 62 */ int       os_set_4byte_key(int keyid, const u32 *data);
/* 63 */ int       os_get_4byte_key(int keyid, u32* buffer);
/* 64 */ int       os_get_key_size(int keyid, int *size);
/* 65 */ int       os_get_key_userdata_size(int keyid, int *size);
/* 66    os_sha1_async */
/* 67    os_sha1(void *SHACarry, void *data, u32 len, u32 SHAMode, void *hash) */
/* 68    os_aes_encrypt_async */
/* 69 */ int       os_aes_encrypt(int keyid, void *iv, const void *in, int len, void *out);
/* 6A    os_aes_decrypt_async */
/* 6B */ int       os_aes_decrypt(int keyid, void *iv, const void *in, int len, void *out);
/* 6C    os_check_sig(const void *hash, int hash_length, int keyid, const void *sig) */
/* 6D    os_hmac(void *out_96bytes, const void* in, int size_in, (optional)void* in_4?bytes, , int keyid, , void *out_20bytes) */
/* 6E    os_hmac_async(,,,,,,,,,) */
/* 6F    os_validate_cert(void *cert, int sig_keyid_a, int sig_keyid_dest) */
/* 70    os_get_device_cert(void *cert) 0x180 bytes */
/* 71    int os_set_key_permissions(int keyid, u32 mask_in) */
/* 72    int os_get_key_permissions(int keyid, u32 *mask_out) */
/* 73    os_generate_random_data(void *data, int data_size) */
/* 74    int os_generate_random_key(int dest_key_id) */
/* 75    os_make_ecc_sig(void *SHAhash, u32 hash_length, int keyid, void *ecc_sig) */
/* 76    os_syscall_76(int ecc_keyid, void *in_64bytes, void *out_180bytes) */
/* 77 */ void os_crash(void); // syscall 0x77 doesn't exist but tries to execute anyway

void os_puts(const char *str); // IOS log

#ifdef __cplusplus
   }
#endif
