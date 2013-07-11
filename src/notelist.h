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
} note_list;

// init note_list struct
note_list * note_list_init(note_list *  notes);

// copy one note list to another
note_list * note_list_copy_notes(note_list * src, note_list * dest);

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
