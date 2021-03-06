﻿#include "Obmap.h"
#include "Gwrite.hpp"

namespace pl
{
	bool pl::Obmap::map2tGrid()
	{
		this->_tGrid.clear();
		//auto &vGrid = *_vGridPtr;
		//size_t p = 0;
		//for (size_t i = 0; i < m_MaxRow; i++)
		//{
		//	for (size_t j = 0; j < m_MaxCol; j++)
		//	{
		//		GridIndex ind(i, j);
		//		this->_tGrid.insert(pair<GridIndex, size_t>(ind, vGrid[p++]));
		//	}
		//}
		bool rowChange = false;
		bool colChange = false;
		if (m_MaxRow % 2)
		{
			rowChange = true;
			m_MaxRow++;
		}
		if (m_MaxCol % 2)
		{
			colChange = true;
			m_MaxCol++;
		}
		for (size_t i = 0; i < m_MaxRow; i++)
		{
			for (size_t j = 0; j < m_MaxCol; j++)
			{
				GridIndex ind(i, j);
				this->_tGrid.insert(pair<GridIndex, size_t>(ind, bex::vertType::WayVert));
			}
		}

		for (auto &it : (*_vObGridInd)) 
			_tGrid[GridIndex(it.row,it.col)] = bex::vertType::ObVert;
		
		_obVertNum = _vObGridInd->size();
		if (rowChange)
		{
			for (size_t i = 0; i < m_MaxCol; i++)
			{
				_tGrid[GridIndex(m_MaxRow - 1, i)] = bex::vertType::ObVert;
				_obVertNum++;
			}
		}
		if (colChange)
		{
			for (size_t i = 0; i < m_MaxRow; i++)
			{
				_tGrid[GridIndex(i , m_MaxCol -1)] = bex::vertType::ObVert;
				_obVertNum++;
			}
		}
		
		_reachableVertNum = m_MaxRow * m_MaxCol - _obVertNum;
		//create graph
		int i = 0;
		for (auto & it : this->_tGrid)
		{
			bex::VertexProperty vp;
			std::pair<size_t, size_t> ind(it.first.first, it.first.second);
			vp.PntIndex = ind;
			vp.Type = it.second;
			vp.EdgeState = false;
			vp.pnt.x(it.first.first + 0.5);
			vp.pnt.y(it.first.second + 0.5);
			boost::add_vertex(vp, this->_tGraph);
			std::pair<int, int> localIndex;
			localIndex = it.first;
			tmap2graph.insert(std::pair<std::pair<int, int>, int>(localIndex, i));
			tgraph2map.insert(std::pair<int, std::pair<int, int>>(i, localIndex));
			i++;
		}
		//add edges of the graph 

		std::pair<bex::VertexIterator, bex::VertexIterator> vi = boost::vertices(this->_tGraph);
		for (bex::VertexIterator vit = vi.first; vit != vi.second; vit++)
		{
			bex::VertexDescriptor vd = *vit;
			if (this->_tGraph[vd].Type != bex::vertType::ObVert)
			{
				auto localIndex = tgraph2map[vd];
				//				auto vlocalIndex = getSearchNeighbor(localIndex, graphType::base);
				auto vlocalIndex = getSearchVerticalNeighbor(localIndex, graphType::base);

				std::vector<int> vvd;
				for (auto &it : vlocalIndex)
				{
					vvd.push_back(tmap2graph[it]);
				}

				for (auto &it : vvd)
				{
					if (this->_tGraph[it].EdgeState == false)
					{
						bex::EdgeProperty ep;
						ep.weight = bg::distance(_tGraph[vd].pnt, _tGraph[it].pnt);
						boost::add_edge(it, vd, ep, _tGraph);
					}
				}
			}
			this->_tGraph[vd].EdgeState = true;
		}
		auto num_edges = boost::num_edges(this->_tGraph);
		cout << "num_edges = " << num_edges << endl;
		auto num_vertex = boost::num_vertices(_tGraph);
		cout << "num_vertex = " << num_vertex << endl;
		return false;
	}

	bool Obmap::map2sGrid()
	{
		this->_sGrid.clear();
		this->_STCGrid.clear();
		this->_STCVirtualGrid.clear();
		for (size_t i = 0; i < m_sMaxRow; i++)
		{
			for (size_t j = 0; j < m_sMaxCol; j++)
			{
				GridIndex ind(i, j);
				STCVert vert;
				vert.x = 2 * i + 1;
				vert.y = 2 * j + 1;
				vert.LB.first = 2 * i;
				vert.LT.first = 2 * i;
				vert.RB.first = 2 * i + 1;
				vert.RT.first = 2 * i + 1;
				vert.LB.second = 2 * j;
				vert.LT.second = 2 * j + 1;
				vert.RB.second = 2 * j;
				vert.RT.second = 2 * j + 1;
				auto tGridIndex = getTgraphGridInd(ind);
				vert._vGridIndex = tGridIndex;
				auto obNum = gridObNum(tGridIndex);
				vert.virtualType = NOVIR;
				STCGridInd indAndType;
				//indAndType.second = STCVertType::Normal;
				indAndType.second = NOVIR;
				indAndType.first = ind;
				bool existVir = false;
				if (obNum == 0)
					vert._vertType = bex::vertType::WayVert;
				if (obNum == 3)
					vert._vertType = bex::vertType::DoubleDiffOb;
				if (obNum == 1)
					vert._vertType = bex::vertType::SingleOb;
				if (obNum == 4)
					vert._vertType = bex::vertType::ObVert;
				if (obNum == 2) {
					vert._vertType = bex::vertType::DoubleSameOb;
					if (_tGrid[vert.LT]== bex::ObVert &&_tGrid[vert.RB] == bex::ObVert)
					{
						auto vertVir = vert;
						vertVir.virtualType = VRT;
						vertVir._vertType = bex::vertType::DoubleDiffOb;
//						indAndType.second = STCVertType::Left;
						indAndType.second = VRT;
						this->_STCVirtualGrid.insert(pair<STCGridInd, STCVert>(indAndType, vertVir));
						vert.virtualType = VLB;
						vertVir._vertType = bex::vertType::DoubleDiffOb;
//						indAndType.second = STCVertType::Right;
						indAndType.second = VLB;
						this->_STCVirtualGrid.insert(pair<STCGridInd, STCVert>(indAndType, vert));
						existVir = true;
						vitualVertSet.insert(ind);
					}
					if (_tGrid[vert.RT] == bex::ObVert && _tGrid[vert.LB] == bex::ObVert)
					{
						auto vertVir = vert;
						vertVir.virtualType = VLT;
						vertVir._vertType = bex::vertType::DoubleDiffOb;
						//indAndType.second = STCVertType::Left;
						indAndType.second = VLT;
						this->_STCVirtualGrid.insert(pair<STCGridInd, STCVert>(indAndType, vertVir));
						vert.virtualType = VRB;
//						indAndType.second = STCVertType::Right;
						indAndType.second = VRB;
						vertVir._vertType = bex::vertType::DoubleDiffOb;
						this->_STCVirtualGrid.insert(pair<STCGridInd, STCVert>(indAndType, vert));
						existVir = true;
						vitualVertSet.insert(ind);
					}					
				}
				if (!existVir) {
					this->_STCVirtualGrid.insert(pair<STCGridInd, STCVert>(indAndType, vert));
					realVertSet.insert(ind);
				}
			}
		}
		//create graph
		int i = 0;
		for (auto & it : this->_STCVirtualGrid)
		{
			bex::VertexProperty vp;
			std::pair<size_t, size_t> ind(it.first.first.first, it.first.first.second);
			vp.PntIndex = ind;
			//auto tGridIndex = getTgraphGridInd(ind);
			vp.pnt.x(it.second.x);
			vp.pnt.y(it.second.y);
			vp.Type = it.second._vertType;
			//if (allObstacle(it.second._vGridIndex))
			//	vp.Type = bex::vertType::ObVert;
			//else
			//	vp.Type = bex::vertType::WayVert;
			vp.EdgeState = false;
			boost::add_vertex(vp, this->_sGraph);
			//std::pair<int, int> localIndex;			
			//localIndex = it.first;
			gridInd2GraphVd.insert(pair<STCGridInd, int>(it.first, i));
			graphVd2GridInd.insert(pair<int, STCGridInd>(i, it.first));
			i++;
		}

		//add virtualType

		//add edges of the graph 

		size_t vertNum = 0;
		std::pair<bex::VertexIterator, bex::VertexIterator> vi = boost::vertices(this->_sGraph);
		for (bex::VertexIterator vit = vi.first; vit != vi.second; vit++)
		{
			bex::VertexDescriptor vd = *vit;
			if (this->_sGraph[vd].Type != bex::vertType::ObVert)
			{
				//auto localIndex = sgraph2map[vd];
				auto gridInd = graphVd2GridInd[vd];
				auto vNeiInd = getSTCNeighbor(gridInd);
				//auto vlocalIndex = getSTCVerticalNeighbor(localIndex);
				std::vector<int> vvd;				
				for (auto &it : vNeiInd)
					vvd.push_back(gridInd2GraphVd[it]);

				for (auto &it : vvd)
				{
					if (this->_sGraph[it].EdgeState == false)
					{
						bex::EdgeProperty ep;
						//ep.weight = bg::distance(_tGraph[vd].pnt, _tGraph[it].pnt);
						boost::add_edge(it, vd, ep, _sGraph);
					}
				}
			}
			this->_sGraph[vd].EdgeState = true;
		}
		auto num_edges = boost::num_edges(this->_sGraph);
		cout << "num_edges = " << num_edges << endl;
		auto num_vertex = boost::num_vertices(_sGraph);
		cout << "num_vertex = " << num_vertex << endl;
		return false;
	}



	void Obmap::writeEdges(size_t const & type)
	{		
		bex::Graph *graphPtr;
		if (type == graphType::base)
			graphPtr = &this->_tGraph;
		else
			graphPtr = &this->_sGraph;
		auto &graph = *graphPtr;
		std::pair<bex::EdgeIterator, bex::EdgeIterator> _b2e_ei = boost::edges(graph);

		vector<size_t> sRow, sCol, tRow, tCol;
		for (auto eit = _b2e_ei.first; eit != _b2e_ei.second; eit++)
		{
			bex::EdgeDescriptor ed = *eit;
			bex::EdgeProperty &ep = graph[ed];
			bex::VertexDescriptor sVertd = boost::source(ed, graph);
			bex::VertexDescriptor tVertd = boost::target(ed, graph);

			bex::VertexProperty &sVert = graph[sVertd];
			bex::VertexProperty &tVert = graph[tVertd];

			sRow.push_back(sVert.PntIndex.first);
			sCol.push_back(sVert.PntIndex.second);

			tRow.push_back(tVert.PntIndex.first);
			tCol.push_back(tVert.PntIndex.second);

		}
		writeDebug(c_deg, "sRow", sRow);
		writeDebug(c_deg, "sCol", sCol);
		writeDebug(c_deg, "tRow", tRow);
		writeDebug(c_deg, "tCol", tCol);
	}

	void Obmap::writeEdgesInPnt(size_t const & type)
	{
		bex::Graph *graphPtr;
		if (type == graphType::base)
			graphPtr = &this->_tGraph;
		else
			graphPtr = &this->_sGraph;
		auto &graph = *graphPtr;
		std::pair<bex::EdgeIterator, bex::EdgeIterator> _b2e_ei = boost::edges(graph);

		vector<double> sPntx, sPnty, tPntx, tPnty;
		for (auto eit = _b2e_ei.first; eit != _b2e_ei.second; eit++)
		{
			bex::EdgeDescriptor ed = *eit;
			bex::EdgeProperty &ep = graph[ed];
			bex::VertexDescriptor sVertd = boost::source(ed, graph);
			bex::VertexDescriptor tVertd = boost::target(ed, graph);

			bex::VertexProperty &sVert = graph[sVertd];
			bex::VertexProperty &tVert = graph[tVertd];

			sPntx.push_back(sVert.pnt.x());
			sPnty.push_back(sVert.pnt.y());

			//cout << "pnt.x " << tVert.pnt.x() << "	pnt.y" << tVert.pnt.y() << endl;
			tPntx.push_back(tVert.pnt.x());
			tPnty.push_back(tVert.pnt.y());

		}
		writeDebug(c_deg, "sPntx", sPntx);
		writeDebug(c_deg, "sPnty", sPnty);
		writeDebug(c_deg, "tPntx", tPntx);
		writeDebug(c_deg, "tPnty", tPnty);

	}

	STCGridInd Obmap::tGridInd2SGridInd(GridIndex const & ind)
	{
		size_t first = floor(double(ind.first) / 2);
		size_t second = floor(double(ind.second) / 2);
		auto stc_ind = GridIndex(first, second);
		if (vitualVertSet.count(stc_ind) == 1) {
			if (ind.first == 2*first && ind.second == 2*second)
				return STCGridInd(stc_ind, VLB);
			if (ind.first == 2*first && ind.second == 2*second +1)
				return STCGridInd(stc_ind, VLT);
			if (ind.first == 2*first + 1 && ind.second == 2*second)
				return STCGridInd(stc_ind, VRB);
			if (ind.first == 2*first  + 1 && ind.second == 2*second +1 )
				return STCGridInd(stc_ind, VRT);
		}
		return STCGridInd(GridIndex(first, second), NOVIR);
	}

	GridIndex Obmap::pnt2IndexInBaseGrid(bex::DPoint const & pnt)
	{
		GridMap * gridPtr = &this->_tGrid;
		double gridStep = 1;
		GridIndex rindex;
		rindex.first = -1;
		rindex.second = -1;

		GridIndex initIndex;
		initIndex.first = 0;
		initIndex.second = 0;

		double _min_x = 0;
		double _min_y = 0;

		double _i_x = 0.5;
		double _i_y = 0.5;


		if (pnt.y()<_min_y)
		{
			return rindex;
		}
		if (pnt.x()<_min_x)
		{
			return rindex;
		}
		//cout << "pnt.x " << pnt.x() << "pnt.y" << pnt.y() << endl;
		//cout << "min_x  " << _min_x << " min_y " << _min_y << endl;
		//cout << "gridStep = " << gridStep << endl;
		auto bais_x = pnt.x() - _min_x;
		auto bais_y = pnt.y() - _min_y;
		auto p_col = floor(bais_x / gridStep);
		auto p_row = floor(bais_y / gridStep);

		//		cout << " :" << this->m_MaxCol << endl;
		if (p_col>m_MaxCol)
		{
			//			cout << "dayu" << endl;
			return rindex;
		}
		if (p_row>this->m_MaxRow)
		{
			//			cout << "dayu" << endl;
			return rindex;
		}
		rindex.first = p_col;
		rindex.second = p_row;
		return rindex;
//		return GridIndex();
	}

	bool Obmap::allConnected(vector<bex::VertexDescriptor> const & v_vd)
	{
		if (v_vd.size() == 1)
			return true;
		using CGraph = boost::adjacency_list<bt::vecS, bt::vecS, bt::undirectedS>;
		CGraph cg;
		for (size_t i = 0; i < v_vd.size(); i++)
		{
			for (size_t j = i; j < v_vd.size(); j++)
			{
				
				if (isConnected(v_vd[i], v_vd[j], pl::graphType::span))
				{
					bt::add_edge(i, j, cg);
					//cout << "t = " << i << " s = " << j << endl;
				}
			}
		}
		vector<int> component(bt::num_vertices(cg));
		if (component.size() < v_vd.size())
			return false;
		int num = bt::connected_components(cg, &component[0]);
		if (num == 1) 
			return true;
		return false;
	}

	bool Obmap::allConnectedBase(vector<bex::VertexDescriptor> const & v_vd)
	{
		if (v_vd.size() == 1)
			return true;
		using CGraph = boost::adjacency_list<bt::vecS, bt::vecS, bt::undirectedS>;
		CGraph cg;
		for (size_t i = 0; i < v_vd.size(); i++)
		{
			for (size_t j = i; j < v_vd.size(); j++)
			{

				if (isConnected(v_vd[i], v_vd[j], pl::graphType::base))
				{
					bt::add_edge(i, j, cg);
					//cout << "t = " << i << " s = " << j << endl;
				}
			}
		}
		vector<int> component(bt::num_vertices(cg));
		if (component.size() < v_vd.size())
			return false;
		int num = bt::connected_components(cg, &component[0]);
		if (num == 1)
			return true;
		return false;
	}

	vector<bex::VertexDescriptor> Obmap::STCGraphVd2TGraphVd(bex::VertexDescriptor const & svd)
	{
		vector<bex::VertexDescriptor> res;
		STCGridInd sgridInd = graphVd2GridInd[svd];
		
		bool ad_VLB = false;
		bool ad_VRT = false;
		bool ad_VLT = false;
		bool ad_VRB = false;
		switch (sgridInd.second)
		{
		case NOVIR: {
			ad_VLB = true;
			ad_VRT = true;
			ad_VLT = true;
			ad_VRB = true;
			break;
		}
		case VLB:
		{
			ad_VLB = true;	break;
		}
		case VRT:
		{
			ad_VRT = true; break;
		}
		case VLT:
		{
			ad_VLT = true; break;
		}
		case VRB:
		{
			ad_VRB = true; break;
		}
		default:
			break;
		}
		if (ad_VLB)
			res.push_back(tmap2graph[_STCVirtualGrid[sgridInd].LB]);
		if (ad_VRT)
			res.push_back(tmap2graph[_STCVirtualGrid[sgridInd].RT]);
		if (ad_VLT)
			res.push_back(tmap2graph[_STCVirtualGrid[sgridInd].LT]);
		if (ad_VRB)
			res.push_back(tmap2graph[_STCVirtualGrid[sgridInd].RB]);
		return res;
	}

	vector<bex::VertexDescriptor> Obmap::STCGraphVd2TGraphVdSp(bex::VertexDescriptor const & svd)
	{
		vector<bex::VertexDescriptor> res;
		STCGridInd sgridInd = graphVd2GridInd[svd];

		res.push_back(tmap2graph[_STCVirtualGrid[sgridInd].LB]);
		res.push_back(tmap2graph[_STCVirtualGrid[sgridInd].RT]);
		res.push_back(tmap2graph[_STCVirtualGrid[sgridInd].LT]);
		res.push_back(tmap2graph[_STCVirtualGrid[sgridInd].RB]);
		return res;

		return vector<bex::VertexDescriptor>();
	}

	bool Obmap::verticalDouble(bex::VertexDescriptor const & cenVd, bex::VertexDescriptor const & neiVd)
	{
		auto neighborIter = boost::adjacent_vertices(cenVd, this->_sGraph);
		auto cenInd = this->graphVd2GridInd[cenVd];
		size_t i = cenInd.first.first;
		size_t j = cenInd.first.second;
		auto neiInd = this->graphVd2GridInd[neiVd];
		//left
		if (obstacleOccupy(cenInd, left))
		{
			if (neiInd.first.first == i +1 && neiInd.first.second == j)
			{
				return true;
			}
			return false;
		}
		//right
		if (obstacleOccupy(cenInd, right))
		{
			if (neiInd.first.first == i - 1 && neiInd.first.second == j)
			{
				return true;
			}
			return false;
		}

		//top
		if (obstacleOccupy(cenInd, top))
		{
			if (neiInd.first.first == i && neiInd.first.second == j -1)
			{
				return true;
			}
			return false;
		}

		//bottom
		if (obstacleOccupy(cenInd, bottom))
		{
			if (neiInd.first.first == i&& neiInd.first.second == j +1)
			{
				return true;
			}
			return false;
		}

		return false;
	}

	std::vector<GridIndex> Obmap::getSearchVerticalNeighbor(GridIndex const & cen_index, size_t const & gridType)
	{
		GridMap *gridPtr;
		if (gridType == graphType::base)
		{
			gridPtr = &this->_tGrid;
		}
		else
		{
			gridPtr = &this->_sGrid;
		}
		auto &grid = (*gridPtr);

		vector<GridIndex> vIndex;

		auto &i = cen_index.first;
		auto &j = cen_index.second;
		auto neighbour = [=](GridIndex &ind, vector<GridIndex>  &vInd) {
			if (grid.count(ind))
			{
				if (grid.at(ind) != bex::vertType::ObVert)
				{
					vInd.push_back(ind);
					return true;
				}
			}
			return false;
		};

		//left
		neighbour(GridIndex(i - 1, j), vIndex);
		//top 
		neighbour(GridIndex(i, j + 1), vIndex);
		//right
		neighbour(GridIndex(i + 1, j), vIndex);
		//botton
		neighbour(GridIndex(i, j - 1), vIndex);

		return vIndex;
	}

	std::vector<bex::VertexDescriptor> Obmap::getSearchVerticalNeighbor(bex::VertexDescriptor const & cvd, size_t const & gridType)
	{
		GridMap *gridPtr;
		GridIndex cen_index;
		if (gridType == graphType::base)
		{
			gridPtr = &this->_tGrid;
			cen_index = this->tgraph2map[cvd];
		}
		else
		{
			gridPtr = &this->_sGrid;
			cen_index = this->sgraph2map[cvd];
		}
		auto &grid = (*gridPtr);

		//vector<pair<GridIndex, size_t>> res;
		vector<GridIndex> vIndex;

		auto &i = cen_index.first;
		auto &j = cen_index.second;
		auto neighbour = [=](GridIndex &ind, vector<GridIndex>  &vInd) {
			if (grid.count(ind))
			{
				if (grid.at(ind) != bex::vertType::ObVert)
				{
					vInd.push_back(ind);
					return true;
				}
			}
			return false;
		};

		//left
		neighbour(GridIndex(i - 1, j), vIndex);
		//top 
		neighbour(GridIndex(i, j + 1), vIndex);
		//right
		neighbour(GridIndex(i + 1, j), vIndex);
		//botton
		neighbour(GridIndex(i, j - 1), vIndex);

		vector<bex::VertexDescriptor> res;
		for (auto &it : vIndex) {
			if (gridType == graphType::base)
			{
				res.push_back(tmap2graph[it]);
			}
			else
			{
				res.push_back(smap2graph[it]);
			}
		}
		return res;
	}

	std::vector<bex::VertexDescriptor> Obmap::getSearchAllNeighbor(bex::VertexDescriptor const & cvd)
	{

		GridMap *gridPtr;
		GridIndex cen_index;
		size_t gridType = graphType::base;
		if (gridType == graphType::base)
		{
			gridPtr = &this->_tGrid;
			cen_index = this->tgraph2map[cvd];
		}
		else
		{
			gridPtr = &this->_sGrid;
			cen_index = this->sgraph2map[cvd];
		}
		auto &grid = (*gridPtr);

		//vector<pair<GridIndex, size_t>> res;
		vector<GridIndex> vIndex;

		auto &i = cen_index.first;
		auto &j = cen_index.second;
		auto neighbour = [=](GridIndex &ind, vector<GridIndex>  &vInd) {
			if (grid.count(ind))
			{
				//if (grid.at(ind) == bex::vertType::WayVert)
				//{
					vInd.push_back(ind);
					return true;
				//				}
			}
			return false;
		};

		//left
		neighbour(GridIndex(i - 1, j), vIndex);
		//top 
		neighbour(GridIndex(i, j + 1), vIndex);
		//right
		neighbour(GridIndex(i + 1, j), vIndex);
		//botton
		neighbour(GridIndex(i, j - 1), vIndex);

		vector<bex::VertexDescriptor> res;
		for (auto &it : vIndex) {
			if (gridType == graphType::base)
			{
				res.push_back(tmap2graph[it]);
			}
			else
			{
				res.push_back(smap2graph[it]);
			}
		}
		return res;
	}

	bool Obmap::inSameSTCMegaBox(bex::VertexDescriptor const & vd0, bex::VertexDescriptor const & vd1)
	{
		auto ind0 = tGridInd2SGridInd(tgraph2map[vd0]);
		auto ind1 = tGridInd2SGridInd(tgraph2map[vd1]);
		if (ind0.first.first == ind1.first.first && ind0.first.second == ind1.first.second)
		{
			return true;
		}
		return false;
	}

	//std::vector<GridIndex> Obmap::getSTCVerticalNeighbor(GridIndex const & cen_index)
	//{
	//	auto &vert = this->_STCGrid[cen_index];


	//	vector<GridIndex> vIndex;

	//	auto &i = cen_index.first;
	//	auto &j = cen_index.second;

	//	//left 
	//	if (adjacent(cen_index,GridIndex(i - 1,j),left))
	//		vIndex.push_back(GridIndex(i - 1, j));
	//	//right
	//	if (adjacent(cen_index, GridIndex(i + 1, j), right))
	//		vIndex.push_back(GridIndex(i + 1, j));
	//	//top
	//	if (adjacent(cen_index, GridIndex(i, j + 1), top))
	//		vIndex.push_back(GridIndex(i, j + 1));
	//	//bottom
	//	if (adjacent(cen_index, GridIndex(i, j - 1), bottom))
	//		vIndex.push_back(GridIndex(i, j - 1));
	//	return vIndex;
	//}

	std::vector<STCGridInd> Obmap::getSTCNeighbor(STCGridInd const & cen_index)
	{

		std::vector<STCGridInd>  res;
		bool ad_left = false;
		bool ad_right = false;
		bool ad_top = false;
		bool ad_bottom = false;

		switch (cen_index.second)
		{
		case NOVIR:
		{
			ad_left = true;
			ad_right = true;
			ad_top = true;
			ad_bottom = true;
			break;
		}
		case VLB:
		{
			ad_left = true;			ad_bottom = true;			break;
		}
		case VRT:
		{
			ad_right = true;			ad_top = true;			break;
		}
		case VLT:
		{
			ad_left = true;				ad_top = true;			break;
		}
		case VRB:
		{
			ad_right = true;			ad_bottom = true;		break;
		}
		default:
			break;
		}

		GridIndex cInd = cen_index.first;
		// this index is without vritual type;
		auto &i = cInd.first;
		auto &j = cInd.second;

		//left 
		if (ad_left)
		{
			STCGridInd neiInd;
			neiInd.first = GridIndex(i - 1, j);
			neiInd.second = NOVIR;
			if (vitualVertSet.count(neiInd.first) == 1)
			{
				if (_STCVirtualGrid.count(STCGridInd(neiInd.first, VRT)) == 1)
					neiInd.second = VRT;
				else
					neiInd.second = VRB;
			}
			if (adjacent(cen_index, neiInd, left))
				res.push_back(neiInd);
		}
		//right 
		if (ad_right)
		{
			STCGridInd neiInd;
			neiInd.first = GridIndex(i + 1, j);
			neiInd.second = NOVIR;
			if (vitualVertSet.count(neiInd.first) == 1)
			{
				if (_STCVirtualGrid.count(STCGridInd(neiInd.first, VLT)) == 1)
					neiInd.second = VLT;
				else
					neiInd.second = VLB;
			}
			if (adjacent(cen_index, neiInd, right))
				res.push_back(neiInd);
		}
		//top 
		if (ad_top)
		{
			STCGridInd neiInd;
			neiInd.first = GridIndex(i, j + 1);
			neiInd.second = NOVIR;
			if (vitualVertSet.count(neiInd.first) == 1)
			{
				if (_STCVirtualGrid.count(STCGridInd(neiInd.first, VRB)) == 1)
					neiInd.second = VRB;
				else
					neiInd.second = VLB;
			}
			if (adjacent(cen_index, neiInd, top))
				res.push_back(neiInd);
		}
		//bottom 
		if (ad_bottom)
		{
			STCGridInd neiInd;
			neiInd.first = GridIndex(i, j - 1);
			neiInd.second = NOVIR;
			if (vitualVertSet.count(neiInd.first) == 1)
			{
				if (_STCVirtualGrid.count(STCGridInd(neiInd.first, VRT)) == 1)
					neiInd.second = VRT;
				else
					neiInd.second = VLT;
			}
			if (adjacent(cen_index, neiInd, bottom))
				res.push_back(neiInd);
		}
		return res;
	}

	vector<GridIndex> Obmap::getTgraphGridInd(GridIndex const & cenInd)
	{
		GridIndex sgridInd = cenInd;
		
		auto maxRow = sgridInd.first * 2;
		auto maxCol = sgridInd.second * 2;

		vector<GridIndex> res;
		GridIndex tgridInd(sgridInd.first * 2, sgridInd.second * 2);
		if ((maxRow == m_MaxRow)&&(maxCol == m_MaxCol))
		{
			res.push_back(tgridInd);
			return res;
		}
		if (maxRow == m_MaxRow)
		{
			res.push_back(tgridInd);
			res.push_back(GridIndex(tgridInd.first, tgridInd.second + 1));
			return res;
		}
		if (maxCol == m_MaxCol)
		{
			res.push_back(tgridInd);
			res.push_back(GridIndex(tgridInd.first + 1, tgridInd.second));
			return res;
		}
		res.push_back(tgridInd);
		res.push_back(GridIndex(tgridInd.first + 1, tgridInd.second));
		res.push_back(GridIndex(tgridInd.first + 1, tgridInd.second + 1));
		res.push_back(GridIndex(tgridInd.first, tgridInd.second + 1));
		return res;
	}

	bool Obmap::allObstacle(vector<GridIndex>  const &vindex)
	{
		for (auto &it: vindex)
		{
			if (this->_tGrid[it] == bex::vertType::WayVert)
				return false;
		}
		return true;
	}

	bool Obmap::gridObstacle(GridIndex const & cenInd)
	{
		if (cenInd.first == -1 && cenInd.second == -1)
			return true;
		if (this->_tGrid[cenInd] == bex::ObVert)
			return true;
		return false;
	}

	int Obmap::gridObNum(vector<GridIndex> const & vindex)
	{
		size_t obNum = 0;
		for (auto &it : vindex)
		{
			if (_tGrid[it] == bex::ObVert)
				obNum++;
		}
		return obNum;
	}

	bool Obmap::obstacleOccupy(GridIndex const & cenInd, int const & dir)
	{
		//if (_STCGrid.count(cenInd) == 0)
		//	return true;
		//auto &vert = this->_STCGrid[cenInd];
		//switch (dir)
		//{
		//case left:
		//{
		//	if (gridObstacle(vert.LB)&& gridObstacle(vert.LT))
		//		return true;
		//	else
		//		return false;
		//}
		//case right:
		//{
		//	if (gridObstacle(vert.RB) && gridObstacle(vert.RT))
		//		return true;
		//	else
		//		return false;
		//}
		//case top:
		//{
		//	if (gridObstacle(vert.RT) && gridObstacle(vert.LT))
		//		return true;
		//	else
		//		return false;
		//}
		//case bottom:
		//{
		//	if (gridObstacle(vert.LB) && gridObstacle(vert.RB))
		//		return true;
		//	else
		//		return false;
		//}
		//default:
		//	break;
		//}
		return false;
	}

	bool Obmap::obstacleOccupy(STCGridInd const & cenInd, int const & dir)
	{
		if (_STCVirtualGrid.count(cenInd) == 0)
			return true;
		auto &vert = this->_STCVirtualGrid[cenInd];
		switch (dir)
		{
		case left:
		{
			if (gridObstacle(vert.LB)&& gridObstacle(vert.LT))
				return true;
			else
				return false;
		}
		case right:
		{
			if (gridObstacle(vert.RB) && gridObstacle(vert.RT))
				return true;
			else
				return false;
		}
		case top:
		{
			if (gridObstacle(vert.RT) && gridObstacle(vert.LT))
				return true;
			else
				return false;
		}
		case bottom:
		{
			if (gridObstacle(vert.LB) && gridObstacle(vert.RB))
				return true;
			else
				return false;
		}
		default:
			break;
		}
		return false;
	}

	bool Obmap::adjacent(GridIndex const & sInd, GridIndex const & tInd, int const & dir)
	{
		//if (_STCGrid.count(sInd) == 0)
		//	return false;
		//if (_STCGrid.count(tInd) == 0)
		//	return false;
		//auto &sVert = this->_STCGrid[sInd];
		//auto &tVert = this->_STCGrid[tInd];
		//switch (dir)
		//{
		//case left:
		//{
		//	if (!obstacleOccupy(sInd, left) && !obstacleOccupy(tInd, right))
		//	{
		//		if (gridObstacle(sVert.LT) && gridObstacle(tVert.RB))
		//			return false;
		//		if (gridObstacle(sVert.LB) && gridObstacle(tVert.RT))
		//			return false;
		//		return true;
		//	}
		//	return false;
		//}
		//case right:
		//{
		//	if (!obstacleOccupy(sInd, right) && !obstacleOccupy(tInd, left))
		//	{
		//		if (gridObstacle(sVert.RT) && gridObstacle(tVert.LB))
		//			return false;
		//		if (gridObstacle(sVert.RB) && gridObstacle(tVert.LT))
		//			return false;
		//		return true;
		//	}
		//	return false;
		//}
		//case top:
		//{
		//	if (!obstacleOccupy(sInd, top) && !obstacleOccupy(tInd, bottom))
		//	{
		//		if (gridObstacle(sVert.RT) && gridObstacle(tVert.LB))
		//			return false;
		//		if (gridObstacle(sVert.LT) && gridObstacle(tVert.RB))
		//			return false;
		//		return true;
		//	}
		//	return false;
		//}
		//case bottom:
		//{
		//	if (!obstacleOccupy(sInd, bottom) && !obstacleOccupy(tInd, top))
		//	{
		//		if (gridObstacle(sVert.RB) && gridObstacle(tVert.LT))
		//			return false;
		//		if (gridObstacle(sVert.LB) && gridObstacle(tVert.RT))
		//			return false;
		//		return true;
		//	}
		//	return false;
		//}
		//default:
		//	break;
		//}
		return false;
	}

	bool Obmap::adjacent(STCGridInd const & sInd, STCGridInd const & tInd, int const & dir)
	{
		if (_STCVirtualGrid.count(sInd) == 0)
			return false;
		if (_STCVirtualGrid.count(tInd) == 0)
			return false;
		auto &sVert = this->_STCVirtualGrid[sInd];
		auto &tVert = this->_STCVirtualGrid[tInd];
		switch (dir)
		{
		case left:
		{
			if (!obstacleOccupy(sInd, left) && !obstacleOccupy(tInd, right))
			{
				if (gridObstacle(sVert.LT) && gridObstacle(tVert.RB))
					return false;
				if (gridObstacle(sVert.LB) && gridObstacle(tVert.RT))
					return false;
				return true;
			}
			return false;
		}
		case right:
		{
			if (!obstacleOccupy(sInd, right) && !obstacleOccupy(tInd, left))
			{
				if (gridObstacle(sVert.RT) && gridObstacle(tVert.LB))
					return false;
				if (gridObstacle(sVert.RB) && gridObstacle(tVert.LT))
					return false;
				return true;
			}
			return false;
		}
		case top:
		{
			if (!obstacleOccupy(sInd, top) && !obstacleOccupy(tInd, bottom))
			{
				if (gridObstacle(sVert.RT) && gridObstacle(tVert.LB))
					return false;
				if (gridObstacle(sVert.LT) && gridObstacle(tVert.RB))
					return false;
				return true;
			}
			return false;
		}
		case bottom:
		{
			if (!obstacleOccupy(sInd, bottom) && !obstacleOccupy(tInd, top))
			{
				if (gridObstacle(sVert.RB) && gridObstacle(tVert.LT))
					return false;
				if (gridObstacle(sVert.LB) && gridObstacle(tVert.RT))
					return false;
				return true;
			}
			return false;
		}
		default:
			break;
		}
		return false;
	}

	bool Obmap::isConnected(bex::VertexDescriptor const & vd1, bex::VertexDescriptor const & vd2, int const &gridType)
	{
		bex::Graph *graphPtr;
		
		if (gridType == graphType::base)
			graphPtr = &this->_tGraph;
		else
			graphPtr = &this->_sGraph;
		//auto ind1 = graphVd2GridInd[vd1];
		//if (vitualVertSet.count(ind1.first) == 1)
		//	return false;
		//auto ind2 = graphVd2GridInd[vd2];
		//if (vitualVertSet.count(ind2.first) == 1)
		//	return false;

		auto &graph = *graphPtr;		
		auto neighborIter = boost::adjacent_vertices(vd1, graph);		
		std::vector<size_t> vVd;
		for (auto ni = neighborIter.first; ni != neighborIter.second; ++ni)
			vVd.push_back(*ni);
		 auto iter = std::find(vVd.begin(), vVd.end(), vd2);
		 if(iter == vVd.end())
			 return false;
		 return true;
	}
	
	bool STCVert::_virtualVertExist()
	{
		return false;
	}

}