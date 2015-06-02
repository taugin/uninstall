#include <assert.h>
#include "uninstall.h"

#ifdef __cplusplus
extern "C" {
#endif

void observer(char *pname, int fd, int wd, char *pkgname, char *report_url, int os_version, char * browser_pkg);

JNIEXPORT jint JNICALL native_init(JNIEnv *env, jobject obj) {
    jint notify_fd = inotify_init();
    LOGD("notify_fd : %d", notify_fd);
    return notify_fd;
}

JNIEXPORT void JNICALL native_killprocess(JNIEnv *env, jobject obj, jint pid) {
    LOGD("kill pid : %d", pid);
    int cur_pid = getpid();
    if (pid == cur_pid) {
        return;
    }
    if (pid == 0) {
        return;
    }
    kill(pid, 9);
}

JNIEXPORT void JNICALL native_observe(JNIEnv *env, jobject obj,
        jint fd, jint wd, jstring packageName, jstring reportUrl, jint osVersion, jstring processName, jstring browser) {
    char pname[256];
    const char *tmp1 = (*env)->GetStringUTFChars(env, processName, 0);
    strcpy(pname, tmp1);
    (*env)->ReleaseStringUTFChars(env, processName, tmp1);

    char pkgname[256];
    const char *tmp2 = (*env)->GetStringUTFChars(env, packageName, 0);
    strcpy(pkgname, tmp2);
    (*env)->ReleaseStringUTFChars(env, packageName, tmp2);

    char report_url[256];
    const char *tmp3 = (*env)->GetStringUTFChars(env, reportUrl, 0);
    strcpy(report_url, tmp3);
    (*env)->ReleaseStringUTFChars(env, reportUrl, tmp3);
    
    char browser_pkg[256];
    const char *tmp4 = (*env)->GetStringUTFChars(env, browser, 0);
    strcpy(browser_pkg, tmp4);
    (*env)->ReleaseStringUTFChars(env, browser, tmp4);

    observer(pname, fd, wd, pkgname, report_url, osVersion, browser_pkg);
}

JNIEXPORT jint JNICALL native_startwatching(JNIEnv *env, jobject obj, jint fd, jstring packageName) {
    char pkgname[256];
    const char *tmp2 = (*env)->GetStringUTFChars(env, packageName, 0);
    strcpy(pkgname, tmp2);
    (*env)->ReleaseStringUTFChars(env, packageName, tmp2);

	char monitor_dir[256] = {0};
	sprintf(monitor_dir, MONITOR_LIBPATH, pkgname);
	LOGD("fd : %d, monitor_dir : %s", fd, monitor_dir);
    return inotify_add_watch(fd, monitor_dir, IN_DELETE);
}

JNIEXPORT void JNICALL native_stopwatching(JNIEnv *env, jobject obj, jint fd, jint wd) {
    LOGD("fd : %d, wd : %d", fd, wd);
    inotify_rm_watch(fd, wd);
}

void sigchld_handler(int sig) {
   pid_t pid;
   int status;
   while((pid =waitpid(-1, &status, WNOHANG)) >0) {
       LOGV("child %d died, exit code : %d", pid, WEXITSTATUS(status));
       break;
   }
}

void setsignal() {
    struct sigaction act;
    act.sa_handler = sigchld_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, 0);
}

void observer(char *pname, int fd, int wd, char *pkgname, char *report_url, int os_version, char *browser_pkg) {
    char buf_fd[16];
    memset(buf_fd, 0, sizeof(buf_fd));
    sprintf(buf_fd, "%d", fd);

    char buf_wd[16];
    memset(buf_wd, 0, sizeof(buf_wd));
    sprintf(buf_wd, "%d", wd);

    char buf_os[16];
    memset(buf_os, 0, sizeof(buf_os));
    sprintf(buf_os, "%d", os_version);

    char *argv[] = {pname, buf_fd, buf_wd, pkgname, report_url, buf_os, browser_pkg, NULL};

    // Set SIGCHLD signal
    // setsignal();

    int pid = fork();
    if (pid == 0) {
    	char monitor_path[256] = {0};
    	sprintf(monitor_path, MONITOR_PATH, pkgname);
        LOGD("start monitor process pid : %d, monitor_path : %s", getpid(), monitor_path);
        execvp(monitor_path, argv);
        LOGE("execvp fail!");
    }
}

static JNINativeMethod gMethods[] = {
	{ "init", 			"()I", (void*)native_init },
	{ "killProcess", 	"(I)V", (void*)native_killprocess },
	{ "observe", 		"(IILjava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;)V", (void*)native_observe },
	{ "startWatching", 	"(ILjava/lang/String;)I", (void*)native_startwatching },
	{ "stopWatching", 	"(II)V", (void*)native_stopwatching }
};

/*
* Register several native methods for one class.
*/
static int registerNativeMethods(JNIEnv* env, const char* className,
        JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;
	clazz = (*env)->FindClass(env, className);
	if (clazz == NULL) {
		LOGD("clazz == NULL");
		return JNI_FALSE;
	}
	if ((*env)->RegisterNatives(env, clazz, gMethods, numMethods) < 0) {
		LOGD("RegisterNatives result < 0");
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

/*
* Register native methods for all classes we know about.
*/
static int registerNatives(JNIEnv* env)
{
	if (!registerNativeMethods(env, JNIREG_CLASS, gMethods, sizeof(gMethods) / sizeof(gMethods[0]))) {
		LOGD("RegisterNatives result < 0");
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = NULL;
	jint result = -1;

	if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOGD("GetEnv != JNI_OK");
		return -1;
	}
	assert(env != NULL);

	if (!registerNatives(env)) {//注册
		LOGD("registerNatives Failure");
		return -1;
	}
	/* success -- return valid version number */
	result = JNI_VERSION_1_4;
	LOGD("Native function register success");
	return result;
}

#ifdef __cplusplus
}
#endif
