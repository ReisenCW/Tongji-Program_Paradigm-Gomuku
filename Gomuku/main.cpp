#define _CRT_SECURE_NO_WARNINGS
#include "stdio.h"
#include <set>
#include <iostream>
#include <climits>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <queue>
using namespace std;

#define _DEBUG_ 0
#define _ONLINE_DEBUG_ 0
#define _TIMER_ 0

//常量与def
typedef long long LL;
const int board_size = 12;
const int dpth = 4;
const int hashIndexSize = 0xffff;//掩码,用于限制位数(二进制对应1111111111111111)
const int hashNoneScore = 99999999;//置换表中的空值

//分数评估表
const int MAX_SCORE = 10000000;
const int MIN_SCORE = -10000000;
const int FIVE_LINE = 1000000;     // 五连分数
const int LIVE_FOUR = 40000;        // 活四(在两个点上下都可以五连)分数
const int BLOCK_FOUR = 3000;       // 冲四(在唯一的一点上下可以五连)分数
const int LIVE_THREE = 3000;   // 活三(可以变成活4)分数
const int BLOCK_THREE = 300;       // 眠三(可以变成冲4)分数
const int LIVE_TWO = 200;          // 活二(可以变成活3)分数
const int BLOCK_TWO = 30;          // 眠二(可以变成眠三)分数
const int LIVE_ONE = 30;           // 活一(可以变成活二)分数
const int BLOCK_ONE = 1;           // 眠一(可以变成眠二)分数

//枚举类
enum Chess {
	None = 0,
	Black = 1,
	White = 2,
};
//结构体
struct Point {
	int x;
	int y;
	bool operator==(const Point& p) const {
		return x == p.x && y == p.y;
	}
	bool operator<(const Point& p) const {
		return tie(x, y) < tie(p.x, p.y);
	}
};

struct Move {
	Point p;
	LL score;
};

struct Pattern {
	string pattern;
	int score;
};

struct HashItem {
	LL checksum;
	int depth;
	LL score;
	enum Flag { ALPHA = 0, BETA = 1, EXACT = 2, EMPTY = 3 } flag;
};

class PositionManager;

//全局变量
Chess field; // 己方颜色
Chess opponent; // 对方颜色
Chess board[board_size][board_size] = { None }; // 棋盘
LL all_score[2]; // 总分数,all_score[0]为己方分数,all_score[1]为对方分数
LL point_score[2][4][board_size * 2];//[chess][direction][index],对于竖和横,会空出来board_size个位置
Move bestMove = { {-1, -1} , MIN_SCORE };
//zobrist
LL boardZobristValue[2][board_size][board_size];//棋盘每个位置的Zobrist值,用于计算当前局面的Zobrist值
LL currentZobristValue;//每一个局面对应一个Zobrist值,currentZobristValue为当前局面的Zobrist值

//vector<Pattern> patterns = {
//	{ "11111" ,  FIVE_LINE   }, // 连五
//	{ "011110",  LIVE_FOUR   }, // 活四
//	{ "211110",  BLOCK_FOUR  }, // 冲四
//	{ "011112",  BLOCK_FOUR  },
//	{ "11011" ,  BLOCK_FOUR  },
//	{ "10111" ,  BLOCK_FOUR  },
//	{ "11101" ,  BLOCK_FOUR  },
//	{ "0011100", LIVE_THREE }, // 活三
//	{ "0011102", LIVE_THREE },
//	{ "2011100", LIVE_THREE },
//	{ "010110",  LIVE_THREE  },
//	{ "011010",  LIVE_THREE  },
//	{ "211100",  BLOCK_THREE }, // 眠三
//	{ "001112",  BLOCK_THREE },
//	{ "210110",  BLOCK_THREE },
//	{ "010112",  BLOCK_THREE },
//	{ "210011",  BLOCK_THREE },
//	{ "110012",  BLOCK_THREE },
//	{ "011000",  LIVE_TWO    }, // 活二
//	{ "001100",  LIVE_TWO    },
//	{ "000110",  LIVE_TWO    },
//	{ "010100",  LIVE_TWO    },
//	{ "001010",  LIVE_TWO    },
//	{ "010010",  LIVE_TWO    },
//	{ "211000",  BLOCK_TWO   }, // 眠二
//	{ "210100",  BLOCK_TWO   },
//	{ "000112",  BLOCK_TWO   },
//	{ "001012",  BLOCK_TWO   },
//	{ "210010",  BLOCK_TWO   },
//	{ "010012",  BLOCK_TWO   },
//	{ "210001",  BLOCK_TWO   },
//	{ "100012",  BLOCK_TWO   },
//	{ "010000",  LIVE_ONE    }, // 活一
//	{ "001000",  LIVE_ONE    },
//	{ "000100",  LIVE_ONE    },
//	{ "000010",  LIVE_ONE    },
//	{ "210000",  BLOCK_ONE   }, // 眠一
//	{ "000012",  BLOCK_ONE   }
//};

//评估函数
LL Evaluate(Chess color);//总评估函数(评估整个棋盘)
LL EvaluatePosition(Chess color, int x, int y);//评估某一位置
//LL PatternScore(Chess color, const string& line);//搜索模式,返回各个匹配的模式的分数
void UpdateScore(int x, int y);//更新分数表

//搜索函数
LL Alpha_Beta(Chess color, LL alpha, LL beta, int depth);//alpha-beta剪枝
Point MakePlay(int depth);//己方下棋

//主函数
void StartGame();//游戏主循环

//其它函数
struct PointComparator {
	bool operator()(const Point& a, const Point& b) const {
		return EvaluatePosition(field, a.x, a.y) > EvaluatePosition(field, b.x, b.y);
	}
};
void UpdateInfo(int x, int y) {
	UpdateScore(x, y);
}

set<Point, PointComparator> GetPossibleMoves(Chess color);//获取所有可能的落子位置

//position manager
struct HistoryPosition {
	HistoryPosition() : chosenPosition({ -1, -1 })
	{}
	Point chosenPosition;						//选择的落子点
	set<Point> addedPositions;	//由于chosenPosition而添加的可能落子点
};

class PositionManager {
public:
	void AddPossiblePos(int x, int y);
	void RecoverLastState();
	set<Point, PointComparator>& GetPossiblePos();
private:
	bool isInBound(int x, int y) {
		return x >= 0 && x < board_size && y >= 0 && y < board_size;
	}
private:
	set<Point> currentPossiblePos;
	vector<HistoryPosition> history;
}ppm;

void PositionManager::AddPossiblePos(int x, int y) {
	HistoryPosition hp;
	hp.chosenPosition = { x, y };
	//把棋子(x,y)周围八个方向的点(Empty,且在棋盘范围内)加入到currentPossiblePos和addedPositions中,每个方向加两个点
	int dir[4][2] = { {1, 0}, {0, 1}, {1, 1}, {1, -1} };
	for (int j = 1; j < 2; j++) {//change 3 to 2!to test
		for (int i = 0; i < 4; i++) {
			int dx = dir[i][0] * j, dy = dir[i][1] * j;
			if (isInBound(x + dx, y + dy) && board[x + dx][y + dy] == None) {
				auto insertRes = currentPossiblePos.insert({ x + dx, y + dy });
				if (insertRes.second)
					hp.addedPositions.insert({ x + dx, y + dy });
			}

			if (isInBound(x - dx, y - dy) && board[x - dx][y - dy] == None) {
				auto insertRes = currentPossiblePos.insert({ x - dx, y - dy });
				if (insertRes.second)
					hp.addedPositions.insert({ x - dx, y - dy });
			}
		}
	}
	//把当前点从currentPossiblePos中删除
	currentPossiblePos.erase({ x, y });
	history.push_back(hp);
}

void PositionManager::RecoverLastState() {
	if (history.empty()) return;//不存在历史记录
	//取出最近一条历史记录
	HistoryPosition hp = history.back();
	history.pop_back();
	//恢复currentPossiblePos(去除added,加上chosen)
	for (const auto& p : hp.addedPositions) {
		currentPossiblePos.erase(p);
	}
	currentPossiblePos.insert(hp.chosenPosition);
}

set<Point, PointComparator>& PositionManager::GetPossiblePos() {
	static set<Point, PointComparator> pp;
	pp.clear();
	for (const auto& p : currentPossiblePos) {
		//if (EvaluatePosition(field, p.x, p.y) > 0)
		pp.insert(p);
	}
	return pp;
}
//置换表
//zobrist
// 生成64位随机数
LL Random64() {
	return (LL)rand() | ((LL)rand() << 15) | ((LL)rand() << 30) | ((LL)rand() << 45) | ((LL)rand() << 60);
}

// 初始化随机数表
void RandomBoardZobristValue() {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < board_size; j++) {
			for (int k = 0; k < board_size; k++) {
				boardZobristValue[i][j][k] = Random64();
			}
		}
	}
}

// 初始化初始局面的Zobrist值
void InitCurrentZobristValue() {
	currentZobristValue = 0;
	for (int i = 0; i < board_size; i++) {
		for (int j = 0; j < board_size; j++) {
			if (board[i][j] != None) {
				currentZobristValue ^= boardZobristValue[(int)board[i][j] - 1][i][j];
			}
		}
	}
}

// 更新Zobrist值
void UpdateZobristValue(int x, int y, Chess color) {
	currentZobristValue ^= boardZobristValue[(int)color - 1][x][y];
}
//存储已经计算过的棋局评分。每个条目包含局面的哈希值、评分、深度和评分类型
HashItem hashItems[hashIndexSize + 1];//下标范围0-1111111111111111

//记录置换表信息
void RecordHashItem(int depth, LL score, HashItem::Flag flag) {
	int index = currentZobristValue & hashIndexSize;
	//仅在以下两种情况下更新置换表: 
		// 1.置换表为空    
		// 2.当前递归深度大于置换表深度,说明当前信息更精准,则替换掉置换表内的信息
	if (hashItems[index].flag == HashItem::EMPTY || hashItems[index].depth < depth) {
		hashItems[index].checksum = currentZobristValue;
		hashItems[index].depth = depth;
		hashItems[index].score = score;
		hashItems[index].flag = flag;
	}
}

//取置换表信息
LL GetHashScore(int depth, int alpha, int beta) {
	int index = currentZobristValue & hashIndexSize;
	//空值
	if (hashItems[index].flag == HashItem::EMPTY) {
		return hashNoneScore;
	}
	if (hashItems[index].checksum == currentZobristValue) {
		//若哈希表深度>=当前深度,则可信,返回哈希表中的值
		if (hashItems[index].depth >= depth) {
			//准确值则直接返回
			if (hashItems[index].flag == HashItem::EXACT) {
				return hashItems[index].score;
			}
			//alpha值说明存储的值是一个下限值(不被敌方剪枝的情况下的最大值)
			if (hashItems[index].flag == HashItem::ALPHA && hashItems[index].score <= alpha) {
				return alpha;
			}
			//beta值说明存储的值是一个上限值(不被己方剪枝的情况下的最小值)
			if (hashItems[index].flag == HashItem::BETA && hashItems[index].score >= beta) {
				return beta;
			}
		}
	}
	return hashNoneScore;
}

//AC自动机实现字符串匹配,用于查找棋型
class TrieNode;
class AC_Auto;
//Trie树节点
class TrieNode {
public:
	friend class AC_Auto;
	TrieNode() : fail(-1), next{ -1, -1, -1 }, length(0), score(0) {}
private:
	char data;//节点存储的字符
	int fail;//失败指针
	int next[3];//子节点
	int length;//当前节点代表的字符串长度
	int score;//当前节点代表的字符串的分数
};

//AC自动机
class AC_Auto {
public:
	AC_Auto();
	void BuildTrieTree();
	void BuildFailPointer();
	LL PatternScore(const string& text);
private:
	vector<TrieNode> nodes;//Trie树
	vector<string> patterns;//模式串
	vector<int> scores;//模式串对应的分数
}AC_Searcher;

AC_Auto::AC_Auto()
	: patterns({ "11111", "011110", "211110", "011112", "11011", "10111", "11101", "0011100", "0011102",
				 "2011100", "010110", "011010", "211100", "001112", "210110", "010112", "210011", "110012", "011000",
				 "001100", "000110", "010100", "001010", "010010", "211000", "210100", "000112", "001012", "210010",
				 "010012","210001", "100012", "010000", "001000", "000100", "000010", "210000", "000012" }), //38
	scores({ FIVE_LINE, LIVE_FOUR, BLOCK_FOUR, BLOCK_FOUR, BLOCK_FOUR, BLOCK_FOUR, BLOCK_FOUR, LIVE_THREE, LIVE_THREE,
			 LIVE_THREE, LIVE_THREE, LIVE_THREE, BLOCK_THREE, BLOCK_THREE, BLOCK_THREE, BLOCK_THREE, BLOCK_THREE, BLOCK_THREE,
			 LIVE_TWO, LIVE_TWO, LIVE_TWO, LIVE_TWO, LIVE_TWO, LIVE_TWO, BLOCK_TWO, BLOCK_TWO, BLOCK_TWO, BLOCK_TWO,
			 BLOCK_TWO, BLOCK_TWO, BLOCK_TWO, BLOCK_TWO, LIVE_ONE, LIVE_ONE, LIVE_ONE, LIVE_ONE, BLOCK_ONE , BLOCK_ONE })
{
	nodes.resize(104);
}

void AC_Auto::BuildTrieTree() {
	nodes.push_back(TrieNode());
	int newNodeIndex = 1;
	for (int i = 0; i < patterns.size(); i++) {
		int cur = 0;
		for (char c : patterns[i]) {
			int index = c - '0';
			if (nodes[cur].next[index] == -1) {
				nodes[cur].next[index] = newNodeIndex;
				nodes[newNodeIndex] = TrieNode();
				nodes[newNodeIndex].data = c;
				newNodeIndex++;
			}
			cur = nodes[cur].next[index];//cur指针向下一个字符
		}
		nodes[cur].length = patterns[i].length();//记录模式串的长度
		nodes[cur].score = scores[i];//记录分数
	}
}

void AC_Auto::BuildFailPointer() {
	queue<int> q;
	for (int i = 0; i < 3; i++) {//root节点的子节点的fail指针都指向root节点
		nodes[nodes[0].next[i]].fail = 0;
		q.push(nodes[0].next[i]);
	}
	while (!q.empty()) {
		int cur = q.front();//取队头
		q.pop();
		for (int i = 0; i < 3; ++i) {
			if (nodes[cur].next[i] != -1) {//如果当前节点的有匹配的子节点
				int fail = nodes[cur].fail;//子节点的fail指针初始化为当前节点的fail指针
				while (nodes[fail].next[i] == -1) {//fail指针指向的节点没有对应的子节点,则继续向上跳转
					fail = nodes[fail].fail;
				}
				nodes[nodes[cur].next[i]].fail = nodes[fail].next[i];//子节点的fail指针指向fail节点的对应子节点
				q.push(nodes[cur].next[i]);//将子节点加入队列
			}
		}
	}
}

LL AC_Auto::PatternScore(const string& text) {
	int cur = 0;
	LL totalScore = 0;
	for (int i = 0; i < text.size(); i++) {
		int c = text[i] - '0';
		while (nodes[cur].next[c] == -1 && cur != 0) {//如果当前节点没有对应的子节点,则匹配失败,跳转到fail指针
			cur = nodes[cur].fail;
		}
		cur = nodes[cur].next[c];//如果有对应的子节点,则指针指向子节点
		if (cur == -1) cur = 0;//如果当前指针cur为-1,说明前面的fail指针全部匹配失败,则指针指向根节点
		int temp = cur;
		while (temp != 0) {
			//如果length为0,说明当前节点不是一个模式串的结尾,则不会进入循环
			//如果length不为0,则说明当前节点是一个模式串的结尾,则计算分数
			totalScore += nodes[temp].score;
			temp = nodes[temp].fail;
		}
	}
	return totalScore;
}

/******************************************************************************************************/
/***********************************************调试函数************************************************/
//Timer
#if _TIMER_
#include <chrono>
#include <string>
class Timer {
public:
	Timer(const string& msg) : m_begin(std::chrono::high_resolution_clock::now()), m_msg(msg) {}
	~Timer() {
		end();
	}
	void end() {
		auto end = std::chrono::high_resolution_clock::now();
		auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_begin);
		printf("DEBUG %s takes : %lld ms\n", m_msg.c_str(), dur.count());
		fflush(stdout);
	}
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
	string m_msg;
};
#endif

//DEBUG打印棋盘
#if _DEBUG_
//打印棋盘
void PrintBoard() {
	printf("\nDEBUG Board:\n");
	// 打印列号
	printf("  ");
	for (int j = 0; j < board_size; j++) {
		printf("\033[32m%x \033[0m", j); // 绿色
	}
	printf("\n");

	for (int i = 0; i < board_size; i++) {
		// 打印行号
		printf("\033[32m%x \033[0m", i); // 绿色
		for (int j = 0; j < board_size; j++) {
			if (board[i][j] == None) {
				printf("0 ");
			}
			else if (board[i][j] == Black) {
				printf("\033[31m1 \033[0m"); // 红色
			}
			else {
				printf("\033[33m2 \033[0m"); // 黄色
			}
		}
		printf("\n");
	}
	printf("\n");
	fflush(stdout);
}
#endif
/***********************************************调试函数************************************************/
//*****************************************************************************************************
//函数实现
set<Point, PointComparator> GetPossibleMoves(Chess color) {
	set<Point, PointComparator> moves;
	int minX = board_size, maxX = 0, minY = board_size, maxY = 0;

	// 找到当前棋局的最左、最右、最上、最下点
	for (int i = 0; i < board_size; i++) {
		for (int j = 0; j < board_size; j++) {
			if (board[i][j] != None) {
				if (i < minX) minX = i;
				if (i > maxX) maxX = i;
				if (j < minY) minY = j;
				if (j > maxY) maxY = j;
			}
		}
	}

	// 扩展2格的范围
	minX = max(0, minX - 2);
	maxX = min(board_size - 1, maxX + 2);
	minY = max(0, minY - 2);
	maxY = min(board_size - 1, maxY + 2);

	// 在扩展后的范围内寻找可能的落子位置
	for (int i = minX; i <= maxX; i++) {
		for (int j = minY; j <= maxY; j++) {
			if (board[i][j] == None) {
				if (EvaluatePosition(color, i, j) > 0)
					moves.insert({ i, j });//按照评估分数从大到小排序
			}
		}
	}

	return moves;
}


Point MakePlay(int depth) {
#if _TIMER_
	Timer t("Alpha_Beta深搜");
#endif
	// 初始化 bestMove
	bestMove = { {-1, -1}, MIN_SCORE };
	//递归,计算模拟位置的分数
	Alpha_Beta(field, MIN_SCORE, MAX_SCORE, depth);
	board[bestMove.p.x][bestMove.p.y] = field;
	UpdateInfo(bestMove.p.x, bestMove.p.y);
	ppm.AddPossiblePos(bestMove.p.x, bestMove.p.y);
	return bestMove.p;
}

LL Alpha_Beta(Chess color, LL alpha, LL beta, int depth) {
	HashItem::Flag flag = HashItem::ALPHA;
	LL score = GetHashScore(depth, alpha, beta);
	//从第二次递归开始,如果置换表中有值,则直接返回
	if (score != hashNoneScore && depth != dpth) {
		return score;
	}
	LL score_self = Evaluate(color);
	LL score_oppo = Evaluate((color == Black) ? White : Black);
	//递归到最后一层,直接返回评估值,以Exact方式存入置换表
	if (depth == 0) {
		RecordHashItem(depth, score_self - score_oppo, HashItem::EXACT);
		return score_self - score_oppo;
	}
	//如果己方或对方五连,则直接返回MAX分数,后面的dpth-depth是为了让遍历层数少(depth更大的)结果更优先
	if (score_self >= FIVE_LINE)
		return MAX_SCORE - 10 - (dpth - depth);
	if (score_oppo >= FIVE_LINE)
		return MIN_SCORE + 10 + (dpth - depth);

	//set<Point, PointComparator> Moves = GetPossibleMoves(color);
	set<Point, PointComparator> Moves = ppm.GetPossiblePos();
	if (Moves.empty()) {
		return score_self - score_oppo;
	}
	Chess oppo = (color == Black) ? White : Black;
	int cnt = 0;
	for (const auto& p : Moves) {
		if (cnt > 10) break;
		//模拟落子
		board[p.x][p.y] = color;
		UpdateZobristValue(p.x, p.y, color);
		UpdateInfo(p.x, p.y);
		ppm.AddPossiblePos(p.x, p.y);

		//递归,计算模拟位置的分数
		LL val = -Alpha_Beta(oppo, -beta, -alpha, depth - 1);//以对手视角进行评估
		//还原棋盘
		board[p.x][p.y] = None;
		UpdateZobristValue(p.x, p.y, color);
		UpdateInfo(p.x, p.y);
		ppm.RecoverLastState();
		//alpha-beta剪枝
		//对手视角,取最小值,如果当前值已经大于beta了,则对手不会选择这个位置,直接返回beta
		if (val >= beta) {
			RecordHashItem(depth, beta, HashItem::BETA);
			return beta;
		}
		//己方视角,取最大值
		if (val > alpha) {
			flag = HashItem::EXACT;
			alpha = val;
			if (depth == dpth) {
				bestMove = { p, alpha };
			}
		}
		cnt++;
	}
	RecordHashItem(depth, alpha, flag);
	return alpha;
}

LL Evaluate(Chess color) {
	if (color == field) {
		return all_score[0];
	}
	else {
		return all_score[1];
	}
}

LL EvaluatePosition(Chess color, int x, int y) {
	//类似于UpdateScore,但是并非更新,而是计算并返回一个None位置下子color得到的分数(己方-敌方),用于模拟落子的过程
	//评估时只考虑左右(水平时)各5格的棋子情况
	if (board[x][y] != None) return 0;
	board[x][y] = color;
	Chess opp = (color == Black) ? White : Black;
	//初始化pattern
	string myPattern[4];//大小为4,分别存储横,竖,左上-右下,右上-左下的棋子pattern
	string oppoPattern[4];
	//如果开头是边界,则模拟边界
	if (x - 5 < 0) {
		myPattern[0] += '2';
		oppoPattern[0] += '2';
	}
	if (y - 5 < 0) {
		myPattern[1] += '2';
		oppoPattern[1] += '2';
	}
	if (x - min(5, min(x, y)) < 0) {
		myPattern[2] += '2';
		oppoPattern[2] += '2';
	}
	if (x + min(5, min(board_size - 1 - x, y)) >= board_size) {
		myPattern[3] += '2';
		oppoPattern[3] += '2';
	}

	//横向
	for (int i = max(0, x - 5); i < min(board_size, x + 6); i++) {
		myPattern[0] += (board[i][y] == color) ? '1' : (board[i][y] == None ? '0' : '2');
		oppoPattern[0] += (board[i][y] == opp) ? '1' : (board[i][y] == None ? '0' : '2');
	}
	//纵向
	for (int i = max(0, y - 5); i < min(board_size, y + 6); i++) {
		myPattern[1] += (board[x][i] == color) ? '1' : (board[x][i] == None ? '0' : '2');
		oppoPattern[1] += (board[x][i] == opp) ? '1' : (board[x][i] == None ? '0' : '2');
	}
	//左上-右下
	for (int i = max(0, x - min(5, min(x, y))), j = max(0, y - min(5, min(x, y))); i < min(board_size, x + 6) && j < min(board_size, y + 6); i++, j++) {
		myPattern[2] += (board[i][j] == color) ? '1' : (board[i][j] == None ? '0' : '2');
		oppoPattern[2] += (board[i][j] == opp) ? '1' : (board[i][j] == None ? '0' : '2');
	}
	//右上-左下
	for (int i = max(0, x + min(5, min(y, board_size - 1 - x))), j = max(0, y - min(5, min(y, board_size - 1 - x))); i >= 0 && j < min(board_size, y + 6); i--, j++) {
		myPattern[3] += (board[i][j] == color) ? '1' : (board[i][j] == None ? '0' : '2');
		oppoPattern[3] += (board[i][j] == opp) ? '1' : (board[i][j] == None ? '0' : '2');
	}
	//如果结尾是边界,则模拟边界
	if (x + 5 >= board_size) {
		myPattern[0] += '2';
		oppoPattern[0] += '2';
	}
	if (y + 5 >= board_size) {
		myPattern[1] += '2';
		oppoPattern[1] += '2';
	}
	if (x + min(5, min(board_size - 1 - x, y)) >= board_size) {
		myPattern[3] += '2';
		oppoPattern[3] += '2';
	}
	if (x - min(5, min(x, y)) < 0) {
		myPattern[2] += '2';
		oppoPattern[2] += '2';
	}
	LL myScore = 0;
	LL oppoScore = 0;

	for (int i = 0; i < 4; i++) {
		//计算己方分数
		myScore += AC_Searcher.PatternScore(myPattern[i]);
		//计算对方分数
		oppoScore += AC_Searcher.PatternScore(oppoPattern[i]);
	}

	//myScore和oppoScore分别减去模拟落子前情况的分数
	myScore -= point_score[0][0][x];
	myScore -= point_score[0][1][y];
	myScore -= point_score[0][2][x - y + board_size];
	myScore -= point_score[0][3][x + y];
	oppoScore -= point_score[1][0][x];
	oppoScore -= point_score[1][1][y];
	oppoScore -= point_score[1][2][x - y + board_size];
	oppoScore -= point_score[1][3][x + y];

	board[x][y] = None;
	return myScore - oppoScore;
}


void UpdateScore(int x, int y) {
	string myPattern[4];//大小为4,分别存储横,竖,左上-右下,右上-左下的棋子pattern
	string oppoPattern[4];
	//横向
	for (int i = 0; i < board_size; i++) {
		myPattern[0] += (board[x][i] == field) ? '1' : (board[x][i] == None ? '0' : '2');
		oppoPattern[0] += (board[x][i] == opponent) ? '1' : (board[x][i] == None ? '0' : '2');
	}
	//纵向
	for (int i = 0; i < board_size; i++) {
		myPattern[1] += (board[i][y] == field) ? '1' : (board[i][y] == None ? '0' : '2');
		oppoPattern[1] += (board[i][y] == opponent) ? '1' : (board[i][y] == None ? '0' : '2');
	}
	//左上-右下
	for (int i = x - min(x, y), j = y - min(x, y); i < board_size && j < board_size; i++, j++) {
		myPattern[2] += (board[i][j] == field) ? '1' : (board[i][j] == None ? '0' : '2');
		oppoPattern[2] += (board[i][j] == opponent) ? '1' : (board[i][j] == None ? '0' : '2');
	}
	//右上-左下
	for (int i = x + min(board_size - 1 - x, y), j = y - min(board_size - 1 - x, y); i >= 0 && j < board_size; i--, j++) {
		myPattern[3] += (board[i][j] == field) ? '1' : (board[i][j] == None ? '0' : '2');
		oppoPattern[3] += (board[i][j] == opponent) ? '1' : (board[i][j] == None ? '0' : '2');
	}

	//首尾加上敌方子,模拟边界
	for (int i = 0; i < 4; i++) {
		myPattern[i] = "2" + myPattern[i] + "2";
		oppoPattern[i] = "2" + oppoPattern[i] + "2";
	}

	LL myScore[4] = { 0 };
	LL oppoScore[4] = { 0 };

	//计算分数

	for (int i = 0; i < 4; i++) {
		//计算己方分数
		myScore[i] += AC_Searcher.PatternScore(myPattern[i]);
		//计算对方分数
		oppoScore[i] += AC_Searcher.PatternScore(oppoPattern[i]);
	}

	//更新分数
	//先减去原来的分数
	for (int i = 0; i < 2; i++) {
		all_score[i] -= point_score[i][0][x];
		all_score[i] -= point_score[i][1][y];
		all_score[i] -= point_score[i][2][x - y + board_size];
		all_score[i] -= point_score[i][3][x + y];
	}
	//更新point_score
	point_score[0][0][x] = myScore[0];
	point_score[0][1][y] = myScore[1];
	point_score[0][2][x - y + board_size] = myScore[2];
	point_score[0][3][x + y] = myScore[3];
	point_score[1][0][x] = oppoScore[0];
	point_score[1][1][y] = oppoScore[1];
	point_score[1][2][x - y + board_size] = oppoScore[2];
	point_score[1][3][x + y] = oppoScore[3];

	//再加上新的分数
	for (int i = 0; i < 2; i++) {
		all_score[i] += point_score[i][0][x];
		all_score[i] += point_score[i][1][y];
		all_score[i] += point_score[i][2][x - y + board_size];
		all_score[i] += point_score[i][3][x + y];
	}
}

//LL PatternScore(Chess color, const string& line) {
//	//如果line中无1,则直接返回0
//	if (line.find("1") == string::npos) return 0;
//	LL totalScore = 0;
//	double colorRate = (color == White) ? 0.45 : 1;
//	for (size_t i = 0; i < patterns.size(); i++) {
//		size_t pos = 0;
//		while ((pos = line.find(patterns[i].pattern, pos)) != string::npos) {
//			totalScore += (LL)(colorRate * patterns[i].score);
//			pos += patterns[i].pattern.size();
//		}
//	}
//	return totalScore;
//}

void InitGame() {
	//初始化Zobrist
	RandomBoardZobristValue();
	InitCurrentZobristValue();
	//初始AC自动机
	AC_Searcher.BuildTrieTree();
	AC_Searcher.BuildFailPointer();
	//初始化棋盘
	board[5][6] = Black;
	UpdateZobristValue(5, 6, Black);
	UpdateInfo(5, 6);
	ppm.AddPossiblePos(5, 6);
	board[5][5] = White;
	UpdateZobristValue(5, 5, White);
	UpdateInfo(5, 5);
	ppm.AddPossiblePos(5, 5);
	board[6][5] = Black;
	UpdateZobristValue(6, 5, Black);
	UpdateInfo(6, 5);
	ppm.AddPossiblePos(6, 5);
	board[6][6] = White;
	UpdateZobristValue(6, 6, White);
	UpdateInfo(6, 6);
	ppm.AddPossiblePos(6, 6);
}

//主函数
void StartGame() {
	char cmd[10];
	//主循环
	while (1) {
		// 接收指令
		int color = 0;
		scanf("%s %d", cmd, &color);
		// 检查指令是否为 START
		if (strcmp(cmd, "START") == 0 && (color == 1 || color == 2)) {
			field = (color == 1) ? Black : White;
			printf("OK\n");
			fflush(stdout);
			opponent = (field == Black) ? White : Black;
			InitGame();
			break;
		}
	}
	//开始游戏
	while (1) {
		scanf("%s", cmd);

		if (strcmp(cmd, "TURN") == 0) {//己方下棋
			Point p = MakePlay(dpth);
#if _DEBUG_
			PrintBoard();
#endif
			printf("%d %d\n", p.x, p.y);
			fflush(stdout);
		}
		else if (strcmp(cmd, "PLACE") == 0) {//对方下棋
			int x, y;
			scanf("%d %d", &x, &y);
			board[x][y] = opponent;
			UpdateZobristValue(x, y, opponent);
			UpdateInfo(x, y);
			ppm.AddPossiblePos(x, y);
		}
		//判断游戏结束
		if (strcmp(cmd, "END") == 0) {
#if _DEBUG_
			int w;
			scanf("%d", &w);
			if (w == 1) {
				printf("DEBUG winner is Black\n");
			}
			else {
				printf("DEBUG winner is White\n");
			}
#endif
			break;
		}
	}
}

//main函数
int main() {
	StartGame();
	return 0;
}