#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

const int table_row = 5; // 预测分析表行数
const int table_col = 8; // 预测分析表列数
const string group_file_name = "group.txt";
const string grammar_file_name = "grammar.txt";

typedef pair<string, string> groupType; // <FIRST, FOLLOW>
typedef pair<string, string> tableType; // <非终结符, 终结符>

class LL1 {
public:
    void initGroup();
    void predictAnalysisTable();
    void printAnalysisTable();

    void initStack();
    void analysisControl(string input);

    bool isTerminal(string str);
    bool isNonterminal(string str);
    bool isDigit(char ch);

    LL1() = default;
    LL1(int row, int col, string gp_file, string gm_file) {
        t_row = row; t_col = col;
        group_file = gp_file; grammar_file = gm_file;
    }
    ~LL1() = default;

private:
    int t_row = table_row;
    int t_col = table_col;
    string group_file = group_file_name;
    string grammar_file = grammar_file_name;
    string in_stack; // 栈内字符串

    vector<string> vt;                         // 终结符
    vector<string> vn;                         // 非终结符，首位为起始符号
    unordered_map<string, groupType> group;    // <FIRST, FOLLOW> 集
    multimap<tableType, string> analysisTable; // 预测分析表
};

// 判断终结符
bool LL1::isTerminal(string str)
{
    auto it = find(vt.begin(), vt.end(), str);
    if (it != vt.end())return true;
    return false;
}

// 判断非终结符
bool LL1::isNonterminal(string str)
{
    auto it = find(vn.begin(), vn.end(), str);
    if (it != vn.end())return true;
    return false;
}

// 判断数字
bool LL1::isDigit(char ch)
{
    if (ch >= '0' && ch <= '9')return true;
    return false;
}

// 初始化FIRST、FOLLOW集
void LL1::initGroup()
{
    int count = 0;
    string s;
    ifstream ifs(group_file);

    while (getline(ifs, s, '\n')) {
        istringstream iss(s);
        string status;
        groupType gp;
        while (iss >> s) {
            if (count == 0) { status = s; }
            if (count == 1) { gp.first = s; }
            if (count == 2) {
                gp.second = s;
                group.insert({ status,gp });
                count = 0;
                continue;
            }
            count++;
        }
    }

    ifs.close();
}

// 构造预测分析表
void LL1::predictAnalysisTable()
{
    ifstream ifs(grammar_file);
    string s, pro, status, first, follow;
    tableType tb;

    //while (getline(ifs, s, ' ') && s != "\n") { vt.push_back(s); }
    //while (getline(ifs, s, ' ') && s != "\n") { vn.push_back(s); }
    for (int i = 0; i < 2; i++) {
        getline(ifs, s, '\n');
        istringstream iss(s);
        while (iss >> s) {
            if (i == 0) { vt.push_back(s); }
            else { vn.push_back(s); }
        }
    }
    
    for (int i = 0; i < (int)vn.size(); i++) {
        int count = 0;
        bool flag = false; // 标志产生式为ε或FIRST(α)中存在ε
        while (getline(ifs, s, '\n')) {
            count = 0;
            istringstream iss(s);
            while (iss >> s) {
                if (count++ == 0) { status = s; continue; }
                else {
                    pro = status + "->" + s;
                    string s_begin;
                    if (s == "num") { s_begin = "num"; }
                    else { s_begin.insert(0, 1, s.front()); }

                    auto it = group.find(s_begin);
                    if (it == group.end()) { 
                        if (isNonterminal(s_begin)) { cout << "Error: FIRST Group Not Found.\n"; }
                    }
                    else { first = it->second.first; }
                    if (first.find("~") != first.npos || s == "~") {
                        flag = true;
                        auto is = group.find(status);
                        if (is == group.end()) { cout << "Error: FOLLOW Group Not Found.\n"; }
                        else { follow = is->second.second; }
                    }

                    if (isNonterminal(s_begin)) {  // α以非终结符开头
                        string a;
                        while (!first.empty()) {
                            a.insert(0, 1, first.back());
                            first.pop_back();
                            if (isTerminal(a) || a == "~") {
                                tb.first = status;
                                if (a == "~") { tb.second = "$"; }
                                else { tb.second = a; }
                                analysisTable.insert({ tb, pro });
                                a.clear(); tb.first.clear(); tb.second.clear();
                            }
                        }
                    }

                    else { // α以终结符开头或 α == "~"
                        tb.first = status;
                        if (s_begin == "num") { tb.second = s_begin; }
                        else if (s_begin == "~") { tb.second = "$"; }
                        else { tb.second = *(s.begin()); }
                        analysisTable.insert({ tb, pro });
                        tb.first.clear(); tb.second.clear();
                    }

                    while (!follow.empty() && flag) {
                        string b;
                        b.push_back(follow.back());
                        reverse(b.begin(), b.end());
                        follow.pop_back();
                        if (isTerminal(b)) {
                            tb.first = status; tb.second = b;
                            analysisTable.insert({ tb, pro });
                            b.clear(); tb.first.clear(); tb.second.clear();
                        }
                    }
                    flag = false;
                }

                first.clear();
                follow.clear();
            }

            auto it = group.find(status);
            if (it != group.end()) { follow = it->second.second; }
            for (auto is = vt.begin(); is != vt.end(); is++) {
                tb.first = status; tb.second = *is;
                auto ir = analysisTable.find(tb);
                if (ir == analysisTable.end()) {
                    if (follow.find(*is) != string::npos) { analysisTable.insert({ tb, "synch" }); }
                    else { analysisTable.insert({ tb, "error" }); }
                    tb.first.clear(); tb.second.clear();
                }
            }
        }
    }

    ifs.close();
}

// 打印预测分析表
void LL1::printAnalysisTable()
{
    for (auto it = vt.begin(); it != vt.end(); it++) { cout << "\t" << *it; }
    for (auto it = vn.begin(); it != vn.end(); it++) {
        cout << "\r\n" << *it;
        for (auto is = vt.begin(); is != vt.end(); is++) {
            auto pro = analysisTable.find({ *it, *is });
            cout << "\t" << pro->second;
        }
    }
}

// 栈初始化
void LL1::initStack()
{
    in_stack = "$" + vn.front();
}

// 预测分析控制程序
void LL1::analysisControl(string input)
{
    int ip = 0;
    string num;
    string back(1, in_stack.back()); // 栈顶
    cout << setw(50) << "Stack"
        << setw(50) << "Input"
        << setw(30) << "Output\n"
        << setw(50) << in_stack
        << setw(50) << input << "\n";
    while (in_stack != "$") {
        if (isTerminal(back) || back == "$") {
            if (back == "num") {
                if (!isDigit(input[ip])) { cout << "Error!\n"; input.erase(0, 1); continue; }
                string n(1, input[ip]); num.append(n);
                //num.insert(num.length() - 1, 1, input[ip]);
                if (!isDigit(input[ip + 1])) {
                    in_stack.erase(in_stack.length() - 3, 3); 
                    input.erase(0, num.length());
                    cout << setw(50) << in_stack << setw(50) << input << "\n";
                    num.clear();
                    ip = 0;
                }
                else { ip++; }
            }
            else if (in_stack.back() == input[ip]) { 
                input.erase(0, 1);
                in_stack.pop_back();
                cout << setw(50) << in_stack << setw(50) << input << "\n";
            }
            else { cout << setw(100) << "Error!\n"; }
        }
        else {
            string s(1, input[ip]);
            if (input.empty()) { s = "$"; }
            if (isDigit(input[ip])) { s = "num"; }
            auto it = analysisTable.find({ back,s });
            if (it != analysisTable.end()) {
                if (it->second == "synch") { in_stack.pop_back(); /*input.erase(0, 1);*/ }
                else if (it->second != "error") {
                    in_stack.pop_back();
                    string pro = it->second.substr(3);
                    while (!pro.empty()) {
                        if (pro != "~") { in_stack.push_back(pro.back()); }
                        pro.pop_back();
                    }
                }
                else { cout << setw(100) << "Error!\n"; input.erase(0, 1); continue; }
                cout << setw(50) << in_stack << setw(50) << input << setw(30) << it->second << "\n";
            }
            else { cout << setw(100) << "Error!\n"; }
        }
        /* Update back */
        back.clear();
        if (in_stack.find_last_of('n') == in_stack.length() - 1) { back.append("num"); }
        else { back.push_back(in_stack.back()); }
    }
    if (input == "$") { cout << "\n" << setw(100) << "Match!\n"; }
    else{ cout << "\n" << setw(100) << "Failed.\n"; }
}

int main()
{
    LL1* predict = new LL1();
    string input;

    predict->initGroup();
    predict->predictAnalysisTable();
    predict->printAnalysisTable();

    cout << "\nPlease enter the string waiting for analysis:";
    cin >> input;
    input.push_back('$');

    predict->initStack();
    predict->analysisControl(input);

    delete predict;
    predict = NULL;

    system("pause");
    return 0;
}