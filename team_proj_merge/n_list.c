#define SUB_LENGTH 1000
#define NUM_THREAD 4

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h> // for thread
#include <linux/slab.h> // for kmalloc
#include <linux/delay.h>
#include <linux/sched.h>
#include "n_list.h"


struct task_struct** thread_ids;

struct list_head* task_list;

//struct completion thread_comp;
DECLARE_COMPLETION(thread_comp);

void n_list_traverse(struct list_head* head, int _to_find, struct timespec64* _spclock)
{
    task_list = kmalloc(NUM_THREAD * sizeof(struct list_head), GFP_KERNEL);
    int i, thread_i=0;
    for (i=0; i<NUM_THREAD; i++)
        INIT_LIST_HEAD(&task_list[i]);
        
    struct list_head* p;
    struct sub_head* todo_sub_head;
    for (p=head->prev; p!=head; p=p->prev)
    {
        todo_sub_head = list_entry(p, struct sub_head, h_list);
        struct task* new_task = kmalloc(sizeof(struct task), GFP_KERNEL);
        new_task->todo = todo_sub_head;
        
        list_add(&new_task->t_list, &(task_list[thread_i]));
        
        thread_i ++;
        if (thread_i==NUM_THREAD) thread_i = 0;
    }
    
    thread_ids = kmalloc(NUM_THREAD*sizeof(struct task_struct*), GFP_KERNEL);
    printk("thread id init\n");
    //////////////////////////
    //init_completion(&thread_comp);
    for (i=0; i<NUM_THREAD; i++)
    {
        struct thread_arg* arg = kmalloc(sizeof(struct thread_arg*), GFP_KERNEL);
        arg-> to_find = _to_find;
        arg-> tasks = &task_list[i];
        arg-> thread_number = i;
        arg-> comp = &thread_comp;
        arg-> spclock = _spclock;
        
        thread_ids[i] = (struct task_struct*) kthread_run(_n_list_traverse, (void*) arg, "TRAVERSE");
    }
    //wait_for_completion(&thread_comp);
}

static int _n_list_traverse(void *_arg)
{
    struct thread_arg* arg = (struct thread_arg*)_arg;
    int to_find = arg->to_find;
    int this_thread_num = arg->thread_number;
    struct completion* c = arg->comp;
    int i;
    struct list_head* task_head = arg->tasks;
    struct timespec64* spclock1 = arg->spclock;

    struct list_head* tp;
    struct list_head* v_head;
    for (tp=task_head->prev; tp!=task_head; tp=tp->prev)
    {
        if (kthread_should_stop())
        {
            printk("thread terminated.\n");
            return 0;
        }
        v_head = &list_entry(tp, struct task, t_list)->todo->v_list;
        struct list_head* vp;
        for (vp = v_head->prev; vp!=v_head; vp=vp->prev)
        {
            struct node* traversed = list_entry(vp, struct node, v_list);
            
            if (traversed->value == to_find)
            {
                ktime_get_real_ts64(spclock1);
                printk("FOUND!!!!!");
                for (i=0; i<NUM_THREAD; i++)
                    if (i!=this_thread_num)
                        kthread_stop(thread_ids[i]);
                //complete_all(c);
                return 0;
            }
        }
    }   
}
    
void new_sub_head(struct list_head *head)
{
    struct sub_head *new = kmalloc(sizeof(struct sub_head), GFP_KERNEL);
    INIT_LIST_HEAD(&new->v_list);
    new->len = 0;
    list_add(&new->h_list, head);
}

void n_list_add(struct list_head *new, struct list_head *head)
{
    if(list_entry(head->next, struct sub_head, h_list)->len >= SUB_LENGTH) 
        new_sub_head(head);
        
    struct sub_head *tmp = list_entry(head->next, struct sub_head, h_list);
    //printk("%d\n", tmp->len);
    
    list_add(new, &tmp->v_list);
    
    list_entry(head->next, struct sub_head, h_list)->len++;
    
    // TODO : sturct node
    list_entry(new, struct node, v_list)->_sub = list_entry(head->next, struct sub_head, h_list); 
}

void n_list_del(struct list_head *entry, struct list_head *head)
{
    struct sub_head *_sub_head = list_entry(entry, struct node, v_list)->_sub;
    list_del(entry);
    
    _sub_head->len--;
    if(_sub_head->len == 0)
        list_del(&_sub_head->h_list);
}

void n_list_del_stable(struct list_head* entry, struct list_head* head)
{
    struct sub_head* now_sub_head_entry = list_entry(entry, struct node, v_list)->_sub;
    struct list_head* now_sub_head = &(now_sub_head_entry->h_list);
    struct list_head* prev_sub_head = now_sub_head->prev;
    struct sub_head* prev_sub_head_entry = list_entry(prev_sub_head, struct sub_head, h_list);
    list_del(entry);
    
    while (prev_sub_head != head)
    {
        struct list_head* origin = (prev_sub_head_entry->v_list).prev;
        struct node* origin_entry = list_entry(origin, struct node, v_list);
        struct node* new = kmalloc(sizeof(struct node),GFP_KERNEL);
        new->value = origin_entry->value;
        new->_sub = now_sub_head_entry;
        
        struct sub_head* temp = origin_entry->_sub;

        list_add(&new->v_list, &(now_sub_head_entry->v_list));
        list_del(origin);
        
        now_sub_head_entry = origin_entry->_sub;
        now_sub_head = now_sub_head->prev;
        prev_sub_head = prev_sub_head->prev;
        prev_sub_head_entry = list_entry(prev_sub_head, struct sub_head, h_list);
    }
    now_sub_head_entry->len--;
    if(now_sub_head_entry->len == 0)
        list_del(&now_sub_head_entry->h_list);
}


struct list_head* n_list_get(int index, struct list_head* head)
{
    struct list_head* current_sub = head->prev;
    struct sub_head* current_sub_entry = list_entry(current_sub, struct sub_head, h_list);
    
    int index_sum = 0, i;
    while(current_sub!=head)
    {
        if (current_sub_entry->len + index_sum >= index) break;
        index_sum += current_sub_entry->len;
        current_sub = current_sub->prev;
        current_sub_entry = list_entry(current_sub, struct sub_head, h_list);
    }
    
    struct list_head* current_list_head;
    current_list_head = &current_sub_entry->v_list;
    current_list_head = current_list_head->prev;
    
    for(i=0; i<index-index_sum; i++)
    {
        current_list_head = current_list_head->prev;
    }
    return current_list_head;
}

struct list_head* n_list_get_stable(int index, struct list_head* head)
{
    int h_index = (int)(index/SUB_LENGTH);
    int v_index = index-(h_index*SUB_LENGTH);
    int i;
    
    struct list_head* hp = head->prev;
    for (i=0; i<h_index; i++)
        hp = hp->prev;
    struct list_head* vp = &(list_entry(hp, struct sub_head, h_list)->v_list);
    for (i=0; i<v_index+1; i++)
        vp = vp->prev;
    return vp;
    
    /*
    struct list_head *current_list=head->prev;
    int i;
    for (i=0;i<index;i++)
    {
        current_list=current_list->prev;
    }
    return current_list;
    */
}

void init_n_list(struct list_head *head)
{
    // printk("init_n_list() called\n");
    INIT_LIST_HEAD(head);
    new_sub_head(head);
}
