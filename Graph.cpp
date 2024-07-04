#include "Graph.h"

void MakeTunnels(CRect* rect, TPixelArray* aPixels)
{
	POINT c = rect->GetCenter();
	RECT r = rect->GetRect();
	int left = r.left - MIN_MARGINE < 0 ? 0 : r.left - MIN_MARGINE;
	int right = r.right + MIN_MARGINE > (*aPixels).size() - 1 ? (*aPixels).size() - 1 : r.right + MIN_MARGINE;
	int top = r.top - MIN_MARGINE < 0 ? 0 : r.top - MIN_MARGINE;
	int bottom = r.bottom + MIN_MARGINE > (*aPixels)[0].size() - 1 ? (*aPixels)[0].size() - 1 : r.bottom + MIN_MARGINE;
	for (int i = left; i <= right; i++)
		(*aPixels)[i][c.y].pOccupied = nullptr;
	for (int i = top; i <= bottom; i++)
		(*aPixels)[c.x][i].pOccupied = nullptr;
}

TConnectorDirection CalculateDirection(POINT s, POINT f)
{
	if (s.x == f.x && s.y == f.y)
		return cdNothing;
	else if (s.x == f.x && s.y > f.y)
		return cdUp;
	else if (s.x < f.x && s.y > f.y)
		return cdUpRight;
	else if (s.x < f.x && s.y == f.y)
		return cdRight;
	else if (s.x < f.x && s.y < f.y)
		return cdDownRight;
	else if (s.x == f.x && s.y < f.y)
		return cdDown;
	else if (s.x > f.x && s.y < f.y)
		return cdDownLeft;
	else if (s.x > f.x && s.y == f.y)
		return cdLeft;
	else if (s.x > f.x && s.y > f.y)
		return cdUpLeft;
	else
		return cdNothing;
}


//Функция вычисляет точки входа/выхода с учетом указанного минимального расстояния до прямоугольника.
bool CalculateSidePoints(CRect* pStartRect, CRect* pFinishRect, TPixelArray* screen, TExitPointsArrays& resultPoints)
{
#define MIN_MARGINE 11

	auto TestBorders = [screen](int x, int y)->bool
		{
			RECT s = { 0,0,(*screen).size(), (*screen)[0].size() };
			return (x >= s.left && y >= s.top && x < s.right && y < s.bottom && !(*screen)[x][y].pOccupied);
		};

	std::vector<TDirectionPoint> startExitPoints;
	std::vector<TDirectionPoint> finishExitPoints;

	POINT startCentralPoint = pStartRect->GetCenter();
	POINT finishCentralPoint = pFinishRect->GetCenter();
	RECT startRect = pStartRect->GetRect();
	RECT finishRect = pFinishRect->GetRect();
	resultPoints.eDirection = ::CalculateDirection(pStartRect->GetCenter(), pFinishRect->GetCenter());

	if (TestBorders(startCentralPoint.x, startRect.top - MIN_MARGINE))
		startExitPoints.push_back({ { startCentralPoint.x, startRect.top - MIN_MARGINE }, cdUp }); //Выход вверх
	if (TestBorders(startRect.right + MIN_MARGINE, startCentralPoint.y))
		startExitPoints.push_back({ { startRect.right + MIN_MARGINE, startCentralPoint.y }, cdRight }); //Выход вправо
	if (TestBorders(startCentralPoint.x, startRect.bottom + MIN_MARGINE))
		startExitPoints.push_back({ { startCentralPoint.x, startRect.bottom + MIN_MARGINE, }, cdDown }); //Выход вниз
	if (TestBorders(startRect.left - MIN_MARGINE, startCentralPoint.y))
		startExitPoints.push_back({ { startRect.left - MIN_MARGINE, startCentralPoint.y }, cdLeft }); //Выход влево

	if (TestBorders(finishCentralPoint.x, finishRect.top - MIN_MARGINE))
		finishExitPoints.push_back({ { finishCentralPoint.x, finishRect.top - MIN_MARGINE,  }, cdUp }); //Выход вверх
	if (TestBorders(finishRect.right + MIN_MARGINE, finishCentralPoint.y))
		finishExitPoints.push_back({ { finishRect.right + MIN_MARGINE, finishCentralPoint.y }, cdRight }); //Выход вправо
	if (TestBorders(finishCentralPoint.x, finishRect.bottom + MIN_MARGINE))
		finishExitPoints.push_back({ { finishCentralPoint.x, finishRect.bottom + MIN_MARGINE }, cdDown }); //Выход вниз
	if (TestBorders(finishRect.left - MIN_MARGINE, finishCentralPoint.y))
		finishExitPoints.push_back({ { finishRect.left - MIN_MARGINE, finishCentralPoint.y }, cdLeft }); //Выход влево

	//--------------------------
	//if (TestBorders(startCentralPoint.x, startRect.top))
	//	startExitPoints.push_back({ { startCentralPoint.x, startRect.top }, cdUp }); //Выход вверх
	//if (TestBorders(startRect.right, startCentralPoint.y))
	//	startExitPoints.push_back({ { startRect.right, startCentralPoint.y }, cdRight }); //Выход вправо
	//if (TestBorders(startCentralPoint.x, startRect.bottom))
	//	startExitPoints.push_back({ { startCentralPoint.x, startRect.bottom, }, cdDown }); //Выход вниз
	//if (TestBorders(startRect.left, startCentralPoint.y))
	//	startExitPoints.push_back({ { startRect.left, startCentralPoint.y }, cdLeft }); //Выход влево

	//if (TestBorders(finishCentralPoint.x, finishRect.top))
	//	finishExitPoints.push_back({ { finishCentralPoint.x, finishRect.top,  }, cdUp }); //Выход вверх
	//if (TestBorders(finishRect.right, finishCentralPoint.y))
	//	finishExitPoints.push_back({ { finishRect.right, finishCentralPoint.y }, cdRight }); //Выход вправо
	//if (TestBorders(finishCentralPoint.x, finishRect.bottom))
	//	finishExitPoints.push_back({ { finishCentralPoint.x, finishRect.bottom }, cdDown }); //Выход вниз
	//if (TestBorders(finishRect.left, finishCentralPoint.y))
	//	finishExitPoints.push_back({ { finishRect.left, finishCentralPoint.y }, cdLeft }); //Выход влево
	//--------------------------

	resultPoints.start = startExitPoints;
	resultPoints.finish = finishExitPoints;

	return !(resultPoints.start.empty() || resultPoints.finish.empty());
}


//Функция определяет направление с наивысшим приоритетом и с низшим приоритетом.
//На выходе - структура из двух множеств напрвлений.
static TDirectionsSet CalculateDirectionSet(POINT s, POINT f)
{
	TDirectionsSet st;
	if (s.x == f.x && s.y == f.y)
		st.straight.insert(cdNothing),st.opposite.insert(cdNothing);
	else if (s.x == f.x && s.y > f.y)
		st.straight.insert(cdUp),st.opposite.insert(cdDown);
	else if (s.x < f.x && s.y > f.y)
		st.straight.insert(cdUp), st.straight.insert(cdRight), st.opposite.insert(cdDown), st.opposite.insert(cdLeft);
	else if (s.x < f.x && s.y == f.y)
		st.straight.insert(cdRight), st.opposite.insert(cdLeft);
	else if (s.x < f.x && s.y < f.y)
		st.straight.insert(cdDown), st.straight.insert(cdRight), st.opposite.insert(cdUp), st.opposite.insert(cdLeft);
	else if (s.x == f.x && s.y < f.y)
		st.straight.insert(cdDown), st.opposite.insert(cdUp);
	else if (s.x > f.x && s.y < f.y)
		st.straight.insert(cdDown), st.straight.insert(cdLeft), st.opposite.insert(cdUp), st.opposite.insert(cdRight);
	else if (s.x > f.x && s.y == f.y)
		st.straight.insert(cdLeft), st.opposite.insert(cdRight);
	else if (s.x > f.x && s.y > f.y)
		st.straight.insert(cdUp), st.straight.insert(cdLeft), st.opposite.insert(cdDown), st.opposite.insert(cdRight);
	else
		st.straight.insert(cdNothing);
	return st;
}
Node::Node(POINT start, POINT finish, int w, std::vector<CRect*> *aRects, TPixelArray* aPixel, Node* prevNode/* = nullptr*/) :
	m_Start(start),
	m_Finish(finish),
	weight(w),
	edges(0),
	m_aPixel(aPixel),
	m_aRects(aRects),
	visited(false),
	prevIndex(prevNode)

{
	(*m_aPixel)[m_Start.x][m_Start.y].pNode = this;
}


Node::~Node()
{
	(*m_aPixel)[m_Start.x][m_Start.y].pNode = nullptr;

/*
	for (int i = 0; i < edges.size() - 1; i++)
	{
		delete edges[i]->indexTo;
		delete edges[i];
	}
*/	
	for (auto itr : edges)
	{
//		delete itr->indexTo; //Убрал рекурсивное удаление нод, опять стека не хватило))) Сейчас ноды удаляются во внешнем цикле.
		delete itr;
	}
	edges.clear();
}

Node* Node::BuldEdges(TNodeDeque& deque1, TNodeDeque& deque2, TNodeDeque& deque3)
{
	visited = true;
	if (m_Start.x == m_Finish.x && m_Start.y == m_Finish.y)
		return this;
	TDirectionsSet aDirection = ::CalculateDirectionSet(m_Start, m_Finish);
	TConnectorDirection a[] = { cdUp,cdDown,cdLeft,cdRight };
	for (auto itr : a)
	{
		int I = (int)itr;
		if (TestPoint(m_Start.x + aDir[I].x, m_Start.y + aDir[I].y))
			if (aDirection.straight.find(itr) != aDirection.straight.end())//Нода с высшим приоритетом
				deque1.push_back(CreateNode(m_Start.x + aDir[I].x, m_Start.y + aDir[I].y));
			else if (aDirection.opposite.find(itr) != aDirection.opposite.end())//Нода с низшим приоритетом
				deque3.push_back(CreateNode(m_Start.x + aDir[I].x, m_Start.y + aDir[I].y));
			else //Оставшиеся ноды - со средним приоритетом.
				deque2.push_back(CreateNode(m_Start.x + aDir[I].x, m_Start.y + aDir[I].y));
	}
	return nullptr;
}

bool Node::TestPoint(int nX, int nY)
{
	bool bInRange = nX >= 0 && nY >= 0 && nX < (*m_aPixel).size() && nY < (*m_aPixel)[0].size();
	if (bInRange)
	{
		CRect* pRect = (*m_aPixel)[nX][nY].pOccupied;
		bool bOccupied = pRect != nullptr;
		return !bOccupied && !(*m_aPixel)[nX][nY].pNode;
	}
	return false;
}

Node* Node::CreateNode(int nX, int nY)
{
	Node* pNode = new Node({ nX, nY }, m_Finish, weight + 1, m_aRects, m_aPixel, this);
	Edge* pEdge = new Edge(pNode,weight+1);
	(*m_aPixel)[nX][nY].pNode = pNode;
	edges.push_back(pEdge);
	return pNode;
}


void Graph::clear() // очищаем весь граф
{
	m_aPixels.clear();
}

void Graph::clear_edges() // очищаем список ребер
{
/*	for (auto node : m_aPixels)
		for (auto node1 : node)
		{
			node1.bVisited
				node.edges.clear();
		}*/
}

void Graph::init_start_values() // инициализируем стартовые параметры
{
	for (auto& node : m_aPixels)
	{
//		node.weight = INF; // помечаем все расстояния до всех вершин как бесконечность
//		node.visited = false; // помечаем, что эти вершины ещё не были посещены
		//node.prevIndex = -1; // напр. мы в эту вершину пока не попали 
//		node.prevIndex = INF;
	}
}
/*
bool read_nodes(std::istream& istream, std::size_t nodes_count, Graph& graph_out) // считываем каждую вершину
{
	graph_out.m_aPixels.clear();

	for (std::size_t i = 0; i < nodes_count; i++)
	{
		decltype(Node::id) id;
		istream >> id;
		graph_out.m_aPixels.push_back({ id });
	}

	return true;
}

bool read_edges(std::istream& istream, std::size_t edges_count, Graph& graph_out) // считываем каждое ребро (3 параметра: id начала ребра id конца ребра и вес ребра)
{
	graph_out.clear_edges();

	for (std::size_t i = 0; i < edges_count; i++)
	{
		int start_id, end_id; // id вершины начала ребра id вершины конца ребра
		int weight;           // вес этого ребра

		istream >> start_id >> end_id;
		istream >> weight;

		auto& nodes_ref = graph_out.m_aPixels;

		// валидны ли ребра (ищем начало и ищем конец)
		auto start_iter = std::find_if(nodes_ref.begin(), nodes_ref.end(), [start_id](const auto& node) { return node.id == start_id; });
		auto end_iter = std::find_if(nodes_ref.begin(), nodes_ref.end(), [end_id](const auto& node) { return node.id == end_id; });

		if (start_iter == nodes_ref.end() || end_iter == nodes_ref.end())
		{
			graph_out.clear_edges();

			return false; // если не находим, то выходим отсюда
		}
		std::size_t index = (end_iter - nodes_ref.begin());
		(*start_iter).edges.push_back(Edge{ weight, index });  // иначе в вершину начала укладываем новое ребро (хранит в себе вес и индкс ребра, на которое будет указывать)
	}

	return true;
}

*/