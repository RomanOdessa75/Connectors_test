#include "CObjects.h"
#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <cstddef> // Для std::size_t

#include <windows.h>
#include <wincodec.h>
#include <gdiplus.h>
//-----------------
#include "graph.h"

static bool IsIntersect(RECT o, RECT r)
{
	if (
		(o.left >= r.left && o.top >= r.top && o.left <= r.right && o.top <= r.bottom) || //Стартовая точка отрезка внутри
		(o.right >= r.left && o.bottom >= r.top && o.right <= r.right && o.bottom <= r.bottom) //Финишная точка отрезка внутри
		)
		return true;//Один из концов отрезка внутри

	if (o.top == o.bottom) //Горизонтальный отрезок
		if ((o.left < r.left && o.right > r.right) && //X по разные стороны
			(o.top >= r.top && o.bottom <= r.bottom)) //Y внутри
			return true;

	if (o.left == o.right) //Вертикальный отрезок
		if ((o.top < r.top && o.bottom > r.bottom) && //Y по разные стороны
			(o.left >= r.left && o.right <= r.right)) //X внутри
			return true;

	return false;
}

CManager::CManager(HWND hWnd) :
	m_pMovingRect(nullptr),
	m_nhWnd(hWnd),
	m_aObjects(0),
	m_aConnectors(0),
	m_nStartX(0),
	m_nStartY(0)
{
	RECT rect;
	GetClientRect(m_nhWnd, &rect);
	m_aPixels.resize(rect.right + 1);
	for (auto i = 0; i <= rect.right; i++)
	{
		m_aPixels[i].resize(rect.bottom + 1);
		for (auto j = 0; j <= rect.bottom; j++)
			m_aPixels[i][j] = { false,nullptr,nullptr };
	}
	return;
}

CManager::~CManager()
{
	ClearAll();
}

void CManager::ClearAll()
{
	HDC dc = GetDC(m_nhWnd);

	for (auto itr : m_aObjects)
	{
		itr->Clear(dc);
		delete itr;
	}
	m_aObjects.clear();

	for (auto itr : m_aConnectors)
	{
		itr->Clear(dc);
		delete itr;
	}
	m_aConnectors.clear();

	ReleaseDC(m_nhWnd, dc);

}

CRect* CManager::GetObject(int nIndex)
{
	if (nIndex < m_aObjects.size())
		return (CRect*)m_aObjects[nIndex];
	return nullptr;
}

CConnector* CManager::GetConnector()
{
	if (m_pMovingRect)
	{
		for (auto itr : m_aConnectors)
			if (((CConnector*)itr)->IsConnect(m_pMovingRect))
				return (CConnector*)itr;
	}
	return nullptr;
}

CConnector* CManager::GetConnector(int nIndex)
{
	if (nIndex < m_aConnectors.size())
		return (CConnector*)m_aConnectors[nIndex];
	return nullptr;
}

int CManager::AddObject(RECT rCoordinates, COLORREF nColor)
{
	m_aObjects.push_back(new CRect(rCoordinates, nColor));
	return m_aObjects.size() - 1;
}

int CManager::AddConnector(COLORREF nColor, CRect* firstRect, CRect* secondRect)
{
	m_aConnectors.push_back(new CConnector(nColor, firstRect, secondRect, &m_aPixels));
	return m_aConnectors.size() - 1;
}

bool CManager::TryToCaptureObject(int nStartX, int nStartY)
{
	for (auto itr : m_aObjects)
		if (((CRect*)itr)->TestHit(nStartX, nStartY))
		{
			m_pMovingRect = (CRect*)itr;
			m_pMovingRect->SetMoving(true);
			BeginMove(nStartX, nStartY);
			return true;
		}
	return false;
}

void CManager::Draw(bool bRectOnly/* = false*/)
{
	HDC dc = GetDC(m_nhWnd);

	if (dc)
	{
		for (auto rect : m_aObjects)
		{
			if (m_pMovingRect != rect)
			{
				SetAreaOccupied(rect, true);
				rect->TestIntersected(dc, m_aObjects);
				rect->Draw(dc);
				if (rect->IsIntersect(m_pMovingRect))
					m_pMovingRect->Draw(dc);
			}
		}

		if (!bRectOnly)
			for (auto conn : m_aConnectors)
			{
				conn->TestIntersected(dc, m_aObjects, true);
				conn->Draw(dc);
			}

		ReleaseDC(m_nhWnd, dc);

	}

}

void CManager::ReDrawAll()
{
	RECT winRect;
	GetClientRect(m_nhWnd, &winRect);
	HDC dc = GetDC(m_nhWnd);
	HBRUSH brush = CreateSolidBrush(GetDCBrushColor(dc));
	FillRect(dc, &winRect, brush);
	DeleteObject(brush);
	Draw();
	ReleaseDC(m_nhWnd, dc);
}

void CManager::MoveTo(int nX, int nY)
{
	if (m_pMovingRect)
	{
		m_pMovingRect->SetIntersect(false);
		SetAreaOccupied(m_pMovingRect, false);
		m_pMovingRect->MoveTo(nX - m_nStartX, nY - m_nStartY);
		m_nStartX = nX;
		m_nStartY = nY;

		HDC dc = GetDC(m_nhWnd);
		if (dc)
		{
			Draw();

			m_pMovingRect->Draw(dc);
			SetAreaOccupied(m_pMovingRect, true);

			ReleaseDC(m_nhWnd, dc);
		}
	}
}

void CManager::BeginMove(int nStartX, int nStartY)
{
	m_nStartX = nStartX;
	m_nStartY = nStartY;
}

void CManager::EndMove()
{
	m_nStartX = m_nStartY = 0;
	if (m_pMovingRect)
	{
		CConnector* pConnector = GetConnector();
		if (nullptr != pConnector)
		{
			HDC dc = GetDC(m_nhWnd);
			pConnector->Clear(dc);
			pConnector->BuildPath(dc, &m_aObjects);
			pConnector->TestIntersected(dc, m_aObjects, true);
			pConnector->Draw(dc);
			ReleaseDC(m_nhWnd, dc);
		}
		m_pMovingRect->SetMoving(false);
	}
	m_pMovingRect = nullptr;
}

void CManager::BuildPathForAllConnectors()
{
	HDC dc = GetDC(m_nhWnd);

	for (auto itr : m_aConnectors)
		((CConnector*)itr)->BuildPath(dc, &m_aObjects);
	ReleaseDC(m_nhWnd, dc);
}


void CManager::SetAreaOccupied(CBaseObjects* pObect, bool bOccupied)
{
	RECT rect = ((CRect*)pObect)->GetRect();
	if (rect.left < MIN_MARGINE)
		rect.left = 0;
	else
		rect.left -= MIN_MARGINE;
	if (rect.right + MIN_MARGINE > m_aPixels.size() - 1)
		rect.right = m_aPixels.size() - 1;
	else
		rect.right += MIN_MARGINE;
	if (rect.top < MIN_MARGINE)
		rect.top = 0;
	else
		rect.top -= MIN_MARGINE;
	if (rect.bottom + MIN_MARGINE > m_aPixels[0].size() - 1)
		rect.bottom = m_aPixels[0].size() - 1;
	else
		rect.bottom += MIN_MARGINE;

	for (auto i = rect.left; i <= rect.right; i++)
		for (auto j = rect.top; j <= rect.bottom; j++)
			if (bOccupied)
				m_aPixels[i][j].pOccupied = (CRect*)pObect;
			else
				m_aPixels[i][j].pOccupied = nullptr;
}

void CManager::RecalcPixels()
{
	for (auto itr : m_aObjects)
		SetAreaOccupied(itr, true);
}

void CManager::ShowOccupied()
{

	HDC dc = GetDC(m_nhWnd);
	if (dc)
	{
		RECT rect = { 0,0,m_aPixels.size() - 1,m_aPixels[0].size() - 1 };

		HBRUSH brush = CreateSolidBrush(0x00FF00);

		FillRect(dc, &rect, brush);
		DeleteObject(brush);

		brush = CreateSolidBrush(0x0000FF);
		HBRUSH brush1 = CreateSolidBrush(0xFF00FF);
		for (auto i = rect.left; i <= rect.right; i++)
			for (auto j = rect.top; j <= rect.bottom; j++)
			{
				if (m_aPixels[i][j].pOccupied)
				{
					RECT r = { i,j,i + 1,j + 1 };
					FillRect(dc, &r, brush);
				}
				if (m_aPixels[i][j].pNode)
				{
					RECT r = { i,j,i + 1,j + 1 };
					FillRect(dc, &r, brush1);
				}
			}
		DeleteObject(brush);
		DeleteObject(brush1);
		ReleaseDC(m_nhWnd, dc);
	}

}


void CBaseObjects::TestIntersected(HDC dc, TBaseObjectsArray aObjects, bool bIteratorDraw/* = false*/)
{
	SetIntersect(false);
	for (auto test : aObjects)
		if (test != this)
			if (IsIntersect(test))
			{
				SetIntersect(true);
				if (bIteratorDraw)
					test->Draw(dc);
				else
					break;
			}
}

//------------------------------------------------------------------------------------------------------------------
void CRect::Draw(HDC dc)
{
	if (m_bNeedUpdate)
		Clear(dc);

	HBRUSH brush = nullptr;

	if (GetIntersected())
		brush = CreateHatchBrush(5, m_nBackground);
	else
		brush = CreateSolidBrush(m_nBackground);


	FillRect(dc, &m_rCoordinates, brush);


	DeleteObject(brush);

	m_rCachedCoordinates = m_rCoordinates;
	m_bNeedUpdate = false;
}

void CRect::Clear(HDC dc)
{
	HBRUSH brush = CreateSolidBrush(GetDCBrushColor(dc));
	FillRect(dc, &m_rCachedCoordinates, brush);
	DeleteObject(brush);
}

void CRect::MoveTo(int nDX, int nDY)
{
	if (!m_bNeedUpdate)
		m_rCachedCoordinates = m_rCoordinates;

	m_rCoordinates.left += nDX;
	m_rCoordinates.right += nDX;
	m_rCoordinates.top += nDY;
	m_rCoordinates.bottom += nDY;

	m_bNeedUpdate = true;
}


POINT CRect::GetCenter()
{
	return { (m_rCoordinates.right - m_rCoordinates.left) / 2 + m_rCoordinates.left,
			 (m_rCoordinates.bottom - m_rCoordinates.top) / 2 + m_rCoordinates.top };
}

bool CRect::TestHit(int nX, int nY)
{
	return	nX >= m_rCoordinates.left && nX <= m_rCoordinates.right &&
		nY >= m_rCoordinates.top && nY <= m_rCoordinates.bottom;
}

bool CRect::IsIntersect(CBaseObjects* pObject)
{
	if (pObject)
	{
		CRect* rect = (CRect*)pObject;
		if (rect == this)
			return false;
		RECT r = rect->GetRect();
		if (::IsIntersect({ m_rCoordinates.left, m_rCoordinates.top, m_rCoordinates.right, m_rCoordinates.top }, r))
			return true;
		if (::IsIntersect({ m_rCoordinates.right, m_rCoordinates.top, m_rCoordinates.right, m_rCoordinates.bottom }, r))
			return true;
		if (::IsIntersect({ m_rCoordinates.left, m_rCoordinates.bottom, m_rCoordinates.right, m_rCoordinates.bottom }, r))
			return true;
		if (::IsIntersect({ m_rCoordinates.left, m_rCoordinates.top, m_rCoordinates.left, m_rCoordinates.bottom }, r))
			return true;
	}
	return false;
}

//--------------------------------------------------------------------------------
void CConnector::CalculateDirection()
{
	if (m_aRect.size() != 2)
		return;
	POINT s = m_aRect[0]->GetCenter();
	POINT f = m_aRect[1]->GetCenter();
	if (s.x == f.x && s.y == f.y)
		m_eDirection = cdNothing;
	else if (s.x == f.x && s.y > f.y)
		m_eDirection = cdUp;
	else if (s.x < f.x && s.y > f.y)
		m_eDirection = cdUpRight;
	else if (s.x < f.x && s.y == f.y)
		m_eDirection == cdRight;
	else if (s.x < f.x && s.y < f.y)
		m_eDirection = cdDownRight;
	else if (s.x == f.x && s.y < f.y)
		m_eDirection = cdDown;
	else if (s.x > f.x && s.y < f.y)
		m_eDirection = cdDownLeft;
	else if (s.x > f.x && s.y == f.y)
		m_eDirection = cdLeft;
	else if (s.x > f.x && s.y > f.y)
		m_eDirection = cdUpLeft;
	else
		m_eDirection = cdNothing;
}

bool CConnector::GetCloserIntersectedPoint(TBaseObjectsArray* aRects, RECT fromPoint, POINT& intersectPoint)
{
	return false;
}

// ------- точка пересечения ------------------------

bool CalculateIntersect(LINE o, RECT r, TIntersectStructure& intersectStr)
{
	intersectStr.Clear();

	if ((o.x1 >= r.left && o.y1 >= r.top && o.x1 <= r.right && o.y1 <= r.bottom) && //Весь отрезок внутри. Структура не назначается
		(o.x2 >= r.left && o.y2 >= r.top && o.x2 <= r.right && o.y2 <= r.bottom))
		return true;

	if (o.y1 == o.y2) //Горизонтальный отрезок
		if (o.y1 >= r.top && o.y2 <= r.bottom) //Y внутри
		{
			//Левая сторона
			if (
				(o.x1 < r.left) && (o.x2 > r.left) || //Точка старта слева, точка финиша справа
				((o.x1 >= r.left && o.y1 >= r.top && o.x1 <= r.right && o.y1 <= r.bottom) && (o.x2 < r.left)) //Точка старта внутри, точка финиша слева
				)
				intersectStr = { {r.left, o.y1}, {r.left,r.top,r.left,r.bottom} };

			//Правая сторона		
			if (
				(o.x1 > r.right) && (o.x2 < r.right) || //Точка старта справа, точка финиша слева
				((o.x1 >= r.left && o.y1 >= r.top && o.x1 <= r.right && o.y1 <= r.bottom) && (o.x2 > r.left)) //Точка старта внутри, точка финиша справа
				)
				intersectStr = { {r.right, o.y1}, {r.right,r.top,r.right,r.bottom} };
		}

	if (o.x1 == o.x2) //Вертикальный отрезок
		if (o.x1 >= r.left && o.x2 <= r.right) //X внутри
		{
			//Верхняя сторона
			if (
				(o.y1 < r.top) && (o.y2 > r.top) || //Точка старта сверху, точка финиша внизу
				((o.x1 >= r.left && o.y1 >= r.top && o.x1 <= r.right && o.y1 <= r.bottom) && (o.y2 < r.top)) //Точка старта внутри, точка финиша вверху
				)
				intersectStr = { {o.x1, r.top}, {r.left,r.top,r.right,r.top} };

			//Нижняя сторона
			if (
				(o.y1 > r.bottom) && (o.y2 < r.bottom) || //Точка старта снизу, точка финиша сверху
				((o.x1 >= r.left && o.y1 >= r.top && o.x1 <= r.right && o.y1 <= r.bottom) && (o.y2 > r.bottom)) //Точка старта внутри, точка финиша внизу
				)
				intersectStr = { {o.x1, r.bottom}, {r.left,r.bottom,r.right,r.bottom} };
		}
	return intersectStr.Assigned();
}
//---------------------------------------------------------------------------------

void CConnector::ClearPath()
{
	m_aPath.clear();
	m_aTryPathes.clear();
	for (auto i = 0; i < (*m_aPixels).size(); i++)
		for (auto j = 0; j < (*m_aPixels)[i].size(); j++)
			(*m_aPixels)[i][j].pNode = nullptr;
}

bool IsValidLine(POINT exitPoint, POINT entryPoint, CRect* rect, TIntersectStructure& intersect)
{
	LINE line = { exitPoint.x, exitPoint.y, entryPoint.x, entryPoint.y };
	if (CalculateIntersect(line, rect->GetRect(), intersect))
		return false;
	return true;
}

bool IsValidLineForRect(POINT exitPoint, POINT entryPoint, CRect* rect, TIntersectStructure& intersect)
{
	LINE line = { exitPoint.x, exitPoint.y, entryPoint.x, entryPoint.y };
	if (CalculateIntersect(line, rect->GetRect(), intersect))
		return false;
	return true;
}

bool IsValidLine(POINT exitPoint, POINT entryPoint, TBaseObjectsArray* aRects, TIntersectStructure& intersect)
{
	for (auto rect : (*aRects))
		if (!IsValidLineForRect(exitPoint, entryPoint, (CRect*)rect, intersect))
			return false;
	return true;
}

CRect* CConnector::GetRemainingRect(TBaseObjectsArray* aRects)
{
	if (aRects->size() >= 2) // Убедимся, что у нас достаточно элементов в массиве
	{
		// Проходим по массиву прямоугольников, начиная с третьего элемента
		for (size_t i = 2; i < aRects->size(); ++i)
		{
			CRect* rect = dynamic_cast<CRect*>((*aRects)[i]);
			// Проверяем, что текущий прямоугольник не равен m_aRect[0] и m_aRect[1]
			if (rect != m_aRect[0] && rect != m_aRect[1])
			{
				return rect;
//				remainingRect = rect->GetRect(); // Присваиваем оставшийся прямоугольник
//				break; // Выходим из цикла, когда нашли нужный прямоугольник
			}
		}
	}

	return nullptr;// remainingRect;
}

size_t CConnector::GetLegCount(TPointsArray aPath)
{
	if (aPath.size() < 2)
		return 0;

	size_t nLegCount = 1;
	POINT currentPoint = aPath[0];
	TConnectorDirection eDirection = ::CalculateDirection(currentPoint, aPath[1]);
	TConnectorDirection eNextDirection = eDirection;
	for (size_t i = 0; i < aPath.size() - 1; i++)
	{
		TConnectorDirection eNextDirection = ::CalculateDirection(aPath[i], aPath[i + 1]);
		if (eNextDirection != eDirection)
		{
			eDirection = eNextDirection;
			nLegCount++;
		}
	}
	return nLegCount;
}


void CConnector::BuildPath(HDC dc, TBaseObjectsArray* aRects)
{

	auto dist = [](POINT p1, POINT p2)
		{
			return abs(p1.x - p2.x) + abs(p1.y - p2.y);
		};

	ClearPath();

	//--------my--------------------------

	POINT fromPoint = m_aRect[0]->GetCenter();
	POINT toPoint = m_aRect[1]->GetCenter();

	AddPoint(fromPoint);

	/*MakeTunnels(m_aRect[0], m_aPixels);
	MakeTunnels(m_aRect[1], m_aPixels);*/

	LONG dXLeft = min(fromPoint.x, toPoint.x);
	LONG dXRight = max(fromPoint.x, toPoint.x);
	LONG dYTop = min(fromPoint.y, toPoint.y);
	LONG dYBottom = max(fromPoint.y, toPoint.y);

	CRect* pRemainingRect = GetRemainingRect(aRects);

	TExitPointsArrays resultPoints;
	if (!CalculateSidePoints(m_aRect[0], m_aRect[1], m_aPixels, resultPoints))
		return;

	// Пробуем соединить каждую точку выхода с каждой точкой входа
	// и выбираем кратчайший путь
	int shortestPathLength = INT_MAX; // Начальная длина пути - максимальное целое число
	std::vector<POINT> shortestPath;   // Кратчайший путь
	std::vector<TPointsArray> m_aPathes;
	std::vector<int> m_aLength;

	for (int i = 0; i < resultPoints.start.size(); ++i)  // Перебираем все точки выхода
	{
		for (int j = 0; j < resultPoints.finish.size(); ++j) // Перебираем все точки входа
		{
			std::vector<POINT> path; // Временный путь для текущей комбинации
			// Добавляем начальную точку (точку выхода)
			POINT exitPoint = resultPoints.start[i].point;
			POINT entryPoint = resultPoints.finish[j].point;
			std::deque<POINT> entryPoints;
			entryPoints.push_back(entryPoint);

			path.insert(path.begin(), m_aRect[0]->GetCenter());

			path.push_back(exitPoint);

			POINT currentPoint = exitPoint; // промежуточная точка
			// Определяем точку пересечения с неподключенным прямоугольником, если есть
			TIntersectStructure intersect;
			LINE line = { exitPoint.x, exitPoint.y, entryPoint.x, entryPoint.y };

			while (currentPoint.x != entryPoints.back().x || currentPoint.y != entryPoints.back().y)
			{
				POINT nextPoint = currentPoint;  // вторая промежуточная точка

				if ((entryPoints.back().x == currentPoint.x) || (entryPoints.back().y == currentPoint.y))
				{
					if (IsValidLine(currentPoint, entryPoints.back(),aRects, intersect))
					{
						path.push_back(entryPoints.back());
						currentPoint = entryPoints.back();
						if (entryPoints.size() > 1)
							entryPoints.pop_back();

					}
					else
					{
						POINT p1 = { intersect.line.x1, intersect.line.y1 };
						POINT p2 = { intersect.line.x2, intersect.line.y2 };
						POINT p3 = dist(intersect.point, p1) < dist(intersect.point, p2) ? p1 : p2;
						TConnectorDirection eDirection = ::CalculateDirection(intersect.point, p3);
						switch (eDirection)
						{
						case cdUp: p3.y = p3.y >= (MIN_MARGINE + 1) ? p3.y -= MIN_MARGINE - 1 : p3.y = 0; break;
						case cdLeft: p3.x = p3.x >= (MIN_MARGINE + 1) ? p3.x -= MIN_MARGINE - 1 : p3.x = 0; break;
						case cdDown:
							p3.y += MIN_MARGINE + 1;
							if (p3.y >= (*m_aPixels)[0].size())
								p3.y = (*m_aPixels)[0].size() - 1;
							break;
						case cdRight:
							p3.x += MIN_MARGINE + 1;
							if (p3.x >= (*m_aPixels).size())
								p3.x = (*m_aPixels).size() - 1;
							break;
						}
						path.pop_back();
						currentPoint = path.back();
						entryPoints.push_back(p3);
						continue;
					}
				}
				else
				{
					if (IsValidLine(currentPoint, { currentPoint.x, entryPoints.back().y }, aRects, intersect))
					{
						// Строим горизонтальный отрезок до следующей точки
						nextPoint.y = entryPoints.back().y;
						path.push_back(nextPoint);
					}
					else if (IsValidLine(currentPoint, { entryPoints.back().x, currentPoint.y }, aRects, intersect))
					{
						// Строим вертикальный отрезок до следующей точки
						nextPoint.x = entryPoints.back().x;
						path.push_back(nextPoint);
					}

					else
					{
						// Не можем построить прямой отрезок, нужно определить дополнительные промежуточные точки
						// Получаем координаты каждой грани неподключенного прямоугольника
						RECT remainingRect = GetRemainingRect(aRects)->GetRect();

						// Расчет координат дополнительной промежуточной точки в зависимости от условий
						POINT intermediatePoint = { 0, 0 }; // Инициализируем точку

						// Проверяем условие для вертикального или горизонтального пересечения
						if (!IsValidLineForRect(currentPoint, { currentPoint.x, entryPoint.y }, pRemainingRect, intersect) /* условие для вертикального пересечения */)
						{
							if ((max(fromPoint.x, toPoint.x) - min(fromPoint.x, toPoint.x)) / 2 + min(fromPoint.x, toPoint.x) < (intersect.line.x2 - intersect.line.x1) / 2 + intersect.line.x1)
							{
								// Координата X дополнительной промежуточной точки на единицу больше координаты X правой грани неподключенного прямоугольника
								nextPoint.x = remainingRect.right + 10;
								nextPoint.y = currentPoint.y;
								path.push_back(nextPoint);
							}
							else
							{
								// Координата X дополнительной промежуточной точки на единицу меньше координаты X левой грани неподключенного прямоугольника
								nextPoint.x = remainingRect.left - 10;
								nextPoint.y = currentPoint.y;
								path.push_back(nextPoint);
							}
						}
						if (!IsValidLineForRect(currentPoint, { currentPoint.y, entryPoint.x }, pRemainingRect, intersect) /* условие для горизонтального пересечения */)
						{
							if ((max(fromPoint.y, toPoint.y) - min(fromPoint.y, toPoint.y)) / 2 + min(fromPoint.y, toPoint.y) < (intersect.line.y2 - intersect.line.y1) / 2 + intersect.line.y1)
							{
								// Координата Y дополнительной промежуточной точки на единицу больше координаты Y нижней грани неподключенного прямоугольника
								nextPoint.x = currentPoint.x;
								nextPoint.y = remainingRect.bottom + 10;
								path.push_back(nextPoint);
							}
							else
							{
								// Координата Y дополнительной промежуточной точки на 10 меньше координаты Y верхней грани неподключенного прямоугольника
								nextPoint.x = currentPoint.x;
								nextPoint.y = remainingRect.top - 10;
								path.push_back(nextPoint);
							}
						}
					}

					// Обновляем текущую точку
					currentPoint = nextPoint;
				}
			}

// Считаем длину текущего пути

			if (true/*!bPathCancelled*/)
			{
				path.push_back(m_aRect[1]->GetCenter());
				int pathLength = 0;
				for (int k = 1; k < path.size(); ++k)
					pathLength += abs(path[k].x - path[k - 1].x) + abs(path[k].y - path[k - 1].y);
				if (pathLength < shortestPathLength)
					shortestPathLength = pathLength;

				m_aPathes.push_back(path);
				m_aLength.push_back(pathLength);
			}
		}
		//		}

	}

	// Копируем кратчайший путь в m_aPath

// Считаем длину текущего пути

	int shortestLegCount = INT_MAX;
	for (int i = 0; i < m_aPathes.size(); i++)
		if (m_aLength[i] = shortestPathLength)
		{
			int nLegCount = GetLegCount(m_aPathes[i]);
			if (nLegCount < shortestLegCount)
			{
				shortestLegCount = nLegCount;
				shortestPath = std::move(m_aPathes[i]);
			}
		}

	m_aPath = std::move(shortestPath);

	// Рисуем коннектор с использованием нового пути
	Draw(dc);

	//AddPoint(toPoint);
}

void CConnector::Draw(HDC dc)
{
	if (!m_aPath.empty())
	{
		HPEN hPen = nullptr;

		MoveToEx(dc, m_aPath[0].x, m_aPath[0].y, NULL);
		if (GetIntersected())
			hPen = CreatePen(PS_DOT, 1, 0x0000FF);
		else
			hPen = CreatePen(PS_SOLID, 1, m_nPenColor);

		HPEN oldPen = (HPEN)SelectObject(dc, hPen);

		for (auto i = 1; i < m_aPath.size(); i++)
			LineTo(dc, m_aPath[i].x, m_aPath[i].y);

		SelectObject(dc, oldPen);

		DeleteObject(hPen);

	}
}

void CConnector::Clear(HDC dc)
{
	if (m_aPath.size() > 0)
	{
		HPEN hPen = CreatePen(PS_SOLID, 1, GetDCBrushColor(dc));
		HPEN oldPen = (HPEN)SelectObject(dc, hPen);

		MoveToEx(dc, m_aPath[0].x, m_aPath[0].y, NULL);
		for (auto i = 1; i < m_aPath.size(); i++)
			LineTo(dc, m_aPath[i].x, m_aPath[i].y);

		SelectObject(dc, oldPen);

		DeleteObject(hPen);
	}
}

bool CConnector::IsConnect(CRect* rect)
{
	for (CRect* itr : m_aRect)
		if (itr == rect)
			return true;
	return false;
}

bool CConnector::IsIntersect(CBaseObjects* pObject)
{
	if (pObject)
	{
		CRect* rect = (CRect*)pObject;
		if (rect == m_aRect[0] || rect == m_aRect[1])
			return false;
		if (m_aPath.size() > 1)
		{
			RECT r = rect->GetRect();
			for (auto i = 0; i < m_aPath.size() - 1; i++)
			{
				RECT o = {
					min(m_aPath[i].x,m_aPath[i + 1].x), min(m_aPath[i].y,m_aPath[i + 1].y),
					max(m_aPath[i].x,m_aPath[i + 1].x), max(m_aPath[i].y,m_aPath[i + 1].y),
				};

				if (::IsIntersect(o, r))
					return true;
			}
		}
	}
	return false;
}

