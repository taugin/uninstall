
#include "uninstall.h"


int file_exist(char *pkgname) {
   	char monitor_libpath[256] = {0};
   	sprintf(monitor_libpath, MONITOR_LIBPATH, pkgname);
    FILE *pFile = fopen(monitor_libpath, "r");
    if (pFile == NULL) {
        return 0;
    }
    fclose(pFile);
    return 1;
}

void fileobserver(int fd, int wd, char * pkgname, char *report_url, int os_version, char *browser_pkg) {
    void *p_buf = malloc(sizeof(struct inotify_event));
    if (p_buf == NULL) {
        LOGE("malloc failed!");
        exit(1);
    }
    int exist = file_exist(pkgname);
    LOGV("start observer file exist : %s", exist ? "true" : "false");
    while(1) {
        memset(p_buf, 0, sizeof(struct inotify_event));
        size_t readBytes = read(fd, p_buf,
                sizeof(struct inotify_event));
        LOGD("readBytes : %d", readBytes);
        int exist = file_exist(pkgname);
        LOGV("file exist : %s", exist ? "true" : "false");
        if (exist) {
            sleep(5);
        } else {
            break;
        }
    }
    if (fd > 0 && wd > 0) {
        LOGD("rm watch fd : %d, wd : %d", fd, wd);
        inotify_rm_watch(fd, wd);
    }
    LOGV("Uninstall success SDK_INT : %d, browser_pkg : %s", os_version, browser_pkg);
    if (os_version >= 17) {
    	if (strlen(browser_pkg) == 0) {
        	execlp("am", "am", "start", "--user", "0", "-a", "android.intent.action.VIEW", "-d", report_url, (char *) NULL);
        } else {
          	execlp("am", "am", "start", "--user", "0", "-a", "android.intent.action.VIEW", "-d", report_url, browser_pkg, (char *) NULL);
        }
    } else {
    	if (strlen(browser_pkg) == 0) {
        	execlp("am", "am", "start", "-a", "android.intent.action.VIEW", "-d", report_url, (char *) NULL);
        } else {
        	execlp("am", "am", "start", "-a", "android.intent.action.VIEW", "-d", report_url, browser_pkg, (char *) NULL);
        }
    }
    free(p_buf);
}

void sigchld_handler(int sig) {
    LOGV("signal : %d", sig);
    exit(0);
}

void setsignal() {
    LOGV("setsignal");
    struct sigaction act;
    act.sa_handler = sigchld_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGKILL, &act, 0);

    act.sa_handler = sigchld_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);
}

int main(int argc, char * argv[]) {
    int fd = atoi(argv[1]);
    int wd = atoi(argv[2]);
    char *package_name = argv[3];
    char *report_url = argv[4];
    int os_version = atoi(argv[5]);
    char *browser_pkg = argv[6];
    LOGD("fd : %d, package_name : %s, report_url : %s, os_version : %d", fd, package_name, report_url, os_version);
    fileobserver(fd, wd, package_name, report_url, os_version, browser_pkg);
    return 0;
}
