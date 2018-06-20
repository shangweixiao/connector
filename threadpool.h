#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

typedef void* tp_work_desc;
typedef void* (*tp_work)(void *);


//modify 5/18/2018 18:47:36 5/18/2018 18:57:41
int threadpool_init();
int threadpool_uninit();
int threadpool_add_task(IN tp_work worker,IN tp_work_desc param);
#endif // __THREADPOOL_H__