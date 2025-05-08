/*
 * Header file for IRIX ASPI test shell
 */

#ifndef __ASPI_TEST_H__
#define __ASPI_TEST_H__

#include "aspi_irix.h"
#include "scsi_debug.h"

/* Buffer size for command line inputs */
#define CMDLINE_SIZE 256
/* Data buffer size for SCSI transfers */
#define DATA_BUFFER_SIZE 8192

/* Parse hexadecimal string into byte array */
int parse_hex_data(const char *hex_str, unsigned char *data, int max_len);

/* Print usage information */
void print_usage(void);

/* Command handlers */
void cmd_check(scsi_debug_t *debug);
void cmd_scan(scsi_debug_t *debug, unsigned char ha_id);
void cmd_ready(scsi_debug_t *debug, unsigned char ha_id, unsigned char id);
void cmd_inquire(scsi_debug_t *debug, unsigned char ha_id, unsigned char id);
void cmd_send(scsi_debug_t *debug, unsigned char ha_id, unsigned char id, const char *hex_data);
void cmd_receive(scsi_debug_t *debug, unsigned char ha_id, unsigned char id, unsigned long size);
void cmd_dumpfile(scsi_debug_t *debug, const char *filename, unsigned char ha_id, unsigned char id, unsigned long size);
void cmd_sendfile(scsi_debug_t *debug, const char *filename, unsigned char ha_id, unsigned char id);

#endif /* __ASPI_TEST_H__ */
