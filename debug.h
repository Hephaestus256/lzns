//
//  debug.h
//  lzns
//
//  Created by Nathan Smith on 6/30/18.
//  Copyright © 2018 Nathan Smith. All rights reserved.
//

#ifndef debug_h
#define debug_h

#include <stdio.h>
#include <string.h>


string show(const string s, int len = -1)
{
	string ret;
	size_t l;
	
	if (len < 0)
		l = s.length();
	else
		l = (size_t)len;
	
	for (int i = 0; i < (int)l; ++i) {
		char c = s[i];
		
		if (c < ' ') {
			ret += "[" + to_string(c) + "]";
		} else {
			ret += string(1, c);
		}
	}
	
	return "'" + ret + "'";
}

string show(const char c)
{
	return show(string(1, c));
}

#endif /* debug_h */
