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

typedef int (*CMP_FUNC)(void*, void*);

bool isNumeric (char* str);
void print_proc (_PROC* proc, bool curly, bool root, bool has_more_than_one_child);
void push (STACK* stack, void* entry);
void* pop (STACK* stack);
void print_prefix (STACK* stack);
int cmp_int (const void* a, const void* b);

bool show_pid = false;
bool sort = false;
bool version = false;

int main(int argc, char *argv[]) {
    bool invalid = false;
    char arg[64];
    for (size_t i=1; i<argc; i++) {
        if (strlen(argv[i]) < 2) {
            invalid = true;
            strcpy (arg, argv[i]);
        }
        else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
            printf ("Knockoff pstree v1.0 by Xueyuan Jiao\n");
            return 0;
        }
        else if (argv[i][0] == '-') {
            if (argv[i][1] == '-') {
                if (!strcmp(argv[i], "--show-pids")) {
                    show_pid = true;
                } else if (!strcmp(argv[i], "--sort")) {
                    sort = true;
                } else if (!strcmp(argv[i], "--version")) {
                    version = true;
                } else {
                    invalid = true;
                    strcpy (arg, argv[i]);
                }
            } else {
                for (size_t j=1; j<strlen(argv[i]); j++) {
                    if (argv[i][j] == 'p') {
                        show_pid = true;
                    } else if (argv[i][j] == 's') {
                        sort = true;
                    } else if (argv[i][j] == 'V') {
                        version = true;
                    } else {
                        invalid = true;
                        strcpy (arg, argv[i]);
                    }
                }
            }
        } else {
            invalid = true;
            strcpy (arg, argv[i]);
        } 
    }

    if (invalid) {
        fprintf (stderr, "unknown argument: %s\n", arg);
        printf ("Usage:\n");
        printf ("--show-pids or -p to display pid for every process\n");
        printf ("--sort or -s to sort the children of every process by their pid\n");
        printf ("--version or -V to display version information\n");
        return 1;
    }
    
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

    if (sort) {
        for (size_t i=0; i<proc_count; i++) {
            if (procs[i].children_count > 1);
            qsort (procs[i].children, procs[i].children_count, sizeof (_PROC*), &cmp_int);
        }
    }

    assert (procs[0].pid == 1 && procs[0].ppid == 0);
    print_proc (procs, false, true, false);
    
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


void print_proc (_PROC* proc, bool curly, bool root, bool has_more_than_one_child)
{
    static STACK indent_depths = {NULL, STACK_CAPACITY, 0};
    if (root) {
        indent_depths.entries = malloc (sizeof(TYPE)*indent_depths.capacity);
    } else if (curly) {
        print_prefix (&indent_depths);
    } else if (has_more_than_one_child) {
        printf ("?????????");
    } else {
        printf ("?????????");
    }
    char name_str[128];
    if (show_pid) {
        sprintf (name_str, "%s(%d)", proc->name, proc->pid);
    } else {
        sprintf (name_str, "%s", proc->name);
    }
    printf ("%s", name_str);
    if (proc->children_count) {
        TYPE* level_info_p = malloc (sizeof(TYPE));
        level_info_p->depth = strlen (name_str);
        level_info_p->vertical_line = proc->children_count > 1;
        push (&indent_depths, level_info_p);
        for (size_t i=0; i<proc->children_count; i++) {
            if (i+1 == proc->children_count) level_info_p->vertical_line = false;
            if (i) {
                print_proc (proc->children[i], true, false, proc->children_count > 1);
            } else {
                print_proc (proc->children[i], false, false, proc->children_count > 1);
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
        if (i+1 == stack->size) {
            if (entry->vertical_line) {
                printf (" ??????");
            } else {
                printf (" ??????");
            }
            break;
        }
        if (entry->vertical_line) {
            printf (" ??? ");
        } else {
            printf ("   ");
        }
    }
}

int cmp_int (const void* a, const void* b) { return ((_PROC*)a)->pid - ((_PROC*)b)->pid; }

