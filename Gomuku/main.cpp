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

//������def
typedef long long LL;
const int board_size = 12;
const int dpth = 4;
const int hashIndexSize = 0xffff;//����,��������λ��(�����ƶ�Ӧ1111111111111111)
const int hashNoneScore = 99999999;//�û����еĿ�ֵ

//����������
const int MAX_SCORE = 10000000;
const int MIN_SCORE = -10000000;
const int FIVE_LINE = 1000000;     // ��������
const int LIVE_FOUR = 8000;        // ����(�����������¶���������)����
const int BLOCK_FOUR = 2000;       // ����(��Ψһ��һ�����¿�������)����
const int LIVE_THREE = 2000;   // ����(���Ա�ɻ�4)����
const int BLOCK_THREE = 400;       // ����(���Ա�ɳ�4)����
const int LIVE_TWO = 120;          // ���(���Ա�ɻ�3)����
const int BLOCK_TWO = 40;          // �߶�(���Ա������)����
const int LIVE_ONE = 20;           // ��һ(���Ա�ɻ��)����
const int BLOCK_ONE = 1;           // ��һ(���Ա���߶�)����

//ö����
enum Chess {
	None = 0,
	Black = 1,
	White = 2,
};
//�ṹ��
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

//ȫ�ֱ���
Chess field; // ������ɫ
Chess opponent; // �Է���ɫ
Chess board[board_size][board_size] = { None }; // ����
LL all_score[2]; // �ܷ���,all_score[0]Ϊ��������,all_score[1]Ϊ�Է�����
LL point_score[2][4][board_size * 2];//[chess][direction][index],�������ͺ�,��ճ���board_size��λ��
Move bestMove = { {-1, -1} , MIN_SCORE };
//zobrist
LL boardZobristValue[2][board_size][board_size];//����ÿ��λ�õ�Zobristֵ,���ڼ��㵱ǰ�����Zobristֵ
LL currentZobristValue;//ÿһ�������Ӧһ��Zobristֵ,currentZobristValueΪ��ǰ�����Zobristֵ

//vector<Pattern> patterns = {
//	{ "11111" ,  FIVE_LINE   }, // ����
//	{ "011110",  LIVE_FOUR   }, // ����
//	{ "211110",  BLOCK_FOUR  }, // ����
//	{ "011112",  BLOCK_FOUR  },
//	{ "11011" ,  BLOCK_FOUR  },
//	{ "10111" ,  BLOCK_FOUR  },
//	{ "11101" ,  BLOCK_FOUR  },
//	{ "0011100", LIVE_THREE }, // ����
//	{ "0011102", LIVE_THREE },
//	{ "2011100", LIVE_THREE },
//	{ "010110",  LIVE_THREE  },
//	{ "011010",  LIVE_THREE  },
//	{ "211100",  BLOCK_THREE }, // ����
//	{ "001112",  BLOCK_THREE },
//	{ "210110",  BLOCK_THREE },
//	{ "010112",  BLOCK_THREE },
//	{ "210011",  BLOCK_THREE },
//	{ "110012",  BLOCK_THREE },
//	{ "011000",  LIVE_TWO    }, // ���
//	{ "001100",  LIVE_TWO    },
//	{ "000110",  LIVE_TWO    },
//	{ "010100",  LIVE_TWO    },
//	{ "001010",  LIVE_TWO    },
//	{ "010010",  LIVE_TWO    },
//	{ "211000",  BLOCK_TWO   }, // �߶�
//	{ "210100",  BLOCK_TWO   },
//	{ "000112",  BLOCK_TWO   },
//	{ "001012",  BLOCK_TWO   },
//	{ "210010",  BLOCK_TWO   },
//	{ "010012",  BLOCK_TWO   },
//	{ "210001",  BLOCK_TWO   },
//	{ "100012",  BLOCK_TWO   },
//	{ "010000",  LIVE_ONE    }, // ��һ
//	{ "001000",  LIVE_ONE    },
//	{ "000100",  LIVE_ONE    },
//	{ "000010",  LIVE_ONE    },
//	{ "210000",  BLOCK_ONE   }, // ��һ
//	{ "000012",  BLOCK_ONE   }
//};

//��������
LL Evaluate(Chess color);//����������(������������)
LL EvaluatePosition(Chess color, int x, int y);//����ĳһλ��
//LL PatternScore(Chess color, const string& line);//����ģʽ,���ظ���ƥ���ģʽ�ķ���
void UpdateScore(int x, int y);//���·�����

//��������
LL Alpha_Beta(Chess color, LL alpha, LL beta, int depth);//alpha-beta��֦
Point MakePlay(int depth);//��������

//������
void StartGame();//��Ϸ��ѭ��

//��������
struct PointComparator {
	bool operator()(const Point& a, const Point& b) const {
		return EvaluatePosition(field, a.x, a.y) > EvaluatePosition(field, b.x, b.y);
	}
};
void UpdateInfo(int x, int y) {
	UpdateScore(x, y);
}

set<Point, PointComparator> GetPossibleMoves(Chess color);//��ȡ���п��ܵ�����λ��

//position manager
struct HistoryPosition {
	HistoryPosition() : chosenPosition({ -1, -1 })
	{}
	Point chosenPosition;						//ѡ������ӵ�
	set<Point> addedPositions;	//����chosenPosition����ӵĿ������ӵ�
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
	//������(x,y)��Χ�˸�����ĵ�(Empty,�������̷�Χ��)���뵽currentPossiblePos��addedPositions��,ÿ�������������
	int dir[4][2] = { {1, 0}, {0, 1}, {1, 1}, {1, -1} };
	for (int j = 1; j < 3; j++) {
		for (int i = 0; i < 4; i++) {
			int dx = dir[i][0]*j, dy = dir[i][1]*j;
			if (isInBound(x + dx, y + dy) && board[x + dx][y + dy] == None) {
				auto insertRes = currentPossiblePos.insert({ x + dx, y + dy });
				if(insertRes.second)
					hp.addedPositions.insert({ x + dx, y + dy });
			}

			if (isInBound(x - dx, y - dy) && board[x - dx][y - dy] == None) {
				auto insertRes = currentPossiblePos.insert({ x - dx, y - dy });
				if (insertRes.second)
					hp.addedPositions.insert({ x - dx, y - dy });
			}
		}
	}
	//�ѵ�ǰ���currentPossiblePos��ɾ��
	currentPossiblePos.erase({ x, y });
	history.push_back(hp);
}

void PositionManager::RecoverLastState() {
	if (history.empty()) return;//��������ʷ��¼
	//ȡ�����һ����ʷ��¼
	HistoryPosition hp = history.back();
	history.pop_back();
	//�ָ�currentPossiblePos(ȥ��added,����chosen)
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
//�û���
//zobrist
// ����64λ�����
LL Random64() {
	return (LL)rand() | ((LL)rand() << 15) | ((LL)rand() << 30) | ((LL)rand() << 45) | ((LL)rand() << 60);
}

// ��ʼ���������
void RandomBoardZobristValue() {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < board_size; j++) {
			for (int k = 0; k < board_size; k++) {
				boardZobristValue[i][j][k] = Random64();
			}
		}
	}
}

// ��ʼ����ʼ�����Zobristֵ
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

// ����Zobristֵ
void UpdateZobristValue(int x, int y, Chess color) {
	currentZobristValue ^= boardZobristValue[(int)color - 1][x][y];
}
//�洢�Ѿ��������������֡�ÿ����Ŀ��������Ĺ�ϣֵ�����֡���Ⱥ���������
HashItem hashItems[hashIndexSize + 1];//�±귶Χ0-1111111111111111

//��¼�û�����Ϣ
void RecordHashItem(int depth, LL score, HashItem::Flag flag) {
	int index = currentZobristValue & hashIndexSize;
	//����������������¸����û���: 
		// 1.�û���Ϊ��    
		// 2.��ǰ�ݹ���ȴ����û������,˵����ǰ��Ϣ����׼,���滻���û����ڵ���Ϣ
	if (hashItems[index].flag == HashItem::EMPTY || hashItems[index].depth < depth) {
		hashItems[index].checksum = currentZobristValue;
		hashItems[index].depth = depth;
		hashItems[index].score = score;
		hashItems[index].flag = flag;
	}
}

//ȡ�û�����Ϣ
LL GetHashScore(int depth, int alpha, int beta) {
	int index = currentZobristValue & hashIndexSize;
	//��ֵ
	if (hashItems[index].flag == HashItem::EMPTY) {
		return hashNoneScore;
	}
	if (hashItems[index].checksum == currentZobristValue) {
		//����ϣ�����>=��ǰ���,�����,���ع�ϣ���е�ֵ
		if (hashItems[index].depth >= depth) {
			//׼ȷֵ��ֱ�ӷ���
			if (hashItems[index].flag == HashItem::EXACT) {
				return hashItems[index].score;
			}
			//alphaֵ˵���洢��ֵ��һ������ֵ(�����з���֦������µ����ֵ)
			if (hashItems[index].flag == HashItem::ALPHA && hashItems[index].score <= alpha) {
				return alpha;
			}
			//betaֵ˵���洢��ֵ��һ������ֵ(����������֦������µ���Сֵ)
			if (hashItems[index].flag == HashItem::BETA && hashItems[index].score >= beta) {
				return beta;
			}
		}
	}
	return hashNoneScore;
}

//AC�Զ���ʵ���ַ���ƥ��,���ڲ�������
class TrieNode;
class AC_Auto;
//Trie���ڵ�
class TrieNode {
public:
	friend class AC_Auto;
	TrieNode() : fail(-1), next{ -1, -1, -1 }, length(0), score(0) {}
private:
	char data;//�ڵ�洢���ַ�
	int fail;//ʧ��ָ��
	int next[3];//�ӽڵ�
	int length;//��ǰ�ڵ������ַ�������
	int score;//��ǰ�ڵ������ַ����ķ���
};

//AC�Զ���
class AC_Auto {
public:
	AC_Auto();
	void BuildTrieTree();
	void BuildFailPointer();
	LL PatternScore(const string& text);
private:
	vector<TrieNode> nodes;//Trie��
	vector<string> patterns;//ģʽ��
	vector<int> scores;//ģʽ����Ӧ�ķ���
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
			cur = nodes[cur].next[index];//curָ������һ���ַ�
		}
		nodes[cur].length = patterns[i].length();//��¼ģʽ���ĳ���
		nodes[cur].score = scores[i];//��¼����
	}
}

void AC_Auto::BuildFailPointer() {
	queue<int> q;
	for (int i = 0; i < 3; i++) {//root�ڵ���ӽڵ��failָ�붼ָ��root�ڵ�
		nodes[nodes[0].next[i]].fail = 0;
		q.push(nodes[0].next[i]);
	}
	while (!q.empty()) {
		int cur = q.front();//ȡ��ͷ
		q.pop();
		for (int i = 0; i < 3; ++i) {
			if (nodes[cur].next[i] != -1) {//�����ǰ�ڵ����ƥ����ӽڵ�
				int fail = nodes[cur].fail;//�ӽڵ��failָ���ʼ��Ϊ��ǰ�ڵ��failָ��
				while (nodes[fail].next[i] == -1) {//failָ��ָ��Ľڵ�û�ж�Ӧ���ӽڵ�,�����������ת
					fail = nodes[fail].fail;
				}
				nodes[nodes[cur].next[i]].fail = nodes[fail].next[i];//�ӽڵ��failָ��ָ��fail�ڵ�Ķ�Ӧ�ӽڵ�
				q.push(nodes[cur].next[i]);//���ӽڵ�������
			}
		}
	}
}

LL AC_Auto::PatternScore(const string& text) {
	int cur = 0;
	LL totalScore = 0;
	for (int i = 0; i < text.size(); i++) {
		int c = text[i] - '0';
		while (nodes[cur].next[c] == -1 && cur != 0) {//�����ǰ�ڵ�û�ж�Ӧ���ӽڵ�,��ƥ��ʧ��,��ת��failָ��
			cur = nodes[cur].fail;
		}
		cur = nodes[cur].next[c];//����ж�Ӧ���ӽڵ�,��ָ��ָ���ӽڵ�
		if (cur == -1) cur = 0;//�����ǰָ��curΪ-1,˵��ǰ���failָ��ȫ��ƥ��ʧ��,��ָ��ָ����ڵ�
		int temp = cur;
		while (temp != 0) {
			//���lengthΪ0,˵����ǰ�ڵ㲻��һ��ģʽ���Ľ�β,�򲻻����ѭ��
			//���length��Ϊ0,��˵����ǰ�ڵ���һ��ģʽ���Ľ�β,��������
			totalScore += nodes[temp].score;
			temp = nodes[temp].fail;
		}
	}
	return totalScore;
}

/******************************************************************************************************/
/***********************************************���Ժ���************************************************/
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

//DEBUG��ӡ����
#if _DEBUG_
//��ӡ����
void PrintBoard() {
	printf("\nDEBUG Board:\n");
	// ��ӡ�к�
	printf("  ");
	for (int j = 0; j < board_size; j++) {
		printf("\033[32m%x \033[0m", j); // ��ɫ
	}
	printf("\n");

	for (int i = 0; i < board_size; i++) {
		// ��ӡ�к�
		printf("\033[32m%x \033[0m", i); // ��ɫ
		for (int j = 0; j < board_size; j++) {
			if (board[i][j] == None) {
				printf("0 ");
			}
			else if (board[i][j] == Black) {
				printf("\033[31m1 \033[0m"); // ��ɫ
			}
			else {
				printf("\033[33m2 \033[0m"); // ��ɫ
			}
		}
		printf("\n");
	}
	printf("\n");
	fflush(stdout);
}
#endif
/***********************************************���Ժ���************************************************/
//*****************************************************************************************************
//����ʵ��
set<Point, PointComparator> GetPossibleMoves(Chess color) {
	set<Point, PointComparator> moves;
	int minX = board_size, maxX = 0, minY = board_size, maxY = 0;

	// �ҵ���ǰ��ֵ��������ҡ����ϡ����µ�
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

	// ��չ2��ķ�Χ
	minX = max(0, minX - 2);
	maxX = min(board_size - 1, maxX + 2);
	minY = max(0, minY - 2);
	maxY = min(board_size - 1, maxY + 2);

	// ����չ��ķ�Χ��Ѱ�ҿ��ܵ�����λ��
	for (int i = minX; i <= maxX; i++) {
		for (int j = minY; j <= maxY; j++) {
			if (board[i][j] == None) {
				if (EvaluatePosition(color, i, j) > 0)
					moves.insert({ i, j });//�������������Ӵ�С����
			}
		}
	}

	return moves;
}


Point MakePlay(int depth) {
#if _TIMER_
	Timer t("Alpha_Beta����");
#endif
	// ��ʼ�� bestMove
	bestMove = { {-1, -1}, MIN_SCORE };
	//�ݹ�,����ģ��λ�õķ���
	Alpha_Beta(field, MIN_SCORE, MAX_SCORE, depth);
	board[bestMove.p.x][bestMove.p.y] = field;
	UpdateInfo(bestMove.p.x, bestMove.p.y);
	ppm.AddPossiblePos(bestMove.p.x, bestMove.p.y);
	return bestMove.p;
}

LL Alpha_Beta(Chess color, LL alpha, LL beta, int depth) {
	HashItem::Flag flag = HashItem::ALPHA;
	LL score = GetHashScore(depth, alpha, beta);
	//�ӵڶ��εݹ鿪ʼ,����û�������ֵ,��ֱ�ӷ���
	if (score != hashNoneScore && depth != dpth) {
		return score;
	}
	LL score_self = Evaluate(color);
	LL score_oppo = Evaluate((color == Black) ? White : Black);
	//�ݹ鵽���һ��,ֱ�ӷ�������ֵ,��Exact��ʽ�����û���
	if (depth == 0) {
		RecordHashItem(depth, score_self - score_oppo, HashItem::EXACT);
		return score_self - score_oppo;
	}
	//���������Է�����,��ֱ�ӷ���MAX����,�����dpth-depth��Ϊ���ñ���������(depth�����)���������
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
		//ģ������
		board[p.x][p.y] = color;
		UpdateZobristValue(p.x, p.y, color);
		UpdateInfo(p.x, p.y);
		ppm.AddPossiblePos(p.x, p.y);

		//�ݹ�,����ģ��λ�õķ���
		LL val = -Alpha_Beta(oppo, -beta, -alpha, depth - 1);//�Զ����ӽǽ�������
		//��ԭ����
		board[p.x][p.y] = None;
		UpdateZobristValue(p.x, p.y, color);
		UpdateInfo(p.x, p.y);
		ppm.RecoverLastState();
		//alpha-beta��֦
		//�����ӽ�,ȡ��Сֵ,�����ǰֵ�Ѿ�����beta��,����ֲ���ѡ�����λ��,ֱ�ӷ���beta
		if (val >= beta) {
			RecordHashItem(depth, beta, HashItem::BETA);
			return beta;
		}
		//�����ӽ�,ȡ���ֵ
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
	//������UpdateScore,���ǲ��Ǹ���,���Ǽ��㲢����һ��Noneλ������color�õ��ķ���(����-�з�),����ģ�����ӵĹ���
	//����ʱֻ��������(ˮƽʱ)��5����������
	if (board[x][y] != None) return 0;
	board[x][y] = color;
	Chess opp = (color == Black) ? White : Black;
	//��ʼ��pattern
	string myPattern[4];//��СΪ4,�ֱ�洢��,��,����-����,����-���µ�����pattern
	string oppoPattern[4];
	//����
	for (int i = max(0, x - 5); i < min(board_size, x + 6); i++) {
		myPattern[0] += (board[i][y] == color) ? '1' : (board[i][y] == None ? '0' : '2');
		oppoPattern[0] += (board[i][y] == opp) ? '1' : (board[i][y] == None ? '0' : '2');
	}
	//����
	for (int i = max(0, y - 5); i < min(board_size, y + 6); i++) {
		myPattern[1] += (board[x][i] == color) ? '1' : (board[x][i] == None ? '0' : '2');
		oppoPattern[1] += (board[x][i] == opp) ? '1' : (board[x][i] == None ? '0' : '2');
	}
	//����-����
	for (int i = max(0, x - min(5, min(x, y))), j = max(0, y - min(5, min(x, y))); i < min(board_size, x + 6) && j < min(board_size, y + 6); i++, j++) {
		myPattern[2] += (board[i][j] == color) ? '1' : (board[i][j] == None ? '0' : '2');
		oppoPattern[2] += (board[i][j] == opp) ? '1' : (board[i][j] == None ? '0' : '2');
	}
	//����-����
	for (int i = max(0, x + min(5, min(y, board_size - 1 - x))), j = max(0, y - min(5, min(y, board_size - 1 - x))); i >= 0 && j < min(board_size, y + 6); i--, j++) {
		myPattern[3] += (board[i][j] == color) ? '1' : (board[i][j] == None ? '0' : '2');
		oppoPattern[3] += (board[i][j] == opp) ? '1' : (board[i][j] == None ? '0' : '2');
	}

	LL myScore = 0;
	LL oppoScore = 0;

	for (int i = 0; i < 4; i++) {
		//���㼺������
		myScore += AC_Searcher.PatternScore(myPattern[i]);
		//����Է�����
		oppoScore += AC_Searcher.PatternScore(oppoPattern[i]);
	}

	//myScore��oppoScore�ֱ��ȥģ������ǰ����ķ���
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
	string myPattern[4];//��СΪ4,�ֱ�洢��,��,����-����,����-���µ�����pattern
	string oppoPattern[4];
	//����
	for (int i = 0; i < board_size; i++) {
		myPattern[0] += (board[x][i] == field) ? '1' : (board[x][i] == None ? '0' : '2');
		oppoPattern[0] += (board[x][i] == opponent) ? '1' : (board[x][i] == None ? '0' : '2');
	}
	//����
	for (int i = 0; i < board_size; i++) {
		myPattern[1] += (board[i][y] == field) ? '1' : (board[i][y] == None ? '0' : '2');
		oppoPattern[1] += (board[i][y] == opponent) ? '1' : (board[i][y] == None ? '0' : '2');
	}
	//����-����
	for (int i = x - min(x, y), j = y - min(x, y); i < board_size && j < board_size; i++, j++) {
		myPattern[2] += (board[i][j] == field) ? '1' : (board[i][j] == None ? '0' : '2');
		oppoPattern[2] += (board[i][j] == opponent) ? '1' : (board[i][j] == None ? '0' : '2');
	}
	//����-����
	for (int i = x + min(board_size - 1 - x, y), j = y - min(board_size - 1 - x, y); i >= 0 && j < board_size; i--, j++) {
		myPattern[3] += (board[i][j] == field) ? '1' : (board[i][j] == None ? '0' : '2');
		oppoPattern[3] += (board[i][j] == opponent) ? '1' : (board[i][j] == None ? '0' : '2');
	}

	LL myScore[4] = { 0 };
	LL oppoScore[4] = { 0 };

	//�������

	for (int i = 0; i < 4; i++) {
		//���㼺������
		myScore[i] += AC_Searcher.PatternScore(myPattern[i]);
		//����Է�����
		oppoScore[i] += AC_Searcher.PatternScore(oppoPattern[i]);
	}

	//���·���
	//�ȼ�ȥԭ���ķ���
	for (int i = 0; i < 2; i++) {
		all_score[i] -= point_score[i][0][x];
		all_score[i] -= point_score[i][1][y];
		all_score[i] -= point_score[i][2][x - y + board_size];
		all_score[i] -= point_score[i][3][x + y];
	}
	//����point_score
	point_score[0][0][x] = myScore[0];
	point_score[0][1][y] = myScore[1];
	point_score[0][2][x - y + board_size] = myScore[2];
	point_score[0][3][x + y] = myScore[3];
	point_score[1][0][x] = oppoScore[0];
	point_score[1][1][y] = oppoScore[1];
	point_score[1][2][x - y + board_size] = oppoScore[2];
	point_score[1][3][x + y] = oppoScore[3];

	//�ټ����µķ���
	for (int i = 0; i < 2; i++) {
		all_score[i] += point_score[i][0][x];
		all_score[i] += point_score[i][1][y];
		all_score[i] += point_score[i][2][x - y + board_size];
		all_score[i] += point_score[i][3][x + y];
	}
}

//LL PatternScore(Chess color, const string& line) {
//	//���line����1,��ֱ�ӷ���0
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
	//��ʼ��Zobrist
	RandomBoardZobristValue();
	InitCurrentZobristValue();
	//��ʼAC�Զ���
	AC_Searcher.BuildTrieTree();
	AC_Searcher.BuildFailPointer();
	//��ʼ������
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

//������
void StartGame() {
	char cmd[10];
	//��ѭ��
	while (1) {
		// ����ָ��
		int color = 0;
		scanf("%s %d", cmd, &color);
		// ���ָ���Ƿ�Ϊ START
		if (strcmp(cmd, "START") == 0 && (color == 1 || color == 2)) {
			field = (color == 1) ? Black : White;
			printf("OK\n");
			fflush(stdout);
			opponent = (field == Black) ? White : Black;
			InitGame();
			break;
		}
	}
	//��ʼ��Ϸ
	while (1) {
		scanf("%s", cmd);

		if (strcmp(cmd, "TURN") == 0) {//��������
			Point p = MakePlay(dpth);
#if _DEBUG_
			PrintBoard();
#endif
			printf("%d %d\n", p.x, p.y);
			fflush(stdout);
		}
		else if (strcmp(cmd, "PLACE") == 0) {//�Է�����
			int x, y;
			scanf("%d %d", &x, &y);
			board[x][y] = opponent;
			UpdateZobristValue(x, y, opponent);
			UpdateInfo(x, y);
			ppm.AddPossiblePos(x, y);
		}
		//�ж���Ϸ����
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

//main����
int main() {
	StartGame();
	return 0;
}
