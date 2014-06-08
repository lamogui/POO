/*******************************************************
Nom ......... : audiostream.hpp
Role ........ : Permet de convertir les signaux en flux 
                tendu pour la carte son
Auteur ...... : Julien DE LOOR
Version ..... : V1.1 
Licence ..... : © Copydown™
********************************************************/

#ifndef __AUDIOSTREAM
#define __AUDIOSTREAM

#include "config.hpp"
#include "signal.hpp"
#include <cstdlib>
#include <SFML/System.hpp>

/*
#if defined(COMPILE_WINDOWS)
#include "bassasio.hpp"
#endif

#include "bass.h"*/


//AudioStream is just a fixed size FIFO buffer
// IN -> audio to diffuse
// OUT -> audio diffused
//But it can be used in invert
//You can lock() an audioStream before use it if you use threads
//All length variables are in samples (not in bytes)
class AudioStream : public sf::Mutex
{
  public:
    AudioStream(unsigned int buffer_length);
    virtual ~AudioStream(); 
    
    //you can resize the buffer only if there is no samples inside (all samples are free)
    bool setBufferLength(unsigned int buffer_length);
    
    //return buffer size in samples
    inline unsigned int getBufferLength()
    {
      return _length;
    }
    
    //try to read samples into a buffer, return the number of samples readed
    unsigned int read(short* buffer, unsigned int len);
    //try to write samples into the buffer, return the number of samples writed
    unsigned int write(const short* buffer, unsigned int len);
    
    //try to write an entire signal into the buffer return true if succeed
    bool writeSignal(const Signal& signal); 
    bool writeStereoSignal(const Signal& left,const Signal& right); 
    //try to read an entire signal from the buffer return true if succeed
    //bool readSignal(Signal& signal); //TODO
    
    inline unsigned int getAvailableSamplesCount()
    {
      return _count;
    }    
    inline unsigned int getFreeSamplesCount()
    {
      return _length-_count;
    }

  protected:
    short* _buffer;
    short* _end; //end of the buffer
    short* _pwrite; // pointeur d’écriture
    short* _pread; // pointeur de lecture
    unsigned int _length; //size in samples
    unsigned int _count; //nombre de samples disponibles
  
};

#endif

