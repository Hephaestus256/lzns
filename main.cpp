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
	
	string in_ext;
	//string file = "COTTAGE"; in_ext = "bmp";
	//string file = "CLOWN"; in_ext = "bmp";
	//string file = "ANGELFISH"; in_ext = "bmp";
	//string file = "ASTRONUT"; in_ext = "bmp";
	//string file = "TUT"; in_ext = "bmp";

	string file = "Diaspora - text"; in_ext = "txt";
	//string file = "Summary - text"; in_ext = "txt";
	//string file = "rep_test2"; in_ext = "txt";
	//string file = "canterbury_corpus/asyoulik"; in_ext = "txt";
	//string file = "canterbury_corpus/plrabn12"; in_ext = "txt";
	//string file = "canterbury_corpus/kennedy"; in_ext = "xls";
	//string file = "canterbury_corpus/enwik8"; in_ext = "dat";
	//string file = "canterbury_corpus/alice29"; in_ext = "txt";
	//string file = "Lenna"; in_ext = "tiff";
	//string file = "silesia_corpus/mr"; in_ext = "";
	//string file = "silesia_corpus/sao"; in_ext = "";
	//string file = "silesia_corpus/dickens"; in_ext = "";
	//string file = "silesia_corpus/x-ray"; in_ext = "";
	//string file = "silesia_corpus/webster"; in_ext = "";
	//string file = "silesia_corpus/nci"; in_ext = "";
	//string file = "monkey"; in_ext = "tiff";
	//string file = "sam_i_am"; in_ext = "txt";
	//string file = "rain_spain"; in_ext = "txt";
	//string file = "misc1";
	
	string ou_ext = "lzn3";
	const int dict_size = 32 * 1024;
	
	cout << "deflating '" + file + "' ..." << endl;
	
	if (in_ext != "") in_ext = "." + in_ext;
	if (ou_ext != "") ou_ext = "." + ou_ext;

	lzns_deflate<dict_size, false> deflate(
		"/Users/hephaestus/Desktop/test_files/" + file + in_ext,
		"/Users/hephaestus/Desktop/test_files/" + file + ou_ext
	);
	
	if (deflate.in_error())
		return 3;
	
	//return 0;
	getchar();
	cout << "inflating..." << endl;
	
	lzns_inflate<dict_size, false> inflate(
		"/Users/hephaestus/Desktop/test_files/" + file + ou_ext,
		"/Users/hephaestus/Desktop/test_files/" + file + "_out" + in_ext,
		"/Users/hephaestus/Desktop/test_files/" + file + in_ext
	);
	
	cout << "done." << endl;
	
	//inflate.show_buffer();
	
	cout << endl;
	return 0;
}
