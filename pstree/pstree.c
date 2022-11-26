#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

const char PATH[] = "~/proc";
const int READ_LEN = 256;

typedef struct _proc _proc;

struct _proc {
    char name[64];
    int pid;
    int ppid;
    _proc** children;
};

bool isNumeric (char* str);

int main(int argc, char *argv[]) {
    DIR* dir;
    struct dirent *entry;
    dir = opendir (PATH);

    int fd;
    char fullpath[64];
    char buf[128];
    
    int pid;
    int ppid;
    char state;
    char name[64];

    int proc_capacity = 64;
    int proc_count = 0;
    _proc* procs = malloc (sizeof(_proc) * proc_capacity);

    /* get information about processes and store it in a list */
    while ((entry = readdir (dir)) != NULL)
    {
        if (isNumeric (entry->d_name))
        {
            sprintf (fullpath, "%s/%s/stat", PATH, entry->d_name);
            fd = open (fullpath, O_RDONLY);
            read (fd, buf, READ_LEN);
            sscanf (buf, "%d (%s) %c %d", pid, name, state, ppid);
            procs[proc_count].pid = pid;
            procs[proc_count].ppid = ppid;
            strcpy (procs[proc_count].name, name);
            procs[proc_count].children = NULL;
            if (++proc_count >= proc_capacity) realloc (procs, sizeof(_proc) * (proc_capacity*=2));
        }
    }

    for (size_t i=0; i<proc_count; i++)
    {
        printf ("process name:\t%s\tpid:\t%d\tppid:\t%d", procs[i].name, procs[i].pid, procs[i].ppid);
    }

    /* find all the children for any given process */
  return 0;
}

bool isNumeric (char* str)
{
    char* ch = str;
    do {
        if (*ch < '0' || *ch > '9') return false;
    } while (*++ch != '\0');
}
