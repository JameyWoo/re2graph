/**
 * ! attention: 由于 ε 无法由 ASCII 表示, 所以空集用字符 ~ 表示
 */

/**
 * 想好需求, 要实现什么功能, 有哪些命令行
 */

#include "algorithms.h"
#include "data_structure.h"

int main(int argc, char *argv[]) {
    string rexp, strm, pngfile;  // * regular expression, string_to_match
    string pattern;              // * 状态图的布局模式
    string graphes;              // * 要画的图形

    // * 参数的解析
    if (argc == 1) {
        help();
        return 0;
    }

    int o;
    string optstring = "hir:s:o:p:g:";  // 可以有的选项, : 代表后面需要跟参数
    while ((o = getopt(argc, argv, optstring.c_str())) != -1) {
        switch (o) {
            case 'h':
                help();
                return 0;
            case '?':
                cout << "error optopt: " << char(optopt) << endl;
                help();
                return 0;
            case 'i':
                cout << "please input the re_expression:\n->$ ";
                cin >> rexp;
                cout << "please input the string to match:\n->$ ";
                cin >> strm;
                break;
            // r 和 s|o配合使用
            case 'r':
                rexp = optarg;
                cout << "re_expresion: " << rexp << endl;
                break;
            case 's':
                strm = optarg;
                cout << "string_to_match: " << strm << endl;
                break;
            case 'o':
                pngfile = optarg;
                cout << "png file: " << pngfile << endl;
                break;
            // 有o才有p
            case 'p':
                pattern = optarg;
                cout << "pattern: " << pattern << endl;
                break;
            case 'g':
                graphes = optarg;
                cout << "graphes: " << graphes << endl;
                break;
        }
    }

    // * 实现步骤, 保存了中间的nfa, dfa, mindfa 等文件在tmp文件夹中, 作为临时文件
    NFA mynfa = re2nfa(rexp);
    writeNfaData(mynfa, "./tmp/tmp.nfa");
    DFA mydfa;
    nfa2dfa(mynfa, mydfa);
    writeDfaData(mydfa, "./tmp/tmp.dfa");
    DFA my_min_dfa = dfa2min(mydfa);
    writeDfaData(my_min_dfa, "./tmp/tmp.mindfa");

    // * 根据输入的信息来执行程序
    if (strm.size()) {
        bool match = dfaPattern(my_min_dfa, strm);
        if (match) {
            cout << "\n*\nmatch succeeded!\n*\n"
                 << endl;
        } else {
            cout << "\n*\nmatch failed!\n*\n"
                 << endl;
        }
    }
    // 根据 pngfile 和 graphes 的选项画图. 如果不指定 graphes, 默认都画出来
    if (pngfile.size()) {
        string raw = pngfile;
        if ((count(graphes.begin(), graphes.end(), 'n') != 0) || graphes.size() == 0) {
            nfa2png(mynfa, pngfile.replace(pngfile.find(".png"), 4, "_nfa.png"), pattern);
            pngfile = raw;
        }
        if ((count(graphes.begin(), graphes.end(), 'd') != 0) || graphes.size() == 0) {
            dfa2png(mydfa, pngfile.replace(pngfile.find(".png"), 4, "_dfa.png"), pattern);
            pngfile = raw;
        }
        if ((count(graphes.begin(), graphes.end(), 'm') != 0) || graphes.size() == 0) {
            dfa2png(my_min_dfa, pngfile.replace(pngfile.find(".png"), 4, "_mindfa.png"), pattern);
            pngfile = raw;
        }
    }

    cout << "program over ... \n";
    return 0;
}