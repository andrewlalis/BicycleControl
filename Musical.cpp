#include "Musical.h"



Musical::Musical()
{
}


Musical::~Musical()
{
}

void Musical::setBPM(short newBPM)
{
	BPM = newBPM;
	Q = 60000 / BPM;
	H = Q * 2;
	E = Q / 2;
	S = Q / 4;
	W = Q * 4;
}

void Musical::setBuzzerPin(short pinNumber)
{
	pin = pinNumber;
}

void Musical::playNote(int note, int duration)
{
	tone(pin, note, duration);
}

//Plays an array as a musical sequence. Array is: {bpm, note1, time1, note2, time2, ...} Length is the number of notes.
void Musical::playSequence(short * sequence, int length)
{
	setBPM(sequence[0]);
	currentSong = sequence;
	songLength = length;
	currentNote = 0;
	lastNoteTime = 0;
}

//Updates the playing song to allow multitasking.
void Musical::update()
{
	if (lastNoteTime > millis() && currentNote < songLength) {
		//It is necessary to play the next note.
		playNote(currentSong[currentNote * 2 + 1], currentSong[currentNote * 2 + 2]);
		lastNoteTime = millis();
		currentNote++;
	}
}
