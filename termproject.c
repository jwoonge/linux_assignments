#define BILLION 1000000000
#define NUM_OF_ENTRY 100000

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h> // for thread
#include <linux/slab.h> // for kmalloc
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/sched.h>
#include "n_list.h"

unsigned long long n_list_insert_time = 0;
unsigned long long n_list_insert_count = 0;
unsigned long long n_list_delete_time = 0;
unsigned long long n_list_delete_count = 0;
unsigned long long n_list_get_time = 0;
unsigned long long n_list_get_count = 0;
unsigned long long n_list_traverse_time = 0;
unsigned long long n_list_traverse_count = 0;
struct timespec64 spclock[2];
void n_list_test_insert(void);
void n_list_test_delete(void);
void n_list_test_get(void);
void n_list_test_traverse(void);
void initialize_ts64(struct timespec64* spclock);
unsigned long long calclock3(struct timespec64* spclock, unsigned long long total_time, unsigned long long total_count);

void initialize_ts64(struct timespec64* spclock)
{
    int i;
    for (i=0; i<2; i++)
    {
        spclock[i].tv_sec = 0;
        spclock[i].tv_nsec = 0;
    }
}
unsigned long long calclock3(struct timespec64* spclock, unsigned long long* total_time, unsigned long long* total_count)
{
    unsigned long long timedelay = 0;
    if (spclock[1].tv_nsec >= spclock[0].tv_nsec)
    {
        temp = spclock[1].tv_sec - spclock[0].tv_sec;
        temp_n = spclock[1].tv_nsec - spclock[0].tv_nsec;
        timedelay = BILLION * temp + temp_n;
    }
    else
    {
        temp = spclock[1].tv_sec - spclock[0].tv_sec -1;
        temp_n = BILLION + spclock[1].tv_nsec - spclock[0].tv_nsec;
        timedelay = BILLION * temp + temp_n;
    }
    __sync_fetch_and_add(total_time, timedelay);
    __sync_fetch_and_add(total_count, 1);
    return timedelay;
}

void n_list_test_insert(void)
{
    initialize_ts64(spclock);
    strut list_head HEAD;
    int i;
    for (i=0; i<100000; i++)
    {
        struct node *new = kmalloc(sizeof(struct node), GFP_KERNEL);
        new->value = i+2000;
        n_list_add(&new->v_list, &HEAD);
    }
    
    ktime_get_real_ts64(&spclock[0]);
    struct node* new = kmalloc(sizeof(struct node), GFP_KERNEL);
    new->value = 5555;
    n_list_add(&new->v_list, &HEAD);
    ktime_get_real_ts64(&spclock[1]);
    calclock3(spclock, &n_list_insert_time, &n_list_insert_count);
}
void n_list_test_delete(void)
{
    initialize_ts64(spclock);
    struct list_head HEAD;
    int i;
    for (i=0; i<100000; i++)
    {
        struct node *new = kmalloc(sizeof(struct node), GFP_KERNEL);
        new->value = i+2000;
        n_list_add(&new->v_list, &HEAD);
    }
    
    struct list_head* to_del = n_list_get(99999, &HEAD);
    
    ktime_get_real_ts64(&spclock[0]);
    n_list_del(to_del, &HEAD);
    ktime_get_real_ts64(&spclock[1]);
    calclock3(spclock, &n_list_delete_time, &n_list_delete_count);
}
void n_list_test_get(void)
{
    initialize_ts64(spclock);
    struct list_head HEAD;
    int i;
    for (i=0; i<100000; i++)
    {
        struct node *new = kmalloc(sizeof(struct node), GFP_KERNEL);
        new->value = i+2000;
        n_list_add(&new->v_list, &HEAD);
    }
    
    for (i=0; i<100000; i++)
    {
        unsigned long long __time = 0;
        unsigned long long __count = 0;
        ktime_get_real_ts64(&spclock[0]);
        struct list_head* found = n_list_get(i, &HEAD);
        ktime_get_real_ts64(&spclock[1]);
        calclock3(spclock, &__time, &__count);
        n_list_delete_time += __time;
    }
    n_list_get_time = (unsigned long long) (n_list_get_time/100000);
}

void n_list_test_traverse(void)
{
    
}

int __init simple_module_init(void)
{
    printk(KERN_EMERG "n_list testing Module\n");
    run();
    return 0;
}

void __exit simple_module_cleanup(void)
{
    printk("n_list testing Module\n");
}

module_init(simple_module_init);
module_exit(simple_module_cleanup);
