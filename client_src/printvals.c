#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#ifndef _WIN32
#include <strings.h>
#endif

#include <vrpn_Shared.h>
#include <vrpn_Button.h>
#include <vrpn_Tracker.h>
#include <vrpn_FileConnection.h>
#include <vrpn_FileController.h>
#include <vrpn_RedundantTransmission.h>
#include <vrpn_DelayedConnection.h>

vrpn_Button_Remote *btn,*btn2;
vrpn_Tracker_Remote *tkr;
vrpn_Connection * c;
vrpn_File_Controller * fc;

vrpn_RedundantRemote * rc;

int   print_for_tracker = 1;	// Print tracker reports?
int beQuiet = 0;
int beRedundant = 0;
int redNum = 0;
double redTime = 0.0;
double delayTime = 0.0;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	handle_pos (void *, const vrpn_TRACKERCB t)
{
	static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%ld.", t.sensor);
	if ((++count % 20) == 0) {
		fprintf(stderr, "\n");
		if (count > 300) {
			printf("Pos, sensor %d = %5.3f, %5.3f, %5.3f", t.sensor,
				t.pos[0], t.pos[1], t.pos[2]);
			printf("  Quat = %5.2f, %5.2f, %5.2f, %5.2f\n",
				t.quat[0], t.quat[1], t.quat[2], t.quat[3]);
			count = 0;
		}
	}
}

void	handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	//static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%ld/", t.sensor);
}

void	handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	//static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%ld~", t.sensor);
}

void	handle_button (void *, const vrpn_BUTTONCB b)
{
	printf("B%ld is %ld\n", b.button, b.state);
}

int handle_gotConnection (void *, vrpn_HANDLERPARAM) {

  if (beRedundant) {
    fprintf(stderr, "printvals got connection;  "
            "initializing redundant xmission.\n");
    rc->set(redNum, vrpn_MsecsTimeval(redTime * 1000.0));
  }

  return 0;
}

int filter_pos (void * userdata, vrpn_HANDLERPARAM p) {

  vrpn_Connection * c = (vrpn_Connection *) userdata;
  int postype = c->register_message_type("Tracker Pos/Quat");

  if (p.type == postype)
    return 0;  // keep position messages

  return 1;  // discard all others
}

/*****************************************************************************
 *
   init - initialize everything
 *
 *****************************************************************************/

void init (const char * station_name, 
           const char * local_logfile, long local_logmode,
           const char * remote_logfile, long remote_logmode,
           const char * NIC)
{
	char devicename [1000];
	//char * hn;
	int port;

  vrpn_int32 gotConn_type;

	// explicitly open up connections with the proper logging parameters
	// these will be entered in the table and found by the
	// vrpn_get_connection_by_name() inside vrpn_Tracker and vrpn_Button

	sprintf(devicename, "Tracker0@%s", station_name);
	if (!strncmp(station_name, "file:", 5)) {
fprintf(stderr, "Opening file %s.\n", station_name);
	  c = new vrpn_File_Connection (station_name);  // now unnecessary!
          if (local_logfile || local_logmode ||
              remote_logfile || remote_logmode)
            fprintf(stderr, "Warning:  Reading from file, so not logging.\n");
	} else {
fprintf(stderr, "Connecting to host %s.\n", station_name);
	  port = vrpn_get_port_number(station_name);
	  //c = new vrpn_Synchronized_Connection
	  c = new vrpn_DelayedConnection
                   (vrpn_MsecsTimeval(0.0),
                    station_name, port,
		    local_logfile, local_logmode,
		    remote_logfile, remote_logmode,
                    1.0, 3, NIC);
          if (delayTime > 0.0) {
            ((vrpn_DelayedConnection *) c)->setDelay
                              (vrpn_MsecsTimeval(delayTime * 1000.0));
            ((vrpn_DelayedConnection *) c)->delayAllTypes(vrpn_TRUE);
          }
	}

	fc = new vrpn_File_Controller (c);

        rc = new vrpn_RedundantRemote (c);

fprintf(stderr, "Tracker's name is %s.\n", devicename);
	tkr = new vrpn_Tracker_Remote (devicename);


	sprintf(devicename, "Button0@%s", station_name);
fprintf(stderr, "Button's name is %s.\n", devicename);
	btn = new vrpn_Button_Remote (devicename);
	sprintf(devicename, "Button1@%s", station_name);
fprintf(stderr, "Button 2's name is %s.\n", devicename);
	btn2 = new vrpn_Button_Remote (devicename);


	// Set up the tracker callback handler
	printf("Tracker update: '.' = pos, '/' = vel, '~' = acc\n");
	tkr->register_change_handler(NULL, handle_pos);
	tkr->register_change_handler(NULL, handle_vel);
	tkr->register_change_handler(NULL, handle_acc);

	// Set up the button callback handler
	printf("Button update: B<number> is <newstate>\n");
	btn->register_change_handler(NULL, handle_button);
	btn2->register_change_handler(NULL, handle_button);

  gotConn_type = c->register_message_type(vrpn_got_connection);

  c->register_handler(gotConn_type, handle_gotConnection, NULL);

}	/* init */


void handle_cntl_c (int) {

  static int invocations = 0;
  const char * n;
  long i;

  fprintf(stderr, "\nIn control-c handler.\n");
/* Commented out this test code for the common case
  if (!invocations) {
    printf("(First press -- setting replay rate to 2.0 -- 3 more to exit)\n");
    fc->set_replay_rate(2.0f);
    invocations++;
    signal(SIGINT, handle_cntl_c);
    return;
  }
  if (invocations == 1) {
    printf("(Second press -- Starting replay over -- 2 more to exit)\n");
    fc->reset();
    invocations++;
    signal(SIGINT, handle_cntl_c);
    return;
  }
  if (invocations == 2) {
    struct timeval t;
    printf("(Third press -- Jumping replay to t+30 seconds -- "
           "1 more to exit)\n");
    t.tv_sec = 30L;
    t.tv_usec = 0L;
    fc->play_to_time(t);
    invocations++;
    signal(SIGINT, handle_cntl_c);
    return;
  }
*/

  // print out sender names

  if (c)
    for (i = 0L; n = c->sender_name(i); i++)
      printf("Knew local sender \"%s\".\n", n);

  // print out type names

  if (c)
    for (i = 0L; n = c->message_type_name(i); i++)
      printf("Knew local type \"%s\".\n", n);


  if (btn)
    delete btn;
  if (btn2)
    delete btn2;
  if (tkr)
    delete tkr;
  if (c)
    delete c;

  exit(0);
}

void main (int argc, char * argv [])
{

#ifdef hpux
  char default_station_name [20];
  strcpy(default_station_name, "ioph100");
#else
  char default_station_name [] = { "ioph100" };
#endif

  const char * station_name = default_station_name;
  const char * local_logfile = NULL;
  const char * remote_logfile = NULL;
  const char * NIC = NULL;
  long local_logmode = vrpn_LOG_NONE;
  long remote_logmode = vrpn_LOG_NONE;
  int	done = 0;
  int   filter = 0;
  int i;

#ifdef	_WIN32
  WSADATA wsaData; 
  int status;
  if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
    fprintf(stderr, "WSAStartup failed with %d\n", status);
    exit(1);
  }
#endif

  if (argc < 2) {
    fprintf(stderr, "Usage:  %s [-ll logfile mode] [-rl logfile mode]\n"
                    "           [-NIC ip] [-filterpos] [-quiet]\n"
                    "           [-red num time] [-delay time] station_name\n"
                    "  -notracker:  Don't print tracker reports\n" 
                    "  -ll:  log locally in <logfile>\n" 
                    "  -rl:  log remotely in <logfile>\n" 
                    "  <mode> is one of i, o, io\n" 
                    "  -NIC:  use network interface with address <ip>\n"
                    "  -filterpos:  log only Tracker Position messages\n"
                    "  -quiet:  ignore VRPN warnings\n"
                    "  -red <num> <time>:  send every message <num>\n"
                    "    times <time> seconds apart\n"
                    "  -delay <time:  delay all messages received by <time>\n"
                    "  station_name:  VRPN name of data source to contact\n"
                    "    one of:  <hostname>[:<portnum>]\n"
                    "             file:<filename>\n",
            argv[0]);
    exit(0);
  }

  // parse args

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-ll")) {
      i++;
      local_logfile = argv[i];
      i++;
      if (strchr(argv[i], 'i')) local_logmode |= vrpn_LOG_INCOMING;
      if (strchr(argv[i], 'o')) local_logmode |= vrpn_LOG_OUTGOING;
    } else if (!strcmp(argv[i], "-rl")) {
      i++;
      remote_logfile = argv[i];
      i++;
      if (strchr(argv[i], 'i')) remote_logmode |= vrpn_LOG_INCOMING;
      if (strchr(argv[i], 'o')) remote_logmode |= vrpn_LOG_OUTGOING;
    } else if (!strcmp(argv[i], "-notracker")) {
      print_for_tracker = 0;
    } else if (!strcmp(argv[i], "-filterpos")) {
      filter = 1;
    } else if (!strcmp(argv[i], "-NIC")) {
      i++;
      NIC = argv[i];
    } else if (!strcmp(argv[i], "-quiet")) {
      beQuiet = 1;
    } else if (!strcmp(argv[i], "-red")) {
      beRedundant = 1;
      i++;
      redNum = atoi(argv[i]);
      i++;
      redTime = atof(argv[i]);
    } else if (!strcmp(argv[i], "-delay")) {
      i++;
      delayTime = atof(argv[i]);
    } else
      station_name = argv[i];
  }

  // initialize the PC/station
  init(station_name, 
       local_logfile, local_logmode,
       remote_logfile, remote_logmode,
       NIC);

  // signal handler so logfiles get closed right
  signal(SIGINT, handle_cntl_c);

  // filter the log if requested
  if (filter && c) {
    c->register_log_filter(filter_pos, c);
  }

  if (beQuiet) {
    vrpn_System_TextPrinter.remove_object(btn);
    vrpn_System_TextPrinter.remove_object(btn2);
    vrpn_System_TextPrinter.remove_object(tkr);
    vrpn_System_TextPrinter.remove_object(rc);
  }

  if (beRedundant) {
    rc->set(redNum, vrpn_MsecsTimeval(redTime * 1000.0));
  }

/* 
 * main interactive loop
 */
  while ( ! done )
    {
        // Run this so control happens!
        c->mainloop();

        // Let the tracker and button do their things
	btn->mainloop();
	btn2->mainloop();
	tkr->mainloop();

        rc->mainloop();
	// Sleep for 1ms so we don't eat the CPU
	vrpn_SleepMsecs(1);
    }

}   /* main */


