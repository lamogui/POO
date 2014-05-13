#include <stdio.h>
#include <stdlib.h>
#include "midi.hpp"
#include <iostream>

//////////MIDI HEAD\\\\\\\\\\\\\\\\\\\\

Midi_head::Midi_head(WORD format, WORD tracks, BYTE frame, BYTE ticks) :
  _gain((float) frame*ticks / (float) Signal::refreshRate),
  _format(format),
  _tracks(tracks),
  _frame(frame),
  _ticks(ticks)
{
  if (!_gain) _gain = 1.f;
 }

Midi_head::~Midi_head() {

}

bool Midi_head::write_to_buffer(unsigned char* buffer, unsigned int size ) const {

  if (size < Midi_head::size) return false;

  //magic
  *buffer++='M';
  *buffer++='T';
  *buffer++='h';
  *buffer++='d';
  *buffer++=0;
  *buffer++=0;
  *buffer++=0;
  *buffer++=6;

  if (_format!=1) {
    printf("Midi_head error: Format %u unsupported yet\n",_format); 
    return false;
  }
  else {
    *buffer++=0;
    *buffer++=1;
  }
  //nombre de piste
  *buffer++=_tracks>>8;
  *buffer++=_tracks & 0xFF;

  //resolution
  *buffer++=0x80|_frame;
  *buffer++=_ticks;
  return true;
}

bool Midi_head::write_to_file(FILE* file) const
{
  unsigned char buffer[Midi_head::size];
  if (write_to_buffer(buffer,Midi_head::size))
  {
    if (Midi_head::size==fwrite((void*)buffer,1,Midi_head::size,file))
    {
      return true;
    }
    printf("Midi_head error: fwrite error !\n"); 
  }
  return false;
}

//////////MIDI TRACK0\\\\\\\\\\\\\\\

Midi_track0::Midi_track0() :
_mpqn(0),
_music_name(""),
_copyright("Millenium Synthesizer"),
_comment("May the force be with you")
{
}

Midi_track0::Midi_track0(const std::string n) :
_mpqn(0),
_music_name(n),
_copyright("Millenium Synthesizer"),
_comment("May the force be with you")
{
}

Midi_track0::~Midi_track0()
{
}

unsigned int Midi_track0::size() const
{
  // Magic + Head + Delta + MetaEvent + EndOfTrack + MetaSize
  unsigned int size = 4 + 4 + 1 + 1 + 1 + 1; //minimum size
  //                   Delta + MetaEvent + Type + length + Specific size
  if (_music_name.size()) size += 1 + 1 + 1 + 1 + _music_name.size() + 1;
  if (_copyright.size()) size += 1 + 1 + 1 + 1 + _copyright.size() + 1;
  if (_comment.size()) size += 1 + 1 + 1 + 1 + _comment.size() + 1;
  if (_mpqn) size += 1 + 1 + 1 + 1 + 3;
  return size;
}


bool Midi_track0::write_to_buffer(unsigned char* buffer, unsigned int s) const
{
  unsigned int target_size=size();
  unsigned char str_length;
  if (s < target_size) return false;
  
  //Magic
  *buffer++='M';
  *buffer++='T';
  *buffer++='r';
  *buffer++='k';
  
  //Head
  *buffer++=(target_size-8)>>24;
  *buffer++=((target_size-8)>>16) & 0xFF;
  *buffer++=((target_size-8)>>8) & 0xFF;
  *buffer++=(target_size-8) & 0xFF;
  
  if (_music_name.size()) {
    //Meta Event track name
    *buffer++=0;    //Delta
    *buffer++=0xFF; //Meta
    *buffer++=0x3;  //Track Name 
    str_length=_music_name.size() & 0x7F; //Beurk here 
    *buffer++=str_length;
    for (unsigned char i=0; i<str_length; i++) 
      *buffer ++= (unsigned char) _music_name[i];
  }
  
  if (_copyright.size()) {
    *buffer++=0;    //Delta
    *buffer++=0xFF; //Meta
    *buffer++=0x2; //Copyright 
    str_length=_copyright.size() & 0x7F; //Beurk here 
    *buffer++=str_length;
    for (unsigned char i=0; i<str_length; i++) 
      *buffer ++= (unsigned char) _copyright[i];
  }
  
  if (_comment.size()) {
    *buffer++=0;    //Delta
    *buffer++=0xFF; //Meta
    *buffer++=0x1;  //Text Event 
    str_length=_comment.size() & 0x7F; //Beurk here 
    *buffer++=str_length;
    for (unsigned char i=0; i<str_length; i++) 
      *buffer ++= (unsigned char) _comment[i];
  }
  
  if (_mpqn) {
    *buffer++=0;     //Delta
    *buffer++=0xFF;  //Meta
    *buffer++=0x51;  //Set Tempo
    *buffer++=3;
    *buffer++=_mpqn>>16;
    *buffer++=(_mpqn>>8) & 0xFF;
    *buffer++=_mpqn&0xFF;
  }
  
  //End of the Track !
  *buffer++=0;
  *buffer++=MIDI_META;
  *buffer++=MIDI_END_OF_TRACK;
  *buffer++=0; //Size used next to the event declaration
  
  return true;
}

bool Midi_track0::write_to_file(FILE* file) const
{
  unsigned int target_size=size();
  unsigned char* buffer = (unsigned char*) malloc(target_size);
  if (write_to_buffer(buffer,target_size))
  {
    if (target_size==fwrite((void*)buffer,1,target_size,file))
    {
      free(buffer);
      return true;
    }
    printf("Midi_track0 error: fwrite error !\n"); 
  }
  else 
  {
    printf("error !\n"); 
  }
  free((void*)buffer);
  return false;
}

//////////MIDI TRACK\\\\\\\\\\\\\\\\\\\\

Midi_track::Midi_track(Midi_head& head) :
_head(&head),
_chunk(0),
_alloc_size(256),
_chunk_size(0)
{
  _chunk=(unsigned char*)malloc(_alloc_size);
}

Midi_track::~Midi_track() {
  free((void*)_chunk);
}

unsigned int Midi_track::size() const
{
  // Magic + ChunkSize + _chunk_size + Delta + MetaEvent + EndOfTrack + MetaSize
  return 4 + 4 + _chunk_size + 1 + 1 + 1 + 1;
}

bool Midi_track::write_to_buffer(unsigned char* buffer, unsigned int s ) const
{
  unsigned int target_size=size();
  if (s < target_size) return false;
  
  //Magic
  *buffer++='M';
  *buffer++='T';
  *buffer++='r';
  *buffer++='k';
  
  //Head
  *buffer++=(target_size-8)>>24;
  *buffer++=((target_size-8)>>16) & 0xFF;
  *buffer++=((target_size-8)>>8) & 0xFF;
  *buffer++=(target_size-8) & 0xFF;
  
  //Copy Custom Datas
  memcpy((void*) buffer,(void*)_chunk,_chunk_size);
  
  //Finally the end of the track
  *buffer++=0;
  *buffer++=MIDI_META;
  *buffer++=MIDI_END_OF_TRACK;
  *buffer++=0; //Size used next to the event declaration
  
  return true;
}

bool Midi_track::write_to_file(FILE* file) const
{
  unsigned int target_size=size();
  unsigned char* buffer = (unsigned char*) malloc(target_size);
  if (write_to_buffer(buffer,target_size))
  {
    if (target_size==fwrite((void*)buffer,1,target_size,file))
    {
      free(buffer);
      return true;
    }
    printf("Midi_track error: fwrite error !\n"); 
  }
  free((void*)buffer);
  return false;
}



void Midi_track::push_varlength(DWORD var) {
  DWORD var_tp=var;
  DWORD bit=0;
  BYTE part;
  check_alloc(4); //Maximum 4 bytes length
  
  //32 bits unsigned integer
  
  //Who is the most significant bit ?
  for (DWORD i=0; i<32; i++) {
    if (var_tp&1) bit=i;
    var_tp>>1;
  }
  part=bit/7;
  
  for (DWORD i=0; i<(unsigned)part+1; i++) {
    if (i==part) {
      _chunk[_chunk_size++] = var & 0x7F;
    }
    else {
      _chunk[_chunk_size++] = ((var>>(7*(part-i))) & 0x7F) | 0x80;
    }
  }
}

void Midi_track::push_midi_event(DWORD midi_delta, BYTE type, BYTE chan, BYTE p1, BYTE p2)
{
  push_varlength(midi_delta);
  std::cout << _chunk_size << " new midi event" << std::endl;
  
  check_alloc(3);
  _chunk[_chunk_size++] = (type & 0xF) << 4 | (chan & 0xF);
  _chunk[_chunk_size++] = p1 & 0x7F;
  _chunk[_chunk_size++] = p2 & 0x7F;
}

