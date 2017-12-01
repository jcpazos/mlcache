#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include <trace/events/mlcache.h>

#define PROCNAME ("mlcache")
#define MLCACHE_DISABLED (0)
#define MAX_WATCH_LIST (50) /* watch at most 10 processes */
#define LIST_SPEC ("list")
#define TREE_SPEC ("tree")
#define LIST_MODE (0)
#define TREE_MODE (1)
#define MAX_PID_LEN (10) /* each PID should have be most 10 bytes long */
#define MAX_WRITE_LEN (MAX_PID_LEN * MAX_WATCH_LIST) /* write at most 100 bytes at a time */
#define MLCACHE_PID_SEP (',') /* PID separator when defining which processes to watch */
#define MLCACHE_MODE_SEP (':')
#define MAX_EVENTS (100000)
#define DISABLE_CMD ("disable")

struct mlcache_event {
		pid_t requester;
		off_t index;
		char type;
};

static long mlcache_pid[MAX_WATCH_LIST] = { MLCACHE_DISABLED };
static int mlcache_cnt = 0;
static int mlcache_mode = -1;
static struct proc_dir_entry *root;
static struct mlcache_event *events;
static long num_events = 0;

static int mlcache_hist_show(struct seq_file *m, void *v) {
		int i;
		struct mlcache_event e;
		char *desc;

		for (i = 0; i < num_events; i++) {
				e = events[i];
				desc = e.type == MLCACHE_HIT ? "hit" : "miss";

				seq_printf(m, "Requester: %ld | Index: %ld | Result: %s\n", (long) e.requester, (long) e.index, desc);
		}

		seq_printf(m, "%ld events.\n", num_events);

		return 0;
}

static int hist_open(struct inode *inode, struct	file *filp) {
		return single_open(filp, mlcache_hist_show, PDE_DATA(inode));
}

static ssize_t mlcache_hist_write(struct file * m, const char *buf, size_t len, loff_t *off) {
		return len;
}

static const struct file_operations hist_fops = {
		.owner = THIS_MODULE,
		.open = hist_open,
		.read = seq_read,
		.write = mlcache_hist_write,
		.llseek = seq_lseek,
		.release = single_release,
};

static void create_proc_entries(void) {
		char *name = kmalloc(MAX_PID_LEN, GFP_KERNEL);

		if (mlcache_mode == LIST_MODE) {
				/* not supported */
		} else if (mlcache_mode == TREE_MODE) {
				snprintf(name, MAX_PID_LEN, "%ld", mlcache_pid[0]);
				proc_create_data(name, 0444, root, &hist_fops, name);
		}
}

static bool list_match(void) {
		int i;

		for (i = 0; i < mlcache_cnt; i++) {
				if (mlcache_pid[i] == current->pid) {
						return true;
				}
		}

		return false;
}

static bool tree_match_recursive(struct task_struct *root) {
		struct list_head *el;
		struct task_struct *child;

		if (root == NULL)
				return false;

		if (current->pid == root->pid)
				return true;

		list_for_each(el, &root->children) {
				child = list_entry(el, struct task_struct, sibling);
				if (current->pid == child->pid)
						return true;

				if (tree_match_recursive(child))
						return true;
		}

		return false;
}

static bool tree_match(void) {
		struct task_struct *root_task;

		root_task = find_task_by_vpid(mlcache_pid[0]);
		if (root_task == NULL)
				return false;

		return tree_match_recursive(root_task);

}

static void mlcache_pageget(void *data, pgoff_t off, int type, pid_t pid) {
		bool match;

		if (mlcache_cnt == 0)
				return;

		if (mlcache_mode == LIST_MODE)
				match = list_match();
		else if (mlcache_mode == TREE_MODE) {
				match = tree_match();
		}
		else
				match = false;

		if (match) {
				struct mlcache_event event;
				event.requester = pid;
				event.index = off;
				event.type = type;

				if (num_events < MAX_EVENTS)
						memcpy(&events[num_events++], &event, sizeof(struct mlcache_event));
		}

}

static int mlcache_filter_show(struct seq_file *m, void *v) {
		if (mlcache_cnt == 0) {
				seq_printf(m, "MLCache is currently not enabled.\n");
		} else {
				int i;

				if (mlcache_mode == LIST_MODE) {
						seq_printf(m, "MLCache is watching a list of processes: [");
						for (i = 0; i < mlcache_cnt; i++) {
								if (i > 0)
										seq_printf(m, ", ");

								seq_printf(m, "%ld", mlcache_pid[i]);
						}

						seq_printf(m, "]\n");

				} else if (mlcache_mode == TREE_MODE) {
						seq_printf(m, "MLCache is watching the tree under process %ld\n", mlcache_pid[0]);

				} else {
						return -EINVAL;
				}
		}
		return 0;
}

static int add_pid(char *buf) {
		long new_pid;

		if (kstrtol(buf, 10, &new_pid) != 0)
				return -EINVAL;

		mlcache_pid[mlcache_cnt++] = new_pid;
		if (mlcache_cnt == MAX_WATCH_LIST)
				return -EINVAL;

		return 1;
}

static ssize_t parse_list(const char *buf, size_t len) {
		int i, j;
		char bufcp[MAX_PID_LEN];

		j = 0;
		for (i = 0; i < len; i++) {
				if (buf[i] == '\n')
						continue;

				if (buf[i] == MLCACHE_PID_SEP) {
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

		mlcache_mode = LIST_MODE;
		return len;
}

static ssize_t parse_tree(const char *buf, size_t len) {
		int i;
		char bufcp[MAX_PID_LEN];

		for (i = 0; i < len; i++)
				bufcp[i] = buf[i];

		bufcp[i] = '\0';
		if (bufcp[i-1] == '\n')
				bufcp[i-1] = '\n';

		if (add_pid(bufcp) < 0)
				return -EINVAL;

		mlcache_mode = TREE_MODE;
		return len;
}

static void mlcache_disable(void) {
		char name[MAX_PID_LEN];

		mlcache_cnt = 0;
		num_events = 0;
		snprintf(name, MAX_PID_LEN, "%ld", mlcache_pid[0]);

		if (mlcache_mode == LIST_MODE) {
				/* not supported */
		} else if (mlcache_mode == TREE_MODE) {
				remove_proc_entry(name, root);
		}
}

static ssize_t mlcache_filter_write(struct file * m, const char *buf, size_t len, loff_t *off) {
		int i, j, ret;
		int max_mode_len = 10;
		char mode[max_mode_len];

		if (len > MAX_WRITE_LEN)
				return -EINVAL;

		if (!strncmp(buf, DISABLE_CMD, strlen(DISABLE_CMD))) {
				mlcache_disable();
				return len;
		}

		j = 0;
		for (i = 0; i < len && i < max_mode_len; i++) {
				if (buf[i] == MLCACHE_MODE_SEP)
						break;

				mode[i] = buf[i];
		}

		mode[i++] = '\0';
		if (!strncmp(mode, LIST_SPEC, max_mode_len))
				ret = parse_list(&buf[i], len - i);
		else if (!strncmp(mode, TREE_SPEC, max_mode_len))
				ret = parse_tree(&buf[i], len - i);
		else
				ret = -EINVAL;

		if (ret > 0) {
				create_proc_entries();
				return len;
		}

		return ret;
}

static int filter_open(struct inode *inode, struct	file *file) {
		return single_open(file, mlcache_filter_show, NULL);
}

static const struct file_operations filter_fops = {
		.owner = THIS_MODULE,
		.open = filter_open,
		.read = seq_read,
		.write = mlcache_filter_write,
		.llseek = seq_lseek,
		.release = single_release,
};

static int __init mlcache_init(void) {
		root = proc_mkdir("mlcache", NULL);
		proc_create("filter", 0666, root, &filter_fops);

		events = kmalloc(MAX_EVENTS * sizeof(struct mlcache_event), GFP_KERNEL);
		if (events == NULL) {
				printk(KERN_WARNING "Could not allocate memory for events");
				return -ENOMEM;
		}

		register_trace_mlcache_event(mlcache_pageget, NULL);
		return 0;
}

static void __exit mlcache_exit(void) {
		remove_proc_entry("mlcache", NULL);
		unregister_trace_mlcache_event(mlcache_pageget, NULL);
		tracepoint_synchronize_unregister();

		if (events != NULL)
				kfree(events);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Renato Costa <renatomc@cs.ubc.ca>, Jose Pazos <jpazos@cs.ubc.ca>");
MODULE_DESCRIPTION("This module collects data about page cache hit/misses to be used as MLCache's training data");
module_init(mlcache_init);
module_exit(mlcache_exit);
