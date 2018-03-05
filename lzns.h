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

string show(string s)
{
	string ret;
	
	for (int i = 0; i < s.length(); ++i) {
		char c = s[i];
		
		if (c < ' ') {
			ret += "[" + to_string(c) + "]";
		} else {
			ret += string(1, c);
		}
	}
	
	return "'" + ret + "'";
}

template <bool disp = false>
class lzns_deflate {
	
	bitfile<false> inp_file;
	bitfile<true> out_file;
	unordered_map<string, int> dic;
	int bucket_ct;
	
public:
	
	string next;

	void reserve_header()
	{
		out_file.put_string(string(8, char(0)));
	}
	
	void write_header()
	{
		out_file.rewind();
		out_file.put_qword(bucket_ct);
	}
	
	lzns_deflate (string p_inp_file, string p_out_file)
	{
		inp_file.open(p_inp_file);
		out_file.open(p_out_file);
		bucket_ct = 0;
		
		init();
		deflate();
	}
	
	void advance_bucket()
	{
		bucket_ct++;
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
		pair<unordered_map<string, int>::iterator, bool> res =
			dic.insert(pair<string, int>(s, -bucket_ct));
		return res.second;
	}
	
	void update(unordered_map<string, int>::iterator iter)
	{
		iter->second = bucket_ct - 1;
	}
	
	bool find_part(string& curr, string& next)
	{
		unordered_map<string, int>::iterator f;
		string s1, s2;
		
		if (next == "") {
			s1 = inp_file.get_byte();
		} else {
			s1 = next;
		}
		
		s2 = inp_file.get_byte();
		f = dic.find(s1 + s2);
		
		if (f == dic.end()) {
			curr = s1;
			next = s2;
			return false;
		} else {
			curr = s1 + s2;
			next = "";
			return true;
		}
	}
	
	string find_longest_rep()
	{
		string s = next;
		char c;
		
		do {
			c = inp_file.get_byte();
			s += string(1, c);
		} while (dic.find(s) != dic.end());
		
		next = string(1, c);
		return s.substr(0, s.length() - 1);
	}
	
	void init()
	{
		if (disp)
			cout << "Initializing" << endl;
		
		reserve_header();
		
		if (disp)
			cout << "Done" << endl;
	}
	
	void deflate()
	{
		string prev = string(1, inp_file.get_byte());
		unordered_map<string, int>::iterator f, prev_f;

		while (!inp_file.at_end()) {
			string curr = string(1, inp_file.get_byte());
			
			if (insert(prev + curr, f)) {
				if (prev_f != dic.end() && prev.length() > 1) {
					if (write_reference(prev_f, f, prev))
						prev_f->second = bucket_ct - 1;
				} else {
					write_literal(prev);
				}
				prev = curr;
			} else {
				prev += curr;
			}
			
			prev_f = f;
		}
		
	}
	
	void deflate7()
	{
		string curr, prev, prev_s;
		unordered_map<string, int>::iterator f, prev_f;

		prev_f = dic.end();
		prev = string(1, inp_file.get_byte());

		while (!inp_file.at_end()) {
			
			curr = string(1, inp_file.get_byte());

			if ((f = dic.find(prev + curr)) == dic.end()) {
				//cout << "prev_s: '" << prev_s << "' s: '" << prev << "'" << endl;

				cout << "       inserting: '" << prev_s + prev << "'" << endl;
				insert(prev_s + prev, prev_f);
				
				prev_s = prev;
				
				if (prev_f != dic.end()) {
					//cout << bucket_ct << ": " << "'" << prev << "': " << prev_f->second << endl;
				} else {
					//cout << bucket_ct << ": " << "'" << prev << "'" << endl;
				}
				cout << "'" << prev << "'" << endl;

				prev = curr;
				
				advance_bucket();
			} else {
				//cout << "    ref: " << f->second << endl;
				prev += curr;
				prev_f = f;
			}
		}
	}
	
	void deflate6()
	{
		string curr, prev;
		unordered_map<string, int>::iterator f;
		
		prev = next;
		
		while (!inp_file.at_end()) {
			curr = find_longest_rep();
			//cout << "prev: '" << prev << "' curr: '" << curr << "'" << endl;
			
			if (insert(prev + curr, f)) {
				cout << bucket_ct << ": " << "'" << prev << "': " << f->second << endl;
				prev = curr;
				advance_bucket();
			} else {
				cout << "    ref: " << f->second << endl;
				prev += curr;
			}
		}
	}
	
	void deflate5()
	{
		string prev, curr, next;
		unordered_map<string, int>::iterator prev_f, curr_f;
		
		reserve_header();
		
		while (!inp_file.at_end()) {
			if (find_part(curr, next)) {
				cout << "'" << curr << "'" << endl;
				curr = "";
			} else {
				cout << "'" << curr << "'" << endl;
				//insert(curr + next);
			}
			
			insert(prev + curr);
			prev = curr;
			curr = next;
		}
	}
	
	void deflate_byte_pair()
	{
		string prev, curr;
		unordered_map<string, int>::iterator prev_f, curr_f;
		
		reserve_header();
		
		while (!inp_file.at_end()) {
			// get current 2-byte string
			curr += inp_file.get_byte();
			if (curr.length() > 2)
				curr = curr.substr(1, 2);
			
			// lookup current 2-byte string
			if (curr.length() > 1) {
				curr_f = dic.find(curr);
				
				if (
					curr_f != dic.end()
					&& bucket_ct - curr_f->second >= 1000000 * 0 + 128 * 1 + 16384 * 0) {
					
					dic.erase(curr_f);
					curr_f = dic.end();
				}
			} else {
				curr_f = dic.end();
			}
			
			if (curr.length() > 1) {
				if (curr_f == dic.end()) {
					if (prev_f == dic.end()) { // a,b,c
						string s = curr.substr(0, 1);
						insert(prev);
						cout << "lit '" << s << "'" << endl;
						write_literal(s);
					} else {								// ab,c
						//update(prev_f);
					}
				} else {
					if (prev_f == dic.end()) { // a,bc
						int offset = bucket_ct - curr_f->second;
						write_reference(offset, "");
						update(curr_f);
						cout << "ref '" << curr << "' " << offset << endl;
						curr = "";
					} else {								 // ab,bc
						curr_f = dic.end();
					}
				}
			}
			
			prev = curr;
			prev_f = curr_f;
		}
	}
	
	void deflate4()
	{
		string prev, curr;
		unordered_map<string, int>::iterator prev_f, curr_f;

		reserve_header();

		while (!inp_file.at_end()) {
			// get current 2-byte string
			curr += inp_file.get_byte();
			if (curr.length() > 2)
				curr = curr.substr(1, 2);
			
			// lookup current 2-byte string
			if (curr.length() > 1) {
				curr_f = dic.find(curr);
			}
			
			// parse cases
			//cout << "prev: '" << prev << "' curr: '" << curr << "' case: " << endl;
			if (prev_f == dic.end()) {
				if (curr_f == dic.end()) { // a,b,c
					if (prev.length() > 1) {
						insert(prev);
						string s = prev.substr(1, 1);
						cout << "literal: '" << s << "'" << endl;
						write_literal(s);
					}
					//cout << "a,b,c" << endl;
				} else if (curr.length() > 1) { // a,bc
					int offset = bucket_ct - curr_f->second;
					cout << "ref    : '" << curr << "' offset: " << offset << endl;
					write_reference(offset, "");
					//cout << "a,bc" << endl;
					curr = "";
					//curr_f = dic.end();
				}
			} else { // ab,c (& ab,bc)
				update(prev_f);
				//cout << "ab,c" << endl;
				curr_f = dic.end();
			}
			cout << endl;
			
			// curr to previous
			prev_f = curr_f;
			prev = curr;
		}
	}
	
	void deflate3()
	{
		string prev, curr;
		unordered_map<string, int>::iterator f;
		unordered_map<string, int>::iterator prev_f;
		int sec = -1, prev_sec = -1;
		
		reserve_header();
		
		while (!inp_file.at_end()) {
			curr += inp_file.get_byte();
			if (curr.length() > 2)
				curr = curr.substr(1, 2);
				
			if (curr.length() == 2) {
				f = dic.find(curr);
				advance_bucket();

				if (f == dic.end()) {
					cout << "literal: '" << curr.substr(0, 1) << "'" << endl;
					write_literal(curr.substr(0, 1));
					sec = -1;
				} else {
					int offset = bucket_ct - f->second;
					
					if (offset < 1 * 128 + 0 * 16384) {
						cout << "ref: '" << curr << "' " << offset << endl;
						write_reference(offset, "");
						prev = curr = "";
						sec = bucket_ct + 0;
					} else {
						cout << "literal2: '" << curr.substr(0, 1) << "'" << endl;
						write_literal(curr.substr(0, 1));
						sec = bucket_ct + 1;
					}
					
					//f->second = bucket_ct - 0;
					//advance_bucket();
					//dic.erase(f);
				}
			}
			
			if (prev.length() == 2) {
				//if (f == dic.end())
				dic.insert(pair<string, int>(prev, bucket_ct));
				
				if (prev_f != dic.end())
					prev_f->second = prev_sec;
			}
			
			prev_f = f;
			prev = curr;
			prev_sec = sec;
		}
	}
	
	void deflate2()
	{
		string prev, curr;
		string s, prev_s;
		
		reserve_header();

		prev = curr = inp_file.get_byte();
		write_literal(curr);
		bucket_ct++;
		cout << bucket_ct << " literal: '" << curr << "'" << endl;

		while (!inp_file.at_end()) {
			unordered_map<string, int>::iterator f ;
			curr = inp_file.get_byte();
			s = prev + curr;
			f = dic.find(s);
			
			if (f != dic.end() && prev_s != "") {
				int offset = bucket_ct - f->second;
				write_reference(offset + 1, "");
				cout << bucket_ct << " ref: '" << s << "'" << endl;
				prev_s = s = "";
				bucket_ct++;
			}
			
			if (prev_s.length() > 1) {
				dic.insert(pair<string, int>(prev_s, bucket_ct));
				cout << "     inserting: '" << prev_s << "'" << endl;
			}
			
			/*
			cout << "      searching for: '" << s << "'" << endl;

			if (prev_s.length() > 1) {
				if (f == dic.end()) {
					dic.insert(pair<string, int>(prev_s, bucket_ct));
					//cout << "     inserting: '" << prev_s << "'" << endl;
				} else {
					cout << "      found: '" << s << "'" << endl;

					int offset = bucket_ct - f->second;
					
					if (offset < 128) {
						//f->second = bucket_ct;
						cout << bucket_ct << " found: '" << prev_s << "' " << offset << endl;
						write_reference(offset + 1);
						curr = "";
						prev_s = "";
						bucket_ct++;
					} else {
						f->second = bucket_ct;
					}
				}
			}
			*/
			
			//if (prev_s != "" && s != "") {
			if (prev_s != "") {
				cout << bucket_ct << " literal: '" << curr << "'" << endl;
				write_literal(curr);
				bucket_ct++;
			}
			
			prev = curr;
			prev_s = s;
		}
		
		close();
	}
	
	void write_literal(string s)
	{
		if (disp)
			cout << bucket_ct << ": literal: " << show(s) << endl;
		
		for (int i = 0; i < s.length(); ++i) {
			out_file.put_bit(false); // false = literal
			out_file.put_byte(s[i]);
			advance_bucket();
		}
	}
	
	void write_reference(int offset, bool nu = false)
	{
		out_file.put_bit(true); // true = reference
		out_file.put_bit(nu); // false = copy, true = create
		out_file.put_varint(offset);
		advance_bucket();
	}

	bool write_reference(
						 unordered_map<string, int>::iterator prev_f,
						 unordered_map<string, int>::iterator f,
						 string s)
	{
		int offset ;
		bool len;
		
		if (disp)
			cout << bucket_ct << ": " << show(s);
		
		if (prev_f->second <= 0) {
			offset = bucket_ct + prev_f->second;
			len = true; // length = 2
			if (disp) cout << "len 2";
		} else {
			offset = bucket_ct - prev_f->second;
			len = false; // length = 1
			if (disp) cout << "len 1";
		}
		
		if (disp)
			cout << " " << offset << endl;
		
		string v = varint::int_to_string(offset);
		
		assert(offset >= 0);
		
		if (true) { // v.length() <= s.length()) { // worth it
			out_file.put_bit(true); // true = reference
			out_file.put_varint(offset);
			
			//*
			if (len) { // length = 2
				if (s.length() > 2) {
					out_file.put_bit(len);
				}
			} else { // length = 1
				//if (s.length() > 1) {
				out_file.put_bit(len);
				//}
			}
			//*/
			
			//if (s.length())
			//out_file.put_bit(len);
			advance_bucket();
			return true;
		} else { // not worth it
			dic.erase(f);
			write_literal(s);
			return false;
		}
	}
	
	void close()
	{
		out_file.flush();
		write_header();
		out_file.close();
	}
	
	~lzns_deflate()
	{
		close();
	}
};


template <bool disp = false, int dict_size = 65536>
class lzns_inflate {

	bitfile<false> inp_file;
	bitfile<true> out_file;
	bitfile<false> orig_file;
	vector<string> bucket;

public:
	
	lzns_inflate(
		string p_inp_file,
		string p_out_file,
		string p_orig_file)
	{
		inp_file.open(p_inp_file);
		out_file.open(p_out_file);
		orig_file.open(p_orig_file);

		bucket.resize(dict_size);
	}
	
	void inflate()
	{
		long long len = inp_file.get_qword();
		int str_len;
		
		for (int b = 0; b < len; ++b) {
			if (inp_file.get_bit()) { // reference
				int n = inp_file.get_varint(str_len);
				int pos = b - n;
				bucket[b % dict_size] = bucket[pos % dict_size];
				
				if (bucket[pos % dict_size].size() == 1 || inp_file.get_bit()) {
					bucket[b % dict_size] += string(1, bucket[(pos + 1) % dict_size][0]);
					if (disp)
						cout << b << ": create: " << show(bucket[b % dict_size]) << " @ " << pos << ", " << n << endl;
				} else {
					if (disp)
						cout << b << ": copy: " << show(bucket[b % dict_size]) << " @ " << pos << ", " << n << endl;
				}
			} else { // literal byte
				bucket[b % dict_size] = string(1, inp_file.get_byte());
				//cout << b << ": literal: " << show(bucket[b % dict_size]) << endl;
			}
			
			string ou = bucket[b % dict_size];
			string orig = orig_file.get_string(ou.length());
			if (disp)
				cout << b << ": orig: " << show(orig) << " ou: " << show(ou) << endl;
			
			if (ou == orig) {
				out_file.put_string(ou);
			} else {
				cout << b << ": orig: " << show(orig) << " ou: " << show(ou) << endl;
				cout << "Ooops";
				getchar();
			}
			
			if (disp)
				cout << endl;
		}
	}
	
	void inflate2()
	{
		long long len = inp_file.get_qword();
		
		for (int b = 0; b < len; ++b) {
			if (inp_file.get_bit()) { // literal byte
				bucket[b & 0xffff] = string(1, inp_file.get_byte());
				//cout << "b:    " << b << endl;
				//cout << "literal: '" << bucket[b & 0xffff] << "'" << endl;
				//cout << endl;
			} else { // reference
				int pos = b - inp_file.get_varint();
				string bu = bucket[pos & 0xffff];
				bucket[b & 0xffff] = bu;
				
				//cout << "b:    " << b << endl;
				//cout << "pos: " << pos << endl;
				if (bu.length() == 1) {
					//cout << "lk - 1: '" << bucket[(pos - 1) & 0xffff] << "'" << endl;
				}
				//cout << "lk - 0: '" << bu << "'" << endl;
				//cout << endl;
				
				if (bu.length() == 1) {
					bucket[b & 0xffff] = bucket[(pos - 1) & 0xffff] + bu;//bucket[b & 0xffff];
				}
			}
			
			//cout << bucket[b & 0xffff];
			out_file.put_string(bucket[b & 0xffff]);
		}
	}
	
	void close()
	{
		inp_file.close();
		out_file.close();
	}
	
	~lzns_inflate()
	{
		close();
	}
};

#endif /* lzns_h */
