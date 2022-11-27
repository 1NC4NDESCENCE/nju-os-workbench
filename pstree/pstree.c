#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define TYPE _level_info

const char PATH[] = "/proc";
const int READ_LEN = 128;
const int CHILDREN_CAPACITY = 8;
const int STACK_CAPACITY = 64;

typedef struct _PROC _PROC;

struct _PROC {
    int pid;
    int ppid;
    int children_count;
    int children_capacity;
    char name[64];
    _PROC** children;
};

typedef struct STACK {
    void** entries;
    int capacity;
    int size;
} STACK;

typedef struct _level_info {
    int depth;
    bool vertical_line;
} _level_info;

bool isNumeric (char* str);
void print_proc (_PROC* proc, bool curly, bool root);
void push (STACK* stack, void* entry);
void* pop (STACK* stack);
void print_prefix (STACK* stack);

int main(int argc, char *argv[]) {
    DIR* dir;
    struct dirent *entry;
    dir = opendir (PATH);

    int fd;
    char fullpath[320];
    char buf[128];
    
    int pid;
    int ppid;
    char state;
    char name[64];

    int proc_capacity = 64;
    int proc_count = 0;
    _PROC* procs = malloc (sizeof(_PROC) * proc_capacity);

    /* get information about processes and store it in a list */
    while ((entry = readdir (dir)) != NULL)
    {
        if (isNumeric (entry->d_name))
        {
            sprintf (fullpath, "%s/%s/stat", PATH, entry->d_name);
            fd = open (fullpath, O_RDONLY);
            read (fd, buf, READ_LEN);
            sscanf (buf, "%d (%s %c %d", &pid, name, &state, &ppid);
            procs[proc_count].pid = pid;
            procs[proc_count].ppid = ppid;
            procs[proc_count].children = NULL;
            procs[proc_count].children_count = 0;
            procs[proc_count].children_capacity = 0;
            name[strlen(name)-1] = 0;
            strcpy (procs[proc_count].name, name);
            if (++proc_count >= proc_capacity) procs = realloc (procs, sizeof(_PROC) * (proc_capacity*=2));
        }
    }

    /* find all the children for any given process */
    for (size_t i=0; i<proc_count; i++) {
        _PROC* proc = &procs[i];
        for (size_t j=0; j<proc_count; j++) {
            if (proc->pid == procs[j].ppid) {
                if (proc->children) {
                    proc->children[proc->children_count] = &procs[j];
                    if (++proc->children_count >= proc->children_capacity) {
                        proc->children = realloc (proc->children, sizeof(_PROC*) * (proc->children_capacity *= 2));
                    }
                } else {
                    proc->children = malloc (sizeof(_PROC*)*CHILDREN_CAPACITY);
                    proc->children_capacity = CHILDREN_CAPACITY;
                    proc->children[0] = &procs[j];
                    proc->children_count++;
                }
            }
        }
    }

    assert (procs[0].pid == 1 && procs[0].ppid == 0);
    print_proc (&procs[0], false, true);
    
    for (size_t i=0; i<proc_count; i++) {
        free (procs[i].children);
    }
    free (procs);
    return 0;
}

bool isNumeric (char* str)
{
    char* ch = str;
    do {
        if (*ch < '0' || *ch > '9') return false;
    } while (*++ch != '\0');
    return true;
}


void print_proc (_PROC* proc, bool curly, bool root)
{
    static STACK indent_depths = {NULL, STACK_CAPACITY, 0};
    if (root) {
        printf ("%s", proc->name);
        indent_depths.entries = malloc (sizeof(TYPE)*indent_depths.capacity);
    } else if (curly) {
        print_prefix (&indent_depths);
        printf (" └-%s", proc->name);
    } else {
        printf ("---%s", proc->name);
    }
    if (proc->children_count) {
        TYPE* level_info_p = malloc (sizeof(TYPE));
        level_info_p->depth = strlen (proc->name);
        level_info_p->vertical_line = proc->children_count > 1;
        push (&indent_depths, level_info_p);
        for (size_t i=0; i<proc->children_count; i++) {
            if (i+1 == proc->children_count) level_info_p->vertical_line = false;
            if (i) {
                print_proc (proc->children[i], true, false);
            } else {
                print_proc (proc->children[i], false, false);
            }
        }
        pop (&indent_depths);
    } else {
        printf ("\n");
    }
    if (root) {
        for (size_t i=0; i<indent_depths.size; i++) {
            free (indent_depths.entries[i]);
        }
        indent_depths.size = 0;
        indent_depths.capacity = STACK_CAPACITY;
        free (indent_depths.entries);
    }
}

void push (STACK* stack, void* entry) {
    if (stack->size + 1 >= stack->capacity) {
        stack->entries = realloc (stack->entries, sizeof (TYPE) * (stack->capacity *= 2));
    }
    stack->entries[stack->size++] = entry;
}

void* pop (STACK* stack) {
    if (!(stack->size--)) perror ("popping empty stack\n");
    return stack->entries[stack->size];
}

void print_prefix (STACK* stack) {
    for (size_t i=0; i<stack->size; i++) {
        TYPE* entry = stack->entries[i];
        printf ("%*s", entry->depth, "");
        if (i+1 == stack->size) break;
        if (entry->vertical_line) {
            printf (" | ");
        } else {
            printf ("   ");
        }
    }
}

