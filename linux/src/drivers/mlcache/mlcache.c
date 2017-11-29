#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <trace/events/mlcache.h>

#define PROCNAME ("mlcache")
#define MLCACHE_DISABLED (0)
#define MAX_WATCH_LIST (10) /* watch at most 10 processes */
#define MAX_PID_LEN (10) /* each PID should have be most 10 bytes long */
#define MAX_WRITE_LEN (MAX_PID_LEN * MAX_WATCH_LIST) /* write at most 100 bytes at a time */
#define MLCACHE_SEP (',') /* PID separator when defining which processes to watch */

static long mlcache_pid[MAX_WATCH_LIST] = { MLCACHE_DISABLED };
static int mlcache_cnt = 0;

static void mlcache_pageget(void *data, pgoff_t off, int type, pid_t pid) {
		int i;
		char *res = type == MLCACHE_HIT ? "hit" : "miss";

		if (mlcache_cnt == 0)
				return;

		printk(KERN_INFO "Requester: %ld | Index: %ld | Result: %s\n", (long) pid, (long) off, res);
		for (i = 0; i < mlcache_cnt; i++) {
				if (mlcache_pid[i] == current->pid) {
						char *res = type == MLCACHE_HIT ? "hit" : "miss";
						printk(KERN_INFO "Requester: %ld | Index: %ld | Result: %s\n", (long) pid, (long) off, res);
						break;
				}
		}
}

static int mlcache_show(struct seq_file *m, void *v) {
		if (mlcache_cnt == 0) {
				seq_printf(m, "MLCache is currently not enabled.\n");
		} else {
				int i;

				seq_printf(m, "MLCache is enabled for processes: [");
				for (i = 0; i < mlcache_cnt; i++) {
						if (i > 0)
								seq_printf(m, ", ");

						seq_printf(m, "%ld", mlcache_pid[i]);
				}

				seq_printf(m, "]\n");
		}
		return 0;
}

static int add_pid(char *buf) {
		long new_pid;

		if (kstrtol(buf, 10, &new_pid) != 0) {
				return -EINVAL;
		}

		mlcache_pid[mlcache_cnt++] = new_pid;
		if (mlcache_cnt == MAX_WATCH_LIST)
				return -EINVAL;

		return 1;
}

static ssize_t mlcache_write(struct file * m, const char *buf, size_t len, loff_t *off) {
		int i, j;
		char bufcp[MAX_PID_LEN];

		if (len > MAX_WRITE_LEN)
				return -EINVAL;

		j = 0;
		mlcache_cnt = 0;
		for (i = 0; i < len; i++) {
				if (buf[i] == '\n')
						continue;

				if (buf[i] == MLCACHE_SEP) {
						bufcp[j] = '\0';

						if (add_pid(bufcp) < 0)
								return -EINVAL;

						j = 0;
						continue;
				}

				bufcp[j++] = buf[i];

		}

		if (j > 0) {
				bufcp[j] = '\0';
				if (add_pid(bufcp) < 0)
						return -EINVAL;
		}

		return len;
}

static int mlcache_open(struct inode *inode, struct	file *file) {
		return single_open(file, mlcache_show, NULL);
}

static const struct file_operations mlcache_fops = {
		.owner = THIS_MODULE,
		.open = mlcache_open,
		.read = seq_read,
		.write = mlcache_write,
		.llseek = seq_lseek,
		.release = single_release,
};

static int __init mlcache_init(void) {
		proc_create("mlcache", S_IRWXUGO, NULL, &mlcache_fops);
		register_trace_mlcache_event(mlcache_pageget, NULL);
		return 0;
}

static void __exit mlcache_exit(void) {
		remove_proc_entry("mlcache", NULL);
		unregister_trace_mlcache_event(mlcache_pageget, NULL);
		tracepoint_synchronize_unregister();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Renato Costa <renatomc@cs.ubc.ca>, Jose Pazos <jpazos@cs.ubc.ca>");
MODULE_DESCRIPTION("This module collects data about page cache hit/misses to be used as MLCache's training data");
module_init(mlcache_init);
module_exit(mlcache_exit);
