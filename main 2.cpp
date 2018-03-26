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
	
	//string file = "COTTAGE";
	//string file = "CLOWN";
	//string file = "Diaspora - text";
	//string file = "canterbury_corpus/asyoulik";
	//string file = "canterbury_corpus/plrabn12";
	string file = "canterbury_corpus/kennedy";
	//string file = "silesia_corpus/mr";
	//string file = "intro";
	//string file = "sam_i_am";
	//string file = "rain_spain";
	//string file = "misc1";
	string in_ext = "xls";
	//string ou_ext = "123";
	
	//string file = "Europa";
	//string file = "bitmaps/LORI";
	string ou_ext = "lzn";
	
	cout << "deflating '" + file + "' ..." << endl;
	
	if (in_ext != "") in_ext = "." + in_ext;
	if (ou_ext != "") ou_ext = "." + ou_ext;

	lzns_deflate<1024 * 1024 * 1> deflate(
		"/Users/hephaestus/Desktop/test_files/" + file + in_ext,
		"/Users/hephaestus/Desktop/test_files/" + file + ou_ext
	);
	
	deflate.close();
	
	//return 0;
	
	cout << "inflating..." << endl;
	
	lzns_inflate<1024 * 1024 * 1> inflate(
		"/Users/hephaestus/Desktop/test_files/" + file + ou_ext,
		"/Users/hephaestus/Desktop/test_files/" + file + "_out" + in_ext,
		"/Users/hephaestus/Desktop/test_files/" + file + in_ext
	);
	
	inflate.inflate();
	inflate.close();
	
	// abracadabra
	// 9, 128, 6, 1, 6
	
	cout << endl;
	return 0;
}
