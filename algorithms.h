/**
 * * 实现了一系列算法, 和数据结构的定义分开
 */

#ifndef __ALGORITHMS_H_
#define __ALGORITHMS_H_

#include "data_structure.h"

// * 关键算法
NFA re2nfa(string rexp);            // 将正则表达式转化为nfa的函数
void nfa2dfa(NFA, DFA&);            // nfa转化为dfa的关键函数
DFA dfa2min(DFA);                   // 将一个DFA最小化
bool dfaPattern(DFA, string);       // 模式匹配dfa, 返回匹配的列表
void nfa2png(NFA, string, string);  // nfa转化为状态图, 可以给出状态图的布局
void dfa2png(DFA, string, string);  // dfa转化为状态图, 可以给出状态图的布局

// * 文件操作
NFA read_from_nfa(string);       // 实现了从nfa文件中读取nfa数据
void writeNfaData(NFA, string);  // 将NFA写入到nfa文件中
DFA readDfaData(string);         // 从.dfa文件中读取dfa信息
void writeDfaData(DFA, string);  // 将转化成的dfa写入到dfa文件中

// * 内部调用
string parse_range(string);                                    // 范围解析的函数, 可以解析出如A-Z, a-zA-Z0-9_- 这一类范围定义的字符
void get_closure(NFA, vector<set<int>>&, int, vector<bool>&);  // 分别是nfa, 全部的closure, 当前状态, 是否访问过
Graph parse_re(string, int&);                                  // 解析re_exp的函数, 递归调用, 传递当前已分配状态进去(当前已分配)
void nfa2dot(NFA, string);                                     // nfa数据结构转化为置顶的dot文件格式
void dfa2dot(DFA, string);                                     // dfa转化为dot格式
void help();                                                   // 打印 帮助信息

#endif  // !__ALGORITHMS_H_