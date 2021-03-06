#define TERM_DUMB                   0
#define TERM_VT100                  1
#define TERM_ANSI                   2
#define MAX_TERM                    3

#define COLOR_CATEGORY_CHANNEL      0
#define COLOR_CATEGORY_COMM         1
#define COLOR_CATEGORY_DISPLAY      2
#define COLOR_CATEGORY_INFO         3
#define COLOR_CATEGORY_COMBAT       4
#define COLOR_CATEGORY_MISC         5
#define COLOR_CATEGORY_COLOR        6
#define MAX_COLOR_CATEGORY          7

#define COLOR_DEFAULT               0
#define COLOR_ROOM_NAME             1
#define COLOR_TELLS                 2
#define COLOR_SAYS                  3
#define COLOR_GOSSIP                4
#define COLOR_PLAYERS               5
#define COLOR_MOBS                  6
#define COLOR_OBJECTS               7
#define COLOR_EXITS                 8
#define COLOR_STATUS                9  /* unused */
#define COLOR_TITLES               10
#define COLOR_CTELL                11
#define COLOR_CHAT                 12
#define COLOR_OOC                  13
#define COLOR_GTELL                14
#define COLOR_AUCTION              15
#define COLOR_INFO                 16
#define COLOR_TO_SELF              17
#define COLOR_TO_GROUP             18
#define COLOR_BY_SELF              19
#define COLOR_BY_GROUP             20
#define COLOR_MILD                 21
#define COLOR_STRONG               22
#define COLOR_BLACK                23
#define COLOR_RED                  24
#define COLOR_BOLD_RED             25
#define COLOR_GREEN                26
#define COLOR_BOLD_GREEN           27
#define COLOR_YELLOW               28
#define COLOR_BOLD_YELLOW          29
#define COLOR_BLUE                 30
#define COLOR_BOLD_BLUE            31
#define COLOR_MAGENTA              32
#define COLOR_BOLD_MAGENTA         33
#define COLOR_CYAN                 34
#define COLOR_BOLD_CYAN            35
#define COLOR_WHITE                36
#define COLOR_BOLD_WHITE           37
#define COLOR_IMMTALK              38
#define COLOR_GODTALK              39
#define COLOR_BUILDCHAN            40
#define COLOR_MISCSET              41
#define COLOR_CODECHAN             42
#define COLOR_AVATARCHAN           43
#define COLOR_NEWS                 44
#define COLOR_NEWBIECHAN           45
#define COLOR_IMPROVES             46
#define COLOR_BASH_TO_SELF         47
#define COLOR_BASH_BY_SELF         48
#define COLOR_CHANT                49
#define MAX_COLOR                  50


#define ANSI_NORMAL                 0
#define ANSI_BOLD                   1
#define ANSI_REVERSE                7
#define ANSI_UNDERLINE              4
#define ANSI_BLACK                 30
#define ANSI_RED                   31
#define ANSI_GREEN                 32
#define ANSI_YELLOW                33
#define ANSI_BLUE                  34
#define ANSI_MAGENTA               35
#define ANSI_CYAN                  36
#define ANSI_WHITE                 37
#define ANSI_BOLD_RED              64*ANSI_BOLD+ANSI_RED
#define ANSI_BOLD_GREEN            64*ANSI_BOLD+ANSI_GREEN
#define ANSI_BOLD_YELLOW           64*ANSI_BOLD+ANSI_YELLOW
#define ANSI_BOLD_BLUE             64*ANSI_BOLD+ANSI_BLUE
#define ANSI_BOLD_MAGENTA          64*ANSI_BOLD+ANSI_MAGENTA
#define ANSI_BOLD_CYAN             64*ANSI_BOLD+ANSI_CYAN
#define ANSI_BOLD_WHITE            64*ANSI_BOLD+ANSI_WHITE


#define VT100_NORMAL                0
#define VT100_BOLD                  1
#define VT100_REVERSE               2
#define VT100_UNDERLINE             3
#define MAX_VT100                   4


typedef const char* term_func  ( int );


class Term_Type
{
  public:
    char*                name;
    int               entries;
    const char**       format;
    term_func*          codes;
    const int*       defaults;
};


extern  const char*      format_vt100  [ ];
extern  const char*      codes_vt100   [ ];
extern  const char*      format_ansi   [ ];
extern  const char*      codes_ansi    [ ];
extern  const char*      color_fields  [ ];
extern  const term_type  term_table    [ ];
extern  const char*      color_key;


const char*   bold_red_v       ( char_data* );
const char*   bold_cyan_v      ( char_data* );
const char*   bold_v           ( char_data* );

const char*   red              ( char_data* );
const char*   green            ( char_data* );
const char*   c_normal         ( char_data* );
const char*   yellow           ( char_data* );
const char*   blue             ( char_data* );

const char*   to_self          ( char_data* );
const char*   by_self          ( char_data* );
const char*   color_scale      ( char_data*, int );
 
void          convert_to_ansi  ( char_data*, const char*, char*, int = -1 );  
void          send_color       ( char_data*, int, const char* );
void          page_color       ( char_data*, int, const char* );

const char*   get_color        ( char_data*, int );  /* PUIOK 24/4/2000 */

inline const char* color_code( char_data* ch, int color )
{
  if( ch->pcdata == NULL || ch->pcdata->terminal == TERM_DUMB )
    return empty_string;

  return term_table[ ch->pcdata->terminal ].codes(
    ch->pcdata->color[ color ] );
}


/*
 *   SCREEN ROUTINES
 */


void  scroll_window  ( char_data* );
void  command_line   ( char_data* );
void  setup_screen   ( char_data* );
void  clear_screen   ( char_data* );
void  reset_screen   ( char_data* );


inline void save_cursor      ( char_data* ch )  { send( ch, "7" ); }
inline void restore_cursor   ( char_data* ch )  { send( ch, "8" ); }
inline void cursor_on        ( char_data* ch )  { send( ch, "[?25h" ); }
inline void cursor_off       ( char_data* ch )  { send( ch, "[?25l" ); }
inline void lock_keyboard    ( char_data* ch )  { send( ch, "[2l" ); }
inline void unlock_keyboard  ( char_data* ch )  { send( ch, "[2h" ); }


inline void move_cursor( char_data* ch, int line, int column )
{
  send( ch, "[%d;%dH", line, column );
}
  

inline void scroll_region( char_data* ch, int top, int bottom )
{
  send( ch, "[%d;%dr", top, bottom );
}






