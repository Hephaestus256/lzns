//
//  main.cpp
//  lzns
//
//  Created by Nathan Smith on 2/4/18.
//  Copyright © 2018 Nathan Smith. All rights reserved.
//

#include <cassert>

//#include </Users/hephaestus/Documents/MyCode/Libraries/bitfile.h>
#include </Users/hephaestus/Documents/MyCode/Libraries/lzns.h>

int main(int argc, const char * argv[]) {
	
	/*
	unsigned long long fib_a = 1;
	unsigned long long fib_b = 1;
	unsigned long long sum = 0;
	
	cout << 1 << endl;
	for (int j = 0; j < 100; ++j) {
		//cout << (unsigned long long)fib_a << endl;
		sum += fib_a;
		unsigned long long tmp = fib_a + fib_b;
		fib_b = fib_a;
		fib_a = tmp;
		cout << j << ": " << sum << endl;
	}
	 */
	
	// 44 bytes (40 bytes with 2b)
	// The rain in Spain stays mainly in the plain.

	/*
	 
	 0: I am Sam
     9:
	 10: Sam I am
	 19:
	 20: That Sam-I-am!
	 35: That Sam-I-am!
	 50: I do not like
	 64: that Sam-I-am!
	 79:
	 80: Do you like green eggs and ham?
	 112:
	 113: I do not like them, Sam-I-am.
	 143: I do not like green eggs and ham.
	 
	 */
	
	string file = "turtle";
	//string file = "Diaspora - text";
	//string file = "canterburycorpus/asyoulik";
	//string file = "canterburycorpus/enwik8";
	//string file = "silesia/mr";
	//string file = "intro";
	//string file = "sam_i_am";
	//string file = "rain_spain";
	//string file = "misc1";
	string ext = "tiff";
	
	//string file = "Europa";
	//string file = "monkey";
	//string ext = "tiff";
	
	cout << "deflating '" + file + "' ..." << endl;
	
	if (ext != "") ext = "." + ext;
	
	lzns_deflate<false> deflate(
		"/Users/hephaestus/Desktop/" + file + ext,
		"/Users/hephaestus/Desktop/" + file + ".lzn"
	);
	
	deflate.close();
	
	//return 0;
	
	cout << "inflating..." << endl;
	
	lzns_inflate<false, 1024 * 1024 * 16> inflate(
		"/Users/hephaestus/Desktop/" + file + ".lzn",
		"/Users/hephaestus/Desktop/" + file + "_out" + ext,
		"/Users/hephaestus/Desktop/" + file + ext
	);
	
	inflate.inflate();
	inflate.close();
	
	// abracadabra
	// 9, 128, 6, 1, 6
	
	cout << endl;
	return 0;
}
