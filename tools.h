#ifndef __TOOLS_H__
#define __TOOLS_H__

#define TOOLS_MAX_INTERFACES 16


// modify bug1  5/18/2018 10:25:21
// modify bug1  5/18/2018 10:27:02
// fixed bug1  5/18/2018 10:28:025/18/2018 19:04:315/18/2018 19:04:31
int tools_init();
int tools_uninit();

void *tools_malloc(IN int size);
void *tools_realloc(IN void *buf,IN int new_size);
void tools_free(IN void *buf);

unsigned int tools_BKDRHash(IN const char *str);

#endif
