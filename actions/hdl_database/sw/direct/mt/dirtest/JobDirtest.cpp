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
#include "time.h"
#include "JobDirtest.h"
#include "constants.h"

JobDirtest::JobDirtest()
    : JobBase(),
      m_worker (NULL),
      m_num_matched_pkt (0),
      m_patt_src_base (NULL),
      m_patt_size (0),
      m_pkt_src_base (NULL),
      m_pkt_size (0),
      m_max_alloc_pkt_size (0),
      m_stat_dest_base (NULL),
      m_stat_size (0),
      m_buff_prep_time (0),
      m_scan_time (0)
{
    //printf ("create new dirtest job\n");
}

JobDirtest::JobDirtest (int in_id, int in_thread_id)
    : JobBase (in_id, in_thread_id),
      m_worker (NULL),
      m_num_matched_pkt (0),
      m_patt_src_base (NULL),
      m_patt_size (0),
      m_pkt_src_base (NULL),
      m_pkt_size (0),
      m_max_alloc_pkt_size (0),
      m_stat_dest_base (NULL),
      m_stat_size (0),
      m_buff_prep_time (0),
      m_scan_time (0)
{
    //printf("create new dirtest job on engine %d\n", in_thread_id);
}

JobDirtest::JobDirtest (int in_id, int in_thread_id, HardwareManagerPtr in_hw_mgr)
    : JobBase (in_id, in_thread_id, in_hw_mgr),
      m_worker (NULL),
      m_num_matched_pkt (0),
      m_patt_src_base (NULL),
      m_patt_size (0),
      m_pkt_src_base (NULL),
      m_pkt_size (0),
      m_max_alloc_pkt_size (0),
      m_stat_dest_base (NULL),
      m_stat_size (0),
      m_buff_prep_time (0),
      m_scan_time (0)
{
    //printf("create new dirtest job on engine %d\n", in_thread_id);
}

JobDirtest::JobDirtest (int in_id, int in_thread_id, HardwareManagerPtr in_hw_mgr, bool in_debug)
    : JobBase (in_id, in_thread_id, in_hw_mgr, in_debug),
      m_worker (NULL),
      m_num_matched_pkt (0),
      m_patt_src_base (NULL),
      m_patt_size (0),
      m_pkt_src_base (NULL),
      m_pkt_size (0),
      m_max_alloc_pkt_size (0),
      m_stat_dest_base (NULL),
      m_stat_size (0),
      m_buff_prep_time (0),
      m_scan_time (0)
{
    //printf("create new dirtest job on engine %d\n", in_thread_id);
}

JobDirtest::~JobDirtest()
{
}

int JobDirtest::run()
{
    uint64_t start_time, elapsed_time;
    start_time = get_usec();

    do {
        if (init()) {
            printf ("ERROR: Failed to perform regex job initializing\n");
            fail();
            return -1;
        }

        if (packet()) {
            printf ("ERROR: Failed to perform regex packet preparing\n");
            fail();
            return -1;
        }
    } while (0);

    elapsed_time = get_usec() - start_time;
    m_buff_prep_time = elapsed_time;
    //printf ("Eng %d Job %d: finished buffer preparing after %lu usec\n", m_thread_id, m_id, m_buff_prep_time);

    start_time = get_usec();

    do {
        // TODO: Only 1 job is allowed to access hardware at a time.
        //boost::lock_guard<boost::mutex> lock (ThreadBase::m_global_mutex);

        if (scan()) {
            printf ("ERROR: Failed to perform regex scanning\n");
            fail();
            return -1;
        }
    } while (0);

    elapsed_time = get_usec() - start_time;
    m_scan_time = elapsed_time;
    //printf ("Eng %d Job %d: finished scanning after %lu usec\n", m_thread_id, m_id, m_scan_time);

    done();

    //printf ("Eng %d Job %d finished with size %zu\n", m_thread_id, m_id, m_pkt_size);

    return 0;
}

void JobDirtest::set_worker (WorkerDirtestPtr in_worker)
{
    m_worker = in_worker;
}

WorkerDirtestPtr JobDirtest::get_worker()
{
    return m_worker;
}

int JobDirtest::init()
{
    //printf("Eng %d: init job %d\n", m_thread_id, m_id);
    if (NULL == m_worker) {
        printf ("ERROR: Worker points to NULL, cannot perform regex job init\n");
        return -1;
    }

    if (NULL == m_hw_mgr) {
        printf ("ERROR: Hardware manager points to NULL, cannot perform regex job init\n");
        return -1;
    }

    // Copy the pattern from worker to job
    m_patt_src_base = m_worker->get_pattern_buffer();
    m_patt_size = m_worker->get_pattern_buffer_size();
   
    // Reset the engine
    m_hw_mgr->reset_engine (m_thread_id);

    return 0;
}

int JobDirtest::packet()
{
    //printf("Eng %d Job %d: prepare packet\n", m_thread_id, m_id);
    if (NULL == m_worker) {
        printf ("ERROR: Worker points to NULL, cannot perform regex packet preparation\n");
        return -1;
    }

    if (NULL == m_pkt_src_base) {
        printf ("ERROR: pkt_src_base is NULL\n");
        return -1;
    }
    if (NULL == m_stat_dest_base) {
        printf ("ERROR: stat_dest_base is NULL\n");
        return -1;
    }

    m_pkt_size = m_worker->get_packet_buffer_size (m_id, m_thread_id);
    memcpy (m_pkt_src_base, m_worker->get_packet_buffer (m_id, m_thread_id), m_pkt_size);
    //printf ("Eng %d Job %d: packet size is %zu\n", m_thread_id, m_id, m_pkt_size);

    return 0;
}

int JobDirtest::scan()
{
    //printf("Eng %d Job %d: scanning...\n", m_thread_id, m_id);
    if (regex_scan_internal (m_hw_mgr->get_capi_card(),
                             ACTION_WAIT_TIME,
                             m_patt_src_base,
                             m_pkt_src_base,
                             m_stat_dest_base,
                             &m_num_matched_pkt,
                             m_patt_size,
                             m_pkt_size,
                             m_stat_size,
                             m_thread_id)) {
        printf ("ERROR: Failed to scan the table\n");
        return -1;
    }

    //printf ("Eng %d Job %d: finish regex_scan with %d matched packets.\n", m_thread_id, m_id, (int)m_num_matched_pkt);

    int count = 0;
    do {
        //printf ("Eng %d: draining %i! \n", m_thread_id, count);
        m_hw_mgr->reg_read(ACTION_STATUS_L, m_thread_id);
        count++;
    } while (count < 10);

    uint32_t reg_data = m_hw_mgr->reg_read(ACTION_STATUS_H, m_thread_id);
    //printf ("Eng %d Job %d: After draining, number of matched packets: %d\n", m_thread_id, m_id, reg_data);
    m_num_matched_pkt = reg_data;

    return 0;
}

int JobDirtest::set_packet_buffer (void* in_pkt_src_base, size_t in_max_alloc_pkt_size)
{
    m_pkt_src_base = in_pkt_src_base;

    if (NULL == m_pkt_src_base) {
        printf ("ERROR: packet buffer assigned: fail\n");
        return -1;
    }

    m_max_alloc_pkt_size = in_max_alloc_pkt_size;
    return 0;
}

int JobDirtest::set_result_buffer (void* in_stat_dest_base, size_t in_stat_size)
{
    m_stat_dest_base = in_stat_dest_base;
    m_stat_size = in_stat_size;
    return 0;
}

size_t JobDirtest::get_num_matched_pkt()
{
    return m_num_matched_pkt;
}

uint64_t JobDirtest::get_buff_prep_time()
{
    return m_buff_prep_time;
}

uint64_t JobDirtest::get_scan_time()
{
    return m_scan_time;
}

void JobDirtest::release_buffer()
{
    m_patt_src_base = NULL;
    m_pkt_src_base = NULL;
    m_stat_dest_base = NULL;
}

void JobDirtest::cleanup()
{
    //printf("Eng %d: clean up job %d\n", m_thread_id, m_id);
    m_hw_mgr = NULL;
    m_worker = NULL;
}