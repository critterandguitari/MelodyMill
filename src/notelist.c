/*
 * notelist.c
 *
 *  Created on: Jul 9, 2013
 *      Author: owen
 */

#include "notelist.h"

// init note_list struct
note_list * note_list_init(note_list *  notes){
	int i = 0;

	notes->len = 0;
	notes->index = 0;

	for (i = 0; i < 256; i++)
		notes->note_list[i] = 0;

	return notes;
}

// copy one note list to another
note_list * note_list_copy_notes(note_list * src, note_list * dest) {

	int i = 0;

	dest->len = src->len;

	for (i = 0; i < src->len; i++)
		dest->note_list[i] = src->note_list[i];

	return dest;

}

// spits out the last note in the list (most recent)
uint8_t note_list_most_recent(note_list * notes) {
	return notes->note_list[notes->len - 1];
}

// checks if a note is on  (0 for off, 1 for on)
uint8_t note_list_check_note(note_list * notes, uint8_t note){
	int i = 0;
	for (i = 0; i < notes->len; i++)
		if (notes->note_list[i] == note)
			return 1;
	return 0;
}

// adds a note to the list
note_list * note_list_note_on(note_list * notes, uint8_t note){
	notes->note_list[notes->len] = note;
	notes->len++;
	return notes;
}

// removes note from list
note_list * note_list_note_off(note_list * notes, uint8_t note){
	int i = 0;

	for (i = 0; i < notes->len; i++){
		if (notes->note_list[i] == note){
			notes->note_list[i] = 0;
			// shift other notes down
			for (; i < (notes->len - 1); i++)
				notes->note_list[i] = notes->note_list[i + 1];
			notes->len--;
			break;
		}
	}
	return notes;
}
