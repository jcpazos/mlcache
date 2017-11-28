#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <trace/events/mlcache.h>

#define PROCNAME ("mlcache")
#define MLCACHE_DISABLED (-1)
#define MAX_PID_LEN (10)

struct proc_dir_entry *root;
long mlcache_pid = MLCACHE_DISABLED;

static void mlcache_pageget(void *data, pgoff_t off, int type, pid_t pid) {
		if (mlcache_pid == MLCACHE_DISABLED)
				return;

		if (mlcache_pid == current->pid) {
				char *res = type == MLCACHE_HIT ? "hit" : "miss";
				printk(KERN_INFO "Got page cache lookup on offset %ld from %ld (%s)\n", (long) off, (long) pid, res);
		}
}

static int mlcache_show(struct seq_file *m, void *v) {
		if (mlcache_pid == MLCACHE_DISABLED) {
				seq_printf(m, "ML Cache is currently not enabled.\n");
		} else {
				seq_printf(m, "ML Cache is enabled for process %ld\n", mlcache_pid);
		}
		return 0;
}

static ssize_t mlcache_write(struct file * m, const char *buf, size_t len, loff_t *off) {
		long new_pid;
		int i;
		char bufcp[MAX_PID_LEN];

		if (len > MAX_PID_LEN)
				return -EINVAL;

		for (i = 0; i < len; i++) {
				bufcp[i] = buf[i];
		}

		/* remove trailing newline, if any */
		if (bufcp[len-1] == '\n')
				bufcp[len-1] = '\0';

		/* content given is not a valid PID */
		if (kstrtol(bufcp, 10, &new_pid) != 0) {
				return -EINVAL;
		}

		mlcache_pid = new_pid;
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
