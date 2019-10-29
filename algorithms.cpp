/**
 * @author: 姬小野
 */

#include "algorithms.h"
#include "data_structure.h"

// * 关键算法

NFA re2nfa(string rexp) {  // 将正则表达式转化为nfa的函数
    // 对rexp进行一些预处理, 处理[] 的情况
    string prexp, middle;
    bool is_mid = false;
    for (auto c : rexp) {
        if (c == '[') {
            is_mid = true;
            prexp.push_back('(');
        } else if (c == ']') {
            string mid = parse_range(middle);
            for (auto m : mid) {
                prexp += m;
                prexp += '|';
            }
            prexp.pop_back();
            prexp.push_back(')');
            middle = "";
            is_mid = false;
        } else {
            if (is_mid)
                middle += c;
            else
                prexp += c;
        }
    }
    // cout << "prexp: " << prexp << endl;
    rexp = prexp;

    int state       = 0;
    Graph nfa_graph = parse_re(rexp, state);
    // cout << "start: " << nfa_graph.start << endl;
    // cout << "end: " << nfa_graph.end << endl;
    // for (auto edge : nfa_graph.edges) {
    //     cout << edge.from << ' ' << edge.cond << ' ' << edge.to << endl;
    // }

    // 获取除规则字符之外的字符
    set<char> char_set;
    char_set.insert('~');
    for (auto c : rexp) {
        if (not(c == '|' || c == '(' || c == ')' || c == '*')) {
            char_set.insert(c);
        }
    }

    // 将数据存入到nfa中
    NFA nfa;
    nfa.char_cnt      = char_set.size();
    nfa.state_cnt     = state;
    nfa.start_state   = 0;
    nfa.end_state_cnt = 1;
    nfa.end_states.push_back(state - 1);
    for (auto s : char_set) {
        nfa.chars.push_back(s);
    }
    for (int i = 0; i < state; ++i) {
        nfa.states.push_back(i);
    }
    vector<map<char, vector<int>>> jump(state);
    for (auto edge : nfa_graph.edges) {
        jump[edge.from][edge.cond].push_back(edge.to);
    }
    nfa.jump = jump;
    return nfa;
}

void nfa2dfa(NFA nfa, DFA &dfa) {
    /**
     * 思路: 
     * 先统一求出所有状态的闭包closure, 因为如果在线求解会导致总是重复计算. 
     * 可以以O(n)的代价求出所有状态的闭包.
     * 然后再进行复杂的子集构造算法
     */
    // ! 所有状态的闭包, 一次性算出, 避免重复计算
    vector<set<int>> closures = vector<set<int>>(nfa.state_cnt, set<int>());
    vector<bool> ext_start(nfa.state_cnt);
    for (int i = 0; i < nfa.state_cnt; ++i) {
        if (closures[i].size() == 0) {
            ext_start[i] = true;
            vector<bool> visted(nfa.state_cnt);
            visted[i] = true;
            get_closure(nfa, closures, i, visted);
            visted[i] = false;
        }
    }
    // * 如果闭包成环, 会出现一种闭包没有求完整的情况. 已用反向边遍历方法解决
    // 但有的状态的闭包一定是完整的. 就是环的开始部分, 所以还需要进行调整. 进行闭包的扩展, 进行边的反向扩展
    map<int, vector<int>> ext_closure_map;
    for (int i = 0; i < nfa.state_cnt; ++i) {
        for (auto m : nfa.jump[i]) {
            if (m.first == '~') {
                for (auto v : m.second) {
                    ext_closure_map[v].push_back(i);
                }
            }
        }
    }
    // cout << "ext_closure_map:" << endl;
    // for (auto m: ext_closure_map) {
    //     for (auto v: m.second) {
    //         cout << m.first << ' ' << v << endl;
    //     }
    // }
    for (int i = 0; i < nfa.state_cnt; ++i) {
        if (ext_start[i]) {
            queue<int> inq;
            vector<int> visted(nfa.state_cnt);
            inq.push(i);
            visted[i] = true;
            while (not inq.empty()) {
                int now = inq.front();
                inq.pop();
                for (auto v : ext_closure_map[now]) {
                    if (visted[v] == false) {
                        closures[v].insert(closures[i].begin(), closures[i].end());
                        inq.push(v);
                        visted[v] = true;
                    }
                }
            }
        }
    }

    // 查看闭包
    // cout << "closures: " << endl;
    // for (int i = 0; i < nfa.state_cnt; ++i) {
    //     for (auto clo : closures[i]) {
    //         cout << clo << ' ';
    //     }
    //     cout << endl;
    // }

    // 进行接受状态的扩展, 因为后面可能会漏掉
    set<int> ext_end_states;
    for (auto x : nfa.end_states) {
        ext_end_states.insert(x);  // 先插入最基本的接受状态
    }
    for (auto closure : closures) {
        for (auto x : closure) {
            if (ext_end_states.count(x)) {
                ext_end_states.insert(closure.begin(), closure.end());
                break;
            }
        }
    }
    // cout << "ext_end_states: ";
    // for (auto s : ext_end_states) {
    //     cout << s << ' ';
    // }
    // cout << endl;

    // ! 进行子集构造算法
    set<int> end_states;                                      // dfa的终止状态
    if (ext_end_states.count(0)) end_states.insert(0);        // 因为先单独将起始状态0加入了工作列表, 所以对他的接收状态需要提前处理
    map<set<int>, int> set_hash;                              // 集合对整数的hash, 也就是对子集编码
    vector<map<char, int>> jump_dfa;                          // dfa的jump
    set<int> set_visted;                                      // 保存一个set是否被访问过的信息
    int set_state                       = 0;                  // 集合的整数状态
    set_hash[closures[nfa.start_state]] = set_state++;        // 初始状态编码为0
    jump_dfa.push_back(map<char, int>());                     // 压入初始状态的map
    set_visted.insert(set_hash[closures[nfa.start_state]]);   // 标记访问过
                                                              // 初始化变量之后开始工作
    queue<set<int>> work_queue;                               // 队列
    work_queue.push(closures[nfa.start_state]);               // 将初始状态闭包集插入工作队列
    while (not work_queue.empty()) {                          // 当工作队列不为空
        set<int> now = work_queue.front();                    // 取出当前工作集合
        work_queue.pop();                                     // 弹出
        for (auto c : nfa.chars) {                            // 遍历每个字符
            if (c == '~') continue;                           // c 不能是~ 也就是ε, 它不可以接下一状态
            set<int> next_state_set;                          // 下一状态
            for (auto state : now) {                          // 遍历每个状态. 要获取当前集合所有状态对某字符的下一状态的闭包
                if (nfa.jump[state].count(c) != 0) {          // 当可以跳转的时候
                    for (auto v : nfa.jump[state][c]) {       // 一个状态对同一个字符可能有好几个下一状态
                        set<int> next_closure = closures[v];  // 求闭包
                        // 要获取所有可以走下去的状态的闭包, 都插入到集合中
                        next_state_set.insert(next_closure.begin(), next_closure.end());
                    }
                }
            }
            if (next_state_set.size() != 0) {                // 如果不是空, 说明是一个心得dfa状态
                if (set_hash.count(next_state_set) == 0) {   // 遇到新状态, 计次, 转化为dfa中的状态
                    set_hash[next_state_set] = set_state++;  // 新状态分配int型状态号
                    for (auto s : next_state_set) {          // 查找当前集合是否存在nfa的接受状态, 如果存在, 那它也是dfa的终止状态
                        if (ext_end_states.count(s) != 0) {
                            end_states.insert(set_state - 1);  // 是终止状态
                            break;
                        }
                    }
                    jump_dfa.push_back(map<char, int>());  // 新状态的map, 用来存储dfa的转移函数
                }
                int set_hash_next_state    = set_hash[next_state_set];  // 一次得到hash值保存, 方便后面操作, 省的每次都查找set
                jump_dfa[set_hash[now]][c] = set_hash_next_state;       // 经过hash之后的dfa状态表示
                if (set_visted.count(set_hash_next_state) == 0) {       // 如果之前没访问过
                    set_visted.insert(set_hash_next_state);             // 插入set_visted中标记访问
                    work_queue.push(next_state_set);                    // 插入work_queu中, 由于set_visted的存在, 只会进一次work_queue
                }
            }
        }
    }

    // 构造dfa, 按照DFA数据定义的顺序
    dfa.char_cnt      = nfa.char_cnt;
    dfa.state_cnt     = set_state;
    dfa.start_state   = 0;
    dfa.end_state_cnt = end_states.size();
    dfa.chars         = nfa.chars;
    for (int i = 0; i < dfa.state_cnt; ++i) {
        dfa.states.push_back(i);
    }
    for (auto s : end_states) {
        dfa.end_states.push_back(s);
    }
    dfa.jump_map = jump_dfa;
}

DFA dfa2min(DFA dfa) {
    // 先分出 终止状态集 和 非终止状态集. 并添加到state2set中
    set<int> no_end, end;
    for (auto s : dfa.states) {
        if (count(dfa.end_states.begin(), dfa.end_states.end(), s)) {
            end.insert(s);
        } else {
            no_end.insert(s);
        }
    }
    vector<set<int>> state2set(dfa.state_cnt);  // 每个状态对应的集合
    for (auto s : no_end) {
        state2set[s] = no_end;
    }
    for (auto s : end) {
        state2set[s] = end;
    }

    // 集合的记录, 以set<int>为key, 方便查找
    map<set<int>, int> set_cnt;
    for (auto s : state2set) {  // 各个集合
        set_cnt[s] = 1;
    }
    int last = set_cnt.size();  // 初始时集合的数量

    while (true) {
        // ! 开始拆分集合
        bool flag = false;  //  标记是否可以划分集合, 如果可以, 则处理完当前划分后马上退出.
        for (auto one : set_cnt) {
            set<int> the_set = one.first;               // 当前准备拆分的集合
            for (auto c : dfa.chars) {                  // 遍历每个字符, 查找可以拆分的情况
                map<set<int>, vector<int>> next2state;  // 下一状态所属集合对当前状态的映射, 可根据这个映射划分集合
                set<int> empty_states;
                bool empty = false;
                for (auto state : the_set) {
                    if (dfa.jump_map[state].count(c) == 0) {
                        // ! 解决当一个状态输入c无法跳转的情况
                        empty = true;
                        empty_states.insert(state);
                    } else {
                        int next = dfa.jump_map[state][c];
                        next2state[state2set[next]].push_back(state);
                    }
                }
                if (next2state.size() > 1 || (empty && next2state.size() == 1)) {  // 考虑某条件下无法扩展的状态
                    for (auto next_states : next2state) {
                        set<int> new_set;  // 新集合
                        for (auto new_set_state : next_states.second) {
                            new_set.insert(new_set_state);
                        }
                        for (auto new_set_state : next_states.second) {
                            state2set[new_set_state] = new_set;
                        }
                    }
                    if (empty) {
                        for (auto s : empty_states) {
                            state2set[s] = empty_states;
                        }
                    }
                    flag = true;
                    break;
                }
                if (flag) break;
            }
            if (flag) break;
        }

        // ! 统计集合的数量, 和上一次进行比较
        set_cnt.clear();            // 清空, 重新统计.
        for (auto s : state2set) {  // 统计split之后集合个数
            set_cnt[s] = 1;
        }
        if (last == set_cnt.size()) break;  // 集合没有变化了, 说明不可再划分
        last = set_cnt.size();
    }

    // ! 合并旧状态为新状态
    DFA min_dfa;
    int new_state_id = 0;
    map<set<int>, int> set2id;
    for (auto one_set : set_cnt) {
        set2id[one_set.first] = new_state_id++;
    }
    vector<map<char, int>> new_jump(new_state_id);
    for (int from = 0; from < dfa.state_cnt; ++from) {
        for (auto m : dfa.jump_map[from]) {
            new_jump[set2id[state2set[from]]][m.first] = set2id[state2set[m.second]];
        }
    }
    min_dfa.jump_map = new_jump;

    min_dfa.char_cnt    = dfa.char_cnt;
    min_dfa.state_cnt   = new_state_id;
    min_dfa.start_state = 0;
    // 添加终止状态
    set<int> end_states_set;
    for (auto one_end_state : dfa.end_states) {
        end_states_set.insert(set2id[state2set[one_end_state]]);
    }
    for (auto s : end_states_set) {
        min_dfa.end_states.push_back(s);
    }
    min_dfa.end_state_cnt = min_dfa.end_states.size();
    min_dfa.chars         = dfa.chars;
    for (int i = 0; i < new_state_id; ++i) {
        min_dfa.states.push_back(i);
    }

    // 成功最小化, 查看各个状态所属的集合
    // for (int i = 0; i < dfa.state_cnt; ++i) {
    //     cout << i << ": ";
    //     for (auto s : state2set[i]) {
    //         cout << s << ' ';
    //     }
    //     cout << endl;
    // }
    return min_dfa;
}

bool dfaPattern(DFA dfa, string pattern) {
    /**
     * 模式匹配函数, 以bool形式返回匹配的结果
     * 直接遍历pattern然后跳转状态, 同时保存所有到达接受状态的模式串就好了
     */
    int state = 0;
    for (auto c : pattern) {  // 遍历待匹配的字符
        char real = c;
        if (count(dfa.chars.begin(), dfa.chars.end(), c) == 0) {  // ! 如果字符在chars没找到, 那么当作OTHER来处理
            c = OTHER;
            if (count(dfa.chars.begin(), dfa.chars.end(), c) == 0) {  // ! 如果dfa本身没有处理OTHER的功能, 那么退出并返回之前的匹配
                cout << "error char: " << real << " !" << endl;
                return false;
            }
        }

        if (dfa.jump_map[state].count(c)) {  // 查看输入字符是否支持当前状态
            state = dfa.jump_map[state][c];
        } else {
            return false;
        }
        if (state == ERROR) return false;  // 超过的最长长度, 可以舍去了
    }
    if (count(dfa.end_states.begin(), dfa.end_states.end(), state)) {  // 查询是否到达接受状态
        return true;
    }
    return false;
}

void nfa2png(NFA nfa, string png_file, string pattern) {
    string raw_png_file = png_file;
    string dot_file     = "./tmp/" + png_file.replace(png_file.find(".png"), 4, ".dot");
    nfa2dot(nfa, dot_file);
    string his_pat = "circo";
    string cmd     = his_pat + " -Tpng " + dot_file + " -o ./image/{test.png}";
    if (pattern.size()) cmd.replace(cmd.find(his_pat), his_pat.size(), pattern);
    cmd.replace(cmd.find("{test.png}"), 10, raw_png_file);
    // cout << "nfa cmd: " << cmd << endl;
    system(cmd.c_str());
}

void dfa2png(DFA dfa, string png_file, string pattern) {
    string raw_png_file = png_file;
    string dot_file     = "./tmp/" + png_file.replace(png_file.find(".png"), 4, ".dot");
    dfa2dot(dfa, dot_file);
    string his_pat = "circo";
    string cmd     = his_pat + " -Tpng " + dot_file + " -o ./image/{test.png}";
    if (pattern.size()) cmd.replace(cmd.find(his_pat), his_pat.size(), pattern);
    cmd.replace(cmd.find("{test.png}"), 10, raw_png_file);
    system(cmd.c_str());
}

// * 工具函数

NFA read_from_nfa(string file) {
    NFA nfa;
    int num;
    string str;
    ifstream readfile;
    readfile.open(file.c_str(), ios::in);

    // 读入nfa中的字符
    readfile >> num;
    for (int i = 0; i < num; ++i) {
        readfile >> str;
        str = parse_range(str);  // 进行范围解析, 返回解析出的字符组成的字符串
        for (auto s : str) {
            nfa.chars.push_back(s);
        }
    }
    nfa.char_cnt = nfa.chars.size();  // 这才是真的字符数量

    // 读入状态, 都是数字, 如果不是0开始, 比如1开始, 则正规化为0开始
    readfile >> nfa.state_cnt;
    int min_state = INT_MAX;
    for (int i = 0; i < nfa.state_cnt; ++i) {
        readfile >> num;
        min_state = min(min_state, num);
        nfa.states.push_back(num);
    }
    for (auto &s : nfa.states) {  // ! 状态正规化, 全都变为0开始
        s -= min_state;           // 减去最小
    }

    // 读入起始状态和接受(终止)状态
    readfile >> nfa.start_state >> nfa.end_state_cnt;
    for (int i = 0; i < nfa.end_state_cnt; ++i) {
        readfile >> num;
        nfa.end_states.push_back(num);
    }
    for (auto &s : nfa.end_states) {  // 这里也要同步正规化状态
        s -= min_state;
    }

    // 读入状态转化函数(边)
    readfile >> num;
    vector<map<char, vector<int>>> tmp = vector<map<char, vector<int>>>(nfa.state_cnt, map<char, vector<int>>());
    for (int i = 0; i < num; ++i) {
        int x, y;
        readfile >> x >> str >> y;
        x -= min_state;
        y -= min_state;
        str = parse_range(str);
        for (auto c : str) {
            if (tmp[x].count(c) == 0) {
                vector<int> tos;
                tos.push_back(y);
                tmp[x][c] = tos;
            } else {
                tmp[x][c].push_back(y);
            }
        }
    }
    nfa.jump = tmp;
    // cout << "NFA load over! " << endl;
    return nfa;
}

void writeNfaData(NFA nfa, string filename) {
    /**
     * 将dfa数据写入到filename文件中.
     * 按照输入的格式, 相当于将输入反了一遍. 
     */
    ofstream write;
    write.open(filename, ios::out | ios::trunc);  // ? 是否设置成没有则创建
    write << nfa.chars.size() << endl;
    for (auto c : nfa.chars) {
        write << c << ' ';
    }
    write << endl
          << nfa.state_cnt << endl;
    for (auto c : nfa.states) {
        write << c << ' ';
    }
    write << endl
          << nfa.start_state << endl
          << nfa.end_state_cnt << endl;
    for (auto c : nfa.end_states) {
        write << c << ' ';
    }
    write << endl;
    // 转换条件的写入
    vector<string> jumps;
    for (auto s : nfa.states) {
        for (auto m : nfa.jump[s]) {
            for (auto v : m.second) {
                jumps.push_back(to_string(s) + " " + m.first + " " + to_string(v) + "\n");
            }
        }
    }
    write << jumps.size() << endl;
    for (auto s : jumps) {
        write << s;
    }
    // cout << "NFA write over!" << endl;
}

DFA readDfaData(string input_file) {
    /*
    各行的含义注解:
    3           // 字符集中的字符个数 (以下两行也可合并成一行)
    / * ^       // 以空格分隔的字符集。^代表任意非/和*的字符
    5           // 状态个数 (以下两行也可合并成一行)
    1 2 3 4 5   // 状态编号。若约定总是用从1开始的连续数字表示，则此行可省略
    1           // 开始状态的编号。若约定为1，则此行可省略
    1           // 结束状态个数。若约定为1，则此行可省略
    5           // 结束状态的编号
    7           // 转移函数的个数
    1 / 2       // 转移函数
    2 * 3
    3 * 4
    3 ^ 3
    4 ^ 3
    4 * 4
    4 / 5
    */
    DFA dfa;
    ifstream readfile;
    readfile.open(input_file.c_str(), ios::in);
    readfile >> dfa.char_cnt;
    string str;
    int num;
    for (int i = 0; i < dfa.char_cnt; ++i) {
        readfile >> str;
        str = parse_range(str);  // 进行范围解析, 返回解析出的字符组成的字符串
        for (auto s : str) {
            dfa.chars.push_back(s);
        }
    }
    readfile >> dfa.state_cnt;
    int min_state = INT_MAX;
    for (int i = 0; i < dfa.state_cnt; ++i) {
        readfile >> num;
        min_state = min(min_state, num);
        dfa.states.push_back(num);
    }
    for (auto &s : dfa.states) {
        s -= min_state;
    }
    readfile >> dfa.start_state >> dfa.end_state_cnt;
    for (int i = 0; i < dfa.end_state_cnt; ++i) {
        readfile >> num;
        dfa.end_states.push_back(num);
    }
    for (auto &s : dfa.end_states) {
        s -= min_state;
    }
    readfile >> num;
    for (int i = 0; i < dfa.state_cnt; ++i) {  // 先把空hash添加到vector
        map<char, int> tmp;
        dfa.jump_map.push_back(tmp);
    }
    for (int i = 0; i < num; ++i) {
        int x, y;
        readfile >> x >> str >> y;
        x -= min_state;
        y -= min_state;
        str = parse_range(str);
        for (auto c : str) {
            // if (dfa.jump_map[x].count(c)) {
            //     correct       = false;
            //     wrong_message = "一个状态输入同一个字符有多个下一状态";
            // }
            dfa.jump_map[x][c] = y;
        }
    }
    // cout << "DFA read over! " << endl;
    return dfa;
}

void writeDfaData(DFA dfa, string filename) {
    /**
     * 将dfa数据写入到filename文件中.
     * 按照输入的格式, 相当于将输入反了一遍.
     * 和输入有一些差异的是, 由于我在保存dfa的时候, 将起始状态变成了0, 所以这个输出更正规
     */
    ofstream write;
    write.open(filename, ios::out | ios::trunc);  // ? 是否设置成没有则创建
    write << dfa.chars.size() << endl;
    for (auto c : dfa.chars) {
        write << c << ' ';
    }
    write << endl
          << dfa.state_cnt << endl;
    for (auto c : dfa.states) {
        write << c << ' ';
    }
    write << endl
          << dfa.start_state << '\n'
          << dfa.end_state_cnt << endl;
    for (auto c : dfa.end_states) {
        write << c << ' ';
    }
    write << endl;
    // todo: 添加转换条件的写入
    vector<string> jumps;
    for (auto s : dfa.states) {
        for (auto iter : dfa.jump_map[s]) {
            jumps.push_back(to_string(s) + " " + iter.first + " " + to_string(iter.second) + "\n");
        }
    }
    write << jumps.size() << endl;
    for (auto s : jumps) {
        write << s;
    }
    // cout << "DFA write over!" << endl;
}

// * 内部调用

string parse_range(string str) {
    /**
     * ! 范围解析的函数
     * ! 可以解析出如A-Z, a-zA-Z0-9_- 这一类范围定义的字符
     * 以字符串形式返回
     */
    if (str.size() == 1) return str;
    string range;
    for (size_t i = 0; i < str.size(); ++i) {
        if (i + 1 < str.size() and str[i + 1] == '-') {  // 识别到一个范围
            for (auto x = str[i]; x <= str[i + 2]; ++x) {
                range += x;
            }
            i += 2;
        } else {
            range += str[i];
        }
    }
    return range;
}

void get_closure(NFA nfa, vector<set<int>> &closures, int state, vector<bool> &visted) {
    /**
     * ! 获取nfa某状态闭包的函数. 存储在closures中
     * !  参数解释: 
     * NFA nfa: 要求闭包的nfa
     * vector<set<int>> &closures: 闭包存储的数据结构
     * int state: 当前要求闭包的状态. 由于dfa, 先求出它可以扩展的状态, 再回来求它
     * vector<bool> &visted: 回溯的标记, 先标记为true, 回溯之后标记为false
     */
    if (closures[state].size()) return;
    for (auto x : nfa.jump[state]) {                                                 // 当前状态的map集
        if (x.first == '~') {                                                        // 当边为 空 时, 可以滑动求闭包
            for (auto y : x.second) {                                                // 遍历下一状态
                if (visted[y] == false) {                                            // 当前递归路径没有被访问过
                    visted[y] = true;                                                // 标记访问
                    get_closure(nfa, closures, y, visted);                           // 递归得到下一状态闭包
                    closures[state].insert(closures[y].begin(), closures[y].end());  // 更新当前状态闭包
                    visted[y] = false;                                               // 撤销标记
                }
            }
        }
    }
    closures[state].insert(state);  // ! ...插入自己......
}

Graph parse_re(string rexp, int &state) {
    // cout << "exp = " << rexp << endl;
    if (rexp.size() == 1) {  // ! 当只有一个字符时, 构造基本的转移函数, 然后返回. 这是这个函数的递归基例
        Graph base;
        base.start = state++;
        base.end   = state++;
        Edge edge(base.start, rexp[0], base.end);
        base.edges.push_back(edge);
        return base;
    }
    vector<Graph> graphes;  // 这个rexp可以解析出来的所有graph, 用vector存储, 方便合并

    // 一种特殊情况 (ac)*|b, 要和 (ac | b*)) 这种区分开来.
    // ! 也就是说如果存在不在括号中的 | , 但括号再rexp中存在的情况, 要先处理 |
    bool has_bra = count(rexp.begin(), rexp.end(), '(');  // 是否有括号
    bool or_out  = false;                                 // | 是否在括号外面
    if (has_bra) {
        stack<char> in_or_out;
        for (auto s : rexp) {
            if (s == '(') {
                in_or_out.push('*');
            } else if (s == ')') {
                in_or_out.pop();
            }
            if (s == '|' && in_or_out.size() == 0) or_out = true;
        }
    }

    if (has_bra && or_out == false) {  // 存在括号, 且如果出现 | , 都在括号之中
        // 检测到有括号, 解析括号
        stack<char> bra;  // 括号, 用来处理括号嵌套的情况
        string sub_rexp;
        for (int i = 0; i < rexp.size(); ++i) {
            char c = rexp[i];
            if (c == '(') {
                if (sub_rexp.size() && bra.size() == 0) {
                    graphes.push_back(parse_re(sub_rexp, state));
                    sub_rexp.clear();
                }
                bra.push('*');
                if (bra.size() != 1) sub_rexp.push_back('(');
            } else if (c == ')') {
                // cout << "2 - " << sub_rexp << endl;
                bra.pop();
                if (bra.size() != 0) sub_rexp.push_back(')');
                if (bra.size() == 0) {  // 最外层括号被弹出, 将括号包裹的部分进行解析
                    // cout << "bar = 0: " << sub_rexp << endl;
                    // 增加对括号闭包的操作
                    if (i + 1 < rexp.size() && (rexp[i + 1] == '*' || rexp[i + 1] == '+')) {  // * 对 * 和 + 的解析
                        if (rexp[i + 1] == '+')
                            graphes.push_back(parse_re(sub_rexp, state));
                        int start = state++;
                        graphes.push_back(parse_re(sub_rexp, state));
                        int end = state++;
                        graphes.back().edges.push_back(Edge(start, '~', graphes.back().start));
                        graphes.back().edges.push_back(Edge(graphes.back().end, '~', graphes.back().start));
                        graphes.back().edges.push_back(Edge(graphes.back().end, '~', end));
                        graphes.back().edges.push_back(Edge(start, '~', end));
                        graphes.back().start = start;
                        graphes.back().end   = end;
                        i++;
                    } else {
                        graphes.push_back(parse_re(sub_rexp, state));
                    }
                    sub_rexp.clear();
                }

                // cout << "1 - " << sub_rexp << endl;
            } else {
                sub_rexp += c;
            }
        }
        // cout << "sub_rexp = " << sub_rexp << endl;
        if (sub_rexp.size()) {
            graphes.push_back(parse_re(sub_rexp, state));
            sub_rexp.clear();
        }
    } else if (count(rexp.begin(), rexp.end(), '|')) {  // ! 解析或, 没有括号的存在
        string sub_rexp;
        Graph merge;
        merge.start = state++;  // 先编号开始, 顺序好看些
        for (auto s : rexp) {
            if (s == '|') {
                if (sub_rexp.size()) {
                    graphes.push_back(parse_re(sub_rexp, state));
                    sub_rexp.clear();
                }
            } else {
                sub_rexp += s;
            }
        }
        if (sub_rexp.size()) {
            graphes.push_back(parse_re(sub_rexp, state));
            sub_rexp.clear();
        }
        merge.end = state++;
        for (auto graph : graphes) {
            merge.edges.push_back(Edge(merge.start, '~', graph.start));
            merge.edges.insert(merge.edges.end(), graph.edges.begin(), graph.edges.end());
            merge.edges.push_back(Edge(graph.end, '~', merge.end));
        }
        return merge;  // ! 处理或的直接返回, 因为和处理简单的并或者括号不一样
    } else {           // 处理普通的没有 | , 也没有括号的情况
        // cout << "rexp = " << rexp << endl;
        for (int i = 0; i < rexp.size(); ++i) {
            string next_rexp;
            next_rexp += rexp[i];
            if (i + 1 < rexp.size() && (rexp[i + 1] == '*' || rexp[i + 1] == '+')) {  // * 对 * 和 + 的解析
                if (rexp[i + 1] == '+')
                    graphes.push_back(parse_re(next_rexp, state));
                int start = state++;
                graphes.push_back(parse_re(next_rexp, state));
                int end = state++;
                graphes.back().edges.push_back(Edge(start, '~', graphes.back().start));
                graphes.back().edges.push_back(Edge(graphes.back().end, '~', graphes.back().start));
                graphes.back().edges.push_back(Edge(graphes.back().end, '~', end));
                graphes.back().edges.push_back(Edge(start, '~', end));
                graphes.back().start = start;
                graphes.back().end   = end;
                i++;
            } else {
                graphes.push_back(parse_re(next_rexp, state));
            }
            next_rexp.clear();
        }
    }

    // 解析多原子单位的合并
    Graph merge;
    merge.start = graphes[0].start;
    merge.end   = graphes.back().end;
    merge.edges = graphes[0].edges;
    for (int i = 1; i < graphes.size(); ++i) {
        merge.edges.push_back(Edge(graphes[i - 1].end, '~', graphes[i].start));
        merge.edges.insert(merge.edges.end(), graphes[i].edges.begin(), graphes[i].edges.end());
    }
    // for (auto edge: merge.edges) {
    //     cout << edge.from << ' ' << edge.cond << ' ' << edge.to << endl;
    // }
    return merge;
}

void nfa2dot(NFA nfa, string dot_file) {
    // 合并起点终点相同的边, 用一个矩阵来存储
    vector<vector<vector<char>>> merge_jump(nfa.state_cnt, vector<vector<char>>(nfa.state_cnt, vector<char>()));
    for (int i = 0; i < nfa.state_cnt; ++i) {
        auto sm = nfa.jump[i];
        for (auto m : sm) {
            for (auto v : m.second) {
                merge_jump[i][v].push_back(m.first);
            }
        }
    }
    // cout << "dot_file: " << dot_file << endl;
    ofstream write;
    write.open(dot_file, ios::out | ios::trunc);
    // 写入文件开始
    write << "digraph {" << endl;
    // 写入状态
    for (int i = 0; i < nfa.state_cnt; ++i) {
        write << "\t" + to_string(i) + " [label=\"" + to_string(i) + "\"";
        if (count(nfa.end_states.begin(), nfa.end_states.end(), i)) {
            write << " color=red shape=doublecircle";  // 如果是终止状态, 那么结点为红色, 且为双圆型
        }
        write << "]" << endl;
    }
    // * 可以合并的边, 检测到连续3个或以上的 +1递增 的条件就合并, 用 - 连接
    for (int i = 0; i < nfa.state_cnt; ++i) {
        for (int j = 0; j < nfa.state_cnt; ++j) {
            if (merge_jump[i][j].size()) {
                string cond = "\"";
                cond += merge_jump[i][j][0];
                cond += ',';
                int ins = 1;
                for (int k = 1; k < merge_jump[i][j].size(); ++k) {
                    if (merge_jump[i][j][k] == merge_jump[i][j][k - 1] + 1) {
                        ins += 1;
                    } else {
                        if (ins >= 3) {
                            for (int l = 0; l < 2 * ins - 1; ++l) {
                                cond.pop_back();
                            }
                            cond += '-';
                            cond += merge_jump[i][j][k - 1];
                            cond += ',';
                            ins = 1;
                        }
                    }
                    cond += merge_jump[i][j][k];
                    cond += ',';
                    // cout << "cond: " << cond << endl;
                }
                if (ins >= 3) {
                    for (int l = 0; l < 2 * ins - 1; ++l) {
                        cond.pop_back();
                    }
                    cond += '-';
                    cond += merge_jump[i][j].back();
                    cond.push_back(',');
                    ins = 1;
                }
                cond.erase(cond.end() - 1);
                cond += '"';
                // cout << "cond = " << cond << endl;
                write << "\t" + to_string(i) + " -> " + to_string(j) + " [label=" + cond + "]" << endl;
            }
        }
    }
    write << "}";
}

void dfa2dot(DFA dfa, string dot_file) {
    // 合并起点终点相同的边, 用一个矩阵来存储
    vector<vector<vector<char>>> merge_jump(dfa.state_cnt, vector<vector<char>>(dfa.state_cnt, vector<char>()));
    for (int i = 0; i < dfa.state_cnt; ++i) {
        auto sm = dfa.jump_map[i];
        for (auto m : sm) {
            merge_jump[i][m.second].push_back(m.first);
        }
    }
    ofstream write;
    write.open(dot_file, ios::out | ios::trunc);
    // 写入文件开始
    write << "digraph {" << endl;
    // 写入状态
    for (int i = 0; i < dfa.state_cnt; ++i) {
        write << "\t" + to_string(i) + " [label=\"" + to_string(i) + "\"";
        if (count(dfa.end_states.begin(), dfa.end_states.end(), i)) {
            write << " color=red shape=doublecircle";  // 如果是终止状态, 那么结点为红色, 且为双圆型
        }
        write << "]" << endl;
    }
    // * 可以合并的边, 检测到连续3个或以上的 +1递增 的条件就合并, 用 - 连接
    for (int i = 0; i < dfa.state_cnt; ++i) {
        for (int j = 0; j < dfa.state_cnt; ++j) {
            if (merge_jump[i][j].size()) {
                string cond = "\"";
                cond += merge_jump[i][j][0];
                cond += ',';
                int ins = 1;
                for (int k = 1; k < merge_jump[i][j].size(); ++k) {
                    if (merge_jump[i][j][k] == merge_jump[i][j][k - 1] + 1) {
                        ins += 1;
                    } else {
                        if (ins >= 3) {
                            for (int l = 0; l < 2 * ins - 1; ++l) {
                                cond.pop_back();
                            }
                            cond += '-';
                            cond += merge_jump[i][j][k - 1];
                            cond += ',';
                            ins = 1;
                        }
                    }
                    cond += merge_jump[i][j][k];
                    cond += ',';
                    // cout << "cond: " << cond << endl;
                }
                if (ins >= 3) {
                    for (int l = 0; l < 2 * ins - 1; ++l) {
                        cond.pop_back();
                    }
                    cond += '-';
                    cond += merge_jump[i][j].back();
                    cond.push_back(',');
                    ins = 1;
                }
                cond.erase(cond.end() - 1);
                cond += '"';
                // cout << "cond = " << cond << endl;
                write << "\t" + to_string(i) + " -> " + to_string(j) + " [label=" + cond + "]" << endl;
            }
        }
    }
    write << "}";
}

void help() {
    cout << endl
         << "Usage: ./main [options hir:s:o:p:g:] [:argument]" << endl
         << "options:" << endl
         << "  -h" << endl
         << "    说明: 打印提示信息并退出程序" << endl
         << "  -i" << endl
         << "    说明: 手动输入正则表达式 和 待匹配字符串" << endl
         << "  -r <re_exp>" << endl
         << "    说明: 作为模式的正则表达式" << endl
         << "  -s <string>" << endl
         << "    说明: 待匹配的字符串" << endl
         << "  -o <png_file>" << endl
         << "    说明: 要输出的png文件路径" << endl
         << "  -p <pattern_name>" << endl
         << "    说明: 输出的状态图要布局的模式: circo | dot | neato | twopi | fdp | patchwork" << endl
         << "  -g <graph_type>" << endl
         << "    说明: 要画的状态图的类型, 默认为三个类型: n | d | m, 可连起来写成字符串如 nm. 他们分别为 nfa | dfa | mindfa" << endl
         << "demos:" << endl
         << "  ./main -i" << endl
         << "    说明: 手动输入正则表达式 和 待匹配字符串, 程序打印匹配结果" << endl
         << "  ./main -r \"(ab*|b)*ca*\" -s aabbacaa" << endl
         << "    说明: 用正则表达式 (ab*|b)*ca* 去匹配字符串 aabbacaa" << endl
         << "  ./main -r \"(ab*|b)*ca*\" -o test.png" << endl
         << "    说明: 将 正则表达式 (ab*|b)*ca* 转化到 test_nfa.png, test_dfa.png, test_mindfa.png 系列图片" << endl
         << "  ./main -r \"(ab*|b)*ca*\" -o test.png -p dot" << endl
         << "    说明: 用 dot模式 生成状态图" << endl
         << "  ./main -r \"(ab*|b)*ca*\" -o test.png -p dot -g nm" << endl
         << "    说明: 指定生成 nfa 和 mindfa 状态图" << endl
         << endl
         << "good luck!" << endl
         << endl;
}