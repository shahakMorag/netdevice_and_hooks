#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "common.h"
#include "log_ring.h"

struct log_entry {
    unsigned long jiffies;
    char * log;
};

struct log_ring_private {
    bool loop_happen;
    loff_t max_log_entries;
    loff_t current_log_entry;
    struct log_entry * log_entries;
};

static struct proc_dir_entry * g_log_file_entry = NULL;
static struct log_ring_private g_log_ring_private = { 0 };

static void * log_ring_start_after_loop(struct seq_file *s, loff_t *pos) {
    loff_t new_pos = *pos + g_log_ring_private.current_log_entry;
    if (new_pos >= g_log_ring_private.max_log_entries) {
        new_pos -= g_log_ring_private.max_log_entries;
    }

    ++*pos;
    return &g_log_ring_private.log_entries[new_pos];
}

static void * log_ring_start(struct seq_file *s, loff_t *pos) {
    if (*pos >= g_log_ring_private.max_log_entries) {
        ++*pos;
        return NULL;
    }

    if (g_log_ring_private.loop_happen) {
        return log_ring_start_after_loop(s, pos);
    }

    if (*pos >= g_log_ring_private.current_log_entry) {
        ++*pos;
        return NULL;
    }

    void * res = &g_log_ring_private.log_entries[*pos];
    ++*pos;
    return res;
}

static void * log_ring_next(struct seq_file *s, void *v, loff_t *pos) {
    return log_ring_start(s, pos);
}

static void log_ring_stop(struct seq_file *s, void *v) {

}

static int log_ring_show(struct seq_file *s, void *v) {
    if (NULL == v) {
        return 0;
    }

    struct log_entry * log_entry = v;

    if (NULL != log_entry->log) {
        seq_printf(s, "[%lu] ", log_entry->jiffies);
        seq_puts(s, log_entry->log);
    }

    return 0;
}

static const struct seq_operations log_ring_ops = {
    .start = log_ring_start,
    .next = log_ring_next,
    .stop = log_ring_stop,
    .show = log_ring_show,
};

bool log_printf(const char * fmt, ...) {
    if (NULL == g_log_ring_private.log_entries) {
        return false;
    }

    if (g_log_ring_private.loop_happen) {
        kfree(g_log_ring_private.log_entries[g_log_ring_private.current_log_entry].log);
    }

    g_log_ring_private.log_entries[g_log_ring_private.current_log_entry].jiffies = jiffies;

    va_list ap;

    va_start(ap, fmt);
    g_log_ring_private.log_entries[g_log_ring_private.current_log_entry].log = kvasprintf(GFP_KERNEL, fmt, ap);
    va_end(ap);

    g_log_ring_private.current_log_entry = (g_log_ring_private.current_log_entry + 1);
    if (g_log_ring_private.current_log_entry >= g_log_ring_private.max_log_entries) {
        g_log_ring_private.current_log_entry -= g_log_ring_private.max_log_entries;
        g_log_ring_private.loop_happen = true;
    }

    return NULL != g_log_ring_private.log_entries[g_log_ring_private.current_log_entry].log;
}


static int log_file_open(struct inode *inode, struct file *file) {
    return seq_open(file, &log_ring_ops);
}

static const struct proc_ops log_ring_file_ops = {
        .proc_open    = log_file_open,
        .proc_read    = seq_read,
        .proc_lseek  = seq_lseek,
        .proc_release = seq_release
};

bool init_log(size_t max_log_entries) {
    g_log_ring_private.current_log_entry = 0;
    g_log_ring_private.max_log_entries = max_log_entries;
    g_log_ring_private.log_entries = kmalloc(sizeof(*g_log_ring_private.log_entries) * g_log_ring_private.max_log_entries, GFP_KERNEL);
    if (NULL == g_log_ring_private.log_entries) {
        return false;
    }

    g_log_file_entry = proc_create("a_log", S_IRUGO, NULL, &log_ring_file_ops);

    return NULL != g_log_file_entry;
}

void remove_log(void) {
    if (NULL != g_log_file_entry) {
       proc_remove(g_log_file_entry);
    }
}