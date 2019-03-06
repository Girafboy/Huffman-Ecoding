#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <io.h>
#include <Windows.h>

using namespace std;

/* Node represent units of a tree */
struct Node {
	int layer;//mean the distance to the root node
	bool isleft;//mean whether the node is a left subnode
	char value;//store the value of node, indicate a character
	int freq;//store the frequency, just as a weight
	Node *lst, *rst;//left side tree node and right side tree node

	/* Construction of Node */
	Node(char value,int freq) {
		this->value = value;
		this->freq = freq;
		this->lst = NULL;
		this->rst = NULL;
	}
	Node(Node* lst, Node* rst, int freq) {
		this->lst = lst;
		this->rst = rst;
		this->freq = freq;
	}
	/* Destruction of Node */
	~Node() {
		delete lst;
		delete rst;
	}
	/* If the node has none subtree-node, it returns true indicating the node is a leaf */
	bool isleaf() {
		if (lst == NULL && rst == NULL)
			return true;
		else
			return false;
	}
};

/* BitMani used to manipulate date in bit level */
class BitMani {
private:
	ifstream * ifs;//Decompress
	ofstream * ofs;//Compress
	int size;//indicate the amount of available bits
	unsigned char mem;//the cathe of bits
	int unvalidnum;//Decompress:indicate the amount of unvalid bits on the bottom of file
	/* Compress: output to file when the memory cathe is full */
	void output() {
		*ofs << mem;
	}
	/* Decompress: input to file when the memory cathe is empty */
	bool input() {
		char temp;
		ifs->get(temp);
		mem = (unsigned char)temp;
		ifs->get(temp);//Note that the last character is a digit indicating amount of remained unvalid bits
		if (ifs->peek()!=EOF) {
			ifs->putback(temp);
			return true;
		}
		return false;
	}

public:
	/*
	 * Construction of BitMani
	 * Note that: 
	 *			ofs is used in Compress
	 *			ifs is used in Decompress
	 */
	BitMani(ofstream* ofs) :size(0) {
		mem = 0x00;
		this->ofs = ofs;
	}
	BitMani(ifstream* ifs) :size(0) {
		mem = 0x00;
		this->ifs = ifs;
	}
	/* Compress:
	 * Add only one bit to the BitMani, it stores in the unsigned char mem,
	 * and it will output to file when mem is full with 8 bits */
	void add(bool bit) {
		mem = (mem | (bit << (7 - size)));
		size++;
		if (size == 8) {
			output();
			size = 0;
			mem = 0x00;
		}
	}
	/* Compress: Tell the size of unused bits in cathe */
	int remain() {
		if (size == 0) return 0;
		return 8 - size;
	}
	/* Compress: Output the remaining bits in the cathe */
	void finish() {
		output();
		size = 0;
		mem = 0x00;
	}
	/* Decompress: 
	 * Get only one bit from BitMani, and it will input from the file
	 * to get a char, when the cathe is empty */
	bool get(char& c) {
		if (size == 0) {
			size = 8;
			if (!input()) 
				return false;
		}	
		size--;
		bool bit= mem & (1 << size);
		if (bit)
			c = '1';
		else
			c = '0';
		return true;
	}
	/* Decompress: 
	 * Get only one bit when the cathe is the last char, and the cathe has 
	 * bits that are not valid */
	bool getremain(char& c) {
		if (size <= unvalidnum)
			return false;
		size--;
		bool bit = mem & (1 << size);
		if (bit)
			c = '1';
		else
			c = '0';
		return true;
	}
	/* Decompress: Get the amount of unvalid bits in the last char */
	void read_unvalidnum() {
		ifs->seekg(-1, ios_base::end);
		*ifs >> unvalidnum;
		ifs->seekg(ios_base::beg);
	}
};

/* Compress Part
--------------------------------------------------------------------*/
/* Read file first and get frequency of every character */
void loadfreq(map<char, int> &charfreq, string inputFilename) {
	ifstream ifs(inputFilename.c_str(), ios::in | ios::binary);
	char c;
	while (ifs.get(c))
		if (charfreq.find(c) != charfreq.end())
			charfreq[c]++;
		else
			charfreq[c] = 1;
	ifs.close();
}
/* Compare node by frequency */
bool cmpnode(Node *lhs, Node *rhs) {
	return lhs->freq > rhs->freq;
}
/* Build tree according to the frequency of characters */
Node* buildtree(map<char, int> &charfreq) {
	vector<Node*> nodelist;
	for (auto &it : charfreq) 
		nodelist.push_back(new Node(it.first, it.second));
	while (nodelist.size() > 1) {
		sort(nodelist.begin(), nodelist.end(), cmpnode);
		Node* n1 = nodelist.back();
		nodelist.pop_back();
		Node* n2 = nodelist.back();
		nodelist.pop_back();
		nodelist.push_back(new Node(n1, n2, n1->freq + n2->freq));
	}
	return nodelist[0];
}
/* Produce the Huffman Encode for characters */
void encoding(Node* tree, map<char, vector<bool>> &char_code) {
	stack<Node*> treenode;
	vector<pair<int,bool>> trace;

	//Cope with situation which has only one node
	if (tree->isleaf()) {
		vector<bool> code;
		code.push_back(1);
		char_code[tree->value] = code;
		return;
	}

	//Pre-order traversal
	treenode.push(tree);
	tree->layer = 0;
	while (!treenode.empty()) {
		Node* root = treenode.top();
		treenode.pop();
		//Back to the fork node and then find next leaf
		while (!trace.empty() && root->layer <= trace.back().first)
			trace.pop_back();
		//Record layer and position of nodes, indicating the code
		if (root->layer > 0)
			trace.push_back(pair<int, bool>(root->layer, root->isleft));
		//Encode one character
		if (root->isleaf()) {
			vector<bool> code;
			for (auto it : trace)
				code.push_back(it.second);
			char_code[root->value] = code;
		}
		//Stack push right side tree node
		if (root->rst != NULL) {
			treenode.push(root->rst);
			root->rst->isleft = false;
			root->rst->layer = root->layer + 1;
		}
		//Stack push left side tree node
		if (root->lst != NULL) {
			treenode.push(root->lst);
			root->lst->isleft = true;
			root->lst->layer = root->layer + 1;
		}
	}
}
/* Output the rule of encoding */
void addlogo(map<char, vector<bool>> char_code, ofstream &ofs) {
	ofs << hex << char_code.size() << '*';//hex encoding with '*' interval, the same as follow
	for (auto it : char_code) {
		ofs.put(it.first);
		int dec = 0;
		for (auto itt : it.second)
			dec = dec * 2 + itt;
		
		ofs << it.second.size() << '*' << hex << dec << '*';
	}
}
/* Explicit: compress */
void compress(string inputFilename, string outputFilename) {
	map<char, int> charfreq;
	map<char, vector<bool>> char_code;
	ofstream ofs(outputFilename.c_str(), ios::out | ios::binary);
	ifstream ifs(inputFilename.c_str(), ios::in | ios::binary);
	BitMani bitset(&ofs);

	//load character frequency
	loadfreq(charfreq, inputFilename);
	if (charfreq.empty())
		return;
	//build tree
	Node* tree = buildtree(charfreq);
	//encoding
	encoding(tree, char_code);
	//output rule of encoding
	addlogo(char_code, ofs);

	//Compress original file
	char c;
	while (ifs.get(c))
		for (auto it : char_code[c])
			bitset.add(it);
	int unvalidnum = bitset.remain();
	if (unvalidnum != 0)
		bitset.finish();

	//output amount of unvalid bits
	ofs << unvalidnum;

	delete tree;
	ofs.close();
	ifs.close();
};
/*
--------------------------------------------------------------------*/


/* Decompress Part
--------------------------------------------------------------------*/
/* Get value of hex string with mark '*' rearwards */
int getNum(ifstream &ifs) {
	char c;
	string charnum;
	while (ifs.get(c)) {
		if (c == '*')break;
		charnum += c;
	}
	stringstream ss;
	ss << charnum;
	int num;
	ss >> hex >> num;
	return num;
}
/* Get the rule of encoding */
void readlogo(map<string,char> &char_code, ifstream &ifs) {
	int charnum = getNum(ifs);//total amount of characters
	for (int i = 0; i < charnum;i++) {
		char character;
		ifs.get(character);//get character
		int length = getNum(ifs);//get the code's length of the character
		int dec = getNum(ifs);//get the hex represent of code

		//transfer the digits to string code
		string code;
		while (dec != 0) {
			if (dec % 2 == 1)
				code += '1';
			else 
				code += '0';
			dec /= 2;
		}
		for (int i = code.length(); i < length; i++)
			code += '0';
		reverse(code.begin(), code.end());

		char_code[code] = character;//produce mapping between code and character
	}
}
/* Explicit: decompress */
void decompress(string inputFilename, string outputFilename) {
	map<string, char> char_code;
	ifstream ifs(inputFilename.c_str(), ios::in | ios::binary);
	ofstream ofs(outputFilename.c_str(), ios::out | ios::binary);
	BitMani bitset(&ifs);

	//Cope with empty file
	if (ifs.peek() == EOF)
		return;
	//Get amount of unvalid bits in the last code
	bitset.read_unvalidnum();
	//Get coding type
	readlogo(char_code, ifs);

	//Decompress the content of file
	char c;
	string code;
	while (bitset.get(c)) {
		code += c;
		if (char_code.find(code) != char_code.end()) {
			ofs << char_code[code];
			code.clear();
		}
	}
	while (bitset.getremain(c)) {
		code += c;
		if (char_code.find(code) != char_code.end()) {
			ofs << char_code[code];
			code.clear();
		}
	}

	ifs.close();
	ofs.close();
}
/*
--------------------------------------------------------------------*/

void usage(string prog) {
  cerr << "Usage: " << endl
      << "    " << prog << "[-d] input_file output_file" << endl;
  exit(2);
}

int main(int argc, char* argv[]) {
	int i;
	string inputFilename, outputFilename;
	bool isDecompress = false;
	for (i = 1; i < argc; i++) {
		if (argv[i] == string("-d")) isDecompress = true;
		else if (inputFilename == "") inputFilename = argv[i];
		else if (outputFilename == "") outputFilename = argv[i];
		else usage(argv[0]);
	}
	if (outputFilename == "") usage(argv[0]);
	if (isDecompress) decompress(inputFilename, outputFilename);
	else compress(inputFilename, outputFilename);

	system("pause");
	return 0;
}
