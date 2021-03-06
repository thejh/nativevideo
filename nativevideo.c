#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

extern char **environ;

//REPLACE WITH YOUR SECRET; MUST BE THE SAME AS IN THE USERSCRIPT!
#define SECRET "xxxxxxxx"

#define DEHEXCHR(x) ((x)-'a')

char *dehex(char *str) {
  int len = strlen(str);
  if (len%2 != 0) {
    fprintf(stderr, "hex string with bad length!\n");
    exit(1);
  }
  len /= 2;
  char *target = malloc(len+1);
  int i;
  for (i=0; i<len; i++) {
    target[i] = (DEHEXCHR(str[2*i+0])<<4) + DEHEXCHR(str[2*i+1]);
  }
  return target;
}

int main(int argc, char *argv[]) {
  uid_t uid = geteuid();
  setreuid(uid, uid);
  
  char *reqpath = getenv("QUERY_STRING");
  if (reqpath == NULL) return 1;

  if (argc == 0) return 1; /* dafuq? */
  if (access("/dev/audio", W_OK)) {
    // start ourselves with full group privileges
    *environ = NULL;
    setenv("QUERY_STRING", reqpath, 0);
    execl("/usr/bin/sg", "sg", "audio", "-c", argv[0], NULL);
    return 1;
  }

  if (strstr(reqpath, SECRET) == NULL) return 1;
  char *vidurl = strchr(reqpath, '-');
  if (vidurl == NULL) return 1;
  vidurl++;
  int aonly = 0;
  if (vidurl[0] == 'x') {
    vidurl = dehex(vidurl+1);
  }
  if (vidurl[0] == '!') { aonly = 1; vidurl++; }
  if (strncmp(vidurl, "http://", 7) && strncmp(vidurl, "https://", 8)) return 1;
  environ = NULL;
  if (!fork()) {
    //int logfd = open("/home/jann/tmp/faillog", O_RDWR);
    int nullfd = open("/dev/null", O_RDWR);
    dup2(nullfd, 0);
    dup2(nullfd, 1);
    dup2(nullfd, 2);
    close(nullfd);
    //close(logfd);
    if (setgid(getegid())) return 1;
    if (setuid(geteuid())) return 1;
    setenv("PATH", "/usr/bin:/bin", 1);
    setenv("DISPLAY", ":0", 1);
    setenv("HOME", "/home/jann", 1);
    system("killall mplayer");
    if (aonly) {
      // yes, this is complicated, but necessary to make mplayer stop
      // decoding the video
      int pipeA[2];
      socketpair(AF_UNIX, SOCK_STREAM, 0, pipeA);

      if (!fork()) {
        close(pipeA[1]);
        dup2(pipeA[0], 1);
        close(pipeA[0]);
        execlp("curl", "curl", "-L", vidurl, NULL);
        return 1;
      }
      close(pipeA[0]);

      int pipeB[2];
      socketpair(AF_UNIX, SOCK_STREAM, 0, pipeB);

      if (!fork()) {
        close(pipeB[1]);
        dup2(pipeA[1], 0);
        dup2(pipeB[0], 1);
        close(pipeA[1]);
        close(pipeB[0]);
        execlp("avconv",
                "avconv","-i","-","-vn","-c:a","copy","-f","webm","-",NULL);
        return 1;
      }
      close(pipeA[1]);
      close(pipeB[0]);
      dup2(pipeB[1], 0);
      close(pipeB[1]);
      execlp("mplayer", "mplayer", "-vo", "null", "-", NULL);
    } else {
      execlp("mplayer", "mplayer", vidurl, NULL);
    }
    return 1; // execlp fail
  }

  printf("Status: 204 No Content\nContent-Length: 0\n\n");
  return 0;
}
