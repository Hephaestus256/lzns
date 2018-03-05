//
//  byte_bits.h
//  lzns
//
//  Created by Nathan Smith on 2/5/18.
//  Copyright © 2018 Nathan Smith. All rights reserved.
//

#ifndef byte_bits_h
#define byte_bits_h

/*
class byte_bits: packs 8 bits into a byte
 
usage:
 
to read bits:
 
	byte_bits bb;

	for (int i = 0; i < 20; ++i) {
		if (bb.at_end()) {
			bb.load(3);
		}
		// use bit here
		cout << bb.get_bit() << ", ";
	}
 */

class byte_bits {
	
	int byte;
	int pos;
	
public:
	
	void init_read()
	{
		pos = 8;
	}
	
	void init_write()
	{
		pos = 0;
		byte = 0;
	}
	
	byte_bits ()
	{
	}
	
	byte_bits (bool read)
	{
		if (read) {
			init_read();
		} else {
			init_write();
		}
	}
	
	bool is_empty()
	{
		return pos <= 0;
	}
	
	bool at_end()
	{
		return pos >= 8;
	}
	
	unsigned char get_byte()
	{
		int by = byte;
		init_write();
		return (unsigned char)by;
	}
	
	void put_byte(unsigned char by)
	{
		pos = 0;
		byte = (int)by;
	}
	
	bool get_bit()
	{
		bool bi = byte & (1 << pos);
		pos++;
		return bi;
	}
	
	bool put_bit(bool bi)
	{
		byte |= ((int)bi << pos);
		pos++;
		return at_end();
	}
};


#endif /* byte_bits_h */
