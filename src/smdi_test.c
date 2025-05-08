/*
 * SMDI test application for IRIX 5.3
 * ANSI C90 compliant for MIPS big-endian architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "smdi.h"
#include "smdi_sample.h"
#include "scsi_debug.h"
#include <dmedia/audioutil.h>
#include <dmedia/audiofile.h>
#include "smdi_aif.h"

#define CMDLINE_SIZE 256
#define MAX_SAMPLES  128

/* External reference to global debug flag */
/* extern int g_debug_enabled; */
static int g_debug_enabled = 0;


/* Progress callback function */
void progress_callback(SMDI_FileTransmissionInfo* fti, DWORD userData) {
    SMDI_TransmissionInfo* ti;
    SMDI_SampleHeader* header;
    DWORD total_bytes;
    DWORD bytes_sent;
    int percent;

    if (fti == NULL || fti->lpTransmissionInfo == NULL) {
        return;
    }

    ti = fti->lpTransmissionInfo;
    
    if (ti->lpSampleHeader == NULL) {
        return;
    }
    
    header = ti->lpSampleHeader;

    /* Calculate total bytes */
    total_bytes = header->dwLength * header->NumberOfChannels * header->BitsPerWord / 8;

    /* Calculate bytes sent so far */
    bytes_sent = ti->dwTransmittedPackets * ti->dwPacketSize;

    /* Adjust for last packet */
    if (bytes_sent > total_bytes) {
        bytes_sent = total_bytes;
    }

    /* Calculate percentage */
    if (total_bytes > 0) {
        percent = (int)((bytes_sent * 100) / total_bytes);
    } else {
        percent = 0;
    }

    /* Print progress */
    printf("\rProgress: %d%% (%lu of %lu bytes)    ", 
           percent, bytes_sent, total_bytes);
    fflush(stdout);
}

/* Print usage information */
void print_usage(void) {
    printf("\n");
    printf("IRIX SMDI Test Shell Commands:\n");
    printf("------------------------------\n");
    printf("help                          - Display this help message\n");
    printf("scan <ha_id>                  - Scan for SMDI devices on host adapter\n");
    printf("list <ha_id> <id>             - List samples on device\n");
    printf("info <ha_id> <id> <sample_id> - Get sample info\n");
    printf("receive <ha_id> <id> <sample_id> <file> - Download sample to file\n");
    printf("send <ha_id> <id> <file> <sample_id>    - Upload file to device\n");
    printf("delete <ha_id> <id> <sample_id>         - Delete sample from device\n");
    printf("debug [on|off]                - Enable/disable debug output\n");
    /* AIF support additions */
    printf("loadaif <file.aif> <sample_id> <ha_id> <id> - Load AIF and send to device\n");
    printf("saveaif <ha_id> <id> <sample_id> <file.aif> - Receive sample and save as AIF\n");
    printf("quit                          - Exit the program\n");
    printf("\n");
}

/* Command: Check if SMDI is available */
void cmd_init(void) {
    unsigned char result;
    
    result = SMDI_Init();
    
    if (result) {
        printf("SMDI is available\n");
    } else {
        printf("SMDI is not available\n");
    }
}

/* Command: Scan for SMDI devices */
void cmd_scan(unsigned char ha_id) {
    int i;
    SCSI_DevInfo dev_info;
    int found;
    int old_debug = g_debug_enabled;
    
    /* This is a debugging function - temporarily enable debugging */
    g_debug_enabled = 1;
    
    printf("Scanning host adapter %d for SMDI devices...\n", ha_id);
    printf("ID | Type       | Vendor   | Product        | SMDI\n");
    printf("---|------------|----------|----------------|------\n");
    
    found = 0;
    
    for (i = 0; i < 16; i++) {
        /* Skip LUN 0 of target 7 (typically host adapter) */
        if (i == 7) {
            continue;
        }
        
        /* Test if unit is ready */
        if (SMDI_TestUnitReady(ha_id, i)) {
            /* Get device info */
            memset(&dev_info, 0, sizeof(SCSI_DevInfo));
            dev_info.dwStructSize = sizeof(SCSI_DevInfo);
            
            SMDI_GetDeviceInfo(ha_id, i, &dev_info);
            
            printf("%2d | ", i);
            
            /* Print device type */
            switch (dev_info.DevType & 0x1F) {
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
            
            /* Print vendor, product */
            printf("| %.8s | %.16s | ", 
                  dev_info.cManufacturer, dev_info.cName);
            
            /* Print SMDI status */
            if (dev_info.bSMDI) {
                printf("Yes\n");
            } else {
                printf("No\n");
            }
            
            found++;
        }
    }
    
    if (found == 0) {
        printf("No devices found on host adapter %d\n", ha_id);
    } else {
        printf("%d device(s) found on host adapter %d\n", found, ha_id);
    }
    
    /* Restore debug state */
    g_debug_enabled = old_debug;
}

/* Command: List samples on device */
void cmd_list(unsigned char ha_id, unsigned char id) {
    SMDI_SampleHeader sh;
    int i;
    int found;
    
    printf("Listing samples on device %d:%d...\n", ha_id, id);
    printf("ID  | Name                           | Rate     | Length   | Bits | Ch\n");
    printf("----|--------------------------------|----------|----------|------|----\n");
    
    found = 0;
    
    for (i = 0; i < MAX_SAMPLES; i++) {
        /* Initialize sample header */
        memset(&sh, 0, sizeof(SMDI_SampleHeader));
        sh.dwStructSize = sizeof(SMDI_SampleHeader);
        
        /* Request sample header */
        if (SMDI_SampleHeaderRequest(ha_id, id, i, &sh) == SMDIM_SAMPLEHEADER && sh.bDoesExist) {
            /* Format sample properties */
            printf("%3d | %-30s | %8d | %8d | %4d | %2d\n",
                   i,
                   sh.cName,
                   (int)(1000000000 / sh.dwPeriod),  /* Convert period to rate */
                   (int)sh.dwLength,
                   (int)sh.BitsPerWord,
                   (int)sh.NumberOfChannels);
            
            found++;
        }
    }
    
    if (found == 0) {
        printf("No samples found on device %d:%d\n", ha_id, id);
    } else {
        printf("%d sample(s) found on device %d:%d\n", found, ha_id, id);
    }
}

/* Command: Get sample info */
void cmd_info(unsigned char ha_id, unsigned char id, unsigned long sample_id) {
    SMDI_SampleHeader sh;
    
    /* Initialize sample header */
    memset(&sh, 0, sizeof(SMDI_SampleHeader));
    sh.dwStructSize = sizeof(SMDI_SampleHeader);
    
    /* Request sample header */
    if (SMDI_SampleHeaderRequest(ha_id, id, sample_id, &sh) != SMDIM_SAMPLEHEADER || !sh.bDoesExist) {
        printf("Sample %lu not found on device %d:%d\n", sample_id, ha_id, id);
        return;
    }
    
    /* Print sample info */
    printf("Sample Information for ID %lu:\n", sample_id);
    printf("Name:            %s\n", sh.cName);
    printf("Sample Rate:     %d Hz\n", (int)(1000000000 / sh.dwPeriod));
    printf("Sample Length:   %lu samples\n", sh.dwLength);
    printf("Channels:        %d\n", (int)sh.NumberOfChannels);
    printf("Bits per Sample: %d\n", (int)sh.BitsPerWord);
    printf("Root Note:       %d\n", (int)sh.wPitch);
    printf("Fine Tune:       %d cents\n", (int)sh.wPitchFraction);
    
    /* Print loop info */
    printf("Loop Type:       ");
    switch (sh.LoopControl) {
        case 0:  printf("None\n"); break;
        case 1:  printf("Forward\n"); break;
        case 2:  printf("Bidirectional\n"); break;
        default: printf("Unknown (%d)\n", (int)sh.LoopControl); break;
    }
    
    if (sh.LoopControl != 0) {
        printf("Loop Start:      %lu\n", sh.dwLoopStart);
        printf("Loop End:        %lu\n", sh.dwLoopEnd);
    }
    
    /* Calculate size */
    printf("Data Size:       %lu bytes\n", 
           (sh.dwLength * sh.NumberOfChannels * sh.BitsPerWord) / 8);
}

/* Command: Download sample to file */
void cmd_receive(unsigned char ha_id, unsigned char id, 
                unsigned long sample_id, const char* filename) {
    SMDI_FileTransfer ft;
    DWORD result;
    
    printf("Downloading sample %lu from device %d:%d to file '%s'...\n", 
           sample_id, ha_id, id, filename);
    
    /* Initialize file transfer structure */
    memset(&ft, 0, sizeof(SMDI_FileTransfer));
    ft.dwStructSize = sizeof(SMDI_FileTransfer);
    ft.HA_ID = ha_id;
    ft.SCSI_ID = id;
    ft.dwSampleNumber = sample_id;
    ft.lpFileName = (char*)filename;
    ft.dwFileType = SF_NATIVE;  /* Use our native format */
    ft.lpCallback = (void*)progress_callback;
    ft.dwUserData = 0;
    ft.bAsync = FALSE;
    ft.lpReturnValue = &result;
    
    /* Perform the download */
    result = SMDI_ReceiveFile(&ft);
    
    printf("\n");
    
    if (result == SMDIM_ENDOFPROCEDURE) {
        printf("Sample downloaded successfully.\n");
    } else {
        printf("Failed to download sample. Error code: 0x%08lX\n", result);
    }
}

/* Command: Upload file to device */
void cmd_send(unsigned char ha_id, unsigned char id, 
             const char* filename, unsigned long sample_id) {
    SMDI_FileTransfer ft;
    DWORD result;
    char sample_name[256];
    const char* basename;
    char* dot;
    
    printf("Uploading file '%s' to device %d:%d as sample %lu...\n", 
           filename, ha_id, id, sample_id);
    
    /* Extract basename from filename for sample name */
    basename = strrchr(filename, '/');
    if (basename) {
        basename++; /* Skip the slash */
    } else {
        basename = filename;
    }
    
    /* Copy basename and remove extension */
    strncpy(sample_name, basename, 255);
    sample_name[255] = '\0';
    dot = strrchr(sample_name, '.');
    if (dot) {
        *dot = '\0';
    }
    
    /* Initialize file transfer structure */
    memset(&ft, 0, sizeof(SMDI_FileTransfer));
    ft.dwStructSize = sizeof(SMDI_FileTransfer);
    ft.HA_ID = ha_id;
    ft.SCSI_ID = id;
    ft.dwSampleNumber = sample_id;
    ft.lpFileName = (char*)filename;
    ft.lpSampleName = sample_name;
    ft.lpCallback = (void*)progress_callback;
    ft.dwUserData = 0;
    ft.bAsync = FALSE;
    ft.lpReturnValue = &result;
    
    /* Perform the upload */
    result = SMDI_SendFile(&ft);
    
    printf("\n");
    
    if (result == SMDIM_ENDOFPROCEDURE || result == SMDIM_ACK) {
        printf("Sample uploaded successfully.\n");
    } else {
        printf("Failed to upload sample. Error code: 0x%08lX\n", result);
    }
}

/* Command: Delete sample from device */
void cmd_delete(unsigned char ha_id, unsigned char id, unsigned long sample_id) {
    DWORD result;
    DWORD error_code;
    SMDI_SampleHeader sh;
    
    printf("Deleting sample %lu from device %d:%d...\n", sample_id, ha_id, id);
    
    /* First verify the sample exists */
    memset(&sh, 0, sizeof(SMDI_SampleHeader));
    sh.dwStructSize = sizeof(SMDI_SampleHeader);
    
    if (SMDI_SampleHeaderRequest(ha_id, id, sample_id, &sh) != SMDIM_SAMPLEHEADER || !sh.bDoesExist) {
        printf("Sample %lu not found on device %d:%d\n", sample_id, ha_id, id);
        return;
    }
    
    /* Perform the deletion */
    result = SMDI_DeleteSample(ha_id, id, sample_id);
    
    /* Handle the result */
    if (result == SMDIM_ACK || result == SMDIM_ENDOFPROCEDURE) {
        printf("Sample deleted successfully.\n");
    } 
    /* Check for specific error codes */
    else if (result == SMDIM_MESSAGEREJECT) {
        /* Get the last error */
        error_code = SMDI_GetLastError();
        
        /* Provide specific error message based on the error code */
        switch (error_code) {
            case SMDIE_OUTOFRANGE:
                printf("Delete failed: Sample ID out of range.\n");
                break;
                
            case SMDIE_NOSAMPLE:
                printf("Delete failed: Sample %lu does not exist on the device.\n", sample_id);
                
                /* Double-check if the sample is really gone */
                memset(&sh, 0, sizeof(SMDI_SampleHeader));
                sh.dwStructSize = sizeof(SMDI_SampleHeader);
                
                if (SMDI_SampleHeaderRequest(ha_id, id, sample_id, &sh) != SMDIM_SAMPLEHEADER || !sh.bDoesExist) {
                    printf("Sample is confirmed to no longer exist (possibly deleted successfully).\n");
                }
                break;
                
            case SMDIE_NOMEMORY:
                printf("Delete failed: Device has insufficient memory for this operation.\n");
                break;
                
            case SMDIE_UNSUPPSAMBITS:
                printf("Delete failed: Unsupported sample bits format.\n");
                break;
                
            default:
                printf("Delete failed with error code: 0x%08lX\n", error_code);
                break;
        }
        
        /* Check if the sample exists after the delete attempt anyway */
        memset(&sh, 0, sizeof(SMDI_SampleHeader));
        sh.dwStructSize = sizeof(SMDI_SampleHeader);
        
        if (SMDI_SampleHeaderRequest(ha_id, id, sample_id, &sh) != SMDIM_SAMPLEHEADER || !sh.bDoesExist) {
            printf("Note: Sample appears to have been deleted despite the error message.\n");
        }
    } else {
        printf("Failed to delete sample. Response: 0x%08lX\n", result);
    }
}

/* Command: Load AIF file and send to device */
void cmd_loadaif(const char* aif_filename, unsigned long sample_id, 
               unsigned char ha_id, unsigned char id) {
    SMDI_Sample* sample;
    SMDI_FileTransfer ft;
    DWORD result;
    char temp_filename[MAX_PATH];
    
    printf("Loading AIF file '%s' and sending to device %d:%d as sample %lu...\n", 
           aif_filename, ha_id, id, sample_id);
    
    /* Load AIF file */
    sample = SMDI_LoadAIFSample(aif_filename);
    if (sample == NULL) {
        printf("Failed to load AIF file '%s'.\n", aif_filename);
        return;
    }
    
    printf("AIF file loaded successfully:\n");
    printf("  Name: %s\n", sample->name);
    printf("  Rate: %lu Hz, Bits: %d, Channels: %d\n", 
           sample->sample_rate, sample->bits_per_sample, sample->channels);
    printf("  Samples: %lu\n", sample->sample_count);
    
    /* Create temporary file in native format */
    sprintf(temp_filename, "/tmp/smdi_temp_%lu.sdmp", sample_id);
    if (!SMDI_SaveSample(sample, temp_filename)) {
        printf("Failed to create temporary file.\n");
        SMDI_FreeSample(sample);
        return;
    }
    
    /* Set up file transfer */
    memset(&ft, 0, sizeof(SMDI_FileTransfer));
    ft.dwStructSize = sizeof(SMDI_FileTransfer);
    ft.HA_ID = ha_id;
    ft.SCSI_ID = id;
    ft.dwSampleNumber = sample_id;
    ft.lpFileName = temp_filename;
    ft.lpSampleName = sample->name;
    ft.lpCallback = (void*)progress_callback;
    ft.dwUserData = 0;
    ft.bAsync = FALSE;
    ft.lpReturnValue = &result;
    
    /* Send the file */
    result = SMDI_SendFile(&ft);
    
    printf("\n");
    
    if (result == SMDIM_ENDOFPROCEDURE || result == SMDIM_ACK) {
        printf("Sample uploaded successfully.\n");
    } else {
        printf("Failed to upload sample. Error code: 0x%08lX\n", result);
    }
    
    /* Clean up */
    SMDI_FreeSample(sample);
    unlink(temp_filename);  /* Remove temporary file */
}

/* Command: Receive sample from device and save as AIF */
void cmd_saveaif(unsigned char ha_id, unsigned char id, 
                unsigned long sample_id, const char* aif_filename) {
    SMDI_Sample* sample;
    SMDI_FileTransfer ft;
    DWORD result;
    char temp_filename[MAX_PATH];
    int is_aifc = 0;
    const char* ext_ptr;
    
    /* Determine if we should use AIFF-C format based on file extension */
    ext_ptr = strstr(aif_filename, ".aifc");
    if (ext_ptr == NULL) {
        ext_ptr = strstr(aif_filename, ".AIFC");
    }
    
    if (ext_ptr != NULL) {
        is_aifc = 1;
    }
    
    printf("Downloading sample %lu from device %d:%d and saving as %s...\n", 
           sample_id, ha_id, id, aif_filename);
    
    /* Create a temporary file to receive sample */
    sprintf(temp_filename, "/tmp/smdi_temp_%lu.sdmp", sample_id);
    
    /* Set up file transfer */
    memset(&ft, 0, sizeof(SMDI_FileTransfer));
    ft.dwStructSize = sizeof(SMDI_FileTransfer);
    ft.HA_ID = ha_id;
    ft.SCSI_ID = id;
    ft.dwSampleNumber = sample_id;
    ft.lpFileName = temp_filename;
    ft.dwFileType = SF_NATIVE;
    ft.lpCallback = (void*)progress_callback;
    ft.dwUserData = 0;
    ft.bAsync = FALSE;
    ft.lpReturnValue = &result;
    
    /* Perform the download */
    result = SMDI_ReceiveFile(&ft);
    
    printf("\n");
    
    if (result != SMDIM_ENDOFPROCEDURE) {
        printf("Failed to download sample. Error code: 0x%08lX\n", result);
        return;
    }
    
    /* Load the temporary sample file */
    sample = SMDI_LoadSample(temp_filename);
    if (sample == NULL) {
        printf("Failed to load downloaded sample.\n");
        unlink(temp_filename);
        return;
    }
    
    /* Save as AIF file */
    if (!SMDI_SaveAIFSample(sample, aif_filename, is_aifc)) {
        printf("Failed to save as AIF file.\n");
        SMDI_FreeSample(sample);
        unlink(temp_filename);
        return;
    }
    
    printf("Sample saved as %s successfully.\n", aif_filename);
    printf("  Name: %s\n", sample->name);
    printf("  Rate: %lu Hz, Bits: %d, Channels: %d\n", 
           sample->sample_rate, sample->bits_per_sample, sample->channels);
    printf("  Samples: %lu\n", sample->sample_count);
    
    /* Clean up */
    SMDI_FreeSample(sample);
    unlink(temp_filename);
}

/* Main function */
int main(int argc, char *argv[]) {
    char cmdline[CMDLINE_SIZE];
    char cmd[CMDLINE_SIZE];
    char arg1[CMDLINE_SIZE];
    char arg2[CMDLINE_SIZE];
    char arg3[CMDLINE_SIZE];
    char arg4[CMDLINE_SIZE];
    int args;
    int debug_enabled = 0;
    
    printf("IRIX SMDI Test Shell\n");
    printf("===================\n");
    printf("Type 'help' for a list of commands\n");
    
    /* Initialize SMDI */
    cmd_init();
    
    while (1) {
        /* Display prompt and get command */
        printf("\nsmdi> ");
        fflush(stdout);
        
        if (fgets(cmdline, CMDLINE_SIZE, stdin) == NULL) {
            break;
        }
        
        /* Parse command line */
        cmd[0] = '\0';
        arg1[0] = '\0';
        arg2[0] = '\0';
        arg3[0] = '\0';
        arg4[0] = '\0';
        
        args = sscanf(cmdline, "%s %s %s %s %s", cmd, arg1, arg2, arg3, arg4);
        
        if (args <= 0 || cmd[0] == '\0') {
            continue;  /* Empty command line */
        }
        
        /* Process command */
        if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
            print_usage();
        }
        else if (strcmp(cmd, "scan") == 0) {
            if (args < 2) {
                printf("Usage: scan <ha_id>\n");
            } else {
                cmd_scan((unsigned char)atoi(arg1));
            }
        }
        else if (strcmp(cmd, "list") == 0) {
            if (args < 3) {
                printf("Usage: list <ha_id> <id>\n");
            } else {
                cmd_list((unsigned char)atoi(arg1), (unsigned char)atoi(arg2));
            }
        }
        else if (strcmp(cmd, "info") == 0) {
            if (args < 4) {
                printf("Usage: info <ha_id> <id> <sample_id>\n");
            } else {
                cmd_info((unsigned char)atoi(arg1), (unsigned char)atoi(arg2), (unsigned long)atol(arg3));
            }
        }
        else if (strcmp(cmd, "receive") == 0) {
            if (args < 5) {
                printf("Usage: receive <ha_id> <id> <sample_id> <file>\n");
            } else {
                cmd_receive((unsigned char)atoi(arg1), (unsigned char)atoi(arg2), 
                           (unsigned long)atol(arg3), arg4);
            }
        }
        else if (strcmp(cmd, "send") == 0) {
            if (args < 5) {
                printf("Usage: send <ha_id> <id> <file> <sample_id>\n");
            } else {
                cmd_send((unsigned char)atoi(arg1), (unsigned char)atoi(arg2), 
                        arg3, (unsigned long)atol(arg4));
            }
        }
        else if (strcmp(cmd, "delete") == 0) {
            if (args < 4) {
                printf("Usage: delete <ha_id> <id> <sample_id>\n");
            } else {
                cmd_delete((unsigned char)atoi(arg1), (unsigned char)atoi(arg2), 
                          (unsigned long)atol(arg3));
            }
        }
        else if (strcmp(cmd, "debug") == 0) {
            if (args < 2 || strcmp(arg1, "on") == 0) {
                g_debug_enabled = 1;
                SMDI_SetDebugMode(1);  /* Set SMDI debug mode */
                printf("Debug output enabled\n");
            }
            else if (strcmp(arg1, "off") == 0) {
                g_debug_enabled = 0;
                SMDI_SetDebugMode(0);  /* Clear SMDI debug mode */
                printf("Debug output disabled\n");
            }
            else {
                printf("Usage: debug [on|off]\n");
            }
        }
        else if (strcmp(cmd, "loadaif") == 0) {
            if (args < 5) {
                printf("Usage: loadaif <file.aif> <sample_id> <ha_id> <id>\n");
            } else {
                cmd_loadaif(arg1, (unsigned long)atol(arg2), 
                          (unsigned char)atoi(arg3), (unsigned char)atoi(arg4));
            }
        }
        else if (strcmp(cmd, "saveaif") == 0) {
            if (args < 5) {
                printf("Usage: saveaif <ha_id> <id> <sample_id> <file.aif>\n");
            } else {
                cmd_saveaif((unsigned char)atoi(arg1), (unsigned char)atoi(arg2),
                           (unsigned long)atol(arg3), arg4);
            }
        }
        else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        }
        else {
            printf("Unknown command: %s\nType 'help' for a list of commands\n", cmd);
        }
    }
    
    printf("\nExiting SMDI Test Shell\n");
    return 0;
}
