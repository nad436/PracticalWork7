#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_SHELLS 128
#define MAX_PATH 1024

char valid_shells[MAX_SHELLS][MAX_PATH];
int shell_count = 0;

void load_standard_shells() {
    FILE *fp = fopen("/etc/shells", "r");
    if (!fp) return;

    char line[MAX_PATH];
    while (fgets(line, sizeof(line), fp) && shell_count < MAX_SHELLS) {
        if (line[0] == '#') continue;
        
        size_t len = strlen(line);
        while (len > 0 && isspace((unsigned char)line[len - 1])) {
            line[--len] = '\0';
        }
        
        if (len > 0) {
            strcpy(valid_shells[shell_count++], line);
        }
    }
    fclose(fp);
}

int is_standard_shell(const char *path) {
    for (int i = 0; i < shell_count; i++) {
        if (strcmp(valid_shells[i], path) == 0) {
            return 1;
        }
    }
    return 0;
}

int is_shell_name(const char *path) {
    size_t len = strlen(path);
    if (len >= 2 && strcmp(path + len - 2, "sh") == 0) return 1;
    if (strstr(path, "/sh") || strstr(path, "/bash") || strstr(path, "/zsh") || 
        strstr(path, "/ksh") || strstr(path, "/csh") || strstr(path, "/tcsh")) {
        return 1;
    }
    return 0;
}

void check_process(const char *pid) {
    char exe_link[MAX_PATH];
    char exe_path[MAX_PATH];
    snprintf(exe_link, sizeof(exe_link), "/proc/%s/exe", pid);

    ssize_t len = readlink(exe_link, exe_path, sizeof(exe_path) - 1);
    if (len == -1) return;
    exe_path[len] = '\0';

    if (is_shell_name(exe_path) && !is_standard_shell(exe_path)) {
        char cmd_path[MAX_PATH];
        char cmd_name[MAX_PATH] = "Unknown";
        snprintf(cmd_path, sizeof(cmd_path), "/proc/%s/comm", pid);
        
        FILE *fp = fopen(cmd_path, "r");
        if (fp) {
            if (fgets(cmd_name, sizeof(cmd_name), fp)) {
                size_t clen = strlen(cmd_name);
                if (clen > 0 && cmd_name[clen - 1] == '\n') cmd_name[clen - 1] = '\0';
            }
            fclose(fp);
        }
        printf("PID: %5s | Name: %-15s | Path: %s\n", pid, cmd_name, exe_path);
    }
}

int main() {
    load_standard_shells();

    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Failed to open /proc");
        return 1;
    }

    printf("Processes running from non-standard shells:\n");
    printf("--------------------------------------------------\n");

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        int is_pid = 1;
        for (int i = 0; entry->d_name[i] != '\0'; i++) {
            if (!isdigit((unsigned char)entry->d_name[i])) {
                is_pid = 0;
                break;
            }
        }
        if (is_pid) {
            check_process(entry->d_name);
        }
    }

    closedir(dir);
    return 0;
}
