//
//  varint.h
//  forward_interleave
//
//  Created by Nathan Smith on 2/4/18.
//  Copyright © 2018 Nathan Smith. All rights reserved.
//

#ifndef varint_h
#define varint_h

using namespace std;

template <class type = long long, int chunk_size = 8>
class varint_base {
	int chunk;
	
public:
	
	type data;
	
	void init(type d = 0)
	{
		data = d;
		chunk = 0;
	}
	
	void init_var()
	{
		init(0);
	}
	
	template <class gen>
	void init_int(gen n)
	{
		// subtract out the built-in values
		type c = 1;
		
		chunk = 0;
		do {
			c <<= (chunk_size - 1);
			n -= c;
			chunk++;
		} while (n >= 0);
		
		data = n + c;
	}
	
	varint_base()
	{
	}
	
	template <class gen>
	varint_base(gen n)
	{
		init_int(n);
	}
	
	// input next chunk of the var number to convert to int
	// returns true if there is another chunk, false if it's done
	// might need to call init()
	template <class gen>
	bool var_to_int(gen n)
	{
		data += n << chunk;
		chunk += type(chunk_size - 1);
		return n >= (1 << (chunk_size - 1));
	}
	
	template <class gen>
	static string int_to_string(gen n)
	{
		varint_base<type, chunk_size> v;
		string ret;
		unsigned int d;

		v.init_int(n);
		
		while (v.int_to_var(d)) {
			ret += string(1, char(d));
		};
		
		return ret;
	}
	
	// generate next chunk of the var number from int
	// returns true if there is another chunk, false if it's done
	// must call init(n) first
	template <class gen>
	bool int_to_var(gen& n)
	{
		n = data & type((1 << (chunk_size - 1)) - 1);
		data >>= (chunk_size - 1);
		
		chunk--;
		if (chunk > 0)
			n += (1 << (chunk_size - 1));
		
		return chunk >= 0;
	}
	
	int chunk_ct()
	{
		int ret = 0;
		type c = 1;
		type d = data;
		
		do {
			c <<= (chunk_size - 1);
			d -= c;
			ret++;
		} while (d >= 0);
		
		return ret;
	}
	
	type get_int()
	{
		return data;
	}
};

typedef varint_base<> varint;


#endif /* varint_h */
