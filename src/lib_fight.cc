#include "ctype.h"
#include "sys/types.h"
#include "stdio.h"
#include "stdlib.h"
#include "define.h"
#include "struct.h"


/*
 *   FIGHT ROUTINES
 */


void* code_attack_weapon( void** argument )
{
  char_data*      ch  = (char_data*) argument[0];
  char_data*  victim  = (char_data*) argument[1];
  int            dam  = (int)        argument[2];
  char*       string  = (char*)      argument[3];
  obj_data*    wield;
  int              i;

  if( string == NULL ) {
    code_bug( "Code_Attack_Weapon: NULL attack string." );
    return NULL;
    }

  if( ch == NULL || victim == NULL 
    || ch->in_room != victim->in_room
    || ch->position <= POS_RESTING ) 
    return NULL;

  wield = ch->Wearing( WEAR_HELD_R );


    // Queue up a leap, if one isn't already queued.
  if ( ch->fighting == NULL && victim->position > POS_DEAD &&
        victim->Is_Valid() ) {
     init_attack( ch, victim );
  } 

  push( );

  i = attack( ch, victim, wield == NULL ? string
    : weapon_attack[ wield->value[3] ], wield,
    wield == NULL ? dam : -1, 0 );
 
  pop( );


  return (void*) i;
}


void* code_attack_room( void** argument )
{
  char_data*        ch  = (char_data*) argument[0];
  int              dam  = (int)        argument[1];
  char*         string  = (char*)      argument[2];
  char_data*       rch;

  if( string == NULL ) {
    code_bug( "Code_Attack_Room: NULL attack string." );
    return NULL;
    }

  if( ch == NULL || ch->position < POS_SLEEPING ) 
    return NULL;

  for( int i = *ch->array-1; i >= 0; i-- ) {
    if( ( rch = character( ch->array->list[i] ) ) != NULL
      && rch != ch && ( rch->pcdata != NULL
      || is_set( &rch->status, STAT_PET ) ) ) {
      damage_physical( rch, ch, dam, string );
      if( !ch->Is_Valid( ) )
        return NULL;
      }
    }

  return NULL;
}


void* code_attack( void** argument )
{
  char_data*      ch  = (char_data*) argument[0];
  char_data*  victim  = (char_data*) argument[1];
  int            dam  = (int)        argument[2];
  char*       string  = (char*)      argument[3];
  int              i;

  if( string == NULL ) {
    code_bug( "Code_Attack: NULL attack string." );
    return NULL;
    }

  if( ch == NULL || victim == NULL
    || ch->in_room != victim->in_room
    || ch->position <= POS_RESTING ) 
    return NULL;



    // Queue up a leap, if one isn't already queued.
  if ( ch->fighting == NULL && victim->position > POS_DEAD &&
        victim->Is_Valid() ) {
     init_attack( ch, victim );
  } 

  push( );

  i = attack( ch, victim, string, NULL, dam, 0 );

  pop( );
   

  return (void*) i;
}


/*
 *   ELEMENTAL ATTACKS
 */


void* element_attack( void** argument, int type )
{
  char_data*      ch  = (char_data*) argument[0];
  char_data*  victim  = (char_data*) argument[1];
  int            dam  = (int)        argument[2];
  char*       string  = (char*)      argument[3];
  int              i;

  if( string == NULL ) {
    code_bug( "Attack: Null string." );
    return NULL;
    }

  // Sphenotus Sept 6/00

  if( ch == NULL || victim == NULL || ch->in_room != victim->in_room || 
      ch->position < POS_FIGHTING ) 
    return NULL;



  // Queue up a leap, if one isn't already queued.
  if ( ch->fighting == NULL && victim->position > POS_DEAD &&
        victim->Is_Valid() ) {
     init_attack( ch, victim );
  }


  push( );

  i = attack( ch, victim, string, NULL, dam, 0, type );

  pop( );


  return (void*) i;
}


#define ea( i )  element_attack( arg, i )

void* code_attack_acid  ( void** arg ) { return ea( ATT_ACID ); }
void* code_attack_cold  ( void** arg ) { return ea( ATT_COLD ); }
void* code_attack_shock ( void** arg ) { return ea( ATT_SHOCK ); }
void* code_attack_fire  ( void** arg ) { return ea( ATT_FIRE ); }

#undef ea


/*
 *   DAMAGE ROUTINES
 */ 


void* element_inflict( void** argument, int type )
{
  char_data*  victim  = (char_data*) argument[0];
  char_data*      ch  = (char_data*) argument[1];
  int            dam  = (int)        argument[2];
  char*       string  = (char*)      argument[3];

  if( string == NULL ) {
    code_bug( "Inflict: Null string." );
    return NULL;
    }

  if( victim != NULL )
    inflict( victim, ch, dam, string );

  return NULL;
}


#define ei( i )  element_inflict( arg, i )

void* code_inflict       ( void** arg ) { return ei( ATT_PHYSICAL ); }
void* code_inflict_acid  ( void** arg ) { return ei( ATT_ACID ); }
void* code_inflict_cold  ( void** arg ) { return ei( ATT_COLD ); }
void* code_inflict_shock ( void** arg ) { return ei( ATT_SHOCK ); }
void* code_inflict_fire  ( void** arg ) { return ei( ATT_FIRE ); }

#undef ei( i )


void* 
dam_message_element( void** argument, int type )
{
  char_data*  ch  = (char_data*) argument[0];
  int        dam  = (int)        argument[1];
  char*   string  = (char*)      argument[2];
  
  if( ch == NULL )
    return NULL;

  switch (type)
  {
  case ATT_ACID:
     dam_message( ch, NULL, dam, string, lookup( acid_index, dam ) );
     break;
  case ATT_COLD:
     dam_message( ch, NULL, dam, string, lookup( cold_index, dam ) );
     break;
  case ATT_SHOCK:
     dam_message( ch, NULL, dam, string, lookup( electric_index, dam ) );
     break;
  case ATT_FIRE:
     dam_message( ch, NULL, dam, string, lookup( fire_index, dam ) );
     break;
  default:
     dam_message( ch, NULL, dam, string, lookup( physical_index, dam ) );
     break;
  }

  return NULL;
}

#define dme( i )  dam_message_element( arg, i )

void* code_dam_message       ( void** arg ) { return dme( ATT_PHYSICAL ); }
void* code_dam_message_acid  ( void** arg ) { return dme( ATT_ACID ); }
void* code_dam_message_cold  ( void** arg ) { return dme( ATT_COLD ); }
void* code_dam_message_shock ( void** arg ) { return dme( ATT_SHOCK ); }
void* code_dam_message_fire  ( void** arg ) { return dme( ATT_FIRE ); }

#undef dme( i )



