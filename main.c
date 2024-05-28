#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <libnotify/notify.h>
#include <string.h>

#define BUFSIZE 128

int OCCUPIED_RAM_THRESHOLD = 10;
int POLL_FREQUENCY = 2;
//char *PROCEESES_TO_KILL[50] = {"sublime","thunar"};
char PROCEESES_TO_KILL[50][20] = {0};
char *KNOWN_CONFIG_OPTIONS[3] = {"OCCUPIED_RAM_THRESHOLD", "POLL_FREQUENCY", "PROCEESES_TO_KILL"};


bool NOTIFICATION_SHOWN = false;

bool check_if_param_in_array(char *string) {
    int len = sizeof(KNOWN_CONFIG_OPTIONS)/sizeof(KNOWN_CONFIG_OPTIONS[0]);
    int i;

    for(i = 0; i < len; ++i)
    {
        printf("%i\n", i);
        if (strcmp(KNOWN_CONFIG_OPTIONS[i], string) == 0)
        {
            return true;
        }
    }
    return false;
}

int count_char_occurrences(char *string, int character) {
    int count = 0;
    char *ptr = strchr(string, character);

    while(ptr != NULL) {
        ptr = strchr(ptr + 1, character);
        count++;
    }

    return count;
}

void get_config(void) {
    char *filename = "/.config/alram.conf";
    char *home = getenv("HOME");
    char delim='=';
    char delimPtr[]="=";

    strncat(home, filename, 21);
    printf("%s\n", home);

    FILE *file;
    char buffer[256];

    if ((file = fopen(home, "r")) == NULL) {
        printf("Error opening config file!\n");
        return;
    }

    while (fgets (buffer, sizeof (buffer), file) != NULL) {

        if (buffer[0] != '\n' && buffer[0] != '#') {
            printf("processing line\n");
            printf("%s\n", buffer);

            if (count_char_occurrences(buffer, delim) != 1) {
                printf("Number of parameters is wrong\n");
                continue;
            }

            char *name;
            char *value;

            name = strtok(buffer, delimPtr);
            value = strtok(NULL, delimPtr);

            if (!check_if_param_in_array(name)) {
                printf("Unrecognized parameter from config %s\n", name);
                continue;
            }

            if (strcmp(name, "OCCUPIED_RAM_THRESHOLD") == 0) {
                OCCUPIED_RAM_THRESHOLD = atoi(value);
            }
            else if (strcmp(name, "POLL_FREQUENCY") == 0) {
                POLL_FREQUENCY = atoi(value);
            }
            else if (strcmp(name, "PROCEESES_TO_KILL") == 0) {
//                PROCEESES_TO_KILL = value;
                char allApps[500];
                int n = sscanf(value, "[%99[^]]]", allApps);
                if (n > 0) {
                    printf("%s\n", allApps);
                    printf("Number of apps to kill %i\n", count_char_occurrences(allApps, ',') + 1);
//                    char **patr = PROCEESES_TO_KILL;
//                    patr[2] = "Sander";
                    int number_of_apps = count_char_occurrences(allApps, ',') + 1;
                    printf("%s\n", allApps);
                    char *token;
                    token = strtok(allApps, ",");

                    for (int i = 0; i < number_of_apps; i++) {
                        printf("looploop: %i\n", i);
                        char *killApp;
                        sscanf(token, "\"%[^\"]\"", killApp);
                        printf("token: %s\n", killApp);
                        strcpy(PROCEESES_TO_KILL[i], killApp);

//                        PROCEESES_TO_KILL[i] = killApp;
                        token = strtok(NULL, ",");
                    }
//                    PROCEESES_TO_KILL[2] = "newkill";
//                    printf("What to kill %s\n" , PROCEESES_TO_KILL[2]);
                }
                }

        }
        }

    printf("OCCUPIED_RAM_THRESHOLD %i:\n", OCCUPIED_RAM_THRESHOLD);
    printf("POLL_FREQUENCY %i:\n", POLL_FREQUENCY);

    for (int u=0;u < (sizeof (PROCEESES_TO_KILL) /sizeof (PROCEESES_TO_KILL[0]));u++) {
        if (strcmp(PROCEESES_TO_KILL[u], "") == 0) {
            break;
        }
        printf("Process added to kill list %s\n", PROCEESES_TO_KILL[u]);
    }
    }


unsigned long long get_available_memory(void)
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


unsigned long long get_total_system_memory(void)
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

unsigned get_free_memory_percentage(void) {
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

    get_config();

    char free_memory_percentage[12];

    while (true) {

        sprintf(free_memory_percentage, "%u", get_free_memory_percentage());
        printf("%s\n", free_memory_percentage);

        if (get_free_memory_percentage() <= OCCUPIED_RAM_THRESHOLD && !NOTIFICATION_SHOWN) {
            show_notification("Occupied RAM threshold exceeded", "If you defined killlist, I will attempt to kill some processes");
            NOTIFICATION_SHOWN = true;

            for (size_t i = 0; i < sizeof(PROCEESES_TO_KILL) / sizeof(PROCEESES_TO_KILL[0]); i++)
            {
                if (strcmp(PROCEESES_TO_KILL[i], "") == 0) {
                    break;
                }
                int processToKill = get_process_pid(PROCEESES_TO_KILL[i]);
                if (processToKill == -1) {
                    return 1;
                } else if (processToKill != 0) {
//                    int kill_result = kill_process(processToKill);
//                    if (kill_result == -1) {
//                        return 1;
//                    }
                }
            }


        } else if (NOTIFICATION_SHOWN && get_free_memory_percentage() >= OCCUPIED_RAM_THRESHOLD) {
            // Once we recover, we can start monitoring again
            NOTIFICATION_SHOWN = false;
        }

        sleep(POLL_FREQUENCY);
    }

}
