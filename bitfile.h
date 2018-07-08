//
//  bitfile.h
//  forward_interleave
//
//  Created by Nathan Smith on 2/4/18.
//  Copyright © 2018 Nathan Smith. All rights reserved.
//

#ifndef bitfile_h
#define bitfile_h

#include <vector>
#include <iostream>
#include <fstream>
#include </Users/hephaestus/Documents/MyCode/Libraries/varint.h>
#include </Users/hephaestus/Documents/MyCode/Libraries/byte_bits.h>

template <bool write = false>
class bitfile {
	
	fstream file;
	size_t size;
	byte_bits bb;
	string queued;
	
public:
	
	bitfile()
	{
	}
	
	bitfile(string file_name)
	{
		open(file_name);
	}
	
	void init()
	{
		queued = "";
	}
	
	void rewind()
	{
		init();
		file.clear();
		file.seekg(0, file.beg);
	}
	
	bool open(string file_name)
	{
		int read_write;
		init();
		
		if (write) {
			read_write = ios::out;
			bb.init_write();
		} else {
			read_write = ios::in;
			bb.init_read();
		}
		
		file.open(file_name, ios::ate | ios::binary	 | read_write);
		
		if (!write) {
			size = file.tellg();
			rewind();
		}
		
		return !file.fail();
	}
	
	unsigned char get_byte()
	{
		char ret;
		file.read(&ret, sizeof(ret));
		return ret;
	}
	
	unsigned short get_word()
	{
		unsigned short int ret;
		file.read(ret, sizeof(ret));
		return ret;
	}
	
	unsigned int get_dword()
	{
		unsigned int ret;
		file.read(&ret, sizeof(ret));
		return ret;
	}
	
	unsigned long get_qword()
	{
		unsigned long ret;
		file.read((char*)&ret, sizeof(ret));
		return ret;
	}
	
	bool get_bit()
	{
		if (bb.at_end())
			bb.put_byte(get_byte());
		
		return bb.get_bit();
	}
	
	long get_varint()
	{
		varint v;
		
		v.init();
		while (v.var_to_int(get_byte()));
		
		return (long)v.get_int();
	}
	
	long get_varint(int& len)
	{
		varint v;
		
		v.init();
		while (v.var_to_int(get_byte())) {
			len++;
		}
		
		return (long)v.get_int();
	}

	string get_string(size_t len)
	{
		string ret;
		
		for (int i = 0; i < len; ++i) {
			ret.push_back(get_byte());
		}
		
		return ret;
	}
	
	void get_void(char* dest, size_t len)
	{
		file.read(dest, len);
	}
	
	void put_void(char* source, size_t len)
	{
		file.write(source, len);
	}
	
	void put_string(const string s)
	{
		if (bb.is_empty()) {
			file.write(&s[0], s.length());
		} else {
			queued += s;
		}
	}
	
	void put_byte(const char by)
	{
		if (bb.is_empty()) {
			file.write(&by, sizeof(by));
		} else {
			queued += string(1, by);
		}
	}
	
	void put_word(const unsigned short w)
	{
		if (bb.is_empty()) {
			file.write((char*)&w, sizeof(w));
		} else {
			queued.resize(queued.size() + sizeof(w));
			copy(w, w + sizeof(w), queued.begin());
		}
	}

	void put_dword(const unsigned int dw)
	{
		if (bb.is_empty()) {
			file.write((char*)&dw, sizeof(dw));
		} else {
			queued.resize(queued.size() + sizeof(dw));
			copy(dw, dw + sizeof(dw), queued.begin());
		}
	}

	void put_qword(long long qw)
	{
		if (bb.is_empty()) {
			file.write((char*)&qw, sizeof(qw));
		} else {
			queued.resize(queued.size() + sizeof(qw));
			std::copy((char*)qw, (char*)qw + sizeof(qw), queued.begin());
		}
	}

	void put_varint(long i)
	{
		string s = varint::int_to_string(i);
		
		if (bb.is_empty()) {
			file.write(&s[0], s.length());
		} else {
			queued += s;
		}
	}
	
	void put_bit(bool bi)
	{
		if (bb.put_bit(bi)) {
			flush();
		}
	}
	
	void flush()
	{
		put_byte(bb.get_byte());
		put_string(queued);
		queued = "";
	}
	
	void close()
	{
		if (write && !bb.is_empty())
			flush();
		
		file.close();
	}
	
	size_t get_size()
	{
		return size;
	}
	
	bool at_end()
	{
		return file.eof();
	}
	
	~bitfile()
	{
		close();
	}
	
};


#endif /* bitfile_h */
