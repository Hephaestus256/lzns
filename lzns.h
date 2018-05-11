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

const int DEFAULT_DICT_SIZE = 1024 * 1024 * 4;

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

template <int dict_size = DEFAULT_DICT_SIZE, bool disp = false>
class lzns_deflate {
	
	enum status_type {begin, error, complete};
	typedef unordered_map<string, int> dic_type;
	typedef dic_type::iterator dic_iter;
	
	int bct; // count for block
	int bac; // accounted for block
	int bpr; // block previous
	bool bmd; // block mode (literal / reference)
	bitfile<false> inp_file;
	bitfile<true> out_file;
	dic_type dic;
	int bucket_ct;
	deque<string> stack;
	status_type status;
	queue<string> que;
	string lit_que;
	queue<int> ref_que;
	
public:
	
	string next;

	bool in_error()
	{
		return status == error;
	}
	
	lzns_deflate (string p_inp_file, string p_out_file)
	{
		status = begin;
		bct = 0;
		bpr = -1;
		bmd = false;
		
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
		
		init();
		deflate();
		close();
		status = complete;
	}
	
	void advance_bucket()
	{
		bucket_ct++;
	}
	
	void update_queue(string s)
	{
		que.push(s);
		
		if (que.size() >= dict_size) { // ensure that offset limit is dict_size - 1
			string str = que.front();

			for (int i = 0; i < 2; ++i) {
				//cout << "	str: " << show(str) << endl;

				dic_iter f = dic.find(str);
			
				if (f != dic.end() && f->first.length() > 1) {
					int o = f->second;
					if (o < 0) o = -o;

					//cout << "	dist: " << bucket_ct - o << endl;
					
					if (bucket_ct - o >= dict_size && dic.find(f->first) != dic.end()) {
						//cout << "	deleting: " << show(f->first) << endl;
						dic.erase(f);
					}
				}
				
				str = str.substr(0, str.length() - 1);
			}
			
			que.pop();
		}
	}
	
	bool insert(string s, unordered_map<string, int>::iterator& f)
	{
		pair<unordered_map<string, int>::iterator, bool> res =
			dic.insert(pair<string, int>(s, -bucket_ct));
		
		f = res.first;
		
		return res.second;
	}
	
	 bool insert(string s)
	{
		cout << "inserting: " << show(s) << endl;
		
		pair<unordered_map<string, int>::iterator, bool> res =
			dic.insert(pair<string, int>(s, -bucket_ct));
		return res.second;
	}
	
	void update(unordered_map<string, int>::iterator iter)
	{
		iter->second = bucket_ct - 1;
	}
	
	void init()
	{
		if (disp)
			cout << "Initializing" << endl;
		
		if (disp)
			cout << "Done" << endl;
	}
	
	// deflate lzns1
	void deflate()
	{
		string prev = string(1, inp_file.get_byte());
		unordered_map<string, int>::iterator f, prev_f, best_f;

		while (!inp_file.at_end()) {
			string curr = string(1, inp_file.get_byte());
			
			if (insert(prev + curr, f) || inp_file.at_end()) {
				if (prev_f != dic.end() && prev.length() > 1) {
					if (write_reference(prev_f, f, prev)) {
						prev_f->second = bucket_ct - 1;
						update_queue(f->first);
					} else {
						//rewrite_literal(prev, curr);
						//write_literal2(prev.substr(0, 1));
						//curr = prev.substr(1, prev.length() - 1);
						update_queue(f->first);
					}
				} else {
					write_literal(prev);
					update_queue(f->first);
					//update_queue(prev);// + curr);//f->first);
					//update_queue(curr);// + curr);//f->first);
				}

				prev = curr;
			} else {
				prev += curr;
			}
			
			prev_f = f;
		}
		
		write_end();
	}
	
	void outp_refs()
	{
		while (!ref_que.empty()) {
			if (disp)
				cout << "writing offset: " << ref_que.front() << endl;
			out_file.put_varint(ref_que.front());
			ref_que.pop();
		}
	}
	
	void outp_lits()
	{
		for (int i = 0; i < lit_que.size(); ++i) {
			if (disp)
				cout << "writing literal: " << show(lit_que[i]) << endl;
			out_file.put_byte(lit_que[i]);
		}
		lit_que = "";
	}
	
	void block_mode(bool mode, bool end = false)
	{
		if (
			(ref_que.size() > 0 && lit_que.size() > 0 && bmd != mode)
			|| (ref_que.size() + lit_que.size() >= 127) || end)
		{
			if (disp)
				cout << "---------------------------------------------------------------------------- block of: x" << endl;
		
			if (bmd) { // offset -> literal
				if (disp) {
					cout << ref_que.size();
					cout << " offset, ";
					cout << lit_que.size();
					cout << " lits" << endl;
				}
				out_file.put_byte(128 + ref_que.size());
				outp_refs();
				outp_lits();
			} else { // literal -> offset
				if (disp) {
					cout << lit_que.size();
					cout << " literal, ";
					cout << ref_que.size();
					cout << " offset" << endl;
				}
				out_file.put_byte(lit_que.size());
				outp_lits();
				outp_refs();
			}
			
			if (disp)
				cout << endl;
		}
		
		bmd = mode;
	}
	
	void write_end()
	{
		ref_que.push(0);
		block_mode(bmd, true);
		cout << "writing end" << endl;
		//out_file.put_byte(0);
	}

	void write_literal(string s)
	{
		if (disp)
			cout << bucket_ct << ": literal: " << show(s) << endl;
		
		for (int i = 0; i < s.length(); ++i) {
			lit_que += s[i];
			advance_bucket();
			block_mode(false);
			
			//cout << "lit: " << show(s[i]) << endl;
		}
	}
	
	bool write_reference(
		dic_iter prev_f, dic_iter f, string s)
	{
		int offset;
		int len;
		
		if (prev_f->second <= 0) {
			offset = bucket_ct + prev_f->second;
			len = 1; // length = 2
		} else {
			offset = bucket_ct - prev_f->second;
			len = 0; // length = 1
		}
		
		if (disp)
			cout << bucket_ct << ":			" << show(s);
		
		if (disp)
			cout << " " << offset << endl;

		//cout << "ref: " << offset << endl;

		assert(offset >= 0);
		
		ref_que.push(offset * 2 + len);
		advance_bucket();

		block_mode(true);

		return true;
	}
	
	void close()
	{
		out_file.flush();
		out_file.close();
	}
	
	~lzns_deflate()
	{
		close();
	}
};


template <int dict_size = DEFAULT_DICT_SIZE, bool disp = false>
class lzns_inflate {
	
	bitfile<false> inp_file;
	bitfile<true> out_file;
	bitfile<false> orig_file;
	int bucket_ct;
	int char_ct;
	string data[2];
	int data_bank;
	vector<int> index;
	
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
		char_ct = 0;
		data_bank = 0;
		
		inp_file.open(p_inp_file);
		out_file.open(p_out_file);
		orig_file.open(p_orig_file);
		
		inflate();
		close();
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
		//cout << "	writing: " << show(data.substr(0, char_ct)) << endl;
		
		//cout << "writing" << endl;
		
		/*
		cout << "	";
		
		int a, b;
		for (int i = 0; i < dict_size; ++i) {
			a = index[i];
			b = index[i + 1];
			cout << "[" << a << "]" << show(data[data_bank].substr(a, b - a));
		}
		cout << " [" << b << "]" << endl;
		*/
		
		data_bank = other_bank();
	}
	
	inline void resize(int len)
	{
		if (char_ct + len > data[data_bank].size()) {
			data[data_bank].resize(data[data_bank].size() + data[data_bank].size() / 2);
		}
	}
	
	inline void update_buffer(string str)
	{
		resize((int)str.length());
		memcpy((char*)data[data_bank].data() + char_ct, (char*)str.data(), str.length());
	}
	
	void write_index()
	{
		index[bucket_ct] = char_ct;
	}
	
	void store_bucket(string str)
	{
		//write_index();
		update_buffer(str);
		
		bucket_ct++;
		char_ct += str.length();

		if (bucket_ct >= dict_size) {
			output();
			write_index();
			bucket_ct = 0;
			char_ct = 0;
		}
	}
	
	string ref_bucket(int v)
	{
		int os = v>> 1;
		int bucket = (bucket_ct - os) & (dict_size - 1);
		int tmp = bucket_ct;
		int l = v & 1;
		int a = index[bucket];
		int b = index[bucket + 1];
		int len = b - a;
		string wrap;
		int bank;
		
		if (bucket < bucket_ct) {
			bank = data_bank;
		} else {
			bank = other_bank();
		}
		
		//cout << bucket_ct << ": " << " from " << bucket << " [" << a << ", " << b << "] ";

		assert(v > 1);
		assert(len > 0);

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
	
	void out_ref1(int v)
	{
		write_index();

		string ref = ref_bucket(v);
		
		//cout << " ref: " << show(ref) << endl;

		store_bucket(ref);
	}
	
	void out_literal(const int len)
	{
		string lit = inp_file.get_string(len);
		
		//cout << bucket_ct << " - literal: " << show(lit) << endl;
		
		for (int i = 0; i < len; ++i) {
			write_index();
			store_bucket(lit.substr(i, 1));
		}
	}
	
	bool out_ref(int n)
	{
		for (int i = 0; i < n - 1; ++i) {
			out_ref1(inp_file.get_varint());
		}
		
		int v = inp_file.get_varint();
		
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
