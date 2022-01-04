#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>
#include <string>

using namespace std;

const int state_num = 16;
const int action_col = 8;
const int goto_col = 3;
const string GRAM_FILE = "grammar.txt";
const string TABLE_FILE = "table_errorhandle.txt";

typedef pair<int, string> tableType; // <状态, 终结符/非终结符>
typedef pair<string, string> proType; // A -> B

class LALR
{
public:
	void init();
	void readGrammar();
	void readTable();

	string searchAction(int state, char nt);
	int searchGoto(int state, char t);
	proType searchPro(int n);
	void analyze(string input);

	bool isTerminal(string str);
	bool isNonterminal(string str);
	bool isDigit(char ch);

private:
	string grammar_filename = GRAM_FILE;
	string table_filename = TABLE_FILE;

	vector<int> state_stack; // 状态栈
	vector<char> symbol_stack; //符号栈
	vector<string> vt; // 终结符
	vector<string> vn; // 非终结符
	vector<proType> pro; // 产生式
	map<tableType, string> actionTable; // action子表
	map<tableType, int> gotoTable; // goto子表

};

// 判断终结符
bool LALR::isTerminal(string str)
{
	auto it = find(vt.begin(), vt.end(), str);
	if (it != vt.end())return true;
	return false;
}

// 判断非终结符
bool LALR::isNonterminal(string str)
{
	auto it = find(vn.begin(), vn.end(), str);
	if (it != vn.end())return true;
	return false;
}

// 判断数字
bool LALR::isDigit(char ch)
{
	if (ch >= '0' && ch <= '9')return true;
	return false;
}

// 读取拓展文法
void LALR::readGrammar()
{
	string s;
	ifstream ifs(grammar_filename);

	for (int i = 0; i < 2; i++) {
		getline(ifs, s, '\n');
		istringstream iss(s);
		while (iss >> s) {
			if (i == 0) { vt.push_back(s); }
			else { vn.push_back(s); }
		}
	}
	//int cnt = 0;
	proType p;
	while (getline(ifs, s, '\n')) {
		istringstream iss(s);
		bool cntflag = false;
		while (iss >> s) {
			if (!cntflag) {
				//pro[cnt].first = s;
				p.first = s;
				cntflag = true;
			}
			else {
				//pro[cnt].second = s;
				p.second = s;
				pro.push_back(p);
				//cnt++;
			}
		}
	}

	ifs.close();
	return;
}

// 读取LR分析表
void LALR::readTable()
{
	string s;
	ifstream ifs(table_filename);
	tableType tb;
	
	for (int i = 0; i < state_num; i++) {
		tb.first = i;
		getline(ifs, s, '\n');
		istringstream iss(s);
		int cnt = 0;
		while (iss >> s) {
			if (cnt < action_col) { // action子表
				tb.second = vt[cnt];
				if (s == "0")
					actionTable.insert({ tb,"error" });
				else
					actionTable.insert({ tb,s });
				cnt++;
			}
			else if (cnt >= action_col && cnt < action_col + goto_col) { // goto子表
				tb.second = vn[cnt - action_col];
				if (s == "0")
					gotoTable.insert({ tb,0 });
				else
					gotoTable.insert({ tb,atoi(s.c_str()) });
				cnt++;
			}
			else cnt = 0;
			tb.second.clear();
		}
		//tb.first.clear();
	}

	ifs.close();
	return;
}

// 初始化，读入文件
void LALR::init()
{
	readGrammar();
	readTable();
}

// 查找action子表
string LALR::searchAction(int state, char nt)
{
	string nter(1, nt);
	auto act = actionTable.find({ state,nter });
	if (act != actionTable.end())
		return act->second;
	else
		return "error";
}

// 查找goto子表
int LALR::searchGoto(int state, char t)
{
	string ter(1, t);
	auto act = gotoTable.find({ state,ter });
	if (act != gotoTable.end())
		return act->second;
	else
		return 0;
}

// 查找归约用的产生式
proType LALR::searchPro(int n)
{
	return pro[n];
}

// LR分析过程
void LALR::analyze(string input)
{
	state_stack.clear(); symbol_stack.clear();
	state_stack.push_back(0);
	symbol_stack.push_back('$');
	char ip; // 展望符
	int state = 0, cnt = 0;
	string in = input;
	string s, act, process;
	string st_stack, sy_stack;
	proType prod;
	cout << setw(10) << "步骤" << setw(40) << "状态栈" << setw(40)
		<< "符号栈" << setw(30) << "输入" << setw(20) << "步骤\n";
	while (1) {
		ip = in[0];
		process.clear();
		if (isDigit(ip))ip = 'n';
		while (ip == 'n' && isDigit(in[1]))
			in.erase(0, 1);
		act = searchAction(state, ip);
		if (act == "error") {
			cout << "Error!\n";
			return;
		}
		else if (act == "ACC") {
			cout << "Accept!\n";
			return;
		}
		else if (act[0] == 'S') { // 移进
			// 压入状态栈
			act.erase(0, 1);
			process = "Shift " + act;
			state = atoi(act.c_str());
			state_stack.push_back(state);
			// 压入符号栈
			symbol_stack.push_back(ip);
			in.erase(0, 1);
		}
		else if (act[0] == 'R') { // 归约
			act.erase(0, 1);
			prod = searchPro(atoi(act.c_str()));
			process = "Reduced by " + prod.first + "->" + prod.second;
			for (int i = 0; i < (int)prod.second.length(); i++) {
				state_stack.pop_back();
				symbol_stack.pop_back();
			}
			symbol_stack.push_back(prod.first.c_str()[0]);
			state = searchGoto(state_stack.back(), symbol_stack.back());
			if (state != 0)
				state_stack.push_back(state);
			else {
				cout << "Error!\n";
				return;
			}
		}
		else if (act[0] == 'E') { // 错误处理子程序
			act.erase(0, 1);
			switch (act[0])
			{
			case '1':
				state_stack.push_back(5);
				symbol_stack.push_back('n');
				state = 5;
				cout << "缺少运算分量!\n";
				break;
			case '2':
				in.erase(0, 1); // 输入中删除右括号
				cout << "不匹配的右括号!\n";
				break;
			case '3':
				state_stack.push_back(6);
				symbol_stack.push_back('+');
				state = 6;
				cout << "缺少运算符!\n";
				break;
			case '4':
				state_stack.push_back(15);
				symbol_stack.push_back(')');
				state = 15;
				cout << "缺少右括号!\n";
				break;
			case '5': // 缺少运算符，并且需要先进行归约
				in.insert(0, "+"); // 输入首位加 + 号
				cout << "缺少运算符!\n";
				break;
			default:
				break;
			}
		}
		else {
			cout << "Error!\n";
			return;
		}
		s.clear();

		for (auto i : state_stack) {
			char buf[4] = { 0 };
			int ret = sprintf_s(buf, 4, "%d", i);
			st_stack.append(buf);
		}
		for (auto i : symbol_stack) {
			string str(1, i);
			sy_stack.append(str);
		}
		++cnt;
		cout << setw(10) << cnt << setw(40) << st_stack << setw(40)
			<< sy_stack << setw(30) << in << setw(20) << process << endl;
		st_stack.clear(); sy_stack.clear();
	}
}

int main()
{
	LALR* ana = new LALR();
	string input;

	ana->init();
	cout << "Please enter the string waiting for analysis:";
	cin >> input;
	input.push_back('$');
	ana->analyze(input);

	delete ana;
	ana = NULL;

	system("pause");
	return 0;
}