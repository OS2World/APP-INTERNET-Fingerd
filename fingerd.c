/*******************************************************************
 * Finger Daemon for TCP/IP for OS/2
 * Compiles with CSET/2
 * by:
 *   Aaron Williams
 *   35900 Turpin Way
 *   Fremont, CA. 94536-2666
 *   (510) 792-0340 after June 18, 1994.
 *   (408) 426-3053 before June 18, 1994.
 *******************************************************************/

/*
 * Include Files.
 */
#include <string.h>
#include <stdlib.h>
#include <types.h>
#include <time.h>
#include <netinet\in.h>
#include <sys\socket.h>
#include <stdio.h>
#include <netdb.h>
#define INCL_DOSQUEUES
#define INCL_DOSNMPIPES
#define INCL_DOSPROCESS

#include <os2.h>


#define PIPESIZE 32768		/* Size of our pipe for I/O redirecting */
#define HF_STDOUT 1


void usage(char *program)
{
  fprintf(stderr, "Usage: %s -f responsefile [-s port|service] [-p protocol] [-d] [-l logfile] [-b]\n", program);
  fprintf(stderr, "\t-f specifies the data file to be displayed\n");
  fprintf(stderr, "\t-s specifies the service to be used (default is finger)\n");
  fprintf(stderr, "\t   A port number may also be specified.\n");
  fprintf(stderr, "\t-p specifies the protocol used (default is tcp)\n");
  fprintf(stderr, "\t-d specifies debugging info is to be displayed\n");
  fprintf(stderr, "\t-l specifies the log file to be used for logging queries\n");
  fprintf(stderr, "\t-b tells %s to beep whenever a finger request is made.\n");
  
  exit(1);
}


/*
 * Server Main.
 */
main(argc, argv)
int argc;
char **argv;
{
    unsigned short port;       /* port server binds to                  */
    char buf[512];             /* buffer for sending and receiving data */
    struct sockaddr_in client; /* client address information            */
    struct sockaddr_in server; /* server address information            */
    struct servent *ser;
    struct hostent *client_info;
    int s;                     /* socket for accepting connections      */
    int ns;                    /* socket connected to client            */
    int namelen;               /* length of client name                 */
    char *servicename;         /* name of service to be used, usually finger */
    char *protocol;            /* name of protocol to be used, usually tcp */
    char *filename;            /* filename of finger data file          */
    char *logfilename;         /* filename where logging info is to be stored */
    int i;			/* Used for counting */
    int bufsize;
    int debugging = 0;         /* indicates whether debugging has been enabled */
    int logging = 0;           /* indicates whether logging has been enabled */
    FILE *finger_data;		/* Finger data file */
    FILE *logfile;		/* log file handle */
    char client_addr[18];      /* address string */
    int beepenable = 0;		/* Beeping enabled */
    int parmsvalid = 0;		/* Set to non-zero when valid parameters */

    /*
     * Check arguments. Should be only one: the port number to bind to.
     */

    servicename = "finger";    /* default service */
    protocol = "tcp";          /* default protocol */

    if (argc < 3)
      usage(argv[0]);          /* if no arguments are passed, presend info */

    i = 1;
    while (i < argc) {
      if (!strcmp(argv[i], "-f")) { /* Check for data file */
	i++;
	if (i >= argc) {	/* Make sure we have enough arguments */
	  fprintf(stderr, "Error: Filename missing!\n");
	  usage(argv[0]);
	}
	filename = argv[i];	/* Get the filename */
	printf("Finger file = %s\n", filename); /* Report it */
	finger_data = fopen(filename, "r"); /* Now check that it exist */

	if (finger_data == NULL) {
	  fprintf(stderr, "Error:  Cannot open finger data file!\n");
	  exit(-1);
	}
	fclose(finger_data);	/* We have a data file, ok */
	parmsvalid++;		/* We have the minimum parameters to run */
	i++;
	continue;
      }
      if (!strcmp(argv[i], "-s")) { /* Service */
	i++;
	if (i >= argc) {	/* Make sure the service string or port */
				/* number is specified */
	  fprintf(stderr, "Error:  Missing service name!\n");
	  usage(argv[0]);
	}

	servicename = argv[i];
	if (!isdigit(servicename[0])) /* If a number it must be the port num */
	  printf("Using %s service\n", servicename); /* Need to verify name */
	else
	  printf("Using port %s\n", servicename);
	i++;
	continue;
      }

      if (!strcmp(argv[i], "-p")) { /* Get protocol */
	i++;
	if (i >= argc) {
	  fprintf(stderr, "Error:  Missing protocol!\n");
	  usage(argv[0]);
	}
	protocol = argv[i];
	if (strcmp(protocol, "tcp") && strcmp(protocol, "udp")) { /* Test it */
	  fprintf(stderr, "Error: Invalid protocol!  Must be tcp or udp!\n");
	  usage(argv[0]);
	}
	printf("Using %s protocol\n", protocol);
	i++;
	continue;
      }

      if (!strcmp(argv[i], "-d")) { /* Get debugging option */
	i++;
	debugging = 1;
	printf("Debugging info is enabled.\n");
	continue;
      }

      if (!strcmp(argv[i], "-l")) { /* Get logging option */
	i++;
	if (i >= argc) {
	  fprintf(stderr, "Error: logfile not specified!\n");
	  usage(argv[0]);
	}
	logfilename = argv[i];
	logfile = fopen(logfilename, "a"); /* Test log file */
	if (logfile == NULL) {
	  fprintf(stderr, "Error: Cannot open logfile %s!\n", 
		  logfilename);
	  exit(1);
	}
	fclose(logfile);
	i++;
	printf("Logging queries to file %s\n", logfilename);
	logging = 1;
	continue;
      }

      if (!strcmp(argv[i], "-b")) { /* Get beep option */
        printf("Beep enabled on finger request.\n");
        beepenable = 1;
        i++;
        continue;
      }
      
      fprintf(stderr, "Unknown option %s\n", argv[i]);
      usage(argv[0]);
    }
				/* Finally!  We're done with parms */
    if (!parmsvalid)		/* Make sure they're valid */
      usage(argv[0]);		/* I.E. We've got a data file name */

    printf("\nFinger Daemon for OS/2\n");
    printf("Brought to you by Aaron Williams\n");
    printf("This program is provided as-is, and no warrenties exist!\n");
    printf("If you make improvements or find any bugs, let me know.\n");
    printf("\nI can be reached via snail mail at:\n\n\tAaron Williams\n");
    printf("\t35900 Turpin Way\n\tFremont, CA. 94536-2666\n");
    printf("\nEmail: aaronw@cats.ucsc.edu (subject to change on June 18, 1994)\n\n");
    /*
     * Initialize with sockets.
     */
    sock_init();
    /*
     * First argument should be the port.
     */
    if (isdigit(servicename[0]))
      port = (unsigned short) atoi(servicename);
    else {
      ser = getservbyname(servicename, protocol);
      if (ser == NULL) {	/* Check for valid service */
	fprintf(stderr, "Error:  Service %s is not available!\n", servicename);
	exit(-1);
      }
      port = ser->s_port >> 8;	/* Convert to port number */
				/* This probably needs to be changed */
				/* for future compatibility */
      if (debugging)
	printf("Service %s using port #%d\n", ser->s_name, port);
    }

    /*
     * Get a socket for accepting connections.
     */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      psock_errno("Socket()");
      fprintf(stderr, "Error getting a socket!\n");
      exit(2);
    }

    /*
     * Bind the socket to the server address.
     */
    server.sin_family = AF_INET;
    server.sin_port   = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
      psock_errno("Bind()");
      fprintf(stderr, "Error binding a socket!\n");
      soclose(s);
      exit(3);
    }

    while (1) {
      if (listen(s, 1) != 0) {	/* Listen for a connection */
	psock_errno("Listen()");
	fprintf(stderr, "Error listening for a connection!\n");
	exit(4);
      }
	
      /*
       * Accept the connection.
       */
      namelen = sizeof(client);
      if ((ns = accept(s, (struct sockaddr *)&client, &namelen)) == -1) {
	psock_errno("Accept()");
	fprintf(stderr, "Error accepting a connection!\n");
	exit(5);
      }

      if (beepenable) {		/* Beep */
        DosBeep(1600,50);
        DosBeep(800,25);
        DosBeep(1600,25);
      }

      if (debugging || logging) {
	sprintf(client_addr, "%d.%d.%d.%d", 
		(client.sin_addr.s_addr  & 255),
		(client.sin_addr.s_addr >> 8) & 255,
		(client.sin_addr.s_addr >> 16) & 255,
		client.sin_addr.s_addr >> 24);
	sethostent(1);
	    
	client_info = gethostbyaddr((char *)(&(client.sin_addr.s_addr)), 
				    sizeof(client.sin_addr.s_addr), 
				    AF_INET);

				/* Handle debug info */
	if (debugging) {
	  if (client_info != NULL)
	    printf("%s (%s) port=%d.  ", client_info->h_name, 
		   client_addr, client.sin_port);
	  else
	    printf("Cannot get client name.  IP=%s.  ", client_addr); 
	}

				/* Handle logging */
	if (logging) {

	  time_t ltime;		/* Time for logging */
	  PSZ pszTime;

	  logfile = fopen(logfilename, "a");
	  if (logfile == NULL)
	    fprintf(stderr, "Warning: Cannot write to log file!\n");
	  else {
	    if (client_info != NULL)
	      fprintf(logfile, "%s (%s) port=%d.  ",
		      client_info->h_name, 
		      client_addr, client.sin_port);
	    else
	      fprintf(logfile, "Cannot get client name.  IP=%s.  ", 
		      client_addr);

				/* Add a time and date stamp */
	    time(&ltime);
	    pszTime = ctime(&ltime);
	    pszTime[strlen(pszTime)-1] = 0; /* Remove final cr */
	    fprintf(logfile, "\n  Logged on %s.  ", pszTime);
	  }
	}
      }
	

      /*
       * Receive the message on the newly connected socket.
       */
      memset(buf, 0, sizeof(buf)); /* Clear memory */
      
      if ((bufsize=recv(ns, buf, sizeof(buf), 0)) == -1) {
	psock_errno("Recv()");
	exit(6);
      }
	    
      if (bufsize > 0) {	/* Make sure we got some data */
	if (debugging) {
	  if (buf[0] == 13)	/* All users just receives a cr */
	    printf("Querying all users\n");
	  else
	    printf("Account=%s", buf);
	  fflush(stdout);
	}
	if (logging) {
	  if (buf[0] == 13)
	    fprintf(logfile, "Querying all users\n");
	  else
	    fprintf(logfile, "Querying account %s\n", buf);
	}
      }
	
      /*
       * Send the message back to the client.
       */
				/* Open finger data file for reading */
      finger_data = fopen(filename, "r");
      if (finger_data == NULL) {
	fprintf(stderr, "Error: Cannot open finger data file!\n");
	exit(7);
      }

				/* Read a line from the data file */
      while (fgets(buf, sizeof(buf), finger_data)) { 

	bufsize = strlen(buf);	/* Get the length of the line */


	if (buf[0] == '~' && buf[1] == 'x') /* Is this a command? (~x) */
	  {			/* Yes, process it as such */
	    ULONG cbRead, cbWritten; /* count of bytes read & written */
	    HFILE hfSave = -1;
	    HFILE hfNew = HF_STDOUT;
	    HPIPE hpR, hpW;
	    RESULTCODES rc;

	    buf[strlen(buf)-1] = 0; /* Remove final cr from command line */
				
	    if (debugging) {	/* If debugging print out command executed */
	      printf(" Executing: %s.\n", buf+2);
	      fflush(stdout);
	    }
	    if (logging) {
	      fprintf(logfile, "  Executing: %s.\n", buf+2);
	      fflush(logfile);
	    }

	    DosDupHandle(HF_STDOUT, &hfSave); /* Save the stdout handle */
				/* Create a pipe */
	    if (DosCreatePipe(&hpR, &hpW, PIPESIZE))
	      {
		fprintf(stderr, "Error opening pipe!\n");
		if (logging)
		  fprintf(logfile, "Error opening pipe!\n");
	      }
	    DosDupHandle(hpW, &hfNew);
				/* This will be changed in the future to */
				/* allow for asynchronous execution.  This */
				/* will also remove any limits on the size */
				/* of the pipe. */
	    system(buf+2);	/* Do the command */
	    
	    DosClose(hpW);
	    DosDupHandle(hfSave, &hfNew); /* Un redirect stdout */

	    do {

	      static CHAR achBuf[PIPESIZE]; /* Buffer for storing pipe */

	      DosRead(hpR, achBuf, sizeof(achBuf), &cbRead); /* Read from pipe */
	      if (send(ns, achBuf, cbRead, 0) < 0) { /* Output to socket */
		psock_errno("Send()");
	      }
	    } while (cbRead);
	    DosClose(hpR);
	  }
	else			/* Not a command line, so send it */
	  if (send(ns, buf, bufsize, 0) < 0) { /* Send the line out the socket */
	    psock_errno("Send()"); /* Report send errors */
	  }
      }
      fclose (finger_data);
      soclose(ns);
      if (logging)
	fclose(logfile);
    }
    soclose(s);
    printf("Server ended successfully\n");
    exit(0);
}






