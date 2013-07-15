/*
 * notelist.h
 *
 *  Created on: Jul 9, 2013
 *      Author: owen
 */

#ifndef NOTELIST_H_
#define NOTELIST_H_

#include "arm_math.h"

typedef struct {
	uint8_t note_list[256];
	uint8_t index;
	uint8_t len;

	uint8_t note_list_last[256];
	uint8_t index_last;
	uint8_t len_last;

} note_list;

// init note_list struct
note_list * note_list_init(note_list *  notes);

// set last = current
note_list * note_list_set_current_to_last(note_list *  notes);

// see if a note_list has changed
uint8_t note_list_changed_length(note_list * notes);

// see if a note_list has changed
uint8_t note_list_changed(note_list * notes);

// copy one note list to another
note_list * note_list_copy_notes(note_list * src, note_list * dest);

// chop or expand to 3 notes
note_list * note_list_make_3(note_list * notes);

// add notes from another note list to end of note list
note_list * note_list_append(note_list * notes, note_list * new_notes);

// transpose x amt
note_list * note_list_transpose(note_list * notes, int amt);

// mirror
note_list * note_list_mirror(note_list * notes);

// copies list n times, n octaves higher each
note_list * note_list_octaves_up(note_list * notes, uint8_t height);

// copies list n times, n octaves lower each
note_list * note_list_octaves_down(note_list * notes, uint8_t height);

// spits out the last note in the list (most recent)
uint8_t note_list_most_recent(note_list * notes);

// checks if a note is on  (0 for off, 1 for on)
uint8_t note_list_check_note(note_list * notes, uint8_t note);

// adds a note to the list
note_list * note_list_note_on(note_list * notes, uint8_t note);

// removes note from list
note_list * note_list_note_off(note_list * notes, uint8_t note);


#endif /* NOTELIST_H_ */
