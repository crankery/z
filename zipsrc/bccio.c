/*
 * bccio.c
 *
 * Borland C specific screen I/O routines for DOS.
 *
 */

#include "ztypes.h"

#include <conio.h>
#include <sys/timeb.h>
#include <sys/types.h>

int bdos(int, unsigned, unsigned);

#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define WHITE 7

#define BRIGHT 0x08
#define FLASH 0x10

static int screen_started = FALSE;
static int cursor_saved = OFF;
static int saved_row = 0;
static int saved_col = 0;
static int current_fg = WHITE;
static int current_bg = BLUE;

static int timed_read_key (int, int);
static int read_key (void);

void initialize_screen (void)
{
    struct text_info ti;

    textmode(C80);

    gettextinfo(&ti);
    if (screen_rows == 0)
        screen_rows = ti.screenheight;
    if (screen_cols == 0)
        screen_cols = ti.screenwidth;
    window(1, 1, screen_cols, screen_rows);
    _wscroll = 0;
    move_cursor(1, 1);
    set_attribute (NORMAL);
    clear_screen ();
    move_cursor (screen_rows / 2, (screen_cols - sizeof ("The story is loading...")) / 2);
    cputs("The story is loading...");

    screen_started = TRUE;

    h_interpreter = INTERP_MSDOS;

}/* initialize_screen */

void restart_screen (void)
{

    cursor_saved = OFF;

    set_byte (H_CONFIG, (get_byte (H_CONFIG) | CONFIG_WINDOWS));

    if (h_type > V3)
        set_byte (H_CONFIG, (get_byte (H_CONFIG) | CONFIG_EMPHASIS | CONFIG_COLOUR));
    set_word (H_FLAGS, get_word (H_FLAGS) & (~GRAPHICS_FLAG));

}/* restart_screen */

void reset_screen (void)
{

    if (screen_started == TRUE) {
        output_new_line ();
        output_string ("[Hit any key to exit.]");
        (void) input_character (0);
        output_new_line ();

        textmode(LASTMODE);
    }

    screen_started = FALSE;

}/* reset_screen */

void clear_screen (void)
{

    clrscr();

}/* clear_screen */

void create_status_window (void)
{

}/* create_status_window */

void delete_status_window (void)
{

}/* delete_status_window */

void select_status_window (void)
{

    save_cursor_position ();
    _setcursortype(_NOCURSOR);

}/* select_status_window */

void select_text_window (void)
{

    _setcursortype(_NORMALCURSOR);
    restore_cursor_position ();

}/* select_text_window */

void clear_line (void)
{
    int i, row, col;

    get_cursor_position (&row, &col);
    for (i = col; i <= screen_rows; i++) {
        move_cursor (row, i);
        display_char (' ');
    }
    move_cursor (row, col);

}/* clear_line */

void clear_text_window (void)
{
    struct text_info ti;
    int row, col;

    get_cursor_position (&row, &col);
    gettextinfo(&ti);
    window(ti.winleft, status_size + 1, ti.winright, ti.winbottom);
    clrscr();
    window(ti.winleft, ti.wintop, ti.winright, ti.winbottom);
    move_cursor (row, col);

}/* clear_text_window */

void clear_status_window (void)
{
    struct text_info ti;
    int row, col;

    get_cursor_position (&row, &col);
    gettextinfo(&ti);
    window(ti.winleft, 1, ti.winright, status_size);
    clrscr();
    window(ti.winleft, ti.wintop, ti.winright, ti.winbottom);
    move_cursor (row, col);

}/* clear_status_window */

void move_cursor (int row, int col)
{

    gotoxy(col, row);

}/* move_cursor */

struct rccoord {
  int row;
  int col;
};

struct rccoord _gettextposition(void)
{
  struct rccoord rc_coord;

  rc_coord.row = wherey();
  rc_coord.col = wherex();
  return rc_coord;
}

void get_cursor_position (int *row, int *col)
{
    struct rccoord rc;

    rc = _gettextposition ();
    *row = rc.row;
    *col = rc.col;

}/* get_cursor_position */

void save_cursor_position (void)
{

    if (cursor_saved == OFF) {
        get_cursor_position (&saved_row, &saved_col);
        cursor_saved = ON;
    }

}/* save_cursor_position */

void restore_cursor_position (void)
{

    if (cursor_saved == ON) {
        move_cursor (saved_row, saved_col);
        cursor_saved = OFF;
    }

}/* restore_cursor_position */

void set_attribute (int attribute)
{
    struct text_info ti;
    int new_attr;

    gettextinfo(&ti);

    if (attribute == NORMAL)
        new_attr = (current_bg << 4)
                 | current_fg;

    if (attribute & REVERSE)
        new_attr = ((ti.attribute & 0x7) << 4)
                 | ((ti.attribute & 0x70) >> 4);

    if (attribute & BOLD)
        new_attr |= ti.attribute | BRIGHT;

    if (attribute & EMPHASIS)
        new_attr = (ti.attribute & 0xF0)
                 | (RED | BRIGHT);

    if (attribute & FIXED_FONT)
        new_attr = ti.attribute;

    textattr(new_attr);

}/* set_attribute */

void display_char (int c)
{
    char string[2];
    string[0] = (char) c;
    string[1] = '\0';
    cputs(string);

}/* display_char */

int input_line (int buflen, char *buffer, int timeout, int *read_size)
{
    struct timeb timenow;
    struct tm *tmptr;
    int c, row, col, target_second, target_millisecond;

    if (timeout != 0) {
        ftime (&timenow);
        tmptr = gmtime (&timenow.time);
        target_second = (tmptr->tm_sec + timeout) % 60;
        target_millisecond = timenow.millitm;
    }

    for ( ; ; ) {

        /* Read a single keystroke */

        do {
            if (timeout == 0)
                c = read_key ();
            else {
                c = timed_read_key (target_second, target_millisecond);
                if (c == -1)
                    return (c);
            }
        } while (c == 0);

        if (c == '\b') {

            /* Delete key action */

            if (*read_size == 0) {

                /* Ring bell if line is empty */

                putchar ('\a');

            } else {

                /* Decrement read count */

                (*read_size)--;

                /* Erase last character typed */

                get_cursor_position (&row, &col);
                move_cursor (row, --col);
                cputs(" ");
                move_cursor (row, col);
            }

        } else {

            /* Normal key action */

            if (*read_size == (buflen - 1)) {

                /* Ring bell if buffer is full */

                putchar ('\a');

            } else {

                /* Scroll line if return key pressed */

                if (c == '\r' || c == '\n') {
                    c = '\n';
                    scroll_line ();
                }

                if (c == '\n' || c >= (unsigned char) '\x080') {

                    /* Return key if it is a line terminator */

                    return ((unsigned char) c);

                } else {

                    /* Put key in buffer and display it */

                    buffer[(*read_size)++] = (char) c;
                    display_char (c);
                }
            }
        }
    }

}/* input_line */

int input_character (int timeout)
{
    struct timeb timenow;
    struct tm *tmptr;
    int c, target_second, target_millisecond;

    if (timeout != 0) {
        ftime (&timenow);
        tmptr = gmtime (&timenow.time);
        target_second = (tmptr->tm_sec + timeout) % 60;
        target_millisecond = timenow.millitm;
    }

    if (timeout == 0)
        c = read_key ();
    else
        c = timed_read_key (target_second, target_millisecond);

    return (c);

}/* input_character */

static int timed_read_key (int target_second, int target_millisecond)
{
    struct timeb timenow;
    struct tm *tmptr;
    int c;

    for ( ; ; ) {

        do {
            ftime (&timenow);
            tmptr = gmtime (&timenow.time);
        } while ((bdos (11, 0, 0) & 0xff) == 0 &&
                 (tmptr->tm_sec != target_second ||
                  (int) timenow.millitm < target_millisecond));

        if ((bdos (11, 0, 0) & 0xff) == 0)
            return (-1);
        else {
            c = read_key ();
            if (c != 0)
                return (c);
        }
    }

}/* timed_read_key */

static int read_key (void)
{
    int c;

    c = bdos (8, 0, 0) & 0xff;
    if (c != '\0' && c != (unsigned char) '\x0e0') {
        if (c == '\x07f')
            c = '\b';
        return (c);
    }
    c = bdos (8, 0, 0) & 0xff;
    if (c == 'H') /* Up arrow */
        return ((unsigned char) '\x081');
    else if (c == 'P') /* Down arrow */
        return ((unsigned char) '\x082');
    else if (c == 'K') /* Left arrow */
        return ((unsigned char) '\x083');
    else if (c == 'M') /* Right arrow */
        return ((unsigned char) '\x084');
    else if (c >= ';' && c <= 'D') /* Function keys F1 to F10 */
        return ((c - ';') + (unsigned char) '\x085');
    else if (c == 'O') /* End (SW) */
        return ((unsigned char) '\x092');
    else if (c == 'Q') /* PgDn (SE) */
        return ((unsigned char) '\x094');
    else if (c == 'G') /* Home (NW) */
        return ((unsigned char) '\x098');
    else if (c == 'I') /* PgUp (NE) */
        return ((unsigned char) '\x09a');

    putchar ('\a');

    return (0);

}/* read_key */

void scroll_line (void)
{
    struct text_info ti;
    int row, col;

    get_cursor_position (&row, &col);
    gettextinfo(&ti);
    if (row == ti.winbottom) {
        window(ti.winleft, status_size + 1, ti.winright, ti.winbottom);
        movetext(ti.winleft, status_size + 2, ti.winright, ti.winbottom,
                 ti.winleft, status_size + 1);
        window(ti.winleft, ti.winbottom, ti.winright, ti.winbottom);
        clrscr();
        window(ti.winleft, ti.wintop, ti.winright, ti.winbottom);
    } else {
        row++;
    }
    move_cursor (row, ti.winleft);

}/* scroll_line */

/*
 * set_colours
 *
 * Sets the screen foreground and background colours.
 *
 */

void set_colours (int foreground, int background)
{
    struct text_info ti;
    int fg, bg;
    int colour_map[] = { BLACK, RED, GREEN, BROWN, BLUE, MAGENTA, CYAN, WHITE };

    /* Translate from Z-code colour values to natural colour values */

    fg = (foreground == 1) ? WHITE : colour_map[foreground - 2];
    bg = (background == 1) ? BLUE : colour_map[background - 2];

    /* Set foreground and background colour */

    textattr((bg << 4) | fg);

    /* Save new foreground and background colours for restoring colour */

    current_fg = (int) (ti.attribute & 0x0F);
    current_bg = (int) ((ti.attribute & 0xF0) >> 4);

}/* set_colours */

/*
 * codes_to_text
 *
 * Translate Z-code characters to machine specific characters. These characters
 * include line drawing characters and international characters.
 *
 * The routine takes one of the Z-code characters from the following table and
 * writes the machine specific text replacement. The target replacement buffer
 * is defined by MAX_TEXT_SIZE in ztypes.h. The replacement text should be in a
 * normal C, zero terminated, string.
 *
 * Return 0 if a translation was available, otherwise 1.
 *
 *  Arrow characters (0x18 - 0x1b):
 *
 *  0x18 Up arrow
 *  0x19 Down arrow
 *  0x1a Right arrow
 *  0x1b Left arrow
 *
 *  International characters (0x9b - 0xa3):
 *
 *  0x9b a umlaut (ae)
 *  0x9c o umlaut (oe)
 *  0x9d u umlaut (ue)
 *  0x9e A umlaut (Ae)
 *  0x9f O umlaut (Oe)
 *  0xa0 U umlaut (Ue)
 *  0xa1 sz (ss)
 *  0xa2 open quote (>>)
 *  0xa3 close quota (<<)
 *
 *  Line drawing characters (0xb3 - 0xda):
 *
 *  0xb3 vertical line (|)
 *  0xba double vertical line (#)
 *  0xc4 horizontal line (-)
 *  0xcd double horizontal line (=)
 *  all other are corner pieces (+)
 *
 */

int codes_to_text (int c, char *s)
{

    /* Characters 24 to 27 and 179 to 218 need no translation */

    if ((c > 23 && c < 28) || (c > 178 && c < 219)) {

        s[0] = (char) c;
        s[1] = '\0';

        return (0);
    }

    /* German characters need translation */

    if (c > 154 && c < 164) {
        char xlat[9] = { 0x84, 0x94, 0x81, 0x8e, 0x99, 0x9a, 0xe1, 0xaf, 0xae };

        s[0] = xlat[c - 155];
        s[1] = '\0';

        return (0);
    }

    return (1);

}/* codes_to_text */
