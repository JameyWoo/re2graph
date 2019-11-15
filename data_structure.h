/**
 * @author: 姬小野
 * * 声明了一系列的数据结构和库
 */

#ifndef __DATA_STRUCTURE_H_
#define __DATA_STRUCTURE_H_

// 加上万能头文件, 防止其他电脑环境不合适
#include<bits/stdc++.h> 
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <ctime>
#include <set>
#include <stack>
#include <vector>
#include <climits>
#include "unistd.h"

using namespace std;

#define OTHER '^'
#define ERROR -1

// NFA结构体
struct NFA {                              // 用一个结构体保存dfa信息
    int char_cnt;                         // 输入字符的类别数
    int state_cnt;                        // 状态的数量
    int start_state;                      // 开始状态
    int end_state_cnt;                    // 终止状态的个数
    vector<char> chars;                   // 可输入各种的字符
    vector<int> states;                   // 状态的编号
    vector<int> end_states;               // 终止状态列表
    vector<map<char, vector<int>>> jump;  // 跳转hash表, 每个state一个hash, 保存这个字符可能到的所有下一状态. 稍微有点复杂
};

// DFA结构体
struct DFA {                          // 用一个结构体保存dfa信息
    int char_cnt;                     // 输入字符的类别数
    int state_cnt;                    // 状态的数量
    int start_state;                  // 开始状态
    int end_state_cnt;                // 终止状态的个数
    vector<char> chars;               // 可输入各种的字符
    vector<int> states;               // 状态的编号
    vector<int> end_states;           // 终止状态列表
    vector<map<char, int>> jump_map;  // 跳转hash表, 每个state一个hash
};

struct Edge {
    int from, to;  // 起点, 终点
    char cond;     // 条件
    Edge() {}
    Edge(int f, char c, int t) : from(f), cond(c), to(t) {}  // 构造函数
};

struct Graph {  // nfa的一个子图, 包含起始状态, 最终状态, 还有一系列边集合
    int start, end;
    vector<Edge> edges;
};

#endif  // !__DATA_STRUCTURE_H_