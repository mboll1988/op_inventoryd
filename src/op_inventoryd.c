#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <spawn.h>
#include <memory.h>

#define BD_NO_CLOSE_FILES 02
#define BD_NO_CHDIR 01
#define BD_NO_UMASK0 010
#define BD_NO_REOPEN_STD_FDS 94
#define BD_MAX_CLOSE 8192
#define BUFSIZE 1024
#define PORT 8080
#if 0



/* Internet address */
struct in_addr {
  unsigned int s_addr; 
};

/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; 
  unsigned short int sin_port;
  struct in_addr sin_addr;
  unsigned char sin_zero[...];
};

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;
  char    **h_aliases;
  int     h_addrtype;
  int     h_length;
  char    **h_addr_list;
}
#endif
static int app_started;
static int running = 0;
static int delay = 1;
static int counter = 0;
static char *pid_file_name = NULL;
static int pid_fd = -1;
static char *app_name = NULL;
static FILE *log_stream;

const char _FLAG_INVENTORYD_RUN[]= "INVENTORYHIPA_RUN";
const char _FLAG_INVENTORYD_ABORT[]= "INVENTORYHIPA_ABORT";
const char _FLAG_INVENTORYD_DONE[]= "INVENTORYHIPA_DONE";


extern char **environ;

void error(char *msg) {
  perror(msg);
  exit(1);
}


void handle_signal(int sig)
{
	if (sig == SIGINT) {
		fprintf(log_stream, "Debug: stopping daemon ...\n");
		if (pid_fd != -1) {
			lockf(pid_fd, F_ULOCK, 0);
			close(pid_fd);
		}
		if (pid_file_name != NULL) {
			unlink(pid_file_name);
		}
		running = 0;
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
	}
	 else if (sig == SIGHUP) {
		fprintf(log_stream, "Debug: reloading daemon config file ...\n");
	//	read_conf_file(1);
	} else if (sig == SIGCHLD) {
		fprintf(log_stream, "Debug: received SIGCHLD signal\n");
	}
}


void print_help(void)
{
	printf("\n Usage: %s [OPTIONS]\n\n", app_name);
	printf("  Options:\n");
	printf("   -h --help                 Print this help\n");
	printf("   -l --log_file  filename   Write logs to the file\n");
	printf("   -d --daemon               Daemonize this application\n");
	printf("   -p --pid_file  filename   PID file used by daemonized app\n");
	printf("\n");
}


int become_daemon(int flags)
{
  int maxfd, fd;
  switch(fork())                    // become background process
  {
    case -1: return -1;
    case 0: break;                  // child falls through
    default: _exit(EXIT_SUCCESS);   // parent terminates
  }

  if(setsid() == -1)                // become leader of new session
    return -1;

  switch(fork())
  {
    case -1: return -1;
    case 0: break;                  // child breaks out of case
    default: _exit(EXIT_SUCCESS);   // parent process will exit
  }

  if(!(flags & BD_NO_UMASK0))
    umask(0);                       // clear file creation mode mask

  if(!(flags & BD_NO_CHDIR))
    chdir("/");                     // change to root directory

  if(!(flags & BD_NO_CLOSE_FILES))  // close all open files
  {
    maxfd = sysconf(_SC_OPEN_MAX);
    if(maxfd == -1)
      maxfd = BD_MAX_CLOSE;         // if we don't know then guess
    for(fd = 0; fd < maxfd; fd++)
      close(fd);
  }

  if(!(flags & BD_NO_REOPEN_STD_FDS))
  {
    close(STDIN_FILENO);
    fd = open("/dev/null", O_RDWR);
    if(fd != STDIN_FILENO)
      return -1;
    if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
      return -2;
    if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
      return -3;
  }

  return 0;
}

pid_t run_inventory_app()
{
    /* /// --->  
     */   
    pid_t pid;
    //char *env_disp = "DISPLAY";
    char *argv[] = {"/usr/bin/firefox", (char*)0};
    
    char* disp_env = "DISPLAY=";
    char* disp_value = (char*) getenv("DISPLAY");
    char* env;
    env = malloc(strlen(disp_env)+1+strlen(disp_value));
    strcpy(env, disp_env);
    strcat(env, disp_value);
    free(env);    
    return pid;
    /*
    //char *env = "DISPLAY=" (char*) getenv("DISPLAY");
    //char *env = (char*)"DISPLAY=:1";
    syslog(LOG_INFO, "%s\n", env);
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    //posix_spawnattr_setflags(&attr, POSIX_SPAWN_USEVFORK);

    int status = posix_spawn(&pid, "/usr/bin/firefox", NULL, &attr, argv, &env);
    if(status != 0)
      {
	syslog(LOG_ERR, "Error starting external application 'InventoryHIPA'");
	return -1;
    }

    return pid;
    */
}


int main(int argc, char *argv[])
{
	static struct option long_options[] = {
	  {"log_file", required_argument, 0, 'l'},
	  {"help", no_argument, 0, 'h'},
	  {"daemon", no_argument, 0, 'd'},
	  {"pid_file", required_argument, 0, 'p'},
	  {NULL, 0, 0, 0}
	};
	int value, option_index = 0, ret;
	char *log_file_name = NULL;
	int start_daemonized = 0;
	int parentfd;
	int childfd;
	int clientlen;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	struct hostent *hostp;
	char buf[BUFSIZE];
	char *hostaddrp;
	int optval;
	int n, ret_d;
	pid_t pid_daemon;
	app_name = argv[0];
	while ((value = getopt_long(argc, argv, "c:l:t:p:d:h", long_options, &option_index)) != -1)
	{
	  switch (value)
	  {
	        case 'l':
		  log_file_name = strdup(optarg);
		  break;
		case 'p':
		  pid_file_name = strdup(optarg);
		  break;
	        case 'h':
		  print_help();
		  return EXIT_SUCCESS;
		case 'd':
		  start_daemonized = 1;
		  break;
	        case '?':
		  print_help();
		  return EXIT_FAILURE;
	        default:
		  break;
		}
	}
	/*
	if(start_daemonized==1)
	  {
	    ret_d = become_daemon(0);
	    if(ret_d)
	      {
		syslog(LOG_USER | LOG_ERR, "error starting");
		closelog();
		return EXIT_FAILURE;
	      }
	  }
	*/
	/* Write to LOG... */
	openlog(argv[0], LOG_PID|LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Started %s", app_name);
	printf("Started %s\n", app_name);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);

	if (log_file_name != NULL) {
		log_stream = fopen(log_file_name, "a+");
		if (log_stream == NULL) {
			syslog(LOG_ERR, "Can not open log file: %s, error: %s",
				log_file_name, strerror(errno));
			printf("Can not open log file: %s, error: %s", log_file_name, strerror(errno));
			log_stream = stdout;
		}
	} else {
		log_stream = stdout;
	}

	parentfd = socket(AF_INET, SOCK_STREAM, 0);
	if (parentfd < 0) 
	  error("ERROR opening socket");

	/* setsockopt: Handy debugging trick that lets 
	 * us rerun the server immediately after we kill it; 
	 * otherwise we have to wait about 20 secs. 
	 * Eliminates "ERROR on binding: Address already in use" error. 
	 */
	optval = 1;
	setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
		   (const void *)&optval , sizeof(int));

	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)PORT);

	if (bind(parentfd, (struct sockaddr *) &serveraddr, 
		 sizeof(serveraddr)) < 0) 
	  error("ERROR on binding");

	if (listen(parentfd, 5) < 0) //n requests
	  error("ERROR on listen");

	running = 1;
	app_started = 0;
	clientlen = sizeof(clientaddr);
	while (running == 1) {
		/* Debug print */
		ret = fprintf(log_stream, "Debug: %d\n", counter++);
		if (ret < 0) {
			syslog(LOG_ERR, "Can not write to log stream: %s, error: %s",
				(log_stream == stdout) ? "stdout" : log_file_name, strerror(errno));
			break;
		}
		ret = fflush(log_stream);
		if (ret != 0) {
			syslog(LOG_ERR, "Can not fflush() log stream: %s, error: %s",
				(log_stream == stdout) ? "stdout" : log_file_name, strerror(errno));
			break;
		}
		childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
		if (childfd < 0)
		  {
		    syslog(LOG_ERR, "ERROR on accept");
		  }
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
				      sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
		  {
		    syslog(LOG_ERR, "Error on gethostbyaddr");
		  }
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
		  {
		    syslog(LOG_ERR, "ERROR on inet_ntoa");
		  }
		syslog(LOG_INFO, "Connection to %s (%s) established...\n",
		       hostp->h_name, hostaddrp);
		
		bzero(buf, BUFSIZE);

		while ((n = read(childfd, buf, BUFSIZE)) > 0)
		  {
		    buf[n] = 0x00;
		    for (int i = n - 1; i >= 0 && (buf[i] == '\n' || buf[i] == '\r' || buf[i] == ' '); i--)
		      {
			buf[i] = '\0';
		      }
		    syslog(LOG_INFO, "Block read: \n<%s>\n", buf);
		    printf("Block read: \n<%s>\n", buf);
		    if(strcmp(buf, _FLAG_INVENTORYD_RUN) == 0)
		      {
			syslog(LOG_INFO, "Flag received: \n<%s>\nStarting InvenotryHIPA applicaiton.\n", buf);
			printf("Flag received: \n<%s>\nStarting InvenotryHIPA applicaiton.\n", buf);
			pid_daemon = run_inventory_app();
			if(pid_daemon < 0 ) // ( kill(pid_daemon,0) == 0))
			  syslog(LOG_ERR, "ERROR starting process InvonteryHIPA");			  
		      }
		    else if(strcmp(buf, _FLAG_INVENTORYD_ABORT) == 0)
		      {
			syslog(LOG_INFO, "Flag received: \n<%s>\nAborting InvenotryHIPA applicaiton.\n", buf);
			printf("Flag received: \n<%s>\nAborting InvenotryHIPA applicaiton.\n", buf);
			if( kill(pid_daemon,0) == 0)
			  kill(pid_daemon, SIGKILL); 
		      }
		    else if(strcmp(buf, _FLAG_INVENTORYD_DONE) == 0)
		      {
			syslog(LOG_INFO, "Flag received: \n<%s>\nClosing InvenotryHIPA applicaiton.\n", buf);
			printf("Flag received: \n<%s>\nClosing InvenotryHIPA applicaiton.\n", buf);
			if( kill(pid_daemon,0) == 0)
			  kill(pid_daemon, SIGTERM);
			//syslog(LOG_INFO, "Flag received: \n<%s>\nClosing InvenotryHIPA applicaiton.\n", buf);
		      }
		    else
		      continue;
		    n = write(childfd, buf, strlen(buf));
		    if (n < 0) 
		      error("ERROR writing to socket");
		
		    /* Real server should use select() or poll() for waiting at
		     * asynchronous event. Note: sleep() is interrupted, when
		     * signal is received. */
		    sleep(delay);
		}
		close(childfd);
	}

	
	if (log_stream != stdout) {
		fclose(log_stream);
	}

	/* Write system log and close it. */
	syslog(LOG_INFO, "Stopped %s", app_name);
	closelog();

	if (log_file_name != NULL) free(log_file_name);
	if (pid_file_name != NULL) free(pid_file_name);

	return EXIT_SUCCESS;
}
