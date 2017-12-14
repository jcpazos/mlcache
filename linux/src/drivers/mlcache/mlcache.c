#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/log2.h>
#include <linux/kernel.h>

#include <trace/events/mlcache.h>

#define PROCNAME ("mlcache")
#define MLCACHE_DISABLED (0)
#define MAX_WATCH_LIST (50) /* watch at most 10 processes */
#define LIST_SPEC ("list") /* watch a list of comma-separated processes */
#define TREE_SPEC ("tree") /* watch every process under a root process */
#define LIST_MODE (0)
#define TREE_MODE (1)
#define MAX_PID_LEN (10) /* each PID should have be most 10 bytes long */
#define MAX_WRITE_LEN (MAX_PID_LEN * MAX_WATCH_LIST) /* write at most 100 bytes at a time */
#define MLCACHE_PID_SEP (',') /* PID separator when defining which processes to watch */
#define MLCACHE_MODE_SEP (':') /* defintions are in the form {list,tree}:{ID}* */
#define DISABLE_CMD ("disable") /* MLCache can be disabled by writing "disable" to the filter file */
#define MLCACHE_SCALE (100) /* the learning model's scaling factor */

static long mlcache_pid[MAX_WATCH_LIST] = { MLCACHE_DISABLED };
static int mlcache_cnt = 0;
static int mlcache_mode = -1;
static struct proc_dir_entry *root;
static unsigned long hits;
static unsigned long misses;

#ifdef CONFIG_MLCACHE_ACTIVE
static unsigned long t;
static long weight_average = 0;
static long items_in_cache = 0;
#endif

#ifdef CONFIG_MLCACHE_ACTIVE
static unsigned long upperBound(int step, int numPlays) {
		//indexing from 0
		if (step !=0 && numPlays != 0) {
				return int_sqrt(MLCACHE_SCALE*MLCACHE_SCALE*2*ilog2(MLCACHE_SCALE*MLCACHE_SCALE*(step+1)) / numPlays);
		}
		return 0;
}

static void update_page_score(struct page *page, long by, bool hit) {
		if (!page->mapping)
				return;
		if (hit) {
				page->mlcache_score -= by;
				page->mlcache_score = page->mlcache_score + upperBound(t-1, page->mlcache_plays) - upperBound(t, page->mlcache_plays)*page->mlcache_plays;
		} else {
				page->mlcache_score = weight_average;
		}
}

static void update_average(struct page *page) {
	weight_average = (weight_average*(items_in_cache-1) + page->mlcache_score)/items_in_cache;
}

static void update_cache_scores(pgoff_t index, struct page *page, struct address_space *mapping, bool hit) {
		void **slot;
		struct radix_tree_iter iter;

		if (hit)
				update_page_score(page, MLCACHE_SCALE, hit);

		rcu_read_lock();

		radix_tree_for_each_slot(slot, &mapping->page_tree, &iter, 0) {
			struct page *p;

			p = radix_tree_deref_slot(slot);
			if (unlikely(!p))
				continue;

			if (radix_tree_exception(p)) {
					if (radix_tree_deref_retry(p))
							slot = radix_tree_iter_retry(&iter);

					continue;
			}

			if (p == page)
					continue;

			if (p->mapping == NULL)
					continue;

			update_page_score(p, -MLCACHE_SCALE, !hit);
		}

		rcu_read_unlock();
}
#endif

static int mlcache_hist_show(struct seq_file *m, void *v) {
		seq_printf(m, "Hits: %ld | Misses: %ld\n", hits, misses);
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

static void mlcache_pageget(void *data, pgoff_t off, pid_t pid, struct page *page, struct address_space *mapping, bool hit) {
		bool match;

		if (mlcache_cnt == 0)
				return;

		if (page == NULL)
				return;

		if (mlcache_mode == LIST_MODE)
				match = list_match();
		else if (mlcache_mode == TREE_MODE)
				match = tree_match();
		else
				match = false;

		if (match) {
				if (hit)
						hits++;
				else
						misses++;

#ifdef CONFIG_MLCACHE_ACTIVE
				t++;
				update_cache_scores(off, page, mapping, hit);
				items_in_cache++;
				update_average(page);
#endif
		}

}

static int mlcache_filter_show(struct seq_file *m, void *v) {
#ifdef CONFIG_MLCACHE_ACTIVE
		seq_printf(m, "[Active] ");
#else
		seq_printf(m, "[Hit/Misses Only] ");
#endif

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
		hits = misses = 0;
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

		hits = misses = 0;
#ifdef CONFIG_MLCACHE_ACTIVE
		t = 0;
#endif
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
