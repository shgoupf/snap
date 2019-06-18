/*
 * Copyright 2019 International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <ctype.h>

#include "pg_capi_internal.h"
#include <snap_tools.h>
#include <snap_s_regs.h>

#include "fregex.h"

/*  defaults */
#define STEP_DELAY      200
#define DEFAULT_MEMCPY_BLOCK    4096
#define DEFAULT_MEMCPY_ITER 1
#define ACTION_WAIT_TIME    10   /* Default in sec */
//#define MAX_NUM_PKT 502400
//#define MAX_NUM_PKT 4096
#define MIN_NUM_PKT 4096
#define MAX_NUM_PATT 1024

#define MEGAB       (1024*1024ull)
#define GIGAB       (1024 * MEGAB)

uint32_t PATTERN_ID = 0;
uint32_t PACKET_ID = 0;
int verbose_level = 0;

void print_error (const char* file, const char* func, const char* line, int rc)
{
    printf ("ERROR: %s %s failed in line %s with return code %d\n", file, func, line, rc);
}

int64_t diff_time (struct timespec* t_beg, struct timespec* t_end)
{
    if (t_end == NULL || t_beg == NULL) {
        return 0;
    }

    return ((t_end-> tv_sec - t_beg-> tv_sec) * 1000000000L + t_end-> tv_nsec - t_beg-> tv_nsec);
}

uint64_t get_usec (void)
{
    struct timeval t;

    gettimeofday (&t, NULL);
    return t.tv_sec * 1000000 + t.tv_usec;
}

void print_time (uint64_t elapsed, uint64_t size)
{
    int t;
    float fsize = (float)size / (1024 * 1024);
    float ft;

    if (elapsed > 10000) {
        t = (int)elapsed / 1000;
        ft = (1000 / (float)t) * fsize;
        elog (DEBUG1, " end after %d msec (%0.3f MB/sec)\n", t, ft);
    } else {
        t = (int)elapsed;
        ft = (1000000 / (float)t) * fsize;
        elog (DEBUG1, " end after %d usec (%0.3f MB/sec)\n", t, ft);
    }
}

void print_time_text (const char* text, uint64_t elapsed, uint64_t size)
{
    int t;
    float fsize = (float)size / (1024 * 1024);
    float ft;

    if (elapsed > 10000) {
        t = (int)elapsed / 1000;
        ft = (1000 / (float)t) * fsize;
        elog (DEBUG1, "%s run time: %d msec (%0.3f MB/sec)\n", text, t, ft);
    } else {
        t = (int)elapsed;
        ft = (1000000 / (float)t) * fsize;
        elog (DEBUG1, "%s run time:  %d usec (%0.3f MB/sec)\n", text, t, ft);
    }
}

float perf_calc (uint64_t elapsed, uint64_t size)
{
    int t;
    float fsize = (float)size / (1024 * 1024);
    float ft;

    t = (int)elapsed / 1000;

    if (t == 0) {
        return 0.0;
    }

    ft = (1000 / (float)t) * fsize;
    return ft;
}


void* alloc_mem (int align, size_t size)
{
    void* a;
    size_t size2 = size + align;

    elog (DEBUG1, "%s Enter Align: %d Size: %zu\n", __func__, align, size);

    if (posix_memalign ((void**)&a, 4096, size2) != 0) {
        perror ("FAILED: posix_memalign()");
        return NULL;
    }

    elog (DEBUG1, "%s Exit %p\n", __func__, a);
    return a;
}

void free_mem (void* a)
{
    elog (DEBUG1, "Free Mem %p\n", a);

    if (a) {
        free (a);
    }
}

void* fill_one_packet (const char* in_pkt, int size, void* in_pkt_addr)
{
    unsigned char* pkt_base_addr = in_pkt_addr;
    int pkt_id;
    uint32_t bytes_used = 0;
    uint16_t pkt_len = size;

    PACKET_ID++;
    // The TAG ID
    pkt_id = PACKET_ID;

    elog (DEBUG2, "PKT[%d] %s len %d\n", pkt_id, in_pkt, pkt_len);

    // The frame header
    for (int i = 0; i < 4; i++) {
        pkt_base_addr[bytes_used] = 0x5A;
        bytes_used ++;
    }

    // The frame size
    pkt_base_addr[bytes_used] = (pkt_len & 0xFF);
    bytes_used ++;
    pkt_base_addr[bytes_used] = 0;
    pkt_base_addr[bytes_used] |= ((pkt_len >> 8) & 0xF);
    bytes_used ++;

    // Skip the reserved bytes
    //for (int i = 0; i < 54; i++) {
    //    pkt_base_addr[bytes_used] = 0;
    //    bytes_used++;
    //}
    memset (pkt_base_addr + bytes_used, 0, 54);
    bytes_used += 54;

    for (int i = 0; i < 4 ; i++) {
        pkt_base_addr[bytes_used] = ((pkt_id >> (8 * i)) & 0xFF);
        bytes_used++;
    }

    // The payload
    //for (int i = 0; i < pkt_len; i++) {
    //    pkt_base_addr[bytes_used] = in_pkt[i];
    //    bytes_used++;
    //}
    memcpy (pkt_base_addr + bytes_used, in_pkt, pkt_len);
    bytes_used += pkt_len;

    // Padding to 64 bytes alignment
    bytes_used--;

    do {
        if ((((uint64_t) (pkt_base_addr + bytes_used)) & 0x3F) == 0x3F) { //the last address of the packet stream is 512bit/64byte aligned
            break;
        } else {
            bytes_used ++;
            pkt_base_addr[bytes_used] = 0x00; //padding 8'h00 until the 512bit/64byte alignment
        }

    } while (1);

    bytes_used++;

    return pkt_base_addr + bytes_used;
}

void* fill_one_pattern (const char* in_patt, void* in_patt_addr)
{
    unsigned char* patt_base_addr = in_patt_addr;
    int config_len = 0;
    unsigned char config_bytes[PATTERN_WIDTH_BYTES];
    int x;
    uint32_t pattern_id;
    uint16_t patt_byte_cnt;
    uint32_t bytes_used = 0;

    for (x = 0; x < PATTERN_WIDTH_BYTES; x++) {
        config_bytes[x] = 0;
    }

    // Generate pattern ID
    PATTERN_ID ++;
    pattern_id = PATTERN_ID;

    elog (DEBUG1, "PATT[%d] %s\n", pattern_id, in_patt);

    fregex_get_config (in_patt,
                       MAX_TOKEN_NUM,
                       MAX_STATE_NUM,
                       MAX_CHAR_NUM,
                       MAX_CHAR_PER_TOKEN,
                       config_bytes,
                       &config_len,
                       0);

    elog (DEBUG2, "Config length (bits)  %d\n", config_len * 8);
    elog (DEBUG2, "Config length (bytes) %d\n", config_len);

    for (int i = 0; i < 4; i++) {
        patt_base_addr[bytes_used] = 0x5A;
        bytes_used++;
    }

    patt_byte_cnt = (PATTERN_WIDTH_BYTES - 4);
    patt_base_addr[bytes_used] = patt_byte_cnt & 0xFF;
    bytes_used ++;
    patt_base_addr[bytes_used] = (patt_byte_cnt >> 8) & 0x7;
    bytes_used ++;

    //for (int i = 0; i < 54; i++) {
    //    patt_base_addr[bytes_used] = 0x00;
    //    bytes_used ++;
    //}

    memset (patt_base_addr + bytes_used, 0, 54);
    bytes_used += 54;

    // Pattern ID;
    for (int i = 0; i < 4; i++) {
        patt_base_addr[bytes_used] = (pattern_id >> (i * 8)) & 0xFF;
        bytes_used ++;
    }

    memcpy (patt_base_addr + bytes_used, config_bytes, config_len);
    bytes_used += config_len;
    //for (int i = 0; i < config_len; i++) {
    //    patt_base_addr[bytes_used] = config_bytes[i];
    //    bytes_used ++;
    //}

    // Padding to 64 bytes alignment
    bytes_used --;

    do {
        if ((((uint64_t) (patt_base_addr + bytes_used)) & 0x3F) == 0x3F) { //the last address of the packet stream is 512bit/64byte aligned
            break;
        } else {
            bytes_used ++;
            patt_base_addr[bytes_used] = 0x00; //padding 8'h00 until the 512bit/64byte alignment
        }

    } while (1);

    bytes_used ++;

    return patt_base_addr + bytes_used;
}

/* Action or Kernel Write and Read are 32 bit MMIO */
void action_write (struct snap_card* h, uint32_t addr, uint32_t data)
{
    int rc;

    rc = snap_mmio_write32 (h, (uint64_t)addr, data);

    if (0 != rc) {
        elog (DEBUG1, "Write MMIO 32 Err\n");
    }

    return;
}

uint32_t action_read (struct snap_card* h, uint32_t addr)
{
    int rc;
    uint32_t data;

    rc = snap_mmio_read32 (h, (uint64_t)addr, &data);

    if (0 != rc) {
        elog (DEBUG1, "Read MMIO 32 Err\n");
    }

    return data;
}

/*
 *  Start Action and wait for Idle.
 */
int action_wait_idle (struct snap_card* h, int timeout, uint64_t* elapsed)
{
    int rc = ETIME;
    uint64_t t_start;   /* time in usec */
    uint64_t td = 0;    /* Diff time in usec */

    /* FIXME Use struct snap_action and not struct snap_card */
    snap_action_start ((void*)h);

    /* Wait for Action to go back to Idle */
    t_start = get_usec();
    rc = snap_action_completed ((void*)h, NULL, timeout);
    td = get_usec() - t_start;

    if (rc) {
        rc = 0;    /* Good */
    } else {
        elog (DEBUG1, "Error. Timeout while Waiting for Idle\n");
    }

    *elapsed = td;
    return rc;
}

void print_control_status (struct snap_card* h)
{
    if (verbose_level > 2) {
        uint32_t reg_data;
        elog (DEBUG3, " READ Control and Status Registers: \n");
        reg_data = action_read (h, ACTION_STATUS_L);
        elog (DEBUG3, "       STATUS_L = 0x%x\n", reg_data);
        reg_data = action_read (h, ACTION_STATUS_H);
        elog (DEBUG3, "       STATUS_H = 0x%x\n", reg_data);
        reg_data = action_read (h, ACTION_CONTROL_L);
        elog (DEBUG3, "       CONTROL_L = 0x%x\n", reg_data);
        reg_data = action_read (h, ACTION_CONTROL_H);
        elog (DEBUG3, "       CONTROL_H = 0x%x\n", reg_data);
    }
}

void soft_reset (struct snap_card* h)
{
    // Status[4] to reset
    action_write (h, ACTION_CONTROL_L, 0x00000010);
    action_write (h, ACTION_CONTROL_H, 0x00000000);
    elog (DEBUG2, " Write ACTION_CONTROL for soft reset! \n");
    action_write (h, ACTION_CONTROL_L, 0x00000000);
    action_write (h, ACTION_CONTROL_H, 0x00000000);
}

void action_regex (struct snap_card* h,
                   void* patt_src_base,
                   void* pkt_src_base,
                   void* stat_dest_base,
                   size_t* num_matched_pkt,
                   size_t patt_size,
                   size_t pkt_size,
                   size_t stat_size)
{
    uint32_t reg_data;

    elog (DEBUG2, " ------ String Match Start -------- \n");
    elog (DEBUG2, " PATTERN SOURCE ADDR: %p -- SIZE: %d\n", patt_src_base, (int)patt_size);
    elog (DEBUG2, " PACKET  SOURCE ADDR: %p -- SIZE: %d\n", pkt_src_base, (int)pkt_size);
    elog (DEBUG2, " STAT    DEST   ADDR: %p -- SIZE(max): %d\n", stat_dest_base, (int)stat_size);

    elog (DEBUG2, " Start register config! \n");
    print_control_status (h);

    action_write (h, ACTION_PATT_INIT_ADDR_L,
                  (uint32_t) (((uint64_t) patt_src_base) & 0xffffffff));
    action_write (h, ACTION_PATT_INIT_ADDR_H,
                  (uint32_t) ((((uint64_t) patt_src_base) >> 32) & 0xffffffff));
    elog (DEBUG2, " Write ACTION_PATT_INIT_ADDR done! \n");

    action_write (h, ACTION_PKT_INIT_ADDR_L,
                  (uint32_t) (((uint64_t) pkt_src_base) & 0xffffffff));
    action_write (h, ACTION_PKT_INIT_ADDR_H,
                  (uint32_t) ((((uint64_t) pkt_src_base) >> 32) & 0xffffffff));
    elog (DEBUG2, " Write ACTION_PKT_INIT_ADDR done! \n");

    action_write (h, ACTION_PATT_CARD_DDR_ADDR_L, 0);
    action_write (h, ACTION_PATT_CARD_DDR_ADDR_H, 0);
    elog (DEBUG2, " Write ACTION_PATT_CARD_DDR_ADDR done! \n");

    action_write (h, ACTION_STAT_INIT_ADDR_L,
                  (uint32_t) (((uint64_t) stat_dest_base) & 0xffffffff));
    action_write (h, ACTION_STAT_INIT_ADDR_H,
                  (uint32_t) ((((uint64_t) stat_dest_base) >> 32) & 0xffffffff));
    elog (DEBUG2, " Write ACTION_STAT_INIT_ADDR done! \n");

    action_write (h, ACTION_PATT_TOTAL_NUM_L,
                  (uint32_t) (((uint64_t) patt_size) & 0xffffffff));
    action_write (h, ACTION_PATT_TOTAL_NUM_H,
                  (uint32_t) ((((uint64_t) patt_size) >> 32) & 0xffffffff));
    elog (DEBUG2, " Write ACTION_PATT_TOTAL_NUM done! \n");

    action_write (h, ACTION_PKT_TOTAL_NUM_L,
                  (uint32_t) (((uint64_t) pkt_size) & 0xffffffff));
    action_write (h, ACTION_PKT_TOTAL_NUM_H,
                  (uint32_t) ((((uint64_t) pkt_size) >> 32) & 0xffffffff));
    elog (DEBUG2, " Write ACTION_PKT_TOTAL_NUM done! \n");

    action_write (h, ACTION_STAT_TOTAL_SIZE_L,
                  (uint32_t) (((uint64_t) stat_size) & 0xffffffff));
    action_write (h, ACTION_STAT_TOTAL_SIZE_H,
                  (uint32_t) ((((uint64_t) stat_size) >> 32) & 0xffffffff));
    elog (DEBUG2, " Write ACTION_STAT_TOTAL_SIZE done! \n");

    // Start copying the pattern from host memory to card
    action_write (h, ACTION_CONTROL_L, 0x00000001);
    action_write (h, ACTION_CONTROL_H, 0x00000000);
    elog (DEBUG2, " Write ACTION_CONTROL for pattern copying! \n");

    print_control_status (h);

    do {
        reg_data = action_read (h, ACTION_STATUS_L);
        elog (DEBUG3, "Pattern Phase: polling Status reg with 0X%X\n", reg_data);

        // Status[23:8]
        if ((reg_data & 0x00FFFF00) != 0) {
            elog (DEBUG1, "Error code got 0X%X\n", ((reg_data & 0x00FFFF00) >> 8));
            exit (EXIT_FAILURE);
        }

        // Status[0]
        if ((reg_data & 0x00000001) == 1) {
            elog (DEBUG1, "Pattern copy done!\n");
            break;
        }
    } while (1);

    // Start working control[2:1] = 11
    action_write (h, ACTION_CONTROL_L, 0x00000006);
    action_write (h, ACTION_CONTROL_H, 0x00000000);
    elog (DEBUG1, " Write ACTION_CONTROL for working! \n");

    do {
        reg_data = action_read (h, ACTION_STATUS_L);
        elog (DEBUG1, "Packet Phase: polling Status reg with 0X%X\n", reg_data);

        // Status[23:8]
        if ((reg_data & 0x00FFFF00) != 0) {
            elog (DEBUG1, "Error code got 0X%X\n", ((reg_data & 0x00FFFF00) >> 8));
            exit (EXIT_FAILURE);
        }

        // Status[0]
        if ((reg_data & 0x00000010) != 0) {
            elog (DEBUG1, "Memory space for stat used up!\n");
            exit (EXIT_FAILURE);
        }

        if ((reg_data & 0x00000006) == 6) {
            elog (DEBUG1, "Work done!\n");

            break;
        }
    } while (1);

    // Stop working
    action_write (h, ACTION_CONTROL_L, 0x00000000);
    action_write (h, ACTION_CONTROL_H, 0x00000000);
    elog (DEBUG2, " Write ACTION_CONTROL for stop working! \n");

    // Flush rest data
    action_write (h, ACTION_CONTROL_L, 0x00000008);
    action_write (h, ACTION_CONTROL_H, 0x00000000);
    elog (DEBUG2, " Write ACTION_CONTROL for stat flushing! \n");

    do {
        reg_data = action_read (h, ACTION_STATUS_L);

        // Status[23:8]
        if ((reg_data & 0x00FFFF00) != 0) {
            elog (DEBUG1, "Error code got 0X%X\n", ((reg_data & 0x00FFFF00) >> 8));
            exit (EXIT_FAILURE);
        }

        // Status[3]
        if ((reg_data & 0x00000008) == 8) {
            elog (DEBUG2, "Stat flush done!\n");
            reg_data = action_read (h, ACTION_STATUS_H);
            elog (DEBUG1, "Number of matched packets: %d\n", reg_data);
            *num_matched_pkt = reg_data;
            break;
        }

        elog (DEBUG3, "Polling Status reg with 0X%X\n", reg_data);
    } while (1);

    // Stop flushing
    action_write (h, ACTION_CONTROL_L, 0x00000000);
    action_write (h, ACTION_CONTROL_H, 0x00000000);
    elog (DEBUG2, " Write ACTION_CONTROL for stop working! \n");

    return;
}

int capi_regex_scan_internal (struct snap_card* dnc,
                              int timeout,
                              void* patt_src_base,
                              void* pkt_src_base,
                              void* stat_dest_base,
                              size_t* num_matched_pkt,
                              size_t patt_size,
                              size_t pkt_size,
                              size_t stat_size)
{
    int rc;
    uint64_t td;

    rc = 0;

    action_regex (dnc, patt_src_base, pkt_src_base, stat_dest_base, num_matched_pkt,
                  patt_size, pkt_size, stat_size);
    elog (DEBUG3, "Wait for idle\n");
    rc = action_wait_idle (dnc, timeout, &td);
    elog (DEBUG3, "Card in idle\n");

    if (0 != rc) {
        return rc;
    }

    return rc;
}

struct snap_action* get_action (struct snap_card* handle,
                                snap_action_flag_t flags, int timeout)
{
    struct snap_action* act;

    act = snap_attach_action (handle, ACTION_TYPE_STRING_MATCH,
                              flags, timeout);

    if (NULL == act) {
        elog (DEBUG1, "Error: Can not attach Action: %x\n", ACTION_TYPE_STRING_MATCH);
        elog (DEBUG1, "       Try to run snap_main tool\n");
    }

    return act;
}

void* capi_regex_compile_internal (const char* patt, size_t* size)
{
    // The max size that should be alloc
    // Assume we have at most 1024 lines in a pattern file
    size_t max_alloc_size = MAX_NUM_PATT * (64 +
                                            (PATTERN_WIDTH_BYTES - 4) +
                                            ((PATTERN_WIDTH_BYTES - 4) % 64) == 0 ? 0 :
                                            (64 - ((PATTERN_WIDTH_BYTES - 4) % 64)));

    void* patt_src_base = alloc_mem (64, max_alloc_size);
    //void* patt_src_base = palloc0 (max_alloc_size);
    void* patt_src = patt_src_base;

    elog (DEBUG1, "PATTERN Source Address Start at 0X%016lX\n", (uint64_t)patt_src);

    if (patt == NULL) {
        elog (DEBUG1, "PATTERN pointer is NULL!\n");
        exit (EXIT_FAILURE);
    }

    //remove_newline (patt);
    // TODO: fill the same pattern for 8 times, workaround for 32x8.
    // TODO: for 64X1, only 1 pattern is needed.
    for (int i = 0; i < 1; i++) {
        elog (DEBUG3, "%s\n", patt);
        patt_src = fill_one_pattern (patt, patt_src);
        elog (DEBUG3, "Pattern Source Address 0X%016lX\n", (uint64_t)patt_src);
    }

    elog (DEBUG1, "Total size of pattern buffer used: %ld\n", (uint64_t) (patt_src - patt_src_base));

    elog (DEBUG1, "---------- Pattern Buffer: %p\n", patt_src_base);

    if (verbose_level > 2) {
        __hexdump (stdout, patt_src_base, (patt_src - patt_src_base));
    }

    (*size) = patt_src - patt_src_base;

    return patt_src_base;
}

// The new function based on PostgreSQL storage backend
char* get_attr (HeapTupleHeader tuphdr,
                TupleDesc tupdesc,
                uint16_t lp_len,
                int attr_id,
                int* out_len)
{
    int         nattrs;
    int         off = 0;
    int         i;
    uint16_t    t_infomask  = tuphdr->t_infomask;
    uint16_t    t_infomask2 = tuphdr->t_infomask2;
    int         tupdata_len = lp_len - tuphdr->t_hoff;
    char*       tupdata = (char*) tuphdr + tuphdr->t_hoff;
    bits8*      t_bits = tuphdr->t_bits;

    nattrs = tupdesc->natts;

    if (nattrs < (t_infomask2 & HEAP_NATTS_MASK))
        ereport (ERROR,
                 (errcode (ERRCODE_DATA_CORRUPTED),
                  errmsg ("number of attributes in tuple header is greater than number of attributes in tuple descriptor")));

    if (attr_id >= nattrs) {
        ereport (ERROR,
                 (errcode (ERRCODE_DATA_CORRUPTED),
                  errmsg ("Given index [%d] is out of range, number of attrs: %d", attr_id, nattrs)));
    }

    for (i = 0; i < nattrs; i++) {
        Form_pg_attribute attr;
        bool        is_null;

        attr = TupleDescAttr (tupdesc, i);

        if (i >= (t_infomask2 & HEAP_NATTS_MASK)) {
            is_null = true;
        } else {
            is_null = (t_infomask & HEAP_HASNULL) && att_isnull (i, t_bits);
        }

        if (!is_null) {
            int         len;

            if (attr->attlen == -1) {
                off = att_align_pointer (off, attr->attalign, -1,
                                         tupdata + off);

                if (VARATT_IS_EXTERNAL (tupdata + off) &&
                    !VARATT_IS_EXTERNAL_ONDISK (tupdata + off) &&
                    !VARATT_IS_EXTERNAL_INDIRECT (tupdata + off))
                    ereport (ERROR,
                             (errcode (ERRCODE_DATA_CORRUPTED),
                              errmsg ("first byte of varlena attribute is incorrect for attribute %d", i)));

                len = VARSIZE_ANY (tupdata + off);
            } else {
                off = att_align_nominal (off, attr->attalign);
                len = attr->attlen;
            }

            if (tupdata_len < off + len)
                ereport (ERROR,
                         (errcode (ERRCODE_DATA_CORRUPTED),
                          errmsg ("unexpected end of tuple data")));

            if (i == attr_id) {
                (*out_len) = len;
                break;
            }

            off = att_addlength_pointer (off, attr->attlen,
                                         tupdata + off);
        }
    }

    return (char*) (tupdata + off);
}

int capi_regex_job_init (CAPIRegexJobDescriptor* job_desc)
{
    if (job_desc == NULL) {
        return -1;
    }

    // Init the job descriptor
    job_desc->card_no            = 0;
    job_desc->timeout            = ACTION_WAIT_TIME;
    job_desc->attach_flags       = 0;
    job_desc->act                = NULL;
    job_desc->patt_src_base      = NULL;
    job_desc->pkt_src_base       = NULL;
    job_desc->stat_dest_base     = NULL;
    job_desc->num_pkt            = 0;
    job_desc->num_matched_pkt    = 0;
    job_desc->pkt_size           = 0;
    job_desc->patt_size          = 0;
    job_desc->pkt_size_wo_hw_hdr = 0;
    job_desc->stat_size          = 0;
    job_desc->pattern            = NULL;
    job_desc->results            = NULL;
    job_desc->curr_result_id     = 0;
    job_desc->t_init             = 0;
    job_desc->t_init             = 0;
    job_desc->t_regex_patt       = 0;
    job_desc->t_regex_pkt        = 0;
    job_desc->t_regex_scan       = 0;
    job_desc->t_regex_harvest    = 0;
    job_desc->t_cleanup          = 0;

    // Prepare the card and action
    elog (DEBUG2, "Open Card: %d\n", job_desc->card_no);
    sprintf (job_desc->device, "/dev/cxl/afu%d.0s", job_desc->card_no);
    job_desc->dn = snap_card_alloc_dev (job_desc->device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);

    if (NULL == job_desc->dn) {
        ereport (ERROR,
                 (errcode (ERRCODE_INVALID_PARAMETER_VALUE),
                  errmsg ("Cannot allocate CARD!")));
        return -1;
    }

    // Reset the hardware
    soft_reset (job_desc->dn);

    elog (DEBUG1, "Start to get action.\n");
    job_desc->act = get_action (job_desc->dn, job_desc->attach_flags, 5 * job_desc->timeout);
    elog (DEBUG1, "Finish get action.\n");

    return 0;
}

int capi_regex_compile (CAPIRegexJobDescriptor* job_desc, const char* pattern)
{
    if (job_desc == NULL) {
        return -1;
    }

    job_desc->patt_src_base = capi_regex_compile_internal (pattern, & (job_desc->patt_size));

    if (job_desc->patt_size == 0 || job_desc->patt_src_base == NULL) {
        return -1;
    }

    return 0;
}

void* capi_regex_pkt_psql_internal (Relation rel, int attr_id, size_t* size, size_t* size_wo_hw_hdr,
                                    size_t* num_pkt, int64_t* t_pkt_cpy)
{
    void* pkt_src_base = NULL;
    void* pkt_src      = NULL;
    int num_blks       = RelationGetNumberOfBlocksInFork (rel, MAIN_FORKNUM);
    TupleDesc tupdesc  = RelationGetDescr (rel);
    struct timespec t_beg, t_end;

    for (int blk_num = 0; blk_num < num_blks; ++blk_num) {

        Buffer buf = ReadBufferExtended (rel, MAIN_FORKNUM, blk_num, RBM_NORMAL, NULL);
        LockBuffer (buf, BUFFER_LOCK_SHARE);

        Page page = (Page) BufferGetPage (buf);
        int num_lines = PageGetMaxOffsetNumber (page);

        // Calculate the size of the packet buffer
        // TODO: assume every block has the same number of lines ...
        if (blk_num == 0) {
            int row_count = num_blks * num_lines;
            // The max size that should be alloc
            size_t max_alloc_size = (row_count < MIN_NUM_PKT ? MIN_NUM_PKT : row_count) * (64 + 2048);

            pkt_src_base = alloc_mem (64, max_alloc_size);
            pkt_src = pkt_src_base;

            elog (DEBUG1, "PACKET Source Address Start at 0X%016lX\n", (uint64_t)pkt_src);
        }

        for (int line_num = 0; line_num <= num_lines; ++line_num) {
            ItemId id = PageGetItemId (page, line_num);
            uint16 lp_offset = ItemIdGetOffset (id);
            uint16 lp_len = ItemIdGetLength (id);
            HeapTupleHeader tuphdr = (HeapTupleHeader) PageGetItem (page, id);

            if (ItemIdHasStorage (id) &&
                lp_len >= MinHeapTupleSize &&
                lp_offset == MAXALIGN (lp_offset)) {

                int attr_len = 0;
                bytea* attr_ptr = DatumGetByteaP (get_attr (tuphdr, tupdesc, lp_len, attr_id, &attr_len));

                attr_len = VARSIZE (attr_ptr) - VARHDRSZ;

                elog (DEBUG3, "PACKET line read with length %d :\n", attr_len);
                elog (DEBUG3, "%s\n", VARDATA (attr_ptr));
                (*size_wo_hw_hdr) += attr_len;
                clock_gettime (CLOCK_REALTIME, &t_beg);
                pkt_src = fill_one_packet (VARDATA (attr_ptr), attr_len, pkt_src);
                clock_gettime (CLOCK_REALTIME, &t_end);
                (*t_pkt_cpy) += diff_time (&t_beg, &t_end);
                elog (DEBUG3, "PACKET Source Address 0X%016lX\n", (uint64_t)pkt_src);
                (*num_pkt)++;
            }
        }

        LockBuffer (buf, BUFFER_LOCK_UNLOCK);
        ReleaseBuffer (buf);
    }

    if (verbose_level > 2) {
        __hexdump (stdout, pkt_src_base, (pkt_src - pkt_src_base));
    }

    (*size) = pkt_src - pkt_src_base;
    elog (DEBUG1, "Total size of packet buffer used: %ld\n", (uint64_t) (pkt_src - pkt_src_base));
    elog (DEBUG1, "Total number of packets to be processed: %zu\n", *num_pkt);

    return pkt_src_base;
}

int capi_regex_pkt_psql (CAPIRegexJobDescriptor* job_desc, Relation rel, int attr_id)
{
    if (job_desc == NULL) {
        return -1;
    }

    job_desc->pkt_src_base = capi_regex_pkt_psql_internal (rel,
                             attr_id,
                             & (job_desc->pkt_size),
                             & (job_desc->pkt_size_wo_hw_hdr),
                             & (job_desc->num_pkt),
                             & (job_desc->t_regex_pkt_copy));

    // Allocate the result buffer per the number of packets in the packet buffer
    // TODO: To reserve twice more spaces in case hardware goes into panic (i.e., writing to more spaces than expected)
    // TODO: hardware issue?
    int real_stat_size = (OUTPUT_STAT_WIDTH / 8) * (job_desc->num_pkt) * 2;
    int stat_size = (real_stat_size % 4096 == 0) ? real_stat_size : real_stat_size + (4096 - (real_stat_size % 4096));

    // At least 4K for output buffer.
    if (stat_size == 0) {
        stat_size = 4096;
    }

    job_desc->stat_dest_base = alloc_mem (64, stat_size);
    job_desc->stat_size = stat_size;

    if (job_desc->pkt_size == 0 ||
        job_desc->pkt_src_base == NULL ||
        job_desc->stat_dest_base == NULL) {
        return -1;
    }

    return 0;
}

int capi_regex_scan (CAPIRegexJobDescriptor* job_desc)
{
    if (job_desc == NULL) {
        return -1;
    }

    if (capi_regex_scan_internal (job_desc->dn,
                                  job_desc->timeout,
                                  job_desc->patt_src_base,
                                  job_desc->pkt_src_base,
                                  job_desc->stat_dest_base,
                                  & (job_desc->num_matched_pkt),
                                  job_desc->patt_size,
                                  job_desc->pkt_size,
                                  job_desc->stat_size)) {

        ereport (ERROR,
                 (errcode (ERRCODE_INVALID_PARAMETER_VALUE),
                  errmsg ("Hardware ERROR!")));
        return -1;
    }

    return 0;
}

int get_results (void* result, size_t num_matched_pkt, void* stat_dest_base)
{
    int i = 0, j = 0;
    uint32_t pkt_id = 0;

    if (result == NULL) {
        elog (DEBUG1, "Invalid result pointer.\n");
        return 1;
    }

    elog (DEBUG1, "---- Results (HW: hardware) ----\n");
    elog (DEBUG1, "PKT(HW) PATT(HW) OFFSET(HW)\n");

    for (i = 0; i < (int)num_matched_pkt; i++) {
        for (j = 4; j < 8; j++) {
            pkt_id |= (((uint8_t*)stat_dest_base)[i * 10 + j] << (j % 4) * 8);
        }

        elog (DEBUG1, "MATCHED PKT: %d\n", pkt_id);
        ((uint32_t*)result)[i] = pkt_id;

        pkt_id = 0;
    }

    return 0;
}

int capi_regex_result_harvest (CAPIRegexJobDescriptor* job_desc)
{
    if (job_desc == NULL) {
        return -1;
    }

    int count = 0;

    // Wait for transaction to be done.
    do {
        //elog (DEBUG3, " Draining %i! \n", count);
        action_read (job_desc->dn, ACTION_STATUS_L);
        count++;
    } while (count < 2);

    uint32_t reg_data = action_read (job_desc->dn, ACTION_STATUS_H);
    elog (DEBUG1, "After draining, number of matched packets: %d\n", reg_data);
    job_desc->num_matched_pkt = reg_data;
    job_desc->results = palloc (job_desc->num_matched_pkt * sizeof (uint32_t));

    if (get_results (job_desc->results, job_desc->num_matched_pkt, job_desc->stat_dest_base)) {
        errno = ENODEV;
        elog (DEBUG1, "ERROR: failed to get results.\n");
        return -1;
    }

    return 0;
}

int capi_regex_job_cleanup (CAPIRegexJobDescriptor* job_desc)
{
    if (job_desc == NULL) {
        return -1;
    }

    snap_detach_action (job_desc->act);
    // Unmap AFU MMIO registers, if previously mapped
    snap_card_free (job_desc->dn);
    elog (DEBUG2, "Free Card Handle: %p\n", job_desc->dn);

    free_mem (job_desc->patt_src_base);
    free_mem (job_desc->pkt_src_base);
    free_mem (job_desc->stat_dest_base);
    pfree (job_desc->results);

    return 0;
}

bool capi_regex_check_relation (Relation rel)
{
    bool retVal = true;

    /* Check that this relation has storage */
    if (rel->rd_rel->relkind == RELKIND_VIEW) {
        ereport (ERROR,
                 (errcode (ERRCODE_WRONG_OBJECT_TYPE),
                  errmsg ("cannot get raw page from view \"%s\"",
                          RelationGetRelationName (rel))));
        retVal = false;
    }

    if (rel->rd_rel->relkind == RELKIND_COMPOSITE_TYPE) {
        ereport (ERROR,
                 (errcode (ERRCODE_WRONG_OBJECT_TYPE),
                  errmsg ("cannot get raw page from composite type \"%s\"",
                          RelationGetRelationName (rel))));
        retVal = false;
    }

    if (rel->rd_rel->relkind == RELKIND_FOREIGN_TABLE) {
        ereport (ERROR,
                 (errcode (ERRCODE_WRONG_OBJECT_TYPE),
                  errmsg ("cannot get raw page from foreign table \"%s\"",
                          RelationGetRelationName (rel))));
        retVal = false;
    }

    return retVal;
}

void print_result (CAPIRegexJobDescriptor* job_desc, char* header_str, char* out_str)
{
    sprintf (header_str, "num_pkt,pkt_size,init,patt,pkt_cpy,pkt_other,hw_re_scan,harvest,cleanup,hw_perf(MB/s),num_matched_pkt\n");
    sprintf (out_str, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%f,%ld",
             job_desc->num_pkt,
             job_desc->pkt_size_wo_hw_hdr,
             job_desc->t_init,
             job_desc->t_regex_patt,
             job_desc->t_regex_pkt_copy,
             job_desc->t_regex_pkt - job_desc->t_regex_pkt_copy,
             job_desc->t_regex_scan,
             job_desc->t_regex_harvest,
             job_desc->t_cleanup,
             perf_calc (job_desc->t_regex_scan / 1000, job_desc->pkt_size_wo_hw_hdr),
             job_desc->num_matched_pkt);
    print_time_text ("|Regex hardware scan|", job_desc->t_regex_scan / 1000, job_desc->pkt_size_wo_hw_hdr);
}

