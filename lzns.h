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

const int DEFAULT_DICT_SIZE = 1024 * 1024 * 4;

string show(const string s)
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
		
		status = complete;
		close();
	}
	
	void advance_bucket()
	{
		bucket_ct++;
	}
	
	void update_queue(string s)
	{
		que.push(s);
		
		if (que.size() > dict_size) {
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
		
		//reserve_header();
		
		if (disp)
			cout << "Done" << endl;
	}
	
	// deflate lzns2
	void deflate_lzns2()
	{
		unordered_map<string, int>::iterator f, prev_f, er;

		string a = string(1, inp_file.get_byte());
		stack.push_front(a);
		
		string b = string(1, inp_file.get_byte());
		stack.push_front(b);

		cout << "init a: " << show(a) << endl;
		cout << "init b: " << show(b) << endl;

		while (!inp_file.at_end()) {
			string s = stack[1] + stack[0];
			f = dic.find(s);
			
			if (f != dic.end()) {
				string e = stack[2] + stack[1];
				er = dic.find(e);
				if (er != dic.end())
					dic.erase(er);
				
				stack.pop_front();
				stack.pop_front();
				stack.push_front(s);

				cout << "found: " << show(s) << " " << f->second << endl;
				//cout << "erasing: " << show(e) << endl;
			} else {
				insert(s);
				string n = string(1, inp_file.get_byte());
				stack.push_front(n);
				cout << "new: " << show(n) << endl;
			}
		}
		stack.pop_front();
		
		cout << "-----------------------------" << endl;
		
		for (int i = (int)stack.size() - 1; i >= 0; --i) {
			cout << i + 1 << ": " << show(stack[i]) << endl;
		}
	}
	
	void rewrite_literal(const string lit, const string nex)
	{
		string a, b;
		dic_iter f;
		
		if (lit.length() != 1) return;
		
		for (int i = 0; i < lit.length() - 1; ++i) {
			a = string(1, lit[i]);
			b = string(1, lit[i + 1]);
			if (!insert(a + b, f)) {
				f->second = -(bucket_ct - 2 + i);
			}
			update_queue(a + b);
		}
		
		/*
		if (!insert(b + nex, f)) {
			f->second = -(bucket_ct - 2 + (int)lit.length() + 1);
			update_queue(b + nex);
		}
		*/
		
		//insert(b + nex);
		//update_queue(b + nex);
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
	
	void write_literal_orig(string s)
	{
		if (disp)
			cout << bucket_ct << ": literal: " << show(s) << endl;
		
		for (int i = 0; i < s.length(); ++i) {
			out_file.put_bit(false); // false = literal
			out_file.put_byte(s[i]);
			advance_bucket();
			//update_queue(s);
			//update_queue(string(1, s[i]));
			//update_queue(s, dic.end());
		}
		
		block_mode(false);

	}
	
	void write_end_orig()
	{
		if (disp)
			cout << "writing end" << endl;
		out_file.put_bit(true); // true = reference
		out_file.put_varint(0);
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
	
	bool write_reference_orig(
						 unordered_map<string, int>::iterator prev_f,
						 unordered_map<string, int>::iterator f,
						 string s)
	{
		int offset ;
		bool len;
		bool ret;
		
		if (prev_f->second <= 0) {
			offset = bucket_ct + prev_f->second;
			len = true; // length = 2
			//if (disp) cout << "len 2 - ";
		} else {
			offset = bucket_ct - prev_f->second;
			len = false; // length = 1
			//if (disp) cout << "len 1 - ";
		}
		
		string v = varint::int_to_string(offset);
		
		assert(offset >= 0);
		
		if (true
			//&& v.length() <= s.length()
			//&&
			//&& offset < dict_size
			) { // worth it
			//if (s.length() != 1)
			
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
			
			if (disp)
				cout << bucket_ct << ":			" << show(s);
			
			if (disp)
				cout << " " << offset << endl;
			
			//if (s.length())
			//out_file.put_bit(len);
			advance_bucket();
			//update_queue(f->first);

			ret = true;
		} else { // not worth it
			//cout << bucket_ct << ": can't do it: " << show(s) << " offset: " << offset << endl;
			//exit(5);
			dic.erase(f);
			write_literal(s);
			ret = false;
		}
		
		//update_queue(f);
		block_mode(true);

		return ret;
	}
	
	void close()
	{
		out_file.flush();
		//write_header();
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
	vector<string> bucket;
	int bucket_ct;
	
public:
	
	lzns_inflate(
		string p_inp_file,
		string p_out_file,
		string p_orig_file)
	{
		bucket_ct = 0;
		
		inp_file.open(p_inp_file);
		out_file.open(p_out_file);
		orig_file.open(p_orig_file);

		bucket.resize(dict_size);
		
		inflate();
		close();
	}
	
	void advance_bucket()
	{
		bucket_ct++;
	}
	
	void out_literal(int n)
	{
		for (int i = 0; i < n; ++i) {
			char c = inp_file.get_byte();
			bucket[bucket_ct % dict_size] = string(1, c);
			if (disp)
				cout << bucket_ct << ": literal: " << show(c) << endl;
			
			output();
			advance_bucket();
		}
	}
	
	void out_ref1(int v)
	{
		int off = bucket_ct - v / 2;
		int len = v & 1;
		
		bucket[bucket_ct % dict_size] = bucket[off % dict_size];
		
		if (len) {
			bucket[bucket_ct % dict_size] += bucket[(off + 1) % dict_size].substr(0, 1);
		}
		
		if (disp)
			cout << bucket_ct << ": offset: " << off << " len: " << 1 + len << " " << show(bucket[bucket_ct % dict_size]) << endl;
		
		output();
		advance_bucket();
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
	
	void output()
	{
		string ou = bucket[bucket_ct % dict_size];
		string orig = orig_file.get_string(ou.length());
		
		if (ou == orig) {
			if (disp)
				cout << "output: " << show(ou) << endl;
			out_file.put_string(ou);
		} else {
			cout << "Ooops";
			cout << bucket_ct << ": orig: " << show(orig) << " ou: " << show(ou) << endl;
			getchar();
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
	
	void inflate_orig()
	{
		//long long len = inp_file.get_qword();
		int str_len;
		bool cont = true;
		
		for (int b = 0; cont; ++b) {
			if (inp_file.get_bit()) { // reference
				int n = inp_file.get_varint(str_len);
				
				if (n <= 0) {
					cout << "[exit signal]" << endl;
					cont = false;
				} else {
					int pos = b - n;
					bucket[b % dict_size] = bucket[pos % dict_size];
					
					if (bucket[pos % dict_size].size() == 1 || inp_file.get_bit()) {
						bucket[b % dict_size] += string(1, bucket[(pos + 1) % dict_size][0]);
						if (disp)
							cout << b << ":		   create: " << show(bucket[b % dict_size]) << " @ " << pos << ", " << n << endl;
					} else {
						if (disp)
							cout << b << ":			   	copy: " << show(bucket[b % dict_size]) << " @ " << pos << ", " << n << endl;
					}
				}
			} else { // literal byte
				bucket[b % dict_size] = string(1, inp_file.get_byte());
				if (disp)
					cout << b << ":	 literal: " << show(bucket[b % dict_size]) << endl;
			}
			
			if (cont) {
				string ou = bucket[b % dict_size];
				string orig = orig_file.get_string(ou.length());
				//if (disp)
				//	cout << b << ": orig: " << show(orig) << " ou: " << show(ou) << endl;
				
				if (ou == orig) {
					out_file.put_string(ou);
				} else {
					cout << "Ooops";
					cout << b << ": orig: " << show(orig) << " ou: " << show(ou) << endl;
					getchar();
				}
				
				if (disp)
					cout << endl;
			}
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
