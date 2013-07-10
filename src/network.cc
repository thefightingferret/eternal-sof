//Comments --YEGGO


#include "ctype.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/telnet.h>
#include "define.h"
#include "struct.h"


int         socket_one;
int           socket_two;
bool            wizlock  = FALSE;    
link_data*    link_list  = NULL; 
link_data*    link_next;

bool   process_output       ( link_data* );
bool   read_link            ( link_data* );
void   stop_idling          ( char_data* );

const    char    go_ahead_str    [] = { IAC, GA, '\0' };
const    char    echo_off_str    [] = { IAC, WILL, TELOPT_ECHO, '\0' };
const    char    echo_on_str     [] = { IAC, WONT, TELOPT_ECHO, '\0' };


/*
 *   PREVIOUS PROCESS SOCKET ROUTINES
 */


/* Function checks how many files are currently opened on the system.  It opens
   a number of links equal to that number */
void recover_links( )
{
  rlimit       lim;
  int            i;
  link_data*  link;
  
  /* Check for the limits on how many files may be opened, and how many are
     currently opened, store the value in lim */
  if( getrlimit( RLIMIT_NOFILE, &lim ) != 0 )
    panic( "Init_Network: error getting file number." );

  /* Create a number of links, from 3 to the current number of open files. */
  for( i = 3; i < (int)( lim.rlim_cur ); i++ )
    if( (int)( write( i, "\n\r", 2 ) ) != - 1 ) { 
      link             = new link_data;
      link->channel    = i;
      link->connected  = CON_INTRO;
      link->next       = link_list;
      link_list        = link;
      }

  echo( "-- New process started. --\n\r" );

  return;
}

/* Step through the 'link' list, getting the host and opening the link
 if it needs it. */
void restart_links( )
{
  link_data*                  link;
  struct sockaddr_in   net_addr_in;
  unsigned int             addrlen;
 
  for( ; ; )
    {
      if( ( link = link_list ) == NULL )
        return;
      link_list = link_list->next;
      
      addrlen = sizeof( net_addr_in  );
      
      /* Determine the peername of the connecting link.  Then write
         the hostname */
      if( getpeername( link->channel, (struct sockaddr*) &net_addr_in,
        &addrlen ) == -1 )
      panic( "Open_Link: Error returned by getpeername." );
      
      /* handles connecting the link and determining the host name */
      write_host( link, (char*) &net_addr_in.sin_addr ); 
    }
}
 
#define IDENT_LEVEL LEVEL_ENTITY
#define IDENT_PERM PERM_SITE_NAMES
#define IDENT_MAND 0

void ident_report( int type, char *token, char *charset, char *userid, link_data *link)
{
  char buf[256] = "";
  FILE* fp;
           
  switch( type )
  {
    case 1:    /* Ident Resolved */
      truncate( userid, 16 );
      truncate( token,  16 );
      link->ident.userid = alloc_string( userid, MEM_LINK );
      if( *charset != '\0' && strcmp( charset, "US-ASCII" ) )
      {
        sprintf( buf, "Ident: Unknown charset: %s", charset );
        bug( buf );
        truncate( charset, 16 );
        sprintf( buf, "%s, %s", token, charset );
        link->ident.token  = alloc_string( buf,  MEM_LINK );
      }
      else
        link->ident.token  = alloc_string( token,  MEM_LINK );
      
      if( link->player != NULL )
      {
        sprintf( buf, "%s is %s@%s (%s)", link->player->descr->name,
          link->ident.userid, link->host, link->ident.token );
        info( "", IDENT_LEVEL, buf, IFLAG_LOGINS, 3, NULL, NULL, IDENT_PERM, IDENT_MAND );
        link->ident.port = 0;
      }
      break;
      
    case 2:    /* General Error */
      link->ident.userid = alloc_string( IDENTERR_GEN, MEM_LINK );
      link->ident.token = alloc_string( token, MEM_LINK );
      sprintf( buf, "Ident Error: %s", token );
      bug( buf );
      link->ident.port = 0;
      break;
         
    case 3:    /* Network Error */
      link->ident.userid = alloc_string( IDENTERR_NET, MEM_LINK );
      link->ident.token = alloc_string( token, MEM_LINK );
      if( link->player != NULL )
      {
        sprintf( buf, "Could not resolve Ident for %s (%s)", link->player->descr->name,
          link->ident.token );
        info( "", IDENT_LEVEL, buf, IFLAG_LOGINS, 3, NULL, NULL, IDENT_PERM, IDENT_MAND );
        link->ident.port = 0;
      }
      break;  
       
    case 4:    /* Ident Error */
      link->ident.userid = alloc_string( IDENTERR_IDT, MEM_LINK );
      link->ident.token = alloc_string( token, MEM_LINK );
      if( link->player != NULL )
      {
        sprintf( buf, "Ident request for %s, remote server repied with %s",
          link->player->descr->name, link->ident.token );
        info( "", IDENT_LEVEL, buf, IFLAG_LOGINS, 3, NULL, NULL, IDENT_PERM, IDENT_MAND );
        link->ident.port = 0;
      }
      break;
      
    case 5:    /* Clean up */
      if( link->player == NULL && link->ident.port != 0 )
      {
        struct in_addr address;
        address.s_addr = link->ident.host;
        sprintf( buf, "Ident result on %s: %s (%s)",
          link->host == NULL ? inet_ntoa(address) : link->host,
          link->ident.userid, link->ident.token );
        info( "", IDENT_LEVEL, buf, IFLAG_LOGINS, 3, NULL, NULL, IDENT_PERM, IDENT_MAND );
        link->ident.port = 0;
        
        fp = open_file( FILE_DIR, "iplog.txt", "a" );
        if( fp == NULL )
          roach( "Could not open ip log file." );
        else
        {
          fprintf( fp, "Ident result on %s: %s (%s)\n",
            link->host == NULL ? inet_ntoa(address) : link->host,
            link->ident.userid, link->ident.token );
          fclose( fp );
        }
      }
      
      if( link->ident.userid != NULL )
      {
        free_string( link->ident.userid, MEM_LINK );
        link->ident.userid = NULL;
      }
      if( link->ident.token != NULL )
      {
        free_string( link->ident.token, MEM_LINK );
        link->ident.token = NULL;
      }
      break;
    default:
      return;
  }
  
  if( link->player != NULL && type != 5 )
  {
    fp = open_file( FILE_DIR, "iplog.txt", "a" );
    if( fp == NULL )
      roach( "Could not open ip log file." );
    else
    {
      fprintf( fp, "Ident: %s is %s: %s (%s)\n", link->player->descr->name,
        link->host, link->ident.userid, link->ident.token );
      fclose( fp );
    }
  }
}

/*
 *   SOCKET ROUTINES
 */


/* This great function takes an open port as input, and accepts a connection
   through that port.  It assigns the net_addr, and sets some basic
   flags on the new link.  It then passes the good stuff to write_host. */
void open_link( int port )
{
  link_data*                  link;
  struct sockaddr         net_addr;
  struct sockaddr_in   net_addr_in;
  unsigned int             addrlen;
  int                      fd_conn;

  /* accepts a new socket into the port, from the connect que */
  addrlen = sizeof( net_addr );
  fd_conn = accept( port, &net_addr, &addrlen );

  /* if no new link was opened, return */
  if( fd_conn < 0 )
    return;

  /* don't delay our MUD, just cause the guy isn't responding */
  fcntl( fd_conn, F_SETFL, O_NDELAY );

  
  addrlen = sizeof( net_addr_in  );

  /* get the peername of the guy trying to connect. */
  if( getpeername( fd_conn, (struct sockaddr*) &net_addr_in,
    &addrlen ) == -1 ) {
    bug( "Open_Link: Error returned by getpeername." );
    return;
    }

  /* create a new link data, attatching this guy to the new fd */
  link             = new link_data;
  link->channel    = fd_conn;
  link->connected  = CON_INTRO;
  
  /* ADDED -- PUIOK 7/1/2000 -- IDENT */
  {
    struct sockaddr_in mud_sock;
    unsigned int mud_socklen;
    int result;
        
    mud_socklen = sizeof( mud_sock );

    mud_sock.sin_port = 0;
    result = getsockname( port, (struct sockaddr*) &mud_sock, &mud_socklen );

  //// ADDED TO REMOVE IDENT:  Rengeleif 9/14/00//////
  //
  //
  //  if( result == 0 && mud_sock.sin_port != 0 )
  //    id_create_request( link, ntohs( mud_sock.sin_port ), &net_addr_in,
  //ident_report );
  //  else
  //    bug( "Open_Link: Error getting mud port, can't do ident." );
  }
  /* -- END PUIOK */

  /* get his host name, and finish creating the link data */
  write_host( link, (char*) &net_addr_in.sin_addr ); 

  /* return */
  return;
}


/* This function creates, opens, and accept connections via a socket through
   the inputed port number.  Sets the flags of this socket to SO_LINGER and
   SO_REUSEADDR.  Also sets the socket to exit on EXEC family funct.  Returns
   the connected socket. */
int open_port( int portnum )
{
  struct sockaddr_in         server;
  struct linger         sock_linger;
  struct hostent*              host;
  char*                    hostname  = static_string( );
  int                          sock;
  int                             i  = 1;
  int                            sz  = sizeof( int ); 

  sock_linger.l_onoff  = 1;  //The linger data is set
  sock_linger.l_linger = 0;  //Linger for 0 seconds before closing socket.

  /* Get the host name of the current processor, then check and
     see if you can get the host. If not, set host name to forestsedge.com */
  if( gethostname( hostname, THREE_LINES ) != 0 ) 
    panic( "Open_Port: Gethostname failed." );
    
    hostname = "209.83.132.90";
    
    host = gethostbyname( hostname );

  if( host == NULL )
    panic( "Open_Port: Error in gethostbyname." );


  /* Open up a socket, using the ARPA internet protocols, and socket 
     bit streaming format */
  if( ( sock = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) 
    panic( "Open_port: error in socket call" );

  
  /* Set the properties of the socket.  SO_LINGER makes it so that if the 
     socket closes it will attempt to pass any lingering information on 
     to the process for the time set in linger above.  SO_REUSEADDR allows 
     for the reuse of local addresses. */
  if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char*) &i, sz ) < 0 ) 
    panic( "Open_port: error in setsockopt (SO_REUSEADDR)" );
  
  if( setsockopt( sock, SOL_SOCKET, SO_LINGER, (char*) &sock_linger,
    sizeof( sock_linger ) ) < 0 ) 
    panic( "Open_port: error in setsockopt (SO_LINGER)" );

  /* Set socket so that it will close if an EXEC family function is called. */
  if( fcntl( sock, F_SETFD, 1 ) == -1 )
    panic( "Open_port: Error setting close on Exec." );

  //Zero out the memory address of the server.
  memset( &server, 0, sizeof( struct sockaddr_in ) );

  //Fill in the appriate information 
  server.sin_family      = AF_INET;;
  server.sin_port        = htons( portnum );

  //copy the address of the server into the appropriate place.
  memcpy( &server.sin_addr, host->h_addr_list[0], host->h_length );

  //Name the socket via bind.
  if( bind( sock, (struct sockaddr*) ( &server ), sizeof( server ) ) ) 
    panic( "Open_port: Error binding port %d at %s.",
      portnum, host->h_name );

  //Listen for connections comming in through the socket, que max is 3
  if( listen( sock, 3 ) ) 
    panic( "Open_port: error in listen" );

  //Let interested parties know about the binding.
  printf( "Binding port %d at %s.\n\r", portnum, host->h_name );

  //Return the activated socket, with connection.
  return sock;
}


/* This function takes the input pipes, checks each of the links in the
   'link' list, and handles the I/O.  If the link is dead (or has idled too
   long) the player is saved, and the link is cut.  If there is a command 
   to execute, it gets set to the interpreter, if there is something to 
   write, it gets written.  Also handles ampersand expansion on incomming
   commands. --YEGGO */
void update_links( void )
{
  char_data*            ch;
  link_data*          link;
  text_data*       receive;
  fd_set          read_set;
  fd_set         write_set;
  fd_set          exec_set;
  struct timeval     start;  
  struct timeval   timeout;  

  gettimeofday( &start, NULL );

  timeout.tv_sec  = 1;
  timeout.tv_usec = 0;

  FD_ZERO( &read_set );      //clear the read set
  FD_ZERO( &write_set );     //clear the write set
  FD_ZERO( &exec_set );      //clear the exception set

  FD_SET( socket_one, &read_set );  //add the read sets to the descriptors
  FD_SET( socket_two, &read_set );  //of the sockets

  //scroll through 'link' list and add the sets to the descriptor values

  for( link = link_list; link != NULL; link = link->next ) {
    FD_SET( link->channel, &read_set  );
    FD_SET( link->channel, &write_set );
    FD_SET( link->channel, &exec_set );
    
    /* ADDED -- PUIOK 7/1/2000 -- IDENT */
//    id_check_request(link);
    
    }

  //poll the sets to see if any new info is available
  if( (int) select( FD_SETSIZE, &read_set, &write_set, &exec_set,
    &timeout ) < 0 ) 
    panic( "Update links: select" );

  //if there is anything to read from socket one, read it :)
  if( FD_ISSET( socket_one, &read_set ) )
    open_link( socket_one );

  //if there is anything to read from socket two, read it
    if( FD_ISSET( socket_two, &read_set ) )
    open_link( socket_two );

  //For each link in the 'link' list, do some stuff to it
  for( link = link_list; link != NULL; link = link_next ) {
    link_next = link->next;  //move the link_next pointer
  
    /* if there are available exceptions, save the character, and close the
      link */
      if( FD_ISSET( link->channel, &exec_set ) ) {
      write( link->player );
      close_socket( link );
      continue;
      }
  
      /* if there is something to read, read it.  First set the idle to zero
     then check and see if there is a player there .. if there is reset
     his time.  Then read the stuff from the link.  If you can't, save 
     the player and close the socket. */
    if( FD_ISSET( link->channel, &read_set ) ) {
      link->idle = 0;            
      if( link->character != NULL ) 
        link->character->timer = current_time;
      if( !read_link( link ) ) { 
        write( link->player );   
        close_socket( link );
        continue;
      }
    }

    //if the link has idled for too long, and the player hasn't connected
    //then close the socket
    if( link->idle++ > 10000 && link->connected != CON_PLAYING ) {
      send( link, "\n\r\n\r-- CONNECTION TIMEOUT --\n\r" );
      close_socket( link, TRUE );
    }
  }//end 'link' list for

  //set the world times (pulse rate, and current time)
  pulse_time[ TIME_READ_INPUT ] = stop_clock( start );  
  gettimeofday( &start, NULL );

  /* Check each link in the 'link' list.  If there is a command in the 
     command recieved buffer, set receive to the command, and set the 
     link->command variable to true.  Call ampersand to do the ampersand
     expansion in the command. Move the command recieved buffer to the next
     command, and reset the idle.  If the the player is playing, stop their
     idling and interpret the command.  Otherwise get the nanny on his ass. */
  for( link = link_list; link != NULL; link = link_next ) {
    link_next = link->next;    

    if( link->command = ( ( receive = link->receive ) != NULL ) ) {
      ampersand( receive );   
      link->receive  = receive->next;
      link->idle     = 0;            

      if( link->connected == CON_PLAYING ) {  
        stop_idling( ch = link->character );
        // The following if statement (including the contents)
        // Zeth 12/18/99
        // ch was NULL in interpret(...)

          if(ch == NULL) {
            char str[MAX_STRING_LENGTH*3];
            sprintf(str,"Null ch in links. Cannot interpret:%s",receive->message.text);
            roach(str);
            }
          else
            interpret( link->character, receive->message.text );
        }  
      else      
        nanny( link, receive->message.text );
           
        delete receive; 
      }
    }

  //Look familiar?  set the world times again
  pulse_time[ TIME_COMMANDS ] = stop_clock( start );  
  gettimeofday( &start, NULL );

  /* Check all of the links.  If they have idled for long enough, and 
     there is something to write, process the output.  If you can't
     process it, save the character and close the link. */
  for( link = link_list; link != NULL; link = link_next ) {
    link_next = link->next; 
    if( link->idle%25 == 0 && FD_ISSET( link->channel, &write_set )
      && !process_output( link ) ) {
      write( link->player );
      close_socket( link );
      }
    }  

  //Do the pulse time thing again.
  pulse_time[ TIME_WRITE_OUTPUT ] = stop_clock( start );  

  return;
}


/*
 *   CLOSING OF SOCKETS
 */


void extract( link_data* prev )
{
  link_data* link;

  for( link = link_list; link != NULL; link = link->next )
    if( link->snoop_by == prev )
      link->snoop_by = NULL;

  send( prev->snoop_by, "Your victim has left the game.\n\r" );

  if( link_next == prev )
    link_next = prev->next;

  remove( link_list, prev );

  if( prev->account != NULL && prev->account->last_login == -1  ) 
    extract( prev->account );

  delete prev;

  return;
}


void close_socket( link_data* link, bool process )
{
  char              buf [ MAX_STRING_LENGTH ];
  char_data*         ch;
  player_data*       pc;
  int         connected  = link->connected;
  FILE* fp;

  if( link->channel == -1 ) {
    bug( "Close_Socket: Closing a dead socket??" );
    return;
    }

  if( ( ch = link->player ) != NULL && ch != link->character )
    do_return( link->character, "" );

  for( int i = 0; i < player_list; i++ ) {
    pc = player_list[i];
    if( pc->Is_Valid( )
      && is_set( &pc->status, STAT_CLONED ) && pc->link == link )
      purge_clone( pc );
  }

  if( process ) {
    link->connected = CON_CLOSING_LINK;
    process_output( link );
    }

  if( ch != NULL ) {
    if( connected == CON_PLAYING ) {
      send_seen( ch, "%s has lost %s link.\n\r",
        ch, ch->His_Her( ) );
      sprintf( buf, "%s has lost link.", ch->descr->name );
      info( buf, LEVEL_APPRENTICE, buf, IFLAG_LOGINS, 1, ch );

       fp = open_file( FILE_DIR, "iplog.txt", "a" );
        if( fp == NULL) {
          roach( "Could not open ip log file." );
          }
        else {
        fprintf( fp, "%s from %s lost link at %s", ch->descr->name,
          ch->link->host, (char*) ctime( &current_time ) );
        fclose( fp );
       }

      ch->link = NULL; 
      }
    else {
      if( ch->shdata->level == 0 && ch->pcdata->pfile != NULL ) 
        extract( ch->pcdata->pfile, link );
      ch->Extract( );
      }  
    }

  /* ADDED -- PUIOK 7/1/2000 -- IDENT */
//  id_kill_request( link );

  shutdown( link->channel, 2 );

  extract( link );

  return;
}


/*
 *   INPUT ROUTINES
 */

/* I think this function erases the inputed line from the characters
   terminal.  Returns false if it couldn't write to the link.  Returns true
   if it did what it should, or if it didn't need to */
bool erase_command_line( char_data* ch )
{
  char* tmp  = static_string( );

  /* If there is no character, the characters terminal is set to dumb, 
     the character isn't playing, or the player is using the status bar ..
     return TRUE */
  if( ch == NULL || ch->pcdata->terminal == TERM_DUMB  
    || ch->link->connected != CON_PLAYING
    || !is_set( ch->pcdata->pfile->flags, PLR_STATUS_BAR ) )
    return TRUE;

  //ASK LON WHAT THIS DOES .. IT'S SOME CRYPTIC COMMAND SEQUENCE
  sprintf( tmp, "[%d;1H[J", ch->pcdata->lines );

  /* Write the above cryptic command sequence to the link.  If you can't
     return false */
  if( (int)( write( ch->link->channel, tmp, strlen( tmp ) ) ) == -1 )
    return FALSE; 
  
  /* Correctly worked, return TRUE */
  return TRUE;
}

#define ISESC( c )    ( c == '\e' )
#define set_1( c )    ( c >= 32 && c <= 47 )
#define set_2( c )    ( c >= 48 && c <= 63 )
#define set_3( c )    ( c >= 64 && c <= 126 )

#define NEWLN( c )    ( c == '\n' || c == '\r' )


/* This function reads the input from the link, adding it to the end of
   any pending commands.  It parses the input for ~'s, turning them into -'s.  
   Returns true if the inputted command was correctly stored in the 
   link->recieve string.  Returns false if there was an error writing to 
   or reading from the link, or if too much input was recieved from
   someone who isn't playing yet */
bool read_link( link_data* link )
{
  char            buf  [ 2*MAX_INPUT_LENGTH+100 ];
  text_data*  receive;
  int          length;
  int           nRead;
  char*         input;
  char*        output;

  strcpy( buf, link->rec_pending );

  length  = strlen( buf );
  nRead   = read( link->channel, buf+length, 100 );

  if( nRead <= 0 )
    return FALSE;

  free_string( link->rec_pending, MEM_LINK );
  link->rec_pending = empty_string;

  buf[ length+nRead ] = '\0';

  if( length+nRead > MAX_INPUT_LENGTH-2 ) {
    if( link->connected != CON_PLAYING )
      return FALSE;
    send( link->character, "!! Truncating input !!\n\r" );
    sprintf( buf+MAX_INPUT_LENGTH-3, "\n\r" );
    }

  /* PUIOK 9/5/2000 -- non-standard NEWLN support, BS, DEL support, filters */

  for( input = output = buf; *input != '\0'; ) {
    
    /* 10/5/2000 -- VT Control Sequence filter */
    if( ISESC( *input ) )
    {
      if( *++input == '[' )  /* Control Sequence */
      {
        while( set_2( *++input ) ) ;
        while( set_1( *input ) ) input++;
        
        if( set_3( *input ) ) input++;
      }
      else  /*  Escape Sequence */
      {
        while( set_1( *input ) ) input++;
        
        if( set_2( *input ) || set_3( *input ) ) input++;
      }
      continue;
    }
    
    /* 10/5/2000 -- Telnet Control Sequence filter */
    if( *input == (char) IAC )
    {
      if( *++input <= (char) DONT && *input >= (char) SE )
      {
        if( *input == (char) SB )
        {
          if( *++input != '\0' )
            while( *++input != '\0'
              && ( *input != (char) IAC || *(input+1) != (char) SE ) ) ;
        }
        else if( *++input <= (char) TELOPT_NEW_ENVIRON
          || *input == (char) TELOPT_EXOPL )
          input++;
      }
      continue;
    }
    
    if( ( *input == '\r' && *(input+1) == '\n' )
      || ( *input == '\n' && *(input+1) == '\r' ) )
      input++;
     
    if( *input != '\n' && *input != '\r' ) {
      if( *input == '\b' || *input == 0x7f ) {  /* BS and DEL support */
        if( output > buf )
          output--;
      }
      else if( isascii( *input ) && isprint( *input ) )
        *output++ = ( *input == '~' ? '-' : *input );
      
      input++;
      continue;
    }

    for( ; --output >= buf && *output == ' '; );
    
    *(++output) = '\0';
 
    if( link->connected != CON_PLAYING )  
      receive = new text_data( buf );
    else if( *buf == '!' )
      receive = new text_data( link->rec_prev );
    else {
      receive = new text_data( subst_alias( link, buf ) );
      free_string( link->rec_prev, MEM_LINK );
      link->rec_prev = alloc_string( receive->message.text, MEM_LINK );
      }
    append( link->receive, receive );
    output = buf;
    
    if( !erase_command_line( link->character ) )
      return FALSE;
    input++;
  }

  *output = '\0';
  link->rec_pending = alloc_string( buf, MEM_LINK ); 

  return TRUE;
}


/*
 *   OUTPUT ROUTINES
 */



bool process_output( link_data* link )
{
  text_data*      output;
  text_data*        next;
  char_data*          ch  = link->character;
  bool        status_bar;

  /* If the character is playing, but there is no character, report the
     bug and return false */
  if( link->connected == CON_PLAYING && ch == NULL ) {
    bug( "Process_Output: Link playing with null character." );
    /*bug( "--     Host = '%s'", link->host );
    bug( "-- Rec_Prev = '%s'", link->rec_prev );*/
    return FALSE;
    }

  /* If the character is playing, has the status bar turned on, and isn't
     using terminal type DUMB, set status bar to TRUE. */
  status_bar = ( link->connected == CON_PLAYING
    && is_set( ch->pcdata->pfile->flags, PLR_STATUS_BAR )
    && ch->pcdata->terminal != TERM_DUMB );

  /* If there is nothing to send, and there is no command pending,
     return TRUE */
  if( link->send == NULL && !link->command )
    return TRUE;

  /* SAVE CURSOR */

  
  if( status_bar ) {
    next       = link->send;
    link->send = NULL;
    scroll_window( ch );
    if( next != NULL )
      send( ch, "\n\r" );
    cat( link->send, next );
    prompt_ansi( link );
    command_line( ch );
    }
  else {
    if( !link->command ) {
      next       = link->send;
      link->send = NULL;
      send( ch, "\n\r" );
      cat( link->send, next );
      }  
    if( link->connected == CON_PLAYING && link->receive == NULL ) 
      prompt_nml( link );
    }

  /* SEND OUTPUT */

  for( ; ( output = link->send ) != NULL; ) {
    if( (int)( write( link->channel, output->message.text,
      output->message.length ) ) == -1 )
      return FALSE; 
    link->send = output->next;
    delete output;
    }

  return TRUE;
}


/*
 *   LOGIN ROUTINES
 */


typedef void login_func( link_data*, char* );


struct login_handle
{
  login_func*  function;
  int          state;
};


void nanny( link_data* link, char* argument )
{
  char_data*        ch;
  int                i;


  login_handle nanny_list [] = {
    { nanny_intro,              CON_INTRO              },
    { nanny_acnt_name,          CON_ACNT_NAME          },
    { nanny_acnt_password,      CON_ACNT_PWD           },
    { nanny_acnt_email,         CON_ACNT_EMAIL         },
    { nanny_acnt_enter,         CON_ACNT_ENTER         },
    { nanny_acnt_confirm,       CON_ACNT_CONFIRM       },
    { nanny_acnt_check,         CON_ACNT_CHECK         },
    { nanny_acnt_check_pwd,     CON_ACNT_CHECK_PWD     },
    { nanny_old_password,       CON_PASSWORD_ECHO      },
    { nanny_old_password,       CON_PASSWORD_NOECHO    },
    { nanny_motd,               CON_READ_MOTD          },
    { nanny_imotd,              CON_READ_IMOTD         },
    { nanny_new_name,           CON_GET_NEW_NAME       },
    { nanny_acnt_request,       CON_ACNT_REQUEST       },
    { nanny_acnt_menu,          CON_ACNT_MENU          },
    { nanny_confirm_password,   CON_CONFIRM_PASSWORD   },
    { nanny_set_term,           CON_SET_TERM           },
    { nanny_show_rules,         CON_READ_GAME_RULES    },
    { nanny_agree_rules,        CON_AGREE_GAME_RULES   },
    { nanny_alignment,          CON_GET_NEW_ALIGNMENT  },
    { nanny_help_alignment,     CON_HELP_ALIGNMENT     },
    { nanny_disc_old,           CON_DISC_OLD           },
    { nanny_help_class,         CON_HELP_CLSS          },
    { nanny_class,              CON_GET_NEW_CLSS       },
    { nanny_help_race,          CON_HELP_RACE          },
    { nanny_race,               CON_GET_NEW_RACE       },
    { nanny_stats,              CON_DECIDE_STATS       },  
    { nanny_help_sex,           CON_HELP_SEX           },
    { nanny_sex,                CON_GET_NEW_SEX        },   
    { nanny_new_password,       CON_GET_NEW_PASSWORD   },
    { nanny_acnt_enter,         CON_CE_ACCOUNT         },
    { nanny_acnt_check_pwd,     CON_CE_PASSWORD        },
    { nanny_acnt_email,         CON_CE_EMAIL           },
    { nanny_acnt_enter,         CON_VE_ACCOUNT         },
    { nanny_ve_validate,        CON_VE_VALIDATE        },
    { nanny_acnt_confirm,       CON_VE_CONFIRM         },
    { NULL,                     -1                     }
    };

  skip_spaces( argument );

  ch = link->character;

  for( i = 0; nanny_list[i].function != NULL; i++ ) 
    if( link->connected == nanny_list[i].state ) {
     
      nanny_list[i].function( link, argument );
      return;
      }

  if( link->connected == CON_PAGE ) {
    write_greeting( link );
    link->connected = CON_INTRO;
    return;
    }

  if( link->connected == CON_FEATURES ) {
    write_greeting( link );
    link->connected = CON_INTRO;
    return;
    }  

  if( link->connected == CON_POLICIES ) {
    help_link( link, "Policy_2" );
    link->connected = CON_PAGE;
    return;
    }


  bug( "Nanny: bad link->connected %d.", link->connected );
  close_socket( link );

  return;
}


/* Function pulls a charcter out of the void, and puts him back where he was */
void stop_idling( char_data* ch )
{
  /* If there is no character, there is no link, the character isn't playing,
     or the character has never been in a room, return */
  if( ch == NULL
    || ch->link == NULL
    || ch->link->connected != CON_PLAYING
    || ch->was_in_room == NULL
    || is_set( &ch->status, STAT_DREAMWALKING ) )
    return;

  /* Otherwise, set the characters timer, and return the character to the room
     that he was in (and set his previous room to the void) */
  ch->timer = current_time;
  if( ch->array != NULL )
    ch->From( );
  ch->To( ch->was_in_room );
  ch->was_in_room = NULL;

  send_seen( ch, "%s has returned from the void.\n\r", ch );

  return;
}


/* oooo */
void write_greeting( link_data* link )
{
  help_link( link, "greeting" );
  send( link, "                   Choice: " );

  return;
}



