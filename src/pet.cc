#include <ctype.h>
#include "stdlib.h"
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"

/* pet_data stuff - zemus - april 9 */
Pet_Data :: Pet_Data( )
{
 max_level = 0;
 level     = 0;
 exp       = 0;

}

/*
 *   NAME FUNCTIONS
 */


void do_name( char_data* ch, char* argument )
{
  char           arg  [ MAX_INPUT_LENGTH ];
  char_data*  victim;

  argument = one_argument( argument, arg );

  if( ( victim = one_character( ch, arg, "name", ch->array ) ) == NULL )
    return;

  if( victim->species == NULL || victim->leader != ch
    || !is_set( &victim->status, STAT_PET ) ) {
    send( ch, "You can only name pets of your own.\n\r" );
    return;
    } 

  if( *argument == '\0' ) {
    send( ch, "What do you want to name them?\n\r" );
    return;
    }

  send( ch, "%s is now named '%s'.\n\r", victim, argument );

  free_string( victim->pet_name, MEM_MOBS );
  victim->pet_name = alloc_string( argument, MEM_MOBS );

  return;
}


/*
 *   PET ROUTINES
 */


void abandon( player_data* pc, char_data* pet )
{
  char* tmp;

  pet->shown = 1;
  send( pc, "You abandon %s.\n\r", pet );

  if( pc->familiar == pet )
    pc->familiar = NULL;

  remove_bit( &pet->status, STAT_PET );
  stop_follower( pet );
  free_string( pet->pet_name, MEM_MOBS );
  pet->pet_name = empty_string; 

  tmp = static_string( );
  sprintf( tmp, "Abandoned %s in room %d.", pet->Name( ),
pet->in_room != NULL ? 0 : pet->in_room->vnum );
  player_log( pc, tmp );      
}


void do_pets( char_data* ch, char* argument )
{
  char_data*        pet;
  char_data*     victim;
  player_data*       pc;
  bool            found  = FALSE;
  int              i, j;
  char             tmp [ MAX_STRING_LENGTH ];

  if( is_mob( ch ) )
    return;

  pc = player( ch );

  if( exact_match( argument, "abandon" ) ) {
    j = atoi( argument );
    for( i = 0; i < ch->followers.size; i++ ) {
      if( is_pet( pet = ch->followers.list[i] ) && --j == 0 ) {
        abandon( pc, pet );
        return;
        }
      }
    send( ch, "You have no pet with that number.\n\r" );
    return;
    }

  if( *argument == '\0' ) {
    victim = ch;
    }
  else if( ch->shdata->level < LEVEL_APPRENTICE ) {
    send( ch, "Unknown syntax - See help pets.\n\r" );
    return;
    }
  else {
    if( ( victim = one_character( ch, argument, "pet", ch->array ) ) == NULL )
      return;

    if( victim->species != NULL ) {
      send( ch, "%s isn't a player and thus won't have pets.\n\r", victim );
      return;
      }     
    }
    
  for( i = j = 0; i < victim->followers.size; i++ ) {
    if( is_pet( pet = victim->followers.list[i] ) ) {
      if( !found ) {
        sprintf( tmp, "%-5s%-20s %-5s %s", "Num", "Name", "Level", "Location" );
        send_underlined( ch, "%s\n\r", tmp );
        found = TRUE;
        }
      send( ch, "%3d  %-20s %-5d %s\n\r", ++j, 
	    pet->Seen_Name( ch ), pet->shdata->level,
        pet->in_room == NULL ? "nowhere??" : pet->in_room->name );
      } 
    }

  if( !found ) {
    if( ch == victim )
      send( ch, "You have no pets.\n\r" );
    else
      send( ch, "%s has no pets.\n\r", victim );
    }
}


void list_pets( char_data* ch, player_data* pc )
{
  char               tmp [ THREE_LINES ];
  char          pet_name [ THREE_LINES ];
  char_data*         pet;
  
  for( int i = 0; i < pc->followers; i++ ) {
    pet = pc->followers[i];
    if( is_set( &pet->status, STAT_PET ) ) {
      strcpy( pet_name, pet->Name( ) );
      truncate( pet_name, 20 );
      sprintf( tmp, "%c%-3c %-20s %4d %-15s %s", 
        ( pet == pc->familiar ) ? 'F' : ' ',
        ( pet->pcdata != NULL ) ? 'S' : ' ', pet_name,
        pet->species != NULL ? pet->species->vnum : 0,
        pc->descr->name,
        ( pet->array != NULL && pet->array->where != NULL ) ?
        pet->array->where->Location( ) : "nowhere" );
      truncate( tmp, 78 );
      page( ch, "%s\n\r", tmp );
      }
    }
}


void do_petlist( char_data* ch, char* argument )
{
  char         tmp [ TWO_LINES ];
  player_data*  pc;
  
  sprintf( tmp, "%-4s %-20s %-4s %-15s %s",
    "Fmlr", "Name", "Vnum", "Master", "Location" );
  page_underlined( ch, "%s\n\r", tmp );
  
  for( int i = 0; i < player_list; i++ ) {
    if( ( pc = player_list[i] )->In_Game( ) )
      list_pets( ch, pc );
    }
}


/*
 *   PET SUPPORT FUNCTIONS
 */


bool has_mount( char_data* ch )
{
  char_data*  mount;
  int            i;

  for( i = 0; i < ch->followers.size; i++ ) {
    mount = ch->followers.list[i];
    if( mount->species != NULL
      && is_set( &mount->species->act_flags, ACT_MOUNT ) ) {
      send( ch, "You are only able to acquire one mount at a time.\n\r" );
      return TRUE;
      }
    }

  return FALSE;
}



bool has_elemental( char_data* ch )
{
  char_data*  buddy;
  int            i;

  for( i = 0; i < ch->followers.size; i++ ) {
    buddy = ch->followers.list[i];
    if( buddy->species != NULL
      && is_set( &buddy->species->act_flags, ACT_ELEMENTAL ) ) {
      return TRUE;
      }
    }

  return FALSE;
}

int number_of_pets( char_data* ch )
{
  char_data*  pet;
  int         num;
  int           i;

  for( i = num = 0; i < ch->followers.size; i++ ) {
    pet = ch->followers.list[i];
    if( is_set( &pet->status, STAT_PET )
      && !is_set( &pet->status, STAT_TAMED )
      && !is_set( &pet->species->act_flags, ACT_MOUNT ) ) 
      num++;
    }

  return num;
}


int pet_levels( char_data* ch )
{
  char_data*    pet;
  int         level;
  int             i;

  for( i = level = 0; i < ch->followers.size; i++ ) {
    pet = ch->followers.list[i];
    if( is_set( &pet->status, STAT_TAMED ) )
      level += pet->shdata->level;
    }

  return level;
}






