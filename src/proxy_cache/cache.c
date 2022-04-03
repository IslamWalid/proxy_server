#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"

static unsigned long
generate_tag(const char *request_line, const char *request_hdrs);

static int
find_empty_line(Cache *cache);

static int
find_victim(Cache *cache);

static int
find_line(Cache *cache, unsigned int tag);

static void
destruct_line(Cache *cache, int idx);

void
cache_init(Cache *cache)
{
    cache->readcnt = 0;
    cache->highest_timestamp = 0;
    sem_init(&cache->write_mutex, 0, 1);
    sem_init(&cache->readcnt_mutex, 0, 1);
    sem_init(&cache->timestamp_mutex, 0, 1);
    memset(cache->cache_set, 0, sizeof(cache->cache_set));
}

void
cache_write(Cache *cache, const char *request_line, const char *request_hdrs,
            const char *response_line, const char *response_hdrs, 
            const void *content, const size_t content_len)
{
    int idx;
    unsigned int tag; 
    size_t object_size;
    char *response_line_copy, *response_hdrs_copy;
    void *content_copy;

    object_size = content_len + strlen(response_line) + strlen(response_hdrs);
    /* Check if the total size of the object not exceeding the MAX_OBJECT_SIZE */
    if (object_size > MAX_OBJECT_SIZE)
        return;

    tag = generate_tag(request_line, request_hdrs);
    response_line_copy = strdup(response_line);
    response_hdrs_copy = strdup(response_hdrs);
    content_copy = malloc(content_len);
    memcpy(content_copy, content, content_len);

    /* Acquire the the write mutex to protect writing process */
    sem_wait(&cache->write_mutex);

    /* Find empty cache line */
    idx = find_empty_line(cache);
    if (idx < 0) {
        idx = find_victim(cache);
        destruct_line(cache, idx);
    }
    
    /* Place cache line */
    cache->cache_set[idx].valid = 1;
    cache->cache_set[idx].tag = tag;
    cache->cache_set[idx].timestamp = ++cache->highest_timestamp;
    cache->cache_set[idx].content_len = content_len;
    cache->cache_set[idx].response_line = response_line_copy;
    cache->cache_set[idx].response_hdrs = response_hdrs_copy;
    cache->cache_set[idx].content = content_copy;

    /* Release the write_mutex */
    sem_post(&cache->write_mutex);
}

int
cache_fetch(Cache *cache, const char *request_line, const char *request_hdrs,
            char **response_line, char **response_hdrs,
            void **content, size_t *content_len)
{
    unsigned int tag;
    int idx, is_cached;

    tag = generate_tag(request_line, request_hdrs);
    sem_wait(&cache->readcnt_mutex);
    cache->readcnt++;
    if (cache->readcnt == 1)    /* First in */
        sem_wait(&cache->write_mutex);
    sem_post(&cache->readcnt_mutex);

    idx = find_line(cache, tag);
    if (idx < 0) {
        is_cached = 0;
    } else {
        is_cached = 1;
        *response_line = strdup(cache->cache_set[idx].response_line);
        *response_hdrs = strdup(cache->cache_set[idx].response_hdrs);

        *content_len = cache->cache_set[idx].content_len;
        *content = malloc(cache->cache_set[idx].content_len);
        memcpy(*content, cache->cache_set[idx].content, 
               cache->cache_set[idx].content_len);

        sem_wait(&cache->timestamp_mutex);
        cache->cache_set[idx].timestamp = ++cache->highest_timestamp;
        sem_post(&cache->timestamp_mutex);
    }

    sem_wait(&cache->readcnt_mutex);
    cache->readcnt--;
    if (cache->readcnt == 0)    /* Last out */
        sem_post(&cache->write_mutex);
    sem_post(&cache->readcnt_mutex);

    return is_cached;
}

static unsigned long
generate_tag(const char *request_line, const char *request_hdrs)
{
    size_t len = strlen(request_line) + strlen(request_hdrs);
    unsigned long hash = 5381;
    char *hash_str;

    /* Build the string that will be hashed */
    hash_str = (char *) malloc(len + 1);
    hash_str[0] = '\0';
    strcat(hash_str, request_line);
    strcat(hash_str, request_hdrs);
    
    for (int i = 0; i < len; i++)
        hash = ((hash << 5) + hash) + hash_str[i]; /* hash * 33 + c */
    
    free(hash_str);
    return hash;
}

static int
find_empty_line(Cache *cache)
{
    for (int i = 0; i < CACHE_LINES; i++) {
        if (cache->cache_set[i].valid) 
            return i;
    }

    return -1;
}

static int
find_victim(Cache *cache)
{
    int idx, least_recent_used;

    idx = 0;
    least_recent_used = cache->cache_set[0].timestamp;
    for (int i = 1; i < CACHE_LINES; i++) {
        if (cache->cache_set[i].timestamp < least_recent_used) {
            idx = i;
            least_recent_used = cache->cache_set[i].timestamp;
        }
    }

    return idx;
}

static int
find_line(Cache *cache, unsigned int tag)
{
    for (int i = 0; i < CACHE_LINES; i++) {
        if (cache->cache_set[i].valid && cache->cache_set[i].tag == tag)
            return i;
    }

    return -1;
}

static void
destruct_line(Cache *cache, int idx)
{
    free(cache->cache_set[idx].response_line);
    free(cache->cache_set[idx].response_hdrs);
    free(cache->cache_set[idx].content);
}
