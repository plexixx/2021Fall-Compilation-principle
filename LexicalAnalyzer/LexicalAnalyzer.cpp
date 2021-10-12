#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>

#pragma warning(disable:4996)

using namespace std;

const char keyword[50][12] = { "break", "case", "char", "const", "continue", "default", "do", "double",
							  "else", "float", "for", "if", "int", "include", "long", "return", "short", "signed", "sizeof", "static",
							  "struct", "switch", "typedef", "unsigned", "void", "define", "main", "while" }; // 关键词
const int buf_size = 128;                                                                                    // 缓冲区大小

int row_num = 0;       // 行数
int char_num = 0;      // 字符数
int word_num = 0;      // 单词数
int status = 0;        // 当前状态
int spcode = 0;        // 种别码
int pstart = 0;        // 起点指示器
int pend = 0;          // 搜索指示器
bool buf_flag = false; // false为左半缓冲区，true为右半缓冲区

char tmp_buf;       // 缓存缓冲区更新前pstart指向的字符 when pstart = buf_size - 1
char buf[buf_size]; // 缓冲区，一分为二
string token;
stringstream ss;

FILE* fp;    // 输入文件指针
FILE* outfp; // 输出文件指针

// 判断是否为字符
bool isLetter(char ch)
{
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
		return true;
	return false;
}

// 判断是否为数字
bool isDigit(char ch)
{
	if (ch >= '0' && ch <= '9')
		return true;
	return false;
}

// 判断标识符&关键词
bool isID()
{
	token = "";
	if (pstart < pend) {
		for (int i = pstart; i < pend; i++)token.push_back(buf[i]);
	}
	else {
		if (pstart == buf_size - 1) { token.push_back(tmp_buf); }
		for (int i = 0; i < pend; i++)token.push_back(buf[i]);
	}

	for (int i = 0; i < 28; i++) {
		if (token.compare(keyword[i]) == 0) { spcode = i + 36; return true; }
	}

	char init = token[0];

	// 非法标识符
	if (init != '_' && !isLetter(init)) { status = -2; return false; }

	for (int i = 1; i < (int)token.length(); i++) {
		// 含非法字符
		if (!isLetter(token[i]) && !isDigit(token[i]) && token[i] != '_') { status = -1; return false; }
	}

	spcode = 0;
	return true;
}

// 判断常数
bool isNum()
{
	token = "";

	if (pstart < pend) {
		for (int i = pstart; i < pend; i++)token.push_back(buf[i]);
	}
	else {
		if (pstart == buf_size - 1) { token.push_back(tmp_buf); }
		for (int i = 0; i < pend; i++)token.push_back(buf[i]);
	}

	for (int i = 0; i < (int)token.length(); i++) {
		if (!isDigit(token[i])) { status = -1; return false; }
	}

	spcode = 1;
	return true;
}

// 单目运算符
void isMono()
{
	switch (status)
	{
	case 9: spcode = 2; break;
	case 13: spcode = 3; break;
	case 17: spcode = 4; break;
	case 21: spcode = 6; break;
	case 24: spcode = 9; break;
	case 27: spcode = 10; break;
	case 31: spcode = 12; break;
	case 34: spcode = 34; break;
	default: break;
	}
	return;
}

// 二目运算符
void isBino()
{
	switch (status)
	{
	case 7: spcode = 13; break;
	case 8: spcode = 21; break;
	case 11: spcode = 14; break;
	case 12: spcode = 22; break;
	case 15: spcode = 64; break;
	case 16: spcode = 23; break;
	case 20: spcode = 18; break;
	case 23: spcode = 15; break;
	case 26: spcode = 16; break;
	case 30: spcode = 19; break;
	case 33: spcode = 20; break;
	case 36: spcode = 17; break;
	default: break;
	}
	return;
}

// 字符匹配
void characterMatch(char ch)
{
	switch (ch)
	{
	case ' ': case 9: case 10: case 13: break;
	case '+': {spcode = 2; status = 6; break; }
	case '-': {spcode = 3; status = 10; break; }
	case '*': {spcode = 4; status = 14; break; }
	case '/': {spcode = 5; status = 53; break; }
	case '!': {spcode = 6; status = 19; break; }
	case '%': {spcode = 7; status = 40; break; }
	case '~': {spcode = 8; status = 51; break; }
	case '&': {spcode = 9; status = 22; break; }
	case '|': {spcode = 10; status = 25; break; }
	case '^': {spcode = 11; status = 41; break; }
	case '=': {spcode = 12; status = 29; break; }
	case ',': {spcode = 25; status = 42; break; }
	case '(': {spcode = 26; status = 43; break; }
	case ')': {spcode = 27; status = 44; break; }
	case '{': {spcode = 28; status = 45; break; }
	case '}': {spcode = 29; status = 46; break; }
	case '[': {spcode = 30; status = 47; break; }
	case ']': {spcode = 31; status = 48; break; }
	case ';': {spcode = 32; status = 49; break; }
	case '<': {spcode = 33; status = 35; break; }
	case '>': {spcode = 34; status = 32; break; }
	case '#': {spcode = 35; status = 50; break; }
	default: {status = -1; break; }
	}
}

// 控制指示器
void controlPointer()
{
	string str;
	switch (status)
	{
	case -2:{
		str = "Invalid token: " + token + "\tLine: " + to_string(row_num + 1) + "\n";
		cout << str;
		fputs(str.c_str(), outfp);
		status = 0; pstart = pend;
		break;
	}
	case -1: {
		char ch = buf[pend++];
		string s1(1, ch);
		str = "Invalid character: " + s1 + "\tLine: " + to_string(row_num + 1) + "\n";
		cout << str;
		fputs(str.c_str(), outfp);
		status = 0; pstart = pend; 
		break; 
	}
	case 0: { pend++; pstart = pend; break; }
	case 1: case 3: case 6: case 10: case 14: case 19:
	case 22: case 25: case 29: case 32: case 35: case 53:
	case 54: case 55: case 57: { pend++; break; }
	case 2: { // 关键字or标识符
		if (isID()) {
			str = "<" + token + "," + to_string(spcode) + ">\n";
			cout << str;
			fputs(str.c_str(), outfp);
			pstart = pend; word_num++;
			status = 0;
		}
		else { status = -2; /*cout << "Invalid ID!" << endl;*/ }
		break;
	}
	case 4: { // 常数
		if (isNum()) {
			str = "<" + token + "," + to_string(spcode) + ">\n";
			cout << str;
			fputs(str.c_str(), outfp);
			pstart = pend; word_num++;
			status = 0;
		}
		else { status = -2; /*cout << "Invalid Number!" << endl;*/ }
		break;
	}
	case 40: case 41: case 42: case 43: case 44: case 45:
	case 46: case 47: case 48: case 49: case 50: case 51: {
		string s(1,buf[pend++]);
		str = "<" + s + "," + to_string(spcode) + ">\n";
		cout << str;
		fputs(str.c_str(), outfp);
		pstart = pend; word_num++;
		status = 0;
		break;
	}
	case 9: case 13: case 17: case 21: case 24: case 27: case 31: case 34: case 37: { // 单目
		isMono();
		string s(1, buf[pstart]);
		str = "<" + s + "," + to_string(spcode) + ">\n";
		cout << str;
		fputs(str.c_str(), outfp);
		pstart = pend; word_num++;
		status = 0;
		break;
	}
	case 7: case 8: case 11: case 12: case 15: case 16: 
	case 20: case 23: case 26: case 30: case 33: case 36: { // 二目
		isBino();
		string s1;
		if (pstart == buf_size - 1) { string s1(1, tmp_buf); }
		else { string s1(1, buf[pstart]); }
		string s2(1, buf[pend++]);
		str = "<" + s1 + s2 + "," + to_string(spcode) + ">\n";
		cout << str;
		fputs(str.c_str(), outfp);
		pstart = pend; status = 0; word_num++;
		break;
	}
	default:
		break;
	}
}

// 状态转换
void convertStatus(char ch)
{
	switch (status)
	{
	case 0: {
		if (ch == 9 || ch == 10 || ch == 13) { break; }
		else if (isDigit(ch)) { status = 3; }
		else if (isLetter(ch) || ch == '_') { status = 1; }
		else { characterMatch(ch); }
		break;
	}
	case 1: {
		if (!isLetter(ch) && !isDigit(ch) && ch != '_') { status = 2; }
		break;
	}
	case 3: {
		if (!isDigit(ch) && !isLetter(ch)) { status = 4; }
		break;
	}
	case 6: {
		if (ch == '+') { status = 7; }
		else if (ch == '=') { status = 8; }
		else { status = 9; }
		break;
	}
	case 10: {
		if (ch == '-') { status = 11; }
		else if (ch == '=') { status = 12; }
		else { status = 13; }
		break;
	}
	case 14: {
		if (ch == '*') { status = 15; }
		else if (ch == '=') { status = 16; }
		else { status = 17; }
		break;
	}
	case 19: {
		if (ch == '=') { status = 20; }
		else { status = 21; }
		break;
	}
	case 22: {
		if (ch == '&') { status = 23; }
		else { status = 24; }
		break;
	}
	case 25: {
		if (ch == '|') { status = 26; }
		else { status = 27; }
		break;
	}
	case 29: {
		if (ch == '=') { status = 30; }
		else { status = 31; }
		break;
	}
	case 32: {
		if (ch == '=') { status = 33; }
		else { status = 34; }
		break;
	}
	case 35: {
		if (ch == '=') { status = 36; }
		else { status = 37; }
		break;
	}
	// 跳过注释
	case 53: {
		if (ch == '/') { status = 54; }
		else if (ch == '*') { status = 55; }
		else { status = 59; }
		break;
	}
	case 54: {
		if (ch == 10) { status = 0; }
		break;
	}
	case 55: {
		if (ch == '*') { status = 57; }
		break;
	}
	case 57: {
		if (ch == '/') { status = 0; }
		else { status = 55; }
		break;
	}
	default:
		break;
	}
	controlPointer();
	if (ch == 10) { row_num++; }
	char_num++;
	return;
}

// 读取文件
bool readFile(string inFile)
{
	fp = fopen(inFile.c_str(), "rb+");
	if (fp == NULL)	return false;
	return true;
}

void closeFile()
{
	string str;
	str = "The total number of characters: " + to_string(char_num) + "\n";
	fputs(str.c_str(), outfp);
	str = "The total number of words: " + to_string(word_num) + "\n";
	fputs(str.c_str(), outfp);
	str = "The total number of rows: " + to_string(row_num + 1) + "\n";
	fputs(str.c_str(), outfp);
	fclose(fp);
	fclose(outfp);
	return;
}

void startAnalyzer()
{
	int num = 0;
	while (num >= 0) {
		if (buf_flag) { //使用右半缓冲区
			while (pend < buf_size && buf[pend] != '\0') { convertStatus(buf[pend]); }
			buf_flag = false;
		}
		else { // 使用左半缓冲区
			if (pstart == buf_size) { pstart = 0; }
			else if (pstart == buf_size - 1) { tmp_buf = buf[pstart]; }		// 如pstart指向buf最后一位，有多种情况，暂存至tmp_buf内
			else if (pstart < buf_size - 1) { for (int i = pstart; i < buf_size; i++)token.push_back(buf[i]); }	// 否则直接按ID/NUM处理，压入token
			memset(buf, 0, sizeof(buf)); // 清空缓冲区
			num = fread(buf, sizeof(char), buf_size, fp);
			if (num <= 0)return;
			pend = 0;
			while (pend < buf_size / 2 && buf[pend] >= 0 && buf[pend] != '\0') { convertStatus(buf[pend]); }
			buf_flag = true;
		}
	}
	return;
}

int main()
{
	string inFile, outFile;
	cout << "Please enter the name of input file:";
	cin >> inFile;
	cout << "Please enter the name of output file:";
	cin >> outFile;
	bool flag = readFile(inFile);
	while (!flag) {
		cout << "Unable to read the input file... Please enter the correct name of input file:";
		cin >> inFile;
		flag = readFile(inFile);
	}
	outfp = fopen(outFile.c_str(), "w");
	if (outfp == NULL) { cout << "Unable to create the output file." << endl; exit(-1); }
	startAnalyzer();
	closeFile();
	return 0;
}