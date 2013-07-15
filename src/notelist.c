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

	notes->len_last = 0;
	notes->index_last = 0;

	for (i = 0; i < 256; i++)
		notes->note_list_last[i] = 0;

	return notes;
}

// set last = current
note_list * note_list_set_current_to_last(note_list *  notes){
	int i = 0;

	notes->len_last = notes->len;
	notes->index_last = notes->index;

	for (i = 0; i < 256; i++)
		notes->note_list_last[i] = notes->note_list[i];

	return notes;
}

// see if a note_list has changed
uint8_t note_list_changed_length(note_list * notes){

	if (notes->len != notes->len_last)
		return 1;
	else
		return 0;

}

// see if a note_list has changed
uint8_t note_list_changed(note_list * notes){
	uint8_t stat;
	uint16_t i;

	if (notes->len != notes->len_last) stat = 1;
	else stat = 0;

	for (i = 0; i < 256; i++) {
		if (notes->note_list[i] != notes->note_list_last[i]) stat = 1;
	}
	return stat;

}

// copy one note list to another
note_list * note_list_copy_notes(note_list * src, note_list * dest) {

	int i = 0;

	dest->len = src->len;

	for (i = 0; i < src->len; i++)
		dest->note_list[i] = src->note_list[i];

	return dest;

}

// chop or expand to 3 notes
note_list * note_list_make_3(note_list * notes){

	if (notes->len >= 3) {
		notes->len = 3;
	}
	if (notes->len == 2) {
		notes->len = 3;
		notes->note_list[2] = notes->note_list[1];
	}
	if (notes->len == 1) {
		notes->len = 3;
		notes->note_list[1] = notes->note_list[0];
		notes->note_list[2] = notes->note_list[0];
	}
	if (notes->len == 0) {
		notes->len = 3;
		notes->note_list[0] = 0;
		notes->note_list[1] = 0;
		notes->note_list[2] = 0;
	}

	return notes;
}

// add notes from another note list to end of note list
note_list * note_list_append(note_list * notes, note_list * new_notes){
	uint8_t i;

	for(i = 0; i < new_notes->len; i++){
		notes->note_list[i + notes->len] = new_notes->note_list[i];

	}
	notes->len += new_notes->len;

	return notes;
}

// transpose x amt
note_list * note_list_transpose(note_list * notes, int amt){
	uint8_t i;
	for (i = 0; i < notes->len; i++){
		notes->note_list[i] = notes->note_list[i] + amt;
	}
	return notes;
}

// mirror
note_list * note_list_mirror(note_list * notes){

	uint8_t i;

	note_list tmp;
	note_list_init(&tmp);
	note_list_copy_notes(notes, &tmp);

	for (i = 0; i < notes->len; i++){
		notes->note_list[i] = tmp.note_list[(notes->len - 1) - i];
	}
	return notes;
}


// copies list n times, n octaves higher each
note_list * note_list_octaves_up(note_list * notes, uint8_t height) {

	int i,j,l  = 0;

	l = notes->len;
	notes->len = 0;
	for (i =0; i < height; i++){
		for (j = 0; j < l; j++) {
			notes->note_list[j + (l * i)] = notes->note_list[j] + (12 * i);
			notes->len++;
		}
	}
	return notes;
}

// copies list n times, n octaves lower each
note_list * note_list_octaves_down(note_list * notes, uint8_t height) {

	int i,j  = 0;

	note_list orig;

	note_list_copy_notes(notes, &orig);

	notes->len = 0;
	for (i =0; i < height; i++){
		for (j = 0; j < orig.len; j++) {
			notes->note_list[j + (orig.len * i)] = orig.note_list[j] + (12 * (height - i - 1));
			notes->len++;
		}
	}
	return notes;
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
