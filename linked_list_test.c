#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/time.h>



#define BILLION 1000000000
unsigned long long calclock3(struct timespec spclock0, struct timespec spclock1, unsigned long long *total_time, unsigned long long *total_count)
{
	long temp, temp_n;
	unsigned long long timedelay=0;
	if (spclock1.tv_nsec >= spclock0.tv_nsec)
	{
		temp = spclock1.tv_sec - spclock0.tv_sec;
		temp_n = spclock1.tv_nsec - spclock0.tv_nsec;
		timedelay = BILLION * temp + temp_n;
	}
	else
	{
		temp = spclock1.tv_sec - spclock0.tv_sec -1;
		temp_n = BILLION + spclock1.tv_nsec - spclock0.tv_nsec;
		timedelay = BILLION * temp + temp_n;
	}
	__sync_fetch_and_add(total_time, timedelay);
	__sync_fetch_and_add(total_count, 1);
	return timedelay;
}

struct my_node
{
	struct list_head list;
	int data;
};


void test_linked_list(int num)
{
	// initialize list
	struct list_head my_list;
	INIT_LIST_HEAD(&my_list);
	
	// insertion
	struct timespec spclock0;
	struct timespec spclock1;
	unsigned long long interval_time;
	unsigned long long count_value;
	unsigned long long t;
	
	clock_gettime(CLOCK_REALTIME, &spclock0);
	int i;
	for (i=0; i<num; i++)
	{
		struct my_node* new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
		new->data = i;
		list_add(&new->list, &my_list);
	}
	clock_gettime(CLOCK_REALTIME, &spclock1);
	t = calclock3(spclock0, spclock1, &interval_time, &count_value);
	printk("insert %d = time : %llu, interval : %llu, count : %llu\n",num,t,interval_time,count_value);
	
	// search
	
	clock_gettime(CLOCK_REALTIME, &spclock0);
	struct my_node* current_node;
	struct list_head* p;
	for (i=0; i<num; i++)
	{
		list_for_each(p, &my_list)
		{
			current_node = list_entry(p, struct my_node, list);
			printk("current_value : %d\n", current_node->data);
		}
	}
	clock_gettime(CLOCK_REALTIME, &spclock1);
	t = calclock3(spclock0, spclock1, &interval_time, &count_value);
	printk("search %d = time : %llu, interval : %llu, count : %llu\n",num,t,interval_time,count_value);
	
	//delete
	clock_gettime(CLOCK_REALTIME, &spclock0);
	struct my_node* tmp;
	for (i=0; i<num; i++)
	{
		list_for_each_entry_safe(current_node, tmp, &my_list, list)
		{
			if (current_node -> data == i)
			{
				list_del(&current_node->list);
				kfree(current_node);
		}
	}
	clock_gettime(CLOCK_REALTIME, &spclock1);
	t = calclock3(spclock0, spclock1, &interval_time, &count_value);
	printk("delete %d = time : %llu, interval : %llu, count : %llu\n",num,t,interval_time,count_value);
}
int __init my_module_init(void)
{
	test_linked_list(1000);
	test_linked_list(10000);
	test_linked_list(100000);
	return 0;
}
void __exit my_module_cleanup(void)
{
	printk("20141261 JaewoongSong\n");
}

	

module_init(my_module_init);
module_exit(my_module_cleanup);
	
