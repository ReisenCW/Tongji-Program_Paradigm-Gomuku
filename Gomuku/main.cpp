#define _CRT_SECURE_NO_WARNINGS
#include "stdio.h"
#include <set>
#include <iostream>
#include <climits>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unordered_map>
using namespace std;

#define _DEBUG_ 1
#define _ONLINE_DEBUG_ 0
#define _TIMER_ 1

//计时器
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

//常量与def
typedef long long LL;
const int board_size = 12;
const int dpth = 4;

//分数评估表
const int MAX_SCORE = 10000000;
const int MIN_SCORE = -10000000;
const int FIVE_LINE = 1000000;     // 五连分数
const int LIVE_FOUR = 4000;        // 活四(在两个点上下都可以五连)分数
const int BLOCK_FOUR = 2200;       // 冲四(在唯一的一点上下可以五连)分数
const int LIVE_THREE = 2000 / 2;   // 活三(可以变成活4)分数
const int BLOCK_THREE = 400;       // 眠三(可以变成冲4)分数
const int LIVE_TWO = 120;          // 活二(可以变成活3)分数
const int BLOCK_TWO = 40;          // 眠二(可以变成眠三)分数
const int LIVE_ONE = 20;           // 活一(可以变成活二)分数
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

//保存棋局
struct HashItem {
	long long checksum;
	int depth;
	int score;
	enum Flag { ALPHA = 0, BETA = 1, EXACT = 2, EMPTY = 3 } flag;
};


//全局变量
Chess field; // 己方颜色
Chess opponent; // 对方颜色
Chess board[board_size][board_size] = { None }; // 棋盘
LL all_score[2]; // 总分数,all_score[0]为己方分数,all_score[1]为对方分数
// point_score[0]为己方分数,point_score[1]为对方分数,point_score[0]的[0],[1],[2],[3]分别存储横,竖,左上-右下,右上-左下的直线分数
LL point_score[2][4][board_size * 2];//[chess][direction][index],对于竖和横,会空出来board_size个位置
Move bestMove = { {-1, -1} , MIN_SCORE };

vector<Pattern> patterns = {
	{ "11111" ,  FIVE_LINE   }, // 连五
	{ "011110",  LIVE_FOUR   }, // 活四
	{ "211110",  BLOCK_FOUR  }, // 冲四
	{ "011112",  BLOCK_FOUR  },
	{ "11011" ,  BLOCK_FOUR  },
	{ "10111" ,  BLOCK_FOUR  },
	{ "11101" ,  BLOCK_FOUR  },
	{ "0011100", LIVE_THREE }, // 活三
	{ "0011102", LIVE_THREE },
	{ "2011100", LIVE_THREE },
	{ "010110",  LIVE_THREE  },
	{ "011010",  LIVE_THREE  },
	{ "211100",  BLOCK_THREE }, // 眠三
	{ "001112",  BLOCK_THREE },
	{ "210110",  BLOCK_THREE },
	{ "010112",  BLOCK_THREE },
	{ "210011",  BLOCK_THREE },
	{ "110012",  BLOCK_THREE },
	{ "011000",  LIVE_TWO    }, // 活二
	{ "001100",  LIVE_TWO    },
	{ "000110",  LIVE_TWO    },
	{ "010100",  LIVE_TWO    },
	{ "001010",  LIVE_TWO    },
	{ "010010",  LIVE_TWO    },
	{ "211000",  BLOCK_TWO   }, // 眠二
	{ "210100",  BLOCK_TWO   },
	{ "000112",  BLOCK_TWO   },
	{ "001012",  BLOCK_TWO   },
	{ "210010",  BLOCK_TWO   },
	{ "010012",  BLOCK_TWO   },
	{ "210001",  BLOCK_TWO   },
	{ "100012",  BLOCK_TWO   },
	{ "010000",  LIVE_ONE    }, // 活一
	{ "001000",  LIVE_ONE    },
	{ "000100",  LIVE_ONE    },
	{ "000010",  LIVE_ONE    },
	{ "210000",  BLOCK_ONE   }, // 眠一
	{ "000012",  BLOCK_ONE   }
};

//评估函数
LL Evaluate(Chess color);//总评估函数(评估整个棋盘)
LL EvaluatePosition(Chess color, int x, int y);//评估某一位置
LL PatternScore(Chess color, const string& line);//搜索模式,返回各个匹配的模式的分数
void UpdateScore(int x, int y);//更新分数表

//搜索函数
LL Alpha_Beta(Chess color, LL alpha, LL beta, int depth);//alpha-beta剪枝
Point MakePlay(int depth);//己方下棋

//游戏函数
void StartGame();//游戏主循环

//其它函数
struct PointComparator {
	bool operator()(const Point& a, const Point& b) const {
		return EvaluatePosition(field, a.x, a.y) > EvaluatePosition(field, b.x, b.y);
	}
};
set<Point, PointComparator> GetPossibleMoves(Chess color);//获取所有可能的落子位置
void UpdateInfo(int x, int y) {
	UpdateScore(x, y);
}

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

//********************************************************************************
//main函数
int main() {
	StartGame();
	return 0;
}
//********************************************************************************

//功能函数实现
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
	return bestMove.p;
}

LL Alpha_Beta(Chess color, LL alpha, LL beta, int depth) {
	if (depth == 0) {
		return Evaluate(color) - Evaluate((color == Black) ? White : Black);
	}

	set<Point, PointComparator> Moves = GetPossibleMoves(color);
	if (Moves.empty()) {
		return Evaluate(color) - Evaluate((color == Black) ? White : Black);
	}
	Chess oppo = (color == Black) ? White : Black;
	int cnt = 0;
	for (const auto& p : Moves) {
		if (cnt > 10) break;
		//模拟落子
		board[p.x][p.y] = color;
		UpdateInfo(p.x, p.y);

		//递归,计算模拟位置的分数
		LL val = -Alpha_Beta(oppo, -beta, -alpha, depth - 1);//以对手视角进行评估
		//还原棋盘
		board[p.x][p.y] = None;
		UpdateInfo(p.x, p.y);
		//alpha-beta剪枝
		//对手视角,取最小值,如果当前值已经大于beta了,则对手不会选择这个位置,直接返回beta
		if (val >= beta) {
			return beta;
		}
		//己方视角,取最大值
		if (val > alpha) {
			alpha = val;
			if (depth == dpth) {
				bestMove = { p, alpha };
			}
		}
		cnt++;
	}
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

	LL myScore = 0;
	LL oppoScore = 0;

	for (int i = 0; i < 4; i++) {
		//计算己方分数
		myScore += PatternScore(color, myPattern[i]);
		//计算对方分数
		oppoScore += PatternScore(opp, oppoPattern[i]);
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

	LL myScore[4] = { 0 };
	LL oppoScore[4] = { 0 };

	//计算分数

	for (int i = 0; i < 4; i++) {
		//计算己方分数
		myScore[i] += PatternScore(field, myPattern[i]);
		//计算对方分数
		oppoScore[i] += PatternScore(opponent, oppoPattern[i]);
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

LL PatternScore(Chess color, const string& line) {
	//如果line中无1,则直接返回0
	if (line.find("1") == string::npos) return 0;
	LL totalScore = 0;
	double colorRate = (color == White) ? 0.45 : 1;
	for (size_t i = 0; i < patterns.size(); i++) {
		size_t pos = 0;
		while ((pos = line.find(patterns[i].pattern, pos)) != string::npos) {
			totalScore += (LL)(colorRate*patterns[i].score);
			pos += patterns[i].pattern.size();
		}
	}
	return totalScore;
}

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
			//初始化棋盘
			board[5][6] = Black;
			UpdateInfo(5, 6);
			board[5][5] = White;
			UpdateInfo(5, 5);
			board[6][5] = Black;
			UpdateInfo(6, 5);
			board[6][6] = White;
			UpdateInfo(6, 6);
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
			UpdateInfo(x, y);
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