#pragma once

#include "CObjects.h"

const auto INF = 1e18;
const size_t nRecourseMax = 20;
typedef std::unordered_set<TConnectorDirection> TDirectionSet;

struct TDirectionsSet
{
	TDirectionSet straight;
	TDirectionSet opposite;
};

struct Node;
struct Edge //����� - ��� ��� � � ����� ������� ��� ����������
{
	Edge(Node* node, int w) :
		indexTo(node),
		weight(w)
	{}
	int weight;
	Node *indexTo;
};

struct Node   //������� �� ������� ��������� �� �� �����
{
	Node(POINT start, POINT finish, int w, std::vector<CRect*> *aRects, TPixelArray *aPixel, Node* prevNode = nullptr);
	~Node();
	bool TestPoint(int nX, int nY);
	Node* CreateNode(int nX, int nY);
	Node* BuldEdges(TNodeDeque& deque1, TNodeDeque& deque2,TNodeDeque& deque3);
	std::vector<Edge*> edges;
	Node* prevIndex;
	int weight;
	bool visited;
	POINT m_Start;
	POINT m_Finish;
	TPixelArray* m_aPixel;
	std::vector<CRect*>* m_aRects;
	static size_t nRecourseDepths;
};

struct Graph // ��������� ������ ����� (TPixel ??)
{
	std::vector<Node*> nodes; // ������ ������ (m_aPixels ??)
	TPixelArray m_aPixels;

	void clear();
	void clear_edges();
	void init_start_values();
};
bool CalculateSidePoints(CRect* pStartRect, CRect* pFinishRect, TPixelArray* screen, TExitPointsArrays& resultPoints);
void MakeTunnels(CRect* rect, TPixelArray* aPixels);
TConnectorDirection CalculateDirection(POINT s, POINT f);


