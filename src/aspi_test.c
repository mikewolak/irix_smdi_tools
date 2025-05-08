/*
 * Interactive test shell for IRIX ASPI implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "aspi_test.h"

/* Global debug structure */
static scsi_debug_t g_debug;

/* Parse hexadecimal string into byte array */
int parse_hex_data(const char *hex_str, unsigned char *data, int max_len)
{
    int i;
    int len;
    char hex_byte[3];
    
    if (hex_str == NULL || data == NULL || max_len <= 0)
    {
        return 0;
    }
    
    /* Remove spaces from hex string */
    len = 0;
    hex_byte[2] = '\0';
    
    for (i = 0; hex_str[i] != '\0' && len < max_len; )
    {
        /* Skip spaces and other non-hex characters */
        if (!isxdigit(hex_str[i]))
        {
            i++;
            continue;
        }
        
        /* Need at least two hex digits */
        if (!isxdigit(hex_str[i]) || !isxdigit(hex_str[i+1]))
        {
            break;
        }
        
        /* Convert hex byte */
        hex_byte[0] = hex_str[i];
        hex_byte[1] = hex_str[i+1];
        data[len++] = (unsigned char)strtol(hex_byte, NULL, 16);
        i += 2;
    }
    
    return len;
}

/* Print usage information */
void print_usage(void)
{
    printf("\n");
    printf("IRIX ASPI Test Shell Commands:\n");
    printf("------------------------------\n");
    printf("help                  - Display this help message\n");
    printf("check                 - Check if ASPI is available\n");
    printf("scan <ha_id>          - Scan for devices on host adapter\n");
    printf("ready <ha_id> <id>    - Test if unit is ready\n");
    printf("inquire <ha_id> <id>  - Get device information\n");
    printf("send <ha_id> <id> <hex_data> - Send data to device\n");
    printf("receive <ha_id> <id> <size>  - Receive data from device\n");
    printf("debug [on|off]        - Enable/disable debug output\n");
    printf("logfile <filename>    - Set debug log file\n");
    printf("dumpfile <filename> <ha_id> <id> <size> - Dump data from device to file\n");
    printf("sendfile <filename> <ha_id> <id> - Send file data to device\n");
    printf("quit                  - Exit the program\n");
    printf("\n");
}

/* Command: Check if ASPI is available */
void cmd_check(scsi_debug_t *debug)
{
    unsigned char result;
    
    result = ASPI_Check(debug);
    
    if (result)
    {
        printf("ASPI is available\n");
    }
    else
    {
        printf("ASPI is not available\n");
    }
}

/* Command: Scan for devices on a host adapter */
void cmd_scan(scsi_debug_t *debug, unsigned char ha_id)
{
    int i;
    unsigned char devtype;
    char inq_data[96];
    int found;
    
    printf("Scanning host adapter %d for devices...\n", ha_id);
    printf("ID | Type       | Vendor   | Product        | Revision\n");
    printf("---|------------|----------|----------------|----------\n");
    
    found = 0;
    
    for (i = 0; i < 16; i++)
    {
        /* Skip LUN 0 of target 7 (typically host adapter) */
        if (i == 7)
        {
            continue;
        }
        
        /* Get device type */
        devtype = ASPI_GetDevType(debug, ha_id, i);
        
        /* If valid device type and device responds to inquiry */
        if (devtype != 0xFF)
        {
            ASPI_InquireDevice(debug, inq_data, ha_id, i);
            printf("%2d | ", i);
            
            /* Print device type */
            switch (devtype & 0x1F)
            {
                case 0x00: printf("Disk      "); break;
                case 0x01: printf("Tape      "); break;
                case 0x02: printf("Printer   "); break;
                case 0x03: printf("Processor "); break;
                case 0x04: printf("WORM      "); break;
                case 0x05: printf("CD-ROM    "); break;
                case 0x06: printf("Scanner   "); break;
                case 0x07: printf("Optical   "); break;
                case 0x08: printf("Changer   "); break;
                case 0x09: printf("Comm      "); break;
                default:   printf("Unknown   "); break;
            }
            
            /* Print vendor, product, revision from inquiry data */
            printf("| %.8s | %.16s | %.4s\n", 
                  &inq_data[8], &inq_data[16], &inq_data[32]);
            
            found++;
        }
    }
    
    if (found == 0)
    {
        printf("No devices found on host adapter %d\n", ha_id);
    }
    else
    {
        printf("%d device(s) found on host adapter %d\n", found, ha_id);
    }
}

/* Command: Test if unit is ready */
void cmd_ready(scsi_debug_t *debug, unsigned char ha_id, unsigned char id)
{
    int ready;
    
    ready = ASPI_TestUnitReady(debug, ha_id, id);
    
    if (ready)
    {
        printf("Device %d:%d is ready\n", ha_id, id);
    }
    else
    {
        printf("Device %d:%d is NOT ready\n", ha_id, id);
    }
}

/* Command: Get device information */
void cmd_inquire(scsi_debug_t *debug, unsigned char ha_id, unsigned char id)
{
    char inq_data[96];
    int i;
    
    memset(inq_data, 0, sizeof(inq_data));
    ASPI_InquireDevice(debug, inq_data, ha_id, id);
    
    printf("Inquiry data for device %d:%d\n", ha_id, id);
    printf("Device type: ");
    switch (inq_data[0] & 0x1F)
    {
        case 0x00: printf("Disk\n"); break;
        case 0x01: printf("Tape\n"); break;
        case 0x02: printf("Printer\n"); break;
        case 0x03: printf("Processor\n"); break;
        case 0x04: printf("WORM\n"); break;
        case 0x05: printf("CD-ROM\n"); break;
        case 0x06: printf("Scanner\n"); break;
        case 0x07: printf("Optical\n"); break;
        case 0x08: printf("Changer\n"); break;
        case 0x09: printf("Communications\n"); break;
        default:   printf("Unknown (0x%02X)\n", inq_data[0] & 0x1F); break;
    }
    
    printf("Vendor: %.8s\n", &inq_data[8]);
    printf("Product: %.16s\n", &inq_data[16]);
    printf("Revision: %.4s\n", &inq_data[32]);
    
    printf("Raw data:\n");
    for (i = 0; i < 36; i++)
    {
        printf("%02X ", (unsigned char)inq_data[i]);
        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

/* Command: Send data to device */
void cmd_send(scsi_debug_t *debug, unsigned char ha_id, unsigned char id, const char *hex_data)
{
    unsigned char data[DATA_BUFFER_SIZE];
    int data_len;
    int success;
    
    /* Parse hex data */
    data_len = parse_hex_data(hex_data, data, DATA_BUFFER_SIZE);
    
    if (data_len == 0)
    {
        printf("Error: Invalid hex data\n");
        return;
    }
    
    printf("Sending %d bytes to device %d:%d...\n", data_len, ha_id, id);
    
    success = ASPI_Send(debug, ha_id, id, data, data_len);
    
    if (success)
    {
        printf("Data sent successfully\n");
    }
    else
    {
        printf("Failed to send data\n");
    }
}

/* Command: Receive data from device */
void cmd_receive(scsi_debug_t *debug, unsigned char ha_id, unsigned char id, unsigned long size)
{
    unsigned char data[DATA_BUFFER_SIZE];
    unsigned long received;
    int i;
    
    if (size > DATA_BUFFER_SIZE)
    {
        printf("Error: Size exceeds buffer limit (%d bytes)\n", DATA_BUFFER_SIZE);
        return;
    }
    
    printf("Receiving %lu bytes from device %d:%d...\n", size, ha_id, id);
    
    received = ASPI_Receive(debug, ha_id, id, data, size);
    
    if (received > 0)
    {
        printf("Received %lu bytes:\n", received);
        
        /* Print received data in hex format */
        for (i = 0; i < received; i++)
        {
            printf("%02X ", data[i]);
            if ((i + 1) % 16 == 0)
            {
                printf("\n");
            }
        }
        if (received % 16 != 0)
        {
            printf("\n");
        }
    }
    else
    {
        printf("Failed to receive data\n");
    }
}

/* Command: Dump data from device to file */
void cmd_dumpfile(scsi_debug_t *debug, const char *filename, unsigned char ha_id, unsigned char id, unsigned long size)
{
    unsigned char *data;
    unsigned long received;
    FILE *fp;
    
    /* Allocate buffer */
    data = (unsigned char *)malloc(size);
    if (data == NULL)
    {
        printf("Error: Failed to allocate memory for %lu bytes\n", size);
        return;
    }
    
    printf("Receiving %lu bytes from device %d:%d...\n", size, ha_id, id);
    
    received = ASPI_Receive(debug, ha_id, id, data, size);
    
    if (received > 0)
    {
        printf("Received %lu bytes\n", received);
        
        /* Write data to file */
        fp = fopen(filename, "wb");
        if (fp != NULL)
        {
            if (fwrite(data, 1, received, fp) == received)
            {
                printf("Data written to file '%s'\n", filename);
            }
            else
            {
                printf("Error writing to file '%s'\n", filename);
            }
            fclose(fp);
        }
        else
        {
            printf("Error opening file '%s' for writing\n", filename);
        }
    }
    else
    {
        printf("Failed to receive data\n");
    }
    
    free(data);
}

/* Command: Send file data to device */
void cmd_sendfile(scsi_debug_t *debug, const char *filename, unsigned char ha_id, unsigned char id)
{
    unsigned char *data;
    unsigned long file_size;
    unsigned long bytes_read;
    int success;
    FILE *fp;
    
    /* Open file */
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Error opening file '%s' for reading\n", filename);
        return;
    }
    
    /* Get file size */
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    /* Allocate buffer */
    data = (unsigned char *)malloc(file_size);
    if (data == NULL)
    {
        printf("Error: Failed to allocate memory for %lu bytes\n", file_size);
        fclose(fp);
        return;
    }
    
    /* Read file */
    bytes_read = fread(data, 1, file_size, fp);
    fclose(fp);
    
    if (bytes_read != file_size)
    {
        printf("Error: Read %lu bytes, expected %lu bytes\n", bytes_read, file_size);
        free(data);
        return;
    }
    
    printf("Sending %lu bytes from file '%s' to device %d:%d...\n", 
           file_size, filename, ha_id, id);
    
    success = ASPI_Send(debug, ha_id, id, data, file_size);
    
    if (success)
    {
        printf("Data sent successfully\n");
    }
    else
    {
        printf("Failed to send data\n");
    }
    
    free(data);
}

/* Main function */
int main(int argc, char *argv[])
{
    char cmdline[CMDLINE_SIZE];
    char cmd[CMDLINE_SIZE];
    char arg1[CMDLINE_SIZE];
    char arg2[CMDLINE_SIZE];
    char arg3[CMDLINE_SIZE];
    char arg4[CMDLINE_SIZE];
    int args;
    
    /* Initialize debug structure */
    scsi_debug_init(&g_debug);
    
    printf("IRIX ASPI Test Shell\n");
    printf("===================\n");
    printf("Type 'help' for a list of commands\n");
    
    while (1)
    {
        /* Display prompt and get command */
        printf("\naspi> ");
        fflush(stdout);
        
        if (fgets(cmdline, CMDLINE_SIZE, stdin) == NULL)
        {
            break;
        }
        
        /* Parse command line */
        cmd[0] = '\0';
        arg1[0] = '\0';
        arg2[0] = '\0';
        arg3[0] = '\0';
        arg4[0] = '\0';
        
        args = sscanf(cmdline, "%s %s %s %s %s", cmd, arg1, arg2, arg3, arg4);
        
        if (args <= 0 || cmd[0] == '\0')
        {
            continue;  /* Empty command line */
        }
        
        /* Process command */
        if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0)
        {
            print_usage();
        }
        else if (strcmp(cmd, "check") == 0)
        {
            cmd_check(&g_debug);
        }
        else if (strcmp(cmd, "scan") == 0)
        {
            if (args < 2)
            {
                printf("Usage: scan <ha_id>\n");
            }
            else
            {
                cmd_scan(&g_debug, (unsigned char)atoi(arg1));
            }
        }
        else if (strcmp(cmd, "ready") == 0)
        {
            if (args < 3)
            {
                printf("Usage: ready <ha_id> <id>\n");
            }
            else
            {
                cmd_ready(&g_debug, (unsigned char)atoi(arg1), (unsigned char)atoi(arg2));
            }
        }
        else if (strcmp(cmd, "inquire") == 0)
        {
            if (args < 3)
            {
                printf("Usage: inquire <ha_id> <id>\n");
            }
            else
            {
                cmd_inquire(&g_debug, (unsigned char)atoi(arg1), (unsigned char)atoi(arg2));
            }
        }
        else if (strcmp(cmd, "send") == 0)
        {
            if (args < 4)
            {
                printf("Usage: send <ha_id> <id> <hex_data>\n");
            }
            else
            {
                cmd_send(&g_debug, (unsigned char)atoi(arg1), (unsigned char)atoi(arg2), arg3);
            }
        }
        else if (strcmp(cmd, "receive") == 0)
        {
            if (args < 4)
            {
                printf("Usage: receive <ha_id> <id> <size>\n");
            }
            else
            {
                cmd_receive(&g_debug, (unsigned char)atoi(arg1), (unsigned char)atoi(arg2), 
                         (unsigned long)atol(arg3));
            }
        }
        else if (strcmp(cmd, "debug") == 0)
        {
            if (args < 2 || strcmp(arg1, "on") == 0)
            {
                g_debug.enabled = 1;
                printf("Debug output enabled\n");
            }
            else if (strcmp(arg1, "off") == 0)
            {
                g_debug.enabled = 0;
                printf("Debug output disabled\n");
            }
            else
            {
                printf("Usage: debug [on|off]\n");
            }
        }
        else if (strcmp(cmd, "logfile") == 0)
        {
            if (args < 2)
            {
                printf("Usage: logfile <filename>\n");
            }
            else
            {
                if (scsi_debug_set_logfile(&g_debug, arg1))
                {
                    printf("Debug log file set to '%s'\n", arg1);
                }
                else
                {
                    printf("Failed to set debug log file to '%s'\n", arg1);
                }
            }
        }
        else if (strcmp(cmd, "dumpfile") == 0)
        {
            if (args < 5)
            {
                printf("Usage: dumpfile <filename> <ha_id> <id> <size>\n");
            }
            else
            {
                cmd_dumpfile(&g_debug, arg1, (unsigned char)atoi(arg2), 
                          (unsigned char)atoi(arg3), (unsigned long)atol(arg4));
            }
        }
        else if (strcmp(cmd, "sendfile") == 0)
        {
            if (args < 4)
            {
                printf("Usage: sendfile <filename> <ha_id> <id>\n");
            }
            else
            {
                cmd_sendfile(&g_debug, arg1, (unsigned char)atoi(arg2), (unsigned char)atoi(arg3));
            }
        }
        else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0)
        {
            break;
        }
        else
        {
            printf("Unknown command: %s\nType 'help' for a list of commands\n", cmd);
        }
    }
    
    printf("\nExiting ASPI Test Shell\n");
    return 0;
}
