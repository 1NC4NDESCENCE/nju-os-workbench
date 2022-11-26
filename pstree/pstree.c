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

const char PATH[] = "/proc";
const int READ_LEN = 128;
const int DEFAULT_CAPACITY = 8;

typedef struct _PROC _PROC;

struct _PROC {
    int pid;
    int ppid;
    int children_count;
    int children_capacity;
    char name[64];
    _PROC** children;
};

bool isNumeric (char* str);
void printProc (_PROC* proc);

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
            if (proc->pid == proc->[j].ppid) {
                if (proc->children) {
                    proc->children[proc->children_count] = &procs[j];
                    if (++proc->children_count >= proc->children_capacity) {
                        proc->children = realloc (proc->children, sizeof(_PROC*) * (proc->children_capacity *= 2));
                    }
                } else {
                    proc->children = malloc (sizeof(_PROC*)*DEFAULT_CAPACITY);
                    proc->children_capacity = DEFAULT_CAPACITY;
                    proc->children[0] = &procs[j];
                    proc.children_count++;
                }
            }
        }
    }

    for (size_t i=0; i<proc_count; i++) {
        printf ("%s\t|", procs[i].name);
        for (size_t j=0; j<procs[i].children_count; j++) {
            printf("\t%s", procs[i].children[j]->name);
        }
        printf ("\n%d children\n", procs[i].children_count);
    }

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
