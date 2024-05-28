#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <glib-2.0/gio/gio.h>
#include <string.h>

#define BUFSIZE 128

struct configStruct {
    int OCCUPIED_RAM_THRESHOLD;
    int POLL_FREQUENCY;
    char PROCEESES_TO_KILL[50][20];
};

struct configStruct config = {
        .OCCUPIED_RAM_THRESHOLD = 10, // In percentage of total ram
        .POLL_FREQUENCY = 2,  // In seconds
        .PROCEESES_TO_KILL = {0}  // From config file

};

const char KNOWN_CONFIG_OPTIONS[3][30] = {"OCCUPIED_RAM_THRESHOLD", "POLL_FREQUENCY", "PROCEESES_TO_KILL"};
bool NOTIFICATION_SHOWN = false;

bool check_if_param_in_array(char *string) {

    for(size_t i = 0; i < sizeof(KNOWN_CONFIG_OPTIONS)/sizeof(KNOWN_CONFIG_OPTIONS[0]); ++i)
    {
        if (strcmp(KNOWN_CONFIG_OPTIONS[i], string) == 0)
        {
            return true;
        }
    }
    return false;
}

int count_char_occurrences(char string[], int character) {
    int count = 0;
    char *ptr = strchr(string, character);

    while((ptr = strchr(ptr, character)) != NULL) {
        count++;
        ptr++;

    }

    return count;
}

void get_config(void) {

    char *filename = "/.config/alram.conf";
    char *home = getenv("HOME");
    char delim='=';
    char delimStr[]="=";

    strncat(home, filename, 21);
    printf("%s\n", home);

    FILE *file;
    char buffer[256];
    file = fopen(home, "r");

    if (file == NULL) {
        perror("Error opening config file!\n");
        return;
    }

    while (fgets (buffer, 256, file)) {

        if (buffer[0] != '\n' && buffer[0] != '#') {

            if (count_char_occurrences(buffer, delim) != 1) {
                printf("Number of parameters is wrong\n");
                continue;
            }

            char *name;
            char *value;

            name = strtok(buffer, delimStr);
            value = strtok(NULL, delimStr);

            if (!check_if_param_in_array(name)) {
                printf("Unrecognized parameter from config: %s\n", name);
                continue;
            }

            if (strcmp(name, "OCCUPIED_RAM_THRESHOLD") == 0) {
                config.OCCUPIED_RAM_THRESHOLD = atoi(value);
            } else if (strcmp(name, "POLL_FREQUENCY") == 0) {
                config.POLL_FREQUENCY = atoi(value);
            }
            else if (strcmp(name, "PROCEESES_TO_KILL") == 0) {
                char allApps[500];
                int n = sscanf(value, "[%99[^]]]", allApps);

                if (n > 0) {
                    printf("All apps found is %s\n", allApps);
                    printf("Number of apps to kill: %i\n", count_char_occurrences(allApps, ',') + 1);

                    int number_of_apps = count_char_occurrences(allApps, ',') + 1;
                    char *token;
                    token = strtok(allApps, ",");

                    for (int i = 0; i < number_of_apps; i++) {
                        char *killApp;
                        sscanf(token, "\"%[^\"]\"", killApp);
                        strcpy(config.PROCEESES_TO_KILL[i], killApp);
                        token = strtok(NULL, ",");
                    }
                }
                }
        }

        }
        fclose(file);

    printf("OCCUPIED_RAM_THRESHOLD: %i\n", config.OCCUPIED_RAM_THRESHOLD);
    printf("POLL_FREQUENCY: %i\n", config.POLL_FREQUENCY);
//
    for (int u=0;u < (sizeof (config.PROCEESES_TO_KILL) /sizeof (config.PROCEESES_TO_KILL[0]));u++) {
        if (strcmp(config.PROCEESES_TO_KILL[u], "") == 0) {
            break;
        }
        printf("Added process to kill list: %s\n", config.PROCEESES_TO_KILL[u]);
    }
}



unsigned long long get_available_memory(void)
{
FILE *file;
// We need to use meminfo, because _SC_AVPHYS_PAGES is a lie
char filename[] = "/proc/meminfo";
unsigned long mem_free = 0;

if ((file = fopen (filename, "r")) != NULL)
{
    char buffer[256];
    while (fgets (buffer, sizeof (buffer), file) != NULL) {
        if (sscanf(buffer, "MemAvailable:\t%lu kB", &mem_free) != 0) {
            break;
        }
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

void show_notification(char summary[], char body[], GApplication *application) {
    GNotification *notification = g_notification_new (summary);
    g_notification_set_body (notification, body);
    GIcon *icon = g_themed_icon_new ("dialog-warning");
    g_notification_set_icon (notification, icon);
    g_application_send_notification (application, NULL, notification);
    g_object_unref (icon);
    g_object_unref (notification);
//    g_object_unref (application);
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
    // Initialize in main, otherwise segfault
    // Use G_APPLICATION_NON_UNIQUE , otherwise malloc(): unaligned fastbin chunk detected
    GApplication *application = g_application_new("org.alram", G_APPLICATION_NON_UNIQUE);
    g_application_register(application, NULL, NULL);

    get_config();

    unsigned free_memory_percentage;

    while (true) {

        free_memory_percentage = get_free_memory_percentage();
        printf("%u\n", free_memory_percentage);

        if (get_free_memory_percentage() <= config.OCCUPIED_RAM_THRESHOLD && !NOTIFICATION_SHOWN) {

            show_notification("Occupied RAM threshold exceeded", "If you defined kill list, I will attempt to kill some processes", application);
            NOTIFICATION_SHOWN = true;

            for (size_t a = 0; a < sizeof(config.PROCEESES_TO_KILL) / sizeof(config.PROCEESES_TO_KILL[0]); a++)
            {
                if (strcmp(config.PROCEESES_TO_KILL[a], "") == 0) {
                    break;
                }

                int processToKill = get_process_pid(config.PROCEESES_TO_KILL[a]);
                if (processToKill == -1) {
                    return 1;
                } else if (processToKill != 0) {
//                    int kill_result = kill_process(processToKill);
//                    if (kill_result == -1) {
//                        return 1;
//                    }
                }
            }


        } else if (NOTIFICATION_SHOWN && get_free_memory_percentage() >= config.OCCUPIED_RAM_THRESHOLD) {
            // Once we recover, we can start monitoring again
            NOTIFICATION_SHOWN = false;
        }

        sleep(config.POLL_FREQUENCY);
    }

}
