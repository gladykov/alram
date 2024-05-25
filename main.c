#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <libnotify/notify.h>

#define BUFSIZE 128

int OCCUPIED_RAM_THRESHOLD = 63;
bool NOTIFICATION_SHOWN = false;
int POLL_FREQUENCY = 2;

char *PROCEESES_TO_KILL[] = {"sublime","thunar"};

unsigned long long get_available_memory ()
{
FILE *file;
// We need to use meminfo, because _SC_AVPHYS_PAGES is a lie
char *filename = "/proc/meminfo";
unsigned long mem_free = 0;

if ((file = fopen (filename, "r")) != NULL)
{
    int found = 0;
    char buffer[256];
    while (found == 0 && fgets (buffer, sizeof (buffer), file) != NULL) {
        found += !mem_free ? sscanf(buffer, "MemAvailable:\t%lu kB", &mem_free) : 0;
    }
    fclose(file);
}

return mem_free * 1000;
}


unsigned long long get_total_system_memory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

unsigned get_free_memory_percentage() {
    return (get_available_memory() / 1.0 / get_total_system_memory()) * 100;
}

void show_notification(char summary[], char body[]) {
    notify_init("Alram");
    NotifyNotification * Hello = notify_notification_new (summary, body, "dialog-warning");
    notify_notification_show (Hello, NULL);
    g_object_unref(G_OBJECT(Hello));
    notify_uninit();
}

int kill_process(int pid){
    printf("Attempting to kill pid of application %i\n", pid);

    char cmd[36] = "kill -INT ";
    char pidChar[10];
    sprintf(pidChar, "%d", pid);
    strncat(cmd, pidChar, 20);

    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    if (pclose(fp)) {
        printf("Process not found\n");
        return 0;
    }
    return 0;
}

int get_process_pid(char name[]){
    printf("Looking for pid of application %s\n", name);

    char cmd[36] = "pgrep --oldest ";
    strncat(cmd, name, 20);

    char buf[BUFSIZE] = {0};
    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    if (fgets(buf, BUFSIZE, fp) != NULL) {
        return atoi(buf);
    }

    if (pclose(fp)) {
        return 0;
    }
    return -1;
}

int main(void) {

    char free_memory_percentage[12];

    while (true) {

        sprintf(free_memory_percentage, "%u", get_free_memory_percentage());
        printf("%s\n", free_memory_percentage);

        if (get_free_memory_percentage() <= OCCUPIED_RAM_THRESHOLD && !NOTIFICATION_SHOWN) {
            show_notification("Occupied RAM exceeded", "Your system has lower amount of ram than threshold");
            NOTIFICATION_SHOWN = true;

            for (size_t i = 0; i < sizeof(PROCEESES_TO_KILL) / sizeof(PROCEESES_TO_KILL[0]); i++)
            {
                int processToKill = get_process_pid(PROCEESES_TO_KILL[i]);
                if (processToKill == -1) {
                    return 1;
                } else if (processToKill != 0) {
                    int kill_result = kill_process(processToKill);
                    if (kill_result == -1) {
                        return 1;
                    }
                }
            }


        } else if (NOTIFICATION_SHOWN && get_free_memory_percentage() >= OCCUPIED_RAM_THRESHOLD) {
            // Once we recover, we can start monitoring again
            NOTIFICATION_SHOWN = false;
        }

        sleep(POLL_FREQUENCY);
    }

}
