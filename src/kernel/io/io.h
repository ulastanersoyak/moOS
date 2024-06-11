#ifndef IO_H
#define IO_H

// input byyte
unsigned char insb (unsigned short port);

// input word
unsigned short insw (unsigned short port);

// output byte
void outb (unsigned short port, unsigned char val);

// output  word
void outw (unsigned short port, unsigned short val);

#endif // !IO_H
