# 使用文档

---



## 项目依赖

此项目的图片生成部分依赖于项目 graphviz, [这是他们的主页](https://www.graphviz.org/)

致谢

如何安装 graphviz 可以参考[这篇文章](https://www.cnblogs.com/onemorepoint/p/8310996.html)

致谢

**请安装好graphviz并配置好环境变量之后再使用本程序的画图功能. 字符串匹配功能不受影响**



## 程序结构

在工程根目录下, 执行 `make` 编译程序, 可得到目标文件 `main.exe`

tmp 文件夹问临时文件, 保存了中间过程产生的nfa, dfa以及dot文件. 