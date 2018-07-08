//
//  lzns.h
//  lzns
//
//  Created by Nathan Smith on 2/9/18.
//  Copyright © 2018 Nathan Smith. All rights reserved.
//

#ifndef lzns_h
#define lzns_h

#include </Users/hephaestus/Documents/MyCode/Libraries/bitfile.h>
#include <unordered_map>
#include <deque>
#include <queue>
#include <stdio.h>
#include <string.h>
#include </Users/hephaestus/Documents/MyCode/Libraries/debug.h>

typedef long bucket_type;
const bucket_type DEFAULT_DICT_SIZE = 1024 * 1024 * 2;

// dict_size is dictionary size... thought about calling it "dic_size" but thought better of it ;-)
template
<bucket_type dict_size = DEFAULT_DICT_SIZE, bool disp = false, bucket_type disp_min = 0, bucket_type disp_max = -1>
class lzns_deflate {
	
	class dict_pair {
	public:
		bucket_type first;
		bucket_type last;
		
		dict_pair ()
		{
		}
		
		dict_pair (bucket_type p_bucket)
		{
			first = last = p_bucket;
		}
	};
	
	class dict_string {
	public:
		bucket_type first;
		bucket_type last;
		bucket_type literal;
		
		dict_string ()
		{
		}
		
		dict_string (bucket_type p_bucket)
		{
			first = last = p_bucket;
		}
	};
	
	enum status_type {begin, error, complete};
	enum block_mode_type {block_init = 0xff, block_literal = 0x00, block_ref = 0x80};
	const bucket_type dict_init = -2;
	const bucket_type end_stream = -3;
	
	status_type status;
	bucket_type bucket_ct;
	string lit_que;
	queue<bucket_type> ref_que;
	bitfile<false> inp_file;
	bitfile<true> out_file;
	dict_pair pairs[256 * 256];
	dict_string* strings;
	string prev_char;
	bucket_type index;
	bucket_type block_ct;
	bucket_type* prev_index;
	block_mode_type prev_mode;
	
public:
	
	inline bool should_display()
	{
		return disp && ((bucket_ct >= disp_min && bucket_ct <= disp_max) || disp_max == -1);
	}
	
	inline bool in_error()
	{
		return status == error;
	}
	
	void init()
	{
		if (should_display())
			cout << "Initializing" << endl;
		
		strings = new dict_string[dict_size];
		//last_ref = NULL;
		index = dict_init;
		prev_index = NULL;
		prev_mode = block_init;
		
		for (bucket_type i = 0; i < 256 * 256; ++i) {
			pairs[i] = dict_init;
		}
		
		for (bucket_type i = 0; i < dict_size; ++i) {
			strings[i] = dict_init;
		}
		
		if (should_display())
			cout << "Done" << endl;
	}
	
	lzns_deflate (string p_inp_file, string p_out_file)
	{
		status = begin;
		
		if (!inp_file.open(p_inp_file)) {
			cout << "Could not open input file '" << p_inp_file << "'" << endl;
			status = error;
			return;
		}
		
		if (!out_file.open(p_out_file)) {
			cout << "Could not open output file '" << p_out_file << "'" << endl;
			status = error;
			return;
		}
		
		bucket_ct = 0;
		block_ct = 0;
		
		init();
		deflate();
		close();
		status = complete;
	}
	
	inline bucket_type pair_index(const string pair)
	{
		unsigned char c1 = (unsigned char)pair[0];
		unsigned char c2 = (unsigned char)pair[1];

		return (bucket_type)c1 | ((bucket_type)c2 << 8);
	}
	
	inline bucket_type string_index(const bucket_type ind, const string str)
	{
		unsigned char c = (unsigned char)str[0];
		return (((bucket_type)ind << 8) | (bucket_type)c) & (dict_size - 1);
	}
	
	inline bucket_type literal(const bucket_type b, string str)
	{
		unsigned char c = (unsigned char)str[0];
		return ((bucket_type)b << 8) | (bucket_type)c;
	}
	
	void output_literal(string lit)
	{
		if (should_display()) {
			cout << bucket_ct << " (" << (block_ct + 1) << ") literal: " << show(lit) << endl;
		}
		
		lit_que += lit;
		block_mode(block_literal, block_ref);
		
		advance_bucket();

	}
	
	void output_literal_x(string lits)
	{
		for (int i = 0; i < lits.length(); ++i) {
			output_literal(lits.substr(i, 1));
		}
	}
	
	bool output_ref(bucket_type abs, bucket_type ref, string s)
	{
		if (should_display()) {
			cout << bucket_ct << " (" << (block_ct + 1) << ") [" << show(s) << " abs: " << abs << " last: " << ref / 2 << " len: " << (ref & 1) << "]" << endl;
		}
		
		if (ref / 2 >= dict_size - 1) { // || (s.length() == 2 && ref > 1024 * 1024)) { // or >= 4 byes!
			//cout << "		" << bucket_ct << ") offset exceeds dictionary size of " << dict_size << ": " << ref / 2 << endl;
			//cout << "		lit instead: " << show(s) << endl;
			output_literal_x(s);
			return false;
		} else {
			ref_que.push(ref);
			block_mode(block_ref, block_literal);
		
			assert((ref >> 1) > 0);
			advance_bucket();

			return true;
		}
	}
	
	// insert a 2-character string into dictionary,  returns alias of pair
	bucket_type insert_pair(string pair)
	{
		index = pair_index(pair);
		bucket_type ret = pairs[index].first;
		
		if (ret != dict_init) { // was found, so mark for possible update
			prev_index = &pairs[index].last;
		}
		
		if (should_display()) {
			//cout << "     index: " << index << " first: " << strings[index].first << " last: " << strings[index].last << endl;
		}
		
		return ret;
	}
	
	bucket_type insert_string(bucket_type ind, string str)
	{
		index = string_index(ind, str);
		bucket_type ret = strings[index].first;
		
		// if strings[index].literal != literal(ind, str) then collision
		// possible optimization: initialize literal with an invalid value, so then you can just test for that.  if true then
		//    it won't have to get and return the strings[index].first value too, it just returns dict_init.
		
		if (ret == dict_init || strings[index].literal != literal(ind, str)) { // not found, so insert
			//strings[index].literal = literal(ind, str);
			ret = dict_init;
		} else { // was found, so mark for possible update
			prev_index = &strings[index].last;
		}
		
		if (should_display()) {
			//cout << "     index: " << index << " first: " << strings[index].first << " last: " << strings[index].last << endl;
		}
		
		return ret;
	}
	
	bool longest_rep()
	{
		string curr, str;
		bucket_type f, prev_f;
		bool ret;
		
		curr = string(1, inp_file.get_byte());
		if (inp_file.at_end()) {
			output_literal(prev_char);
			return false;
		}
		
		f = insert_pair(prev_char + curr);
		
		if (f == dict_init) { // pair not found, just inserted
			output_literal(prev_char);
			pairs[index].first = (bucket_ct - 1);
			pairs[index].last = (bucket_ct - 1) * 2 - 2 + 1;
			ret = true;
		} else { // pair found in dictionary
			str += prev_char;
			
			do {
				str += curr;
				curr = string(1, inp_file.get_byte());
				prev_f = f;
				
				if (inp_file.at_end()) {
					f = dict_init;
				} else {
					f = insert_string(f, curr);
				}
			} while (f != dict_init);
			
			bucket_type last_ref = *prev_index;
			bucket_type off = bucket_ct * 2 - last_ref;
			
			if (output_ref(last_ref / 2 + 1, off, str)) {
				*prev_index = (bucket_ct - 1) * 2;
				strings[index].literal = literal(prev_f, curr);
				strings[index].first = (bucket_ct - 1);
				strings[index].last = (bucket_ct - 1) * 2 - 2 + 1;
			}

			ret = !inp_file.at_end();
		}
		
		//if (should_display()) {
		//	cout << "-----------------str: " << show(str) << endl;
		//}
		
		prev_char = curr;
		return ret;
	}
	
	void deflate()
	{
		prev_char = string(1, inp_file.get_byte());

		while (longest_rep()) {
			//advance_bucket();
		}
		
		write_end();
	}
	
	inline void advance_bucket()
	{
		bucket_ct++;
	}
	
	void outp_refs()
	{
		bucket_type ct = 0;
		
		while (!ref_que.empty()) {
			ct++;
			if (should_display()) {
				//cout << "    writing offset: (" << ct << ") " << ref_que.front() << endl;
			}
			out_file.put_varint(ref_que.front());
			ref_que.pop();
		}
	}
	
	void outp_lits()
	{
		for (bucket_type i = 0; i < lit_que.size(); ++i) {
			if (should_display()) {
				//cout << "    writing literal: " << show(lit_que[i]) << endl;
			}
			out_file.put_byte(lit_que[i]);
		}
		lit_que = "";
	}
	
	inline void block_mode(block_mode_type this_mode, block_mode_type other_mode, bool is_end = false)
	{
		if (prev_mode == other_mode || block_ct >= 126 || is_end) {

			if (block_ct >= 126 && prev_mode != other_mode) block_ct++;
			//if (block_ct >= 126) block_ct++;
			
			if (should_display()) {
				if (prev_mode == block_literal) {
					cout << "---------------------------------------------------------------------------- /\\ block of: " << block_ct << " lit" << endl;
				} else {
					cout << "---------------------------------------------------------------------------- /\\ block of: " << block_ct << " ref" << endl;
				}
			}
			
			unsigned char b = (unsigned char)prev_mode + block_ct;
			if (should_display()) {
				//cout << "header: " << (bucket_type)b << " (" << (b & 127) << ")" << endl;
			}
			out_file.put_byte(b);

			if (this_mode == block_literal) { // x offsets, 1 literal
				outp_refs();
				outp_lits();
			} else { // x literals, 1 offset
				outp_lits();
				outp_refs();
			}
			
			prev_mode = block_init;
			block_ct = 0;
		} else {
			prev_mode = this_mode;
			block_ct++;
		}
	}

	void write_end()
	{
		ref_que.push(0);
		
		if (prev_mode == block_literal) {
			block_mode(block_ref, prev_mode);
		} else {
			block_ct++;
			block_mode(block_literal, prev_mode);
		}
		
		cout << "writing end" << endl;
	}
	
	void close()
	{
		getchar();
		
		if (strings != NULL) {
			//delete[] strings;
		}
		
		out_file.flush();
		out_file.close();
	}
	
	~lzns_deflate()
	{
		close();
	}

};


template
<bucket_type dict_size = DEFAULT_DICT_SIZE, bool disp = false, bucket_type disp_min = 0, bucket_type disp_max = -1, bool all_diffs = false>
class lzns_inflate {
	
	bitfile<false> inp_file;
	bitfile<true> out_file;
	bitfile<false> orig_file;
	bucket_type bucket_ct;
	bucket_type bucket_abs; // for debugging
	bucket_type char_ct;
	string data[2];
	int data_bank;
	vector<bucket_type> index;
	
public:
	
	lzns_inflate(
				 string p_inp_file,
				 string p_out_file,
				 string p_orig_file)
	{
		data[0].resize(dict_size);
		data[1].resize(dict_size);

		index.resize(dict_size + 1);
		
		bucket_ct = 0;
		bucket_abs = 0;
		char_ct = 0;
		data_bank = 0;
		
		inp_file.open(p_inp_file);
		out_file.open(p_out_file);
		orig_file.open(p_orig_file);
		
		inflate();
		close();
	}
	
	inline bool should_display()
	{
		return disp && ((bucket_abs >= disp_min && bucket_abs <= disp_max) || disp_max == -1);
	}
	
	int other_bank(int bank = -1)
	{
		if (bank >= 0) {
			return int(!bank);
		} else {
			return int(!data_bank);
		}
	}
	
	void output()
	{
		out_file.put_void((char*)data[data_bank].data(), char_ct);
		data_bank = other_bank();
	}
	
	inline void resize(bucket_type len)
	{
		if (char_ct + len > data[data_bank].size()) {
			data[data_bank].resize(data[data_bank].size() + data[data_bank].size() / 2);
		}
	}
	
	inline void update_buffer(string str)
	{
		string orig = orig_file.get_string(str.length());
		if (orig != str && (should_display() || all_diffs)) {
			cout << "		diff - " << bucket_abs << " orig: " << show(orig) << " decoded: " << show(str) << endl;
			getchar();
		}
		
		resize((bucket_type)str.length());
		memcpy((char*)data[data_bank].data() + char_ct, (char*)str.data(), str.length());
	}
	
	inline void write_index()
	{
		index[bucket_ct] = char_ct;
	}
	
	void store_bucket(string str)
	{
		update_buffer(str);
		
		bucket_ct++;
		bucket_abs++;
		char_ct += str.length();

		if (bucket_ct >= dict_size) {
			output();
			write_index();
			bucket_ct = 0;
			char_ct = 0;
		}
	}
	
	string ref_bucket(bucket_type v)
	{
		bucket_type os = v >> 1;
		bucket_type bucket = (bucket_ct - os) & (dict_size - 1);
		bucket_type l = v & 1;
		bucket_type a = index[bucket];
		bucket_type b = index[bucket + 1];
		bucket_type len = b - a;
		string wrap;
		int bank;
		
		if (bucket < bucket_ct) {
			bank = data_bank;
		} else {
			bank = other_bank();
		}
		
		if (bucket < dict_size - 1 || os == 1) {
			len += l;
		} else {
			wrap = data[other_bank(bank)].substr(0, l);
		}
		
		if (os == 1) {
			return data[bank].substr(a, len - l) + data[bank].substr(a, l);
		} else {
			return data[bank].substr(a, len) + wrap;
		}
	}
	
	void out_ref1(bucket_type v)
	{
		bucket_type l = v & 1;
		bucket_type os = v >> 1;
		bucket_type bucket = (bucket_ct - os) & (dict_size - 1);

		if (os >= dict_size - 1) {
			cout << "		" << bucket_abs << ") offset exceeds dictionary size of " << dict_size << ": " << os << endl;
		}
		
		write_index();
		string ref = ref_bucket(v);
		if (should_display())
			cout << bucket_abs << ") ref: " << show(ref) << " offset abs: " << bucket << " rel: " << v / 2 << " l: " << l << endl;
		store_bucket(ref);
	}
	
	void out_literal(const int len)
	{
		string lit = inp_file.get_string(len);
		
		for (int i = 0; i < len; ++i) {
			write_index();
			if (should_display())
				cout << bucket_abs << ") lit: " << show(lit.substr(i, 1)) << endl;
			store_bucket(lit.substr(i, 1));
		}
	}
	
	bool out_ref(int n)
	{
		for (int i = 0; i < n - 1; ++i) {
			out_ref1(inp_file.get_varint());
		}
		
		bucket_type v = inp_file.get_varint();
		
		if (v > 0) {
			out_ref1(v);
			return true;
		} else {
			return false;
		}
	}
	
	void inflate()
	{
		bool cont = true;
		
		while (cont) {
			int block = inp_file.get_byte();
			if (should_display()) {
				cout << "---------------------------------------------------------------------------- \\/ block of: " << (block & 127) << endl;
			}

			if (block >= 128) { // N offsets, 1 literal
				block -= 128;
				cont = out_ref(block);
				if (cont && block < 127)
					out_literal(1);
			} else { // N literals, 1 offset
				out_literal(block);
				if (block < 127)
					cont = out_ref(1);
			}
		}
	}
	
	void close()
	{
		output();
		inp_file.close();
		out_file.close();
	}
	
	~lzns_inflate()
	{
	}
};

#endif /* lzns_h */
