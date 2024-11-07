#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif

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

static int running = 0;
static int delay = 1;
static int counter = 0;
static char *pid_file_name = NULL;
static int pid_fd = -1;
static char *app_name = NULL;
static FILE *log_stream;


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


/**
 * \brief This function will daemonize this app
 */
static void daemonize()
{
	pid_t pid = 0;
	int fd;
	pid = fork();
	if (pid < 0)
	{
	  exit(EXIT_FAILURE);
	}
	if (pid > 0)
	{
	  exit(EXIT_SUCCESS);
	}
	if (setsid() < 0)
	{
	  exit(EXIT_FAILURE);
	}
	signal(SIGCHLD, SIG_IGN);
	pid = fork();
	if (pid < 0)
	{
	  exit(EXIT_FAILURE);
	}
	if (pid > 0)
	{
	  exit(EXIT_SUCCESS);
	}
	umask(0);
	chdir("/");
	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
	{
		close(fd);
	}
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");

	if (pid_file_name != NULL)
	{
		char str[256];
		pid_fd = open(pid_file_name, O_RDWR|O_CREAT, 0640);
		if (pid_fd < 0)
		{
			/* Can't open lockfile */
			exit(EXIT_FAILURE);
		}
		if (lockf(pid_fd, F_TLOCK, 0) < 0)
		{
			/* Can't lock file */
			exit(EXIT_FAILURE);
		}

		sprintf(str, "%d\n", getpid());
		write(pid_fd, str, strlen(str));
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
	int n;
	
	app_name = argv[0];
	while ((value = getopt_long(argc, argv, "c:l:t:p:dh", long_options, &option_index)) != -1)
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
		  //case 'd':
		  //start_daemonized = 1;
		  //break;
	        case '?':
		  print_help();
		  return EXIT_FAILURE;
	        default:
		  break;
		}
	}
	//if(start_daemonized==1)
	daemonize();

	/* Write to LOG... */
	openlog(argv[0], LOG_PID|LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Started %s", app_name);

	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);

	if (log_file_name != NULL) {
		log_stream = fopen(log_file_name, "a+");
		if (log_stream == NULL) {
			syslog(LOG_ERR, "Can not open log file: %s, error: %s",
				log_file_name, strerror(errno));
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
		  error("ERROR on accept");

		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
				      sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
		  error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
		  error("ERROR on inet_ntoa\n");
		printf("server established connection with %s (%s)\n", 
		       hostp->h_name, hostaddrp);
    
		bzero(buf, BUFSIZE);
		n = read(childfd, buf, BUFSIZE);
		if (n < 0) 
		  error("ERROR reading from socket");
		printf("server received %d bytes: %s", n, buf);
    
		/* 
		 * write: echo the input string back to the client 
		 */
		n = write(childfd, buf, strlen(buf));
		if (n < 0) 
		  error("ERROR writing to socket");
		
		/* Real server should use select() or poll() for waiting at
		 * asynchronous event. Note: sleep() is interrupted, when
		 * signal is received. */
		sleep(delay);
	}
	close(childfd);
	
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
