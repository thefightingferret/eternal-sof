#include "ctype.h"
#include "sys/types.h"
#include "stdio.h"
#include "stdlib.h"
#include "syslog.h"
#include "unistd.h"
#include "define.h"
#include "struct.h"


typedef int pfcomp_func   ( pfile_data*, pfile_data* );


pfile_data*       ident_list  [ MAX_PFILE ];
pfile_data**      pfile_list  = NULL;
pfile_data**      high_score  = NULL;
pfile_data**      death_list  = NULL;
pfile_data**       site_list  = NULL;
int                max_pfile  = 0;
int                 max_high  = 0;
int                max_death  = 0;
int             site_entries  = 0;


int          search          ( pfile_data**, int, pfile_data*, pfcomp_func );
pfcomp_func  compare_exp;
pfcomp_func  compare_death; 
  

/* Expcat - Zemus March 21 */
const char* expcat[ ] = { "Weapons", "Language", "Offensive Spells",
"Assistive Spells", "Healing Spells", "Trade", "Alchemy", "Fighting",
"Theft", "Misc",  "Navigation", "Religion", "Music" };

// routines to display abilities to roach

/* Might need em at some point
#include "ctype.h"
#include "sys/types.h"
#include "stdio.h"
#include "stdlib.h"
#include "define.h"
#include "struct.h"
*/

void do_abil_local( char_data* ch, char* argument )
{
  char        buf  [ MAX_INPUT_LENGTH ];
  char        arg  [ MAX_INPUT_LENGTH ];
  int        i, j;
  int    ps1, ps2;     
  int        cost;
  int      length;
  int        clss;
  bool      found  = FALSE;

const char* skill_cat_name [ MAX_SKILL_CAT ] = { "weapon", "language",
  "spell", "physical", "trade" };

  if( not_player( ch ) )
    return;  

  argument = one_argument( argument, arg );

  /* What class is the player? */
  clss = ch->pcdata->clss;

  /* If not looking for the skills available
   * for a specific class then print current players skills
   */
  if( *argument == '\0' ) {                                                    
    length = strlen( arg );
    for( j = 0; j < MAX_SKILL_CAT; j++ ) {
      if( !strncasecmp( skill_cat_name[j], arg, length ) ) {
       for( i = 0; i < MAX_SKILL; i++ ) {
          if( skill_table[i].category != j
            || ( cost = skill_table[i].prac_cost[clss] ) == -1
            || skill_table[i].level[clss] > ch->shdata->level )
            continue;
          if( !found ) {
            printf( "Skill             Level   Cost   Pracs
Prerequistes\n" );
            found = TRUE;
            }
          if( ch->shdata->skill[i] == 0 )
            sprintf( arg, " unk" );
          else
            sprintf( arg, "%d ", ch->shdata->skill[i] );                       
          sprintf( buf, "%-18s%4s", skill_table[i].name, arg );
          ps1 = skill_table[i].pre_skill[0];
          ps2 = skill_table[i].pre_skill[1];
          if( ch->shdata->skill[i] != 10 )
            sprintf( buf+strlen(buf), "%8d%6d%6s",
              ch->shdata->level*cost*cost, cost, "" );
          else
            sprintf( buf+strlen(buf), "%20s", "" );
          if( ps1 != SKILL_NONE )
            sprintf( buf+strlen(buf), "%s [%d]", skill_table[ps1].name,
              skill_table[i].pre_level[0] );
          if( ps2 != SKILL_NONE )
            sprintf( buf+strlen(buf), " & %s [%d]", skill_table[ps2].name,
              skill_table[i].pre_level[1] );
          sprintf( buf+strlen(buf), "\n\r" );
          printf( "%s", buf );
          }                                                                    
        if( !found )
          printf( "Player has no %s skills.\n", skill_cat_name[j] );
        return;
        }
      }
    printf( "Unknown skill category.\n\r" );
    return;
    }

}

// show_equipment needed here

void do_eq_local( char_data* ch, char_data* victim )
{
  char*       format  = "%-22s %-42s %s\n\r";
  char*          tmp  = static_string( );
  bool         found  = FALSE;
  obj_data**    list  = (obj_data**) victim->wearing.list;
  int           i, j;

  for( i = 0; i < victim->wearing; i = j ) {

    if( victim != ch )
      for( j = i+1; j < victim->wearing
        && list[i]->position == list[j]->position; j++ )
        if( is_set( list[j]->pIndexData->extra_flags, OFLAG_COVER ) )
          i = j;  

    for( j = i; j < victim->wearing
      && list[i]->position == list[j]->position; j++ ) {
      if( ch == victim || list[j]->Seen( ch ) ) {
        if( !found ) {
          printf( "+++ Equipment +++\n" );
          sprintf( tmp, format, "Body Location", "Item", "Condition" );
          printf( tmp );
          found = TRUE;
          }
        printf( format,
          j == i ? where_name[ list[i]->position ] : "",
          list[j]->Name( ch, 1, TRUE ),
          list[j]->condition_name( ch, TRUE ) );
        }
      }
    } 

  if( !found ) {
    if( ch == victim )
      printf( "Player has nothing equipped.\n\r" );
    else
      printf( "%s has nothing equipped.\n\r", victim );
    }
  else
  {
    if( ch == victim )
      printf( "\n\rWeight: %.2f lbs\n\r",
        (float) ch->wearing.weight/100 );
  }
} 

/*
 *   PFILE_DATA CLASS
 */ 


Pfile_Data :: Pfile_Data( const char* string  )
{
  record_new( sizeof( pfile_data ), MEM_PFILE );

  name       = alloc_string( string, MEM_PFILE );
  pwd        = empty_string;
  last_host  = empty_string;
  homepage   = empty_string;

  account = NULL;
  mail    = NULL;
  clan    = NULL;

  last_on     = current_time;
  created     = current_time;

  level       = 0;
  clss        = 0;
  race        = 0;
  sex         = SEX_RANDOM;
  trust       = 0;
  bounty      = 0;
  flags[0]    = ( 1 << PLR_GOSSIP ) | ( 1 << PLR_PROMPT );
  flags[1]    = 0;
  settings    = 0;
  ident       = -1;
  rank        = -1;
  guesses     = 0;

  icq           = empty_string;
  banned        = -1;
  pkcount       = 0;
  bounty        = 0;
  hunter        = empty_string;

  vzero( vote, MAX_VOTE );
}


Pfile_Data :: ~Pfile_Data( )
{
  record_delete( sizeof( pfile_data ), MEM_PFILE );

  if( ident > 0 ) {
    remove_list( pfile_list, max_pfile, this );
    remove_list( site_list, site_entries, this );
    ident_list[ident] = NULL; 
    }

  if( rank >= 0 ) {
    for( int i = rank+1; i < max_high; i++ )
      high_score[i]->rank--;            
    remove( high_score, max_high, rank );
    } 

  clear_auction( this );

  if( clan != NULL ) 
    remove_member( this );

  for( int i = 0; i < max_pfile; i++ ) 
    for( int j = 0; j < MAX_VOTE; j++ ) 
      if( pfile_list[i]->vote[j] == this )
        pfile_list[i]->vote[j] = NULL;

  free_string( name,      MEM_PFILE );
  free_string( pwd,       MEM_PFILE );
  free_string( last_host, MEM_PFILE );
  free_string( homepage,  MEM_PFILE );
  free_string( icq,       MEM_PFILE );
}


void extract( pfile_data* pfile, link_data* prev )
{
  player_data*   victim;
  link_data*       link;
  link_data*       next;
  int               pos  = 0;

  for( link = link_list; link != NULL; link = next ) {
    next = link->next; 
    if( link->pfile == pfile && link != prev ) {
      send( link,
        "\n\r\n\r+++ Character was deleted - Closing link +++\n\r" );
      close_socket( link, TRUE );
      }
    }

  for( int i = 0; i < player_list; i++ ) {
    victim = player_list[i];
    if( victim->Is_Valid( )
      && ( pos = search( victim->pcdata->recognize, pfile->ident ) ) >= 0 )
      remove( victim->pcdata->recognize, pos );
    }

  for( int i = 0; i < obj_list; i++ )
    if( obj_list[i]->owner == pfile )
      obj_list[i]->owner = NULL;

  delete pfile; 
}


void modify_pfile( char_data* ch )
{
  player_data*  pc = (player_data*)ch;
  pfile_data* pfile = ch->pcdata->pfile;
 
  pfile->race      = ch->shdata->race;
  pfile->clss      = ch->pcdata->clss; 
  pfile->sex       = ch->sex;
  pfile->level     = ch->shdata->level;
  pfile->trust     = ch->pcdata->trust;
  pfile->exp       = ch->exp;

  if( pfile->ident < 1 || pfile->ident > MAX_PFILE ) 
    pfile->ident = assign_ident( );

  if( ident_list[pfile->ident] == NULL ) {
    add_list( pfile_list, max_pfile, pfile );
    add_list( site_list, site_entries, pfile ); 
    ident_list[pfile->ident] = pfile;

    if( ch->shdata->level < LEVEL_APPRENTICE ) {
      add_list( high_score, max_high, pfile );
      add_list( death_list, max_death, pfile );
      }
    }
  else {
    if( ident_list[pfile->ident] != pfile ) {

// TEMPORARY CODE TO EXTRACT CORRUPT PFILE DATA - Zerli

//      pfile->ident = assign_ident( );
//      add_list(pfile_list, max_pfile, pfile );
//      add_list( site_list, site_entries, pfile ); 
//      ident_list[pfile->ident] = pfile;

	roach( "Corrupt character found, extracting data." ); 
	roach( "-- Name = %s", pfile->name );

//	printf( "-- Name = %s", pfile->name );

// List abilities, skills, spells, languages
//	printf( "-- Listing abilities\n" );
//	do_abil_local( ch, "s" );
//	do_abil_local( ch, "p" );
//	do_abil_local( ch, "w" );
//	do_abil_local( ch, "l" );
//	do_abil_local( ch, "t" );

// List equipment
//	printf( "-- Listing equipment\n" );
//	do_eq_local( ch, ch );

roach( "Idents are: %d %d", pfile->ident,
ident_list[pfile->ident]->ident);

// DIe test
//      panic( "-- Id = %d", pfile->ident );
      }
    }
  write( pc );
}


/*
 *   SITE LIST ROUTINES
 */


int site_search( const char* word )
{
  int      min  = 0;
  int    value;
  int      mid;
  int      max;

  if( site_list == NULL )
    return -1;

  max = site_entries-1;

  for( ; ; ) {
    if( max < min )
      return -min-1;

    mid    = (max+min)/2;
    value  = rstrcasecmp( site_list[mid]->last_host, word );

    if( value == 0 ) 
      break;
    if( value < 0 )
      min = mid+1;
    else 
      max = mid-1;
    }

  for( ; mid > 0; mid-- )
    if( strcasecmp( site_list[mid-1]->last_host, word ) )
      break;

  return mid;
}


/*
 *   PLAYER NAME LIST
 */


void add_list( pfile_data**& list, int& size, pfile_data* pfile )
{
  int pos;
  int   i;

  if( &list == &site_list ) {
    pos = site_search( pfile->last_host );
    }
  else if( &list == &high_score ) {
    pos = search( high_score, max_high, pfile, compare_exp );
    if( pos < 0 )
      pos = -pos-1;
    pfile->rank = pos; 
    for( i = pos; i < max_high; i++ )
      high_score[i]->rank++;
    }
  else if( &list == &death_list ) {
    pos = search( death_list, max_death, pfile, compare_death );
    }
  else {
    if( ( pos = pntr_search( pfile_list, max_pfile, pfile->name ) ) >= 0 ) {
      roach( "Add_list: Repeated name." );
      panic( "-- Name = '%s'", pfile->name );
      }
    }

  insert( list, size, pfile, pos < 0 ? -pos-1 : pos );
}


void remove_list( pfile_data**& list, int& size, pfile_data* pfile )
{
  int  ident  = pfile->ident;
  int    pos;

  if( ident < 0 )
    return;

  if( &list == &site_list ) {
    for( pos = site_search( pfile->last_host );
      pos < site_entries && ( pos < 0 || site_list[pos] != pfile ); pos++ ) 
      if( pos < 0
        || strcasecmp( site_list[pos]->last_host, pfile->last_host ) ) {
        roach( "Remove_Site: Pfile not found!?" );
        roach( "-- Pfile = %s", pfile->name );
        roach( "--  Host = %s", pfile->last_host );
        for( pos = 0; pos < size; pos++ )
          if( site_list[pos] == pfile ) {
            remove( list, size, pos );
            roach( "Remove_Site: Pfile found at wrong position!" );
            break;
	    }          
        return;
        }
    }
  else {
    pos = pntr_search( list, size, pfile->name );
    }

  if( pos < 0 ) {
    roach( "Remove_list: Pfile not found in list!?" );
    return;
    }

  remove( list, size, pos );
}


int assign_ident( )
{
  register int i;

  for( i = 1; i < MAX_PFILE; i++ )
    if( ident_list[i] == NULL )
      return i;

  roach( "Assign_ident: ident_list full." );
  exit( 1 );
}


/*
 *   SEARCH ROUTINES
 */


pfile_data* player_arg( char*& )
{
  return NULL;
}
 

pfile_data* find_pfile( const char* name, char_data* ch )
{
  int      i;

  i = pntr_search( pfile_list, max_pfile, name );

  if( i >= 0 )
    return pfile_list[i];

  if( -i-1 < max_pfile
    && !strncasecmp( pfile_list[-i-1]->name, name, strlen( name ) ) )
    return pfile_list[-i-1];

  send( ch, "No such player exists.\n\r" );

  return NULL;
} 


pfile_data* find_pfile_exact( const char *name )
{
  int i;

  i = pntr_search( pfile_list, max_pfile, name );

  return( i < 0 ? (pfile_data*) NULL : pfile_list[i] );
}


pfile_data* find_pfile_substring( const char* name )
{
  char      tmp  [ 4 ];
  char*  search;
  int         i;

  memcpy( tmp, name, 3 );
  tmp[3] = '\0'; 
 
  if( ( i = pntr_search( pfile_list, max_pfile, tmp ) ) < 0 )
    i = -i-1;

  for( ; i < max_pfile; i++ ) {
    search = pfile_list[i]->name;
    if( strncasecmp( search, tmp, 3 ) )
      break;
    if( !strncasecmp( search, name, strlen( search ) ) )
      return pfile_list[i];
    }

  return NULL;
}


player_data* find_player( pfile_data* pfile )
{
  for( int i = 0; i < player_list; i++ )
    if( player_list[i]->position != POS_EXTRACTED
      && player_list[i]->pcdata->pfile == pfile
      && player_list[i]->in_room != NULL )
      return player_list[i];

  return NULL;
}


/*
 *   PLAYER FILE COMMANDS
 */


void forced_quit( player_data* ch, bool crash )
{
  char              tmp  [ TWO_LINES ];
  link_data*   link_new;
  link_data*  link_next;
  link_data*       link  = ch->link;
  wizard_data*      imm  = wizard( ch );
  FILE* fp;
  
  if( !is_set( &ch->status, STAT_CLONED ) )
  {
    fp = open_file( FILE_DIR, "iplog.txt", "a" );
    if( fp == NULL ) {
       roach( "could not open ip log file");
    }
    else {  
      fprintf( fp, "%s quit at %s", ch->descr->name,
        (char*) ctime( &current_time ) );
      fclose( fp );
    }
    
    if( ch->switched != NULL )
      do_return( ch->switched, "" );
  
    if( link != NULL ) {
      if( link->connected != CON_PLAYING ) {
        close_socket( link, TRUE );
        return;
      }    
      reset_screen( ch );
      convert_to_ansi( ch, "Thank you for visiting @yThe Summers of Faldyor@n.\n\r", tmp );
      send( link, tmp );
    }
  }

  send_seen( ch, "%s has left the game.\n\r", ch );

  sprintf( tmp, "%s has quit.", ch->descr->name );
  info( "", ( imm != NULL && is_set( ch->pcdata->pfile->flags, PLR_WIZINVIS )
    ? imm->wizinvis : 0 ), tmp, IFLAG_LOGINS, 1, ch );
  
  if( is_set( &ch->status, STAT_CLONED ) )
  {
    purge_clone( ch );
    return;
  }

  if( !crash )
    remove_bit( ch->pcdata->pfile->flags, PLR_CRASH_QUIT ); 

  write( ch );
  reference( ch, ch->contents, -2 );

  for( link_new = link_list; link_new != NULL; link_new = link_next ) {
    link_next = link_new->next; 
    if( link_new->character != NULL && link_new != link
      && !strcmp( link_new->character->descr->name, ch->descr->name ) ) {
      write_to_buffer( link_new, "\n\r\n\rFor security reasons closing link : please reconnect.\n\r" );
      close_socket( link_new, TRUE );
      }
    }

  ch->Extract( );

  if( link != NULL ) 
    close_socket( link, TRUE );
}


void do_quit( char_data* ch, char* )
{
  room_data*     room;
  player_data*     pc;

  if( is_mob( ch ) )
    return;

  pc = player( ch );
  
  if( is_set( &pc->status, STAT_CLONED ) )
  {
    forced_quit( pc, is_set( &pc->status, STAT_FORCED ) );
    return;
  }

  remove_bit( &ch->status, STAT_CANGROUP );

  if( opponent( ch ) != NULL ) {
    send( ch, "You can't quit while fighting.\n\r" );
    return;
    }

  if( ch->position < POS_STUNNED  ) {
    send( ch, "You're not DEAD yet.\n\r" );
    return;
    }

  if( ch->shdata->level < LEVEL_APPRENTICE
    && can_pkill( ch, NULL, FALSE ) ) {
    send( ch, "You cannot quit in rooms which allow pkill.\n\r" );
    return;
    }

  forced_quit( pc, is_set( &pc->status, STAT_FORCED ) );
}


void do_delete( char_data* ch, char* argument )
{
  char*            tmp  = static_string( );
  player_data*  pc;

  if( is_mob( ch ) ) 
    return;

  pc = (player_data*) ch;
  
  if( is_set( &pc->status, STAT_CLONED ) )
  {
    purge_clone( pc );
    return;
  }

  if( ch->shdata->level > 5 && ch->pcdata->trust < LEVEL_APPRENTICE ) {
    send( ch, "Characters over level five may not delete.  Time has shown that\n\rplayers often delete and then regret the action.  It also serves no purpose.\n\rIf you no longer wish to play just quit and do not return.\n\r" );
    return;
    }

  if( strcmp( argument, ch->pcdata->pfile->pwd ) ) {
    send( ch,
      "You must type delete <password> to delete you character.\n\r" );
    return;
    }

  send_seen( ch, "%s throws %sself on %s sword.\n\r", ch,
    ch->Him_Her( ), ch->His_Her( ) );
  send_seen( ch, "Death gratefully takes %s's spirit.\n\r", ch );

  clear_screen( ch );
  reset_screen( ch );

  send( ch, "Your character has been vaporized.\n\r" );
  send( ch, "Death is surprisingly peaceful.\n\r" );
  send( ch, "Good night.\n\r" );

  sprintf( tmp, "%s has deleted %sself.", ch->descr->name,
    ch->Him_Her( ) );
  info( tmp, LEVEL_APPRENTICE, tmp, IFLAG_LOGINS, 1, ch );

  player_log( ch, "Deleted." );
  purge( pc );
}


void purge( player_data* ch )
{
  pfile_data*  pfile  = ch->pcdata->pfile;
  
  delete_file( PLAYER_DIR,      ch->descr->name, FALSE );
  delete_file( PLAYER_PREV_DIR, ch->descr->name, FALSE );
  delete_file( BACKUP_DIR,      ch->descr->name, FALSE );
  delete_file( MAIL_DIR,        ch->descr->name, FALSE );
  delete_file( EXTRA_DIR,       ch->descr->name, FALSE );

  if( ch->link != NULL && boot_stage == 2 ) {
    ch->link->player = NULL;
    close_socket( ch->link, TRUE );
    }

  ch->Extract( );
  extract( pfile );
}


/*
 *  SAVE
 */


void do_save( char_data* ch, char* )
{
  player_data* pc;

  if( is_mob( ch ) )
    return;

  if( is_set( &ch->status, STAT_CLONED ) && ch->link != NULL )
    pc = (player_data*) ch->link->player;
  else
    pc = (player_data*) ch;

  if( !is_set( &ch->status, STAT_FORCED )
    && ch->save_time-180 > current_time ) { 
    send( ch, "You are saving your character too frequently - request denied.\n\r" );
    }
  else {
    write( pc );
    reference( pc, ch->contents, -1 );
    send( ch, "Your character has been saved.\n\r" );
    }
}


/*
 *   INFORMATIONAL COMMANDS 
 */


void do_title( char_data* ch, char* argument )
{
  char            tmp  [ MAX_INPUT_LENGTH ];
  wizard_data*    imm;
  int           flags;
  char   first_letter;  /* PUIOK 30/12/1999 */

  if( is_mob( ch ) )
    return;

  if( ch->shdata->level < 10 ) {
    send( ch, "You must be at least level 10 to set your title.\n\r" );
    return;
    }

 /* notitle - zemus august 1 */
  if( is_set( &ch->status, STAT_NOTITLE ) ) {
    send( ch, "You may not set your title.\n\r" );
    return;
    }

  if( !get_flags( ch, argument, &flags, "l", "Title" ) )
    return;;

  if( is_set( &flags, 0 ) && has_permission( ch, PERM_SHUTDOWN ) ) {
    if( strlen( argument ) > 13 ) {
      send( ch, "Your level title must be less than 13 characters.\n\r" );
      return;
      }
    imm = wizard( ch );
    free_string( imm->level_title, MEM_WIZARD );
    imm->level_title = alloc_string( argument, MEM_WIZARD );
    send( ch, "Your level title has been set to '%s'.\n\r", 
      argument );
    return;
    }

  if( *argument == '\0' ) {
    send( ch, "What do you want to set your title to?\n\r" );
    return;
    }
  
  first_letter = *next_visible( argument );  /* PUIOK 30/12/1999 */
  sprintf( tmp, "%s%s", first_letter == ',' || first_letter == '\'' 
       ? "" : " ", argument );
  withcolor_trunc( tmp, 52-strlen( ch->descr->name ) );
  
  free_string( ch->pcdata->title, MEM_PLAYER );
  ch->pcdata->title = alloc_string( tmp, MEM_PLAYER );

  send( ch, "Your title has been changed.\n\r" );
}


void do_password( char_data* ch, char* argument )
{
  char       arg  [ MAX_INPUT_LENGTH ];
  char**     pwd;
  int      flags;

  if( is_mob( ch ) || is_set( &ch->status, STAT_CLONED )
    || !get_flags( ch, argument, &flags, "a", "Password" ) )
    return;

  argument = one_argument( argument, arg );

  if( *arg == '\0' || *argument == '\0' ) {
    send( "Syntax: password <old> <new>.\n\r", ch );
    return;
    }

  if( is_set( &flags, 0 ) ) {
    if( ch->pcdata->pfile->account == NULL ) {
      send( ch, "You don't have an account to change the password of.\n\r" );
      return;
    }
    pwd = &ch->pcdata->pfile->account->pwd;
  }
  else
    pwd = &ch->pcdata->pfile->pwd;

  if( strcmp( arg, *pwd ) ) {
    send( "Wrong password.\n\r", ch );
    bug( "Changing Password: Incorrect guess - %s.", ch->descr->name );
    return;
    }

  if( strlen( argument ) < 5 ) {
    send( ch, "New password must be at least five characters long.\n\r" );
    return;
    }

  free_string( *pwd, MEM_PFILE );
  *pwd = alloc_string( argument, MEM_PFILE );

  if( is_set( &flags, 0 ) ) {
    send( ch, "Account password changed.\n\r" );
    save_accounts( );
    }
  else {
    send( ch, "Password changed.\n\r" );
    write( (player_data*) ch );
    }

  return;
}


/*
 *   HIGH SCORE ROUTINES
 */


const char* high_score_name( int i, char_data* ch )
{
  if( i < 0 || i >= max_high )
    return "---";

  return( is_incognito( high_score[i], ch ) ? "???"
    : high_score[i]->name );
}


void do_high( char_data* ch, char* argument )
{
  char       tmp  [ TWO_LINES ];
  char        r1  [ 8 ];
  char        r2  [ 8 ];
  int        who  [ 20 ];
  int     c1, c2;
  int          i;
  int       clss  = -1;
  int       race  = -1;

  if( is_confused_pet( ch ) )
    return;
  
  if( isalpha( *argument ) ) {
    for( clss = 0; clss < MAX_CLSS; clss++ )
      if( matches( argument, clss_table[clss].name ) )
        break;

    for( race = 0; race < MAX_PLYR_RACE; race++ )
      if( matches( argument, race_table[race].name ) )
        break;

    if( race == MAX_PLYR_RACE && clss == MAX_CLSS ) {
      send( ch, "Illegal syntax - see help high score.\n\r" );
      return;
      }

    c1 = min( max( atoi( argument ), 1 ), max_high );

    for( i = c2 = 0; c2-c1 < 19 && i < max_high; i++ )
      if( ( clss == MAX_CLSS || high_score[i]->clss == clss )
        && ( race == MAX_PLYR_RACE || high_score[i]->race == race )
        && ++c2 >= c1 )
        who[c2-c1] = i;

    for( i = max( c2-c1+1, 0 ); i < 20; i++ )
      who[i] = -1;
  
    c2 = c1+10;
    }
  else {
    if( isdigit( *argument ) ) {
      c1 = max( min( atoi( argument ), max_high-20 ), 1 );
      c2 = c1+10;
      }
    else {
      c1 = 1;
      c2 = max( 11, min( ch->pcdata->pfile->rank-5, max_high-10 ) );
      }
    }

  send( ch, "              _         _   _  _   _   _         _  ___ \n\r" );
  send( ch, "       |_| | | _ |_|   |_  |  | | |_| |_   |  | |_   |  \n\r" );
  send( ch,"       | | | |_| | |    _| |_ |_| | \\ |_   |_ |  _|  |  \n\r" );
  send( ch, "\n\r\n\r" );

  for( i = 0; i < 10; i++ ) {
    sprintf( r1,  "[%d]", c1+i );
    sprintf( r2,  "[%d]", c2+i );
    sprintf( tmp, "%12s  %-20s %5s  %s\n\r",
      r1, high_score_name( clss == -1 ? c1+i-1 : who[i], ch ),
      r2, high_score_name( clss == -1 ? c2+i-1 : who[10+i], ch ) );
    send( tmp, ch );
    }
}   


void update_score( char_data* ch )
{
  int          pos;
  pfile_data*  pfile;

  if( ch->pcdata == NULL )
    return;

  pfile       = ch->pcdata->pfile;
  pfile->exp  = ch->exp;

  if( pfile->rank == -1 )
    return;

  for( pos = pfile->rank; pos > 0; pos-- ) {
    if( compare_exp( pfile, high_score[pos-1] ) <= 0 )
      break;
    high_score[pos-1]->rank++;
    swap( high_score[pos], high_score[pos-1] );
    }

  for( ; pos < max_high-2; pos++ ) {
    if( compare_exp( pfile, high_score[pos+1] ) >= 0 )
      break;
    high_score[pos+1]->rank--;
    swap( high_score[pos], high_score[pos+1] );
    }

  pfile->rank = pos;
}


/*
 *   SEARCH HIGH & DEATH LISTS
 */
 

int compare_exp( pfile_data *p1, pfile_data* p2 )
{
  if( p1->level != p2->level ) 
    return( p1->level > p2->level ? 1 : -1 );
  
  if( p1->exp < p2->exp )
    return -1;

  return( p1->exp > p2->exp );
}


int compare_death( pfile_data* p1, pfile_data* p2 )
{
  if( p1->deaths != p2->deaths ) 
    return( p1->deaths > p2->deaths ? 1 : -1 );
  
  return 0; 
}


int search( pfile_data** list, int max, pfile_data* pfile,
  pfcomp_func* compare )
{
  int     min  = 0;
  int     mid;
  int   value;

  if( list == NULL )
    return -1;

  max--;

  for( ; ; ) {
    if( max < min )
      return -min-1;

    mid    = (max+min)/2;
    value  = ( *compare )( pfile, list[mid] );

    if( value == 0 )
      break;
    if( value < 0 )
      min = mid+1;
    else 
      max = mid-1;
    }

  return mid;
}


bool spam_warning( char_data* ch )
{
  if( ch == NULL || ch->pcdata == NULL
    || !is_set( ch->pcdata->pfile->flags, PLR_CONTINUOUS ) )
    return FALSE;

  send( ch,
    "Doing that in continuous mode is a sure way of getting spam dropped.\n\r" );
  return TRUE;
}

