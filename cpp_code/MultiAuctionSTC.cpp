﻿#include "MultiAuctionSTC.h"

//void pl::MultiAuction::setStartPnt(vector<size_t> const & vStartPntRow, vector<size_t> const & vStartPntCol)
//{
//	this->_vStartPnt.clear();
//	for (size_t i = 0; i < vStartPntRow.size(); i++)
//	{
//		_vStartPnt.push_back(GridIndex(vStartPntRow[i], vStartPntCol[i]));
//	}
//}

namespace pl
{
	void pl::MultiAuctionSTC::process()
	{
		_GridMap.clear();
		size_t gridNum = boost::num_vertices(_ob_sGraph);
		for (size_t i = 0; i < gridNum; i++)
			_GridMap.insert(pair<bex::VertexDescriptor, int>(i, -1));

		formSpanningTree();
		auction();
		this->_vTreeSgs.clear();
		this->_vTreeSgs.resize(this->_robNum);
		getSpanningTreeSgs();
		exchange2TreeSgs();

		getNewGraph();
		searchVitualPath();
		generateRealPath();
		writeMultiPath();
		wirteMultiGraphSeg();		
		calMakeSpan();
	}

	void MultiAuctionSTC::writeRobGraph()
	{
		writeDebug(c_deg, "robNum", _robNum);
		for (size_t i = 0; i < _robNum; i++)
		{
			string str_row = "row";
			string str_col = "col";
			str_row += std::to_string(i);
			str_col += std::to_string(i);
			vector<size_t> vRow, vCol;

			for (auto &it : (_vRobSetPtr->at(i)))
			{
				auto &ind = _ob_graphVd2GridInd[it];
				bool ad_VLB = false;
				bool ad_VRT = false;
				bool ad_VLT = false;
				bool ad_VRB = false;
				switch (ind.second)
				{
				case NOVIR:{
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
				if(ad_VLB)
					if (_ob_tGrid[_ob_STCGrid[ind].LB] == bex::WayVert)
					{
						vRow.push_back(_ob_STCGrid[ind].LB.first);
						vCol.push_back(_ob_STCGrid[ind].LB.second);
					}
				if (ad_VRT)
					if (_ob_tGrid[_ob_STCGrid[ind].RT] == bex::WayVert)
					{
						vRow.push_back(_ob_STCGrid[ind].RT.first);
						vCol.push_back(_ob_STCGrid[ind].RT.second);
					}
				if (ad_VLT)
					if (_ob_tGrid[_ob_STCGrid[ind].LT] == bex::WayVert)
					{
						vRow.push_back(_ob_STCGrid[ind].LT.first);
						vCol.push_back(_ob_STCGrid[ind].LT.second);
					}
				if (ad_VRB)
					if (_ob_tGrid[_ob_STCGrid[ind].RB] == bex::WayVert)
					{
						vRow.push_back(_ob_STCGrid[ind].RB.first);
						vCol.push_back(_ob_STCGrid[ind].RB.second);
					}
			}
			writeDebug(c_deg, str_row, vRow);
			writeDebug(c_deg, str_col, vCol);
		}

		//for (size_t i = 0; i < _robNum; i++)
		//{
		//	string str_row = "neiRow";
		//	string str_col = "neiCol";
		//	str_row += std::to_string(i);
		//	str_col += std::to_string(i);
		//	vector<size_t> vRow, vCol;
		//	for (auto &it : (_vRobNeiPtr->at(i)))
		//	{				
		//		vRow.push_back(_ob_sgraph2map[it].first);
		//		vCol.push_back(_ob_sgraph2map[it].second);
		//	}
		//	writeDebug(c_deg, str_row, vRow);
		//	writeDebug(c_deg, str_col, vCol);
		//}
	}
	void MultiAuctionSTC::formSpanningTree()
	{

		typedef bt::adjacency_list < bt::vecS, bt::vecS, bt::undirectedS,
			bt::property<bt::vertex_distance_t, int>, bt::property < bt::edge_weight_t, int > > SGraph;
		typedef std::pair < int, int >SE;
		auto &graph = _ob_sGraph;
		const int num_nodes = bt::num_vertices(graph);
		vector<size_t> vSvd, vTvd;
		vector<bex::VertexDescriptor> vvd;
		for (size_t i = 0; i < num_nodes; i++)
			vvd.push_back(i);

		for (size_t i = 0; i < vvd.size(); i++)
		{
			for (size_t j = i; j < vvd.size(); j++)
			{
				if (_mainMap.isConnected(vvd[i], vvd[j], pl::graphType::span))
				{
					vSvd.push_back(i);
					vTvd.push_back(j);
				}
			}
		}
		const int edgeNum = vSvd.size();
		SE * edgesPtr = new SE[edgeNum];
		int * weightPtr = new int[edgeNum];
		for (size_t i = 0; i < vSvd.size(); i++)
		{
			//cout << "index = " << i << endl;
			edgesPtr[i] = SE(vSvd[i], vTvd[i]);
			weightPtr[i] = 1;
		}


		SGraph g(edgesPtr, edgesPtr + edgeNum, weightPtr, num_nodes);

		std::vector < bt::graph_traits < SGraph >::vertex_descriptor >
			p(num_vertices(g));
		bt::prim_minimum_spanning_tree(g, &p[0]);

		vector<double> sPntx, sPnty, tPntx, tPnty;
		cout << "p size = " << p.size() << endl;
		for (std::size_t i = 0; i < p.size(); ++i)
		{
			cout << i << endl;
			cout << i << " vvd[i]" << vvd[i] << " vvd[p[i]]" << vvd[p[i]] << endl;
			//auto sg = bex::DSegment(_ob_sGraph[vvd[i]].pnt, _ob_sGraph[vvd[p[i]]].pnt);
			//_vTreeSgs[robID].push_back(sg);

			bex::VertexProperty &sVert = graph[vvd[i]];
			bex::VertexProperty &tVert = graph[vvd[p[i]]];

			sPntx.push_back(sVert.pnt.x());
			sPnty.push_back(sVert.pnt.y());

			//cout << "pnt.x " << tVert.pnt.x() << "	pnt.y" << tVert.pnt.y() << endl;
			tPntx.push_back(tVert.pnt.x());
			tPnty.push_back(tVert.pnt.y());
		}
		writeDebug(c_deg, "g_sPntx", sPntx);
		writeDebug(c_deg, "g_sPnty", sPnty);
		writeDebug(c_deg, "g_tPntx", tPntx);
		writeDebug(c_deg, "g_tPnty", tPnty);
	}

	void MultiAuctionSTC::testAstar()
	{
		
	}

	void MultiAuctionSTC::writeMultiGraph()
	{
		vector<double> sPntx, sPnty, tPntx, tPnty;
		for (size_t i = 0; i < _robNum; i++)
		{
			auto &graph = this->_m_vGraph[i];
			std::pair<bex::EdgeIterator, bex::EdgeIterator> _b2e_ei = boost::edges(graph);
			//string str_sPntx = "sPntx";
			//string str_sPnty = "sPnty";
			//string str_tPntx = "tPntx";
			//string str_tPnty = "tPnty";
			//str_sPntx += std::to_string(i);
			//str_sPnty += std::to_string(i);
			//str_tPntx += std::to_string(i);
			//str_tPnty += std::to_string(i);
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
		}
		writeDebug(c_deg, "m_sPntx", sPntx);
		writeDebug(c_deg, "m_sPnty", sPnty);
		writeDebug(c_deg, "m_tPntx", tPntx);
		writeDebug(c_deg, "m_tPnty", tPnty);

	}
	

	void MultiAuctionSTC::writeMultiPath()
	{
		for (size_t i = 0; i < _robNum; i++)
		{
			string str_x = "path_x";
			string str_y = "path_y";
			str_x += std::to_string(i);
			str_y += std::to_string(i);
			vector<double> path_x, path_y;

			for (auto &it:this->_vpath[i])
			{
				path_x.push_back(it.x());
				path_y.push_back(it.y());
			}
			writeDebug(c_deg, str_x, path_x);
			writeDebug(c_deg, str_y, path_y);
		}
	}

	void MultiAuctionSTC::wirteMultiGraphSeg()
	{
		vector<double> sPntx, sPnty, tPntx, tPnty;
		for (size_t i = 0; i < _robNum; i++)
		{
			auto &treeSgs = _vTreeSgs[i];
			for (auto &it:treeSgs)
			{
				sPntx.push_back(it.first.x());
				sPnty.push_back(it.first.y());
				tPntx.push_back(it.second.x());
				tPnty.push_back(it.second.y());
			}
		}
		writeDebug(c_deg, "sPntx", sPntx);
		writeDebug(c_deg, "sPnty", sPnty);
		writeDebug(c_deg, "tPntx", tPntx);
		writeDebug(c_deg, "tPnty", tPnty);
	}

	void MultiAuctionSTC::writeMultiGraphEdge()
	{

	}


	void pl::MultiAuctionSTC::auction()
	{
		_vRobSetPtr = make_shared<vector<set<size_t>>>(_robNum);
		_vRobGridPtr = make_shared<vector<vector<GridIndex>>>(_robNum);
		_vRobMapPtr = make_shared<vector<map<size_t, size_t>>>(_robNum);
		_vRobEdgePtr = make_shared<vector<vector<STCEdge>>>(_robNum);
		_vRobNeiPtr = make_shared<vector<set<bex::VertexDescriptor>>>(_robNum);
		_vRobSleepPtr = make_shared<vector<bool>>(_robNum, false);
		_vRobEstCostPtr = make_shared<vector<double>>(_robNum);
		vector<bool> allAucEd(bt::num_vertices(_ob_sGraph), false);

		auto aucCompleted = [](vector<bool> const &vb) {
			auto iter = std::find(vb.begin(), vb.end(), false);
			if (iter == vb.end())
				return true;
			return false;
		};

		size_t circleTime = 0;
		size_t maxcircleTime = _mainMap.getReachableVertNum() * 1.2;
		std::default_random_engine eng;
		eng.seed(randomSeed);
		std::uniform_int_distribution<int> dis(0, _robNum - 1);
		for (size_t i = 0; i < _robNum; i++)
		{
			//_vRobGridPtr->at(i).push_back(_vStartPnt[i]);
			auto ind = _mainMap.tGridInd2SGridInd(_vStartPnt[i]);
			_vRobSetPtr->at(i).insert(_ob_gridInd2GraphVd[ind]);	
			_GridMap[_ob_gridInd2GraphVd[ind]] = i;
			updateNeiGraph(i);
		}
		do
		{
			size_t aucNeer = circleTime++%_robNum;
			bex::VertexDescriptor aucVertID;

			if (circleTime == 253)
				c_deg << "";
			bool allOccupied = calAucVertID(aucNeer, aucVertID);
			//c_deg << "circleTime = " << circleTime << endl;
			//c_deg << "aucNeer = " << aucNeer << endl;
			//c_deg << "aucVertID = " << aucVertID << endl;
			//c_deg << "aucVertGridInd.first =  " << this->_ob_sgraph2map[aucVertID].first <<
			//	"aucVertGrid.second  = " << this->_ob_sgraph2map[aucVertID].second << endl;
			if (_ob_sgraph2map[aucVertID].first == 9 && _ob_sgraph2map[aucVertID].second == 9)
			{
				cout << "bug" << endl;
			}
			if (aucVertID == 155)
				c_deg << "";
			if (aucNeer == 0 && _ob_sgraph2map[aucVertID].first == 19 && _ob_sgraph2map[aucVertID].second == 6)
			{
				c_deg << " " << endl;
			}
			
			size_t robWinner = maxBiddingRob(aucVertID);
			//_vRobSetPtr->at(robWinner).insert(aucVertID);
			if (allOccupied)
			{
				if (robWinner != _GridMap[aucVertID])
				{
					//earse loser
					auto &robLoserSet = _vRobSetPtr->at(_GridMap[aucVertID]);
					robLoserSet.erase(aucVertID);
					updateNeiGraph(_GridMap[aucVertID]);
					//update winner
					auto &robWinnerSet = _vRobSetPtr->at(robWinner);
					robWinnerSet.insert(aucVertID);
					_GridMap[aucVertID] = robWinner;
					updateNeiGraph(robWinner);
				}
			}
			else
			{
				auto &robWinnerSet = _vRobSetPtr->at(robWinner);
				robWinnerSet.insert(aucVertID);
				_GridMap[aucVertID] = robWinner;
				updateNeiGraph(robWinner);
				
			}
#ifdef _DEBUG
			_vRobGridPtr->at(robWinner).push_back(_ob_sgraph2map[aucVertID]);
#endif // _DEBUG
			cout << "circleTime = " << circleTime << endl;
			if ( circleTime >= 1000/*maxcircleTime*/)
			{
				break;
			}
		} while (!aucCompleted(*_vRobSleepPtr));

		vector<size_t> vSize;
		size_t allNumSize = 0;

		for (size_t i = 0; i < _robNum; i++)
		{
			c_deg << "rob" << i << "	set size = " << _vRobSetPtr->at(i).size() << endl;
			//c_deg << "rob" << i << "	set size = " << _vRobSetPtr->at(i).size() << endl;
			vSize.push_back(_vRobSetPtr->at(i).size());
			allNumSize += _vRobSetPtr->at(i).size();
		}
		auto maxSize = *std::max_element(vSize.begin(), vSize.end());
		auto minSize = *std::min_element(vSize.begin(), vSize.end());

		c_deg << "max  = " << maxSize << " min = " << minSize << endl;
		c_deg << "allNumSize = " << allNumSize << endl;
		cout << "safe" << endl;
	}

	void MultiAuctionSTC::getSpanningTreeSgs()
	{
		_vLeafSet.clear();
		_vLeafSet.resize(_robNum);
		_vLeafSTCSet.clear();
		_vLeafSTCSet.resize(_robNum);
		_vNoLeafSTCSet.clear();
		_vNoLeafSTCSet.resize(_robNum);
		_vNoPathInd.clear();
		_vNoPathInd.resize(_robNum);
		//_vvBoolTree.clear();
		//_vvBoolTree.resize(_robNum);
		typedef bt::adjacency_list < bt::vecS, bt::vecS, bt::undirectedS,
			bt::property<bt::vertex_distance_t, int>, bt::property < bt::edge_weight_t, int > > SGraph;
		typedef std::pair < int, int >SE;
		auto &graph = _ob_sGraph;
		for (size_t robID = 0; robID < _robNum; robID++)
		{
			cout << "rob is = " << robID << endl;
			//auto &graph = _mainMap.getGraph(graphType::span);
			//const int num_nodes = bt::num_vertices(graph);
			const int num_nodes = this->_vRobSetPtr->at(robID).size();

			vector<size_t> vSvd, vTvd;
			vector<bex::VertexDescriptor> vvd;
			for (auto &it : this->_vRobSetPtr->at(robID))
				vvd.push_back(it);
			//auto &vBoolTree = _vvBoolTree[robID];
			//vBoolTree.assign(vvd.size(), true);
			vector<bool> vBoolTree(vvd.size(),true);		
			// 等于true时，正常计算
			// 等于false时，非正常计算
	//		for (auto iter = vvd.begin(); iter != vvd.end();iter++)
	//		{
	//			if (_mainMap.vitualVertSet.count(_ob_graphVd2GridInd[*iter].first) == 1)
	//			{
	//				auto c_vvd = vvd;
	//				c_vvd.erase(iter - vvd.begin() + c_vvd.begin());
	//				if (_mainMap.allConnected(c_vvd))
	//				{
	//					//cout << " +" << vvd[iter - vvd.begin()] << endl;
	//					vBoolTree[iter - vvd.begin()] = false;
	//					//_vLeafSTCSet[robID].push_back(*iter);
	//					_vLeafSTCSet[robID].insert(*iter);
	//					//auto wtfff = iter - vvd.begin();
	//					//_vLeafSet[robID].push_back(iter - vvd.begin());
	//				}
	//				else
	//				{
	//					_vNoLeafSTCSet[robID].insert(*iter);
	////					cout << " -" << vvd[iter - vvd.begin()] << endl;
	//				}
	//			}
	//		}
			for (size_t i = 0; i < vvd.size(); i++)
			{
				for (size_t j = i; j < vvd.size(); j++)
				{
					//assume the virtual vertex dosen't connected
						if (isAdjacent(vvd[i], vvd[j]))
						{
							vSvd.push_back(i);
							vTvd.push_back(j);
						}
				}
			}
			const int edgeNum = vSvd.size();
			SE * edgesPtr = new SE[edgeNum];
			int * weightPtr = new int[edgeNum];
			for (size_t i = 0; i < vSvd.size(); i++)
			{
				//cout << "index = " << i << endl;
				edgesPtr[i] = SE(vSvd[i], vTvd[i]);
				weightPtr[i] = 1;
			}


			SGraph g(edgesPtr, edgesPtr + edgeNum, weightPtr, num_nodes);

			std::vector < bt::graph_traits < SGraph >::vertex_descriptor >
				p(num_vertices(g));
			bt::prim_minimum_spanning_tree(g, &p[0]);
			for (std::size_t i = 0; i != p.size(); ++i)
			{
				_vRobEdgePtr->at(robID).push_back(STCEdge(vvd[i], vvd[p[i]]));

				//cout << i << " vvd[i]" << vvd[i] << " vvd[p[i]]" << vvd[p[i]] << endl;
				auto sg = bex::DSegment(_ob_sGraph[vvd[i]].pnt, _ob_sGraph[vvd[p[i]]].pnt);
				_vTreeSgs[robID].push_back(sg);
			}
		}
#ifdef _DEBUG
		cout << "get spanning tree sgs success" << endl;
#endif // _DEBUG		
	}

	void MultiAuctionSTC::getNewGraph()
	{
		auto &graph = _ob_tGraph;
		_vRobSetBasePtr = make_shared<vector<set<size_t>>>(_robNum);
		_vT2local.clear();
		_vlocal2T.clear();
		_vSlocal2T.clear();
		_vT2Slocal.clear();
		for (size_t p = 0; p < _robNum; p++)
		{
			//if (p != 1)
			//	continue;
			bex::Graph new_graph;
			set<bex::VertexDescriptor> &robBaseSet = this->_vRobSetBasePtr->at(p);
			auto &robSet = _vRobSetPtr->at(p);
			map<size_t, size_t> T2local;
			map<size_t, size_t> local2T;
			map<size_t, size_t> Slocal2T;
			map<size_t, size_t> T2Slocal;
			bex::VertexDescriptor localVd = 0;
			size_t gSTCInd = 0;
			for (auto &it : robSet)
			{

				//auto &it = robSet[i];
				bex::VertexDescriptor svd = it;
				if (svd == 180)
					cout << "162" << endl;
				// 此处添加..
				vector<bex::VertexDescriptor> vvd;
				if (_vNoLeafSTCSet[p].count(svd) == 0)
				{
					vvd = _mainMap.STCGraphVd2TGraphVd(svd);
					if (_vLeafSTCSet[p].count(svd) == 1)
					{
						_vLeafSet[p].push_back(localVd);
					}
					for (auto &vd : vvd)
					{
						robBaseSet.insert(vd);
						//_vRobBasePtr->at(p).push_back(vd);
						bex::VertexProperty vp = graph[vd];
						vp.EdgeState = false;
						vp.NeighbourState = false;
						vp.QueueState = false;
						bt::add_vertex(vp, new_graph);
						T2local.insert(pair<size_t, size_t>(vd, localVd));
						local2T.insert(pair<size_t, size_t>(localVd, vd));
						localVd++;
					}
				}
				else
				{

					//此处存在问题
					//导致转换 混乱
					auto realVd = _mainMap.STCGraphVd2TGraphVd(svd);
					vvd = _mainMap.STCGraphVd2TGraphVdSp(svd);
					for (auto &vd : vvd)
					{
						robBaseSet.insert(vd);
						if (vd != realVd.back())
						{
							_vNoPathInd[p].insert(localVd);
						}
						bex::VertexProperty vp = graph[vd];
						vp.EdgeState = false;
						vp.NeighbourState = false;
						vp.QueueState = false;
						bt::add_vertex(vp, new_graph);
						T2Slocal.insert(pair<size_t, size_t>(vd, localVd));
						Slocal2T.insert(pair<size_t, size_t>(localVd, vd));
						localVd++;
						//_vRobBasePtr->at(p).push_back(vd);
					}
				}
			}
			_vT2local.push_back(T2local);
			_vlocal2T.push_back(local2T);
			_vT2Slocal.push_back(T2Slocal);
			_vSlocal2T.push_back(Slocal2T);
			//add edge 
			std::pair<bex::VertexIterator, bex::VertexIterator> vi = boost::vertices(new_graph);
			for (bex::VertexIterator vit = vi.first; vit != vi.second; vit++)
			{
				bex::VertexDescriptor vd = *vit;
				if (vd == 193)
					cout << "wtf" << endl;
				if (local2T.count(vd) == 1)
				{
					/*			cout << "___" << endl;
					cout << " new_graph[vd].pnt x = " << new_graph[vd].pnt.x()
					<< " new_graph[vd].pnt y = " << new_graph[vd].pnt.y() << endl;
					*/
					auto tvd = local2T[vd];
					auto nei_vvd = _mainMap.getSearchAllNeighbor(tvd);
					//auto vlocalIndex = getVerticalNeighbour(localIndex);
					//getSearchNeighbor(localIndex, graphType::span);
					std::vector<int> vvd;
					for (auto &it : nei_vvd)
					{
						if (robBaseSet.count(it) == 1)
						{
							if (T2Slocal.count(it) == 1)
							{
								if (std::find(_vLeafSet[p].begin(), _vLeafSet[p].end(), it) == _vLeafSet[p].end())
								{
									vvd.push_back(T2Slocal[it]);
								}
							}
							if (T2local.count(it) == 1)
							{
								vvd.push_back(T2local[it]);
							}
							//						cout << "nei__it " << it << endl;
							//vvd.push_back(T2local[it]);
						}
					}
					for (auto &it : vvd)
					{
						//					cout << "vvd_it  = " << it << endl;

						if (new_graph[it].EdgeState == false)
						{
							bex::DSegment sg(new_graph[it].pnt, new_graph[vd].pnt);
							//cout << " new_graph[it].pnt x = " << new_graph[it].pnt.x()
							//	<< " new_graph[it].pnt y = " << new_graph[it].pnt.y() << endl;


							if (this->treeIntersection(sg, p))
								//判断是否相交
							{
								//								std::cout << "inter" << endl;
							}
							else
							{
								bex::EdgeProperty ep;
								//ep.weight = bg::distance(sGraph[vd].pnt, sGraph[it].pnt);
								boost::add_edge(it, vd, ep, new_graph);
							}
						}
					}
				}

				if (Slocal2T.count(vd) == 1)
				{
					auto tvd = Slocal2T[vd];
					auto nei_vvd = _mainMap.getSearchAllNeighbor(tvd);
					//auto vlocalIndex = getVerticalNeighbour(localIndex);
					//getSearchNeighbor(localIndex, graphType::span);
					std::vector<int> vvd;
					for (auto &it : nei_vvd)
					{
						if (robBaseSet.count(it) == 1)
						{
							if (T2Slocal.count(it) == 1)
							{
								vvd.push_back(T2Slocal[it]);
							}
							else
							{
								vvd.push_back(T2local[it]);
							}
							//cout << "nei__it " << it << endl;
						}
					}
					for (auto &it : vvd)
					{
						if (new_graph[it].EdgeState == false)
						{
							bex::DSegment sg(new_graph[it].pnt, new_graph[vd].pnt);
							//cout << " new_graph[it].pnt x = " << new_graph[it].pnt.x()
							//	<< " new_graph[it].pnt y = " << new_graph[it].pnt.y() << endl;


							if (this->treeIntersection(sg, p))
								//判断是否相交
							{
								//								std::cout << "inter" << endl;
							}
							else
							{
								bex::EdgeProperty ep;
								//ep.weight = bg::distance(sGraph[vd].pnt, sGraph[it].pnt);
								boost::add_edge(it, vd, ep, new_graph);
							}
						}
					}
				}
				new_graph[vd].EdgeState = true;
				//size_t num_edges = bt::num_edges(new_graph);
				//cout << "num_edges = " << num_edges << endl;
			}

			this->_m_vGraph.push_back(new_graph);
		}
	}

	void MultiAuctionSTC::searchVitualPath()
	{
		_vpathIndex.clear();
		_vpath.clear();
		_vpath.resize(_robNum);
		_vpathIndex.resize(_robNum);
		for (size_t p = 0; p < _robNum; p++)
		{
			//if (p != 1)
			//	continue;
			auto _startPnt = this->_vStartPnt[p];
			GridIndex  cenGridIndex = _startPnt;
			bex::VertexDescriptor totalCenVd = this->_ob_tmap2graph[cenGridIndex];

			bex::VertexDescriptor cenVd = _vT2local[p][totalCenVd];
			bex::VertexDescriptor canVd(cenVd);
			size_t cenDir = DirType::center;
			size_t canDir = DirType::center;

			auto &_pathIndex = _vpathIndex[p];
			auto &_path = _vpath[p];
			auto &localGraph = _m_vGraph[p];
			auto &leafSet = _vLeafSet[p];
			size_t i = 0;
			do
			{
#ifdef _DEBUG
				//	cout << "i = " << i++ << endl;
#endif // _DEBUG
				cenVd = canVd;
				if (cenVd == 152)
					cout << "wtf" << endl;
				cenDir = canDir;
				//canDir = cenDir;
				_pathIndex.push_back(cenVd);
				auto neighborsIter = boost::adjacent_vertices(cenVd, localGraph);
				bex::VertexProperty &cenVp = localGraph[cenVd];
				cenVp.NeighbourState = true;
				_path.push_back(cenVp.pnt);
				vector<bex::VertexDescriptor> vNeighbors;
				for (auto ni = neighborsIter.first; ni != neighborsIter.second; ++ni)
				{
					if (find(leafSet.begin(), leafSet.end(), *ni) == leafSet.end())
					{
						auto &wtf = localGraph[*ni];
						if (!localGraph[*ni].NeighbourState)
						{
							vNeighbors.push_back(*ni);
						}
					}
					else
					{

						bex::VertexDescriptor g_it, g_cenVd;
						if (_vlocal2T[p].count(*ni) == 1)
							g_it = _vlocal2T[p][*ni];
						else
							g_it = _vSlocal2T[p][*ni];
						if (_vlocal2T[p].count(cenVd) == 1)
							g_cenVd = _vlocal2T[p][cenVd];
						else
							g_cenVd = _vSlocal2T[p][cenVd];
						//此处需要修改
						if (_mainMap.isConnected(g_it, g_cenVd, graphType::base))
						{
							localGraph[*ni].NeighbourState = true;
							leafSet.erase(find(leafSet.begin(), leafSet.end(), *ni));
							_pathIndex.push_back(*ni);
							_path.push_back(localGraph[*ni].pnt);
							_pathIndex.push_back(cenVd);
							_path.push_back(cenVp.pnt);
						}
					}
				}
				canVd = cenVd;
				vector<size_t> vCanVd;
				bool chsSameMega = false;
				//
				for (auto &it : vNeighbors)
				{
					auto &vp = localGraph[it];
					//has not been covered
					if (vp.NeighbourState == false)
					{
						bex::VertexDescriptor g_it, g_cenVd;
						if (_vlocal2T[p].count(it) == 1)
							g_it = _vlocal2T[p][it];
						else
							g_it = _vSlocal2T[p][it];
						if (_vlocal2T[p].count(cenVd) == 1)
							g_cenVd = _vlocal2T[p][cenVd];
						else
							g_cenVd = _vSlocal2T[p][cenVd];

						if (_mainMap.inSameSTCMegaBox(g_it, g_cenVd))
						{
							canVd = it;
							chsSameMega = true;
							break;
						}
						else
						{
							vCanVd.push_back(it);
						}
					}
				}
				//no vertex in the same mega box
				if (!chsSameMega)
				{
					if (!vCanVd.empty())
					{
						if ((vCanVd.size() == 1) || (cenDir == DirType::center))
						{
							canVd = vCanVd.back();
						}
						else
						{
							vector<size_t> vDir;
							for (auto &it : vCanVd)
							{
								if (cenDir == getDir(cenVd, it, p))
									canVd = it;
							}
						}
					}
				}
				canDir = getDir(cenVd, canVd, p);
		} while (canVd != cenVd);
		//cout << "0-0" << endl;
	}
		cout << "path___success" << endl;
	}

	void MultiAuctionSTC::generateRealPath()
	{
		auto vVitualPathIndex = _vpathIndex;
		//auto vVitualPath = _vpath;
		_vpathIndex.clear();
		_vpath.clear();
		_vpath.resize(_robNum);
		_vpathIndex.resize(_robNum);

		for (size_t p = 0; p < _robNum; p++)
		{

			aplan.i_setSize.clear();
			set<GridIndex> _robBaseSet;
			for (auto &it : _vRobSetBasePtr->at(p))
			{
				_robBaseSet.insert(_ob_tgraph2map[it]);
			}
			aplan.i_setSize = _robBaseSet;
			vector<GridIndex> allWayPnt;
			auto &_vitualPathIndex = vVitualPathIndex[p];
			size_t wayPntCount = 0;
			for (size_t i = 0; i < _vitualPathIndex.size(); i++)
			{
				auto  &ind = this->_ob_tgraph2map[_vlocal2T[p][_vitualPathIndex[i]]];
				if (_ob_tGrid[ind] == bex::vertType::WayVert && _vNoPathInd[p].count(_vitualPathIndex[i]) == 0)
				{
					wayPntCount++;
				}
			}
			set<GridIndex> cmpWayPnt;
			//auto _startPnt = this->_vStartPnt[]
			auto _startPnt = this->_vStartPnt[p];
			auto &_pathIndex = _vpathIndex[p];
			auto &_path = _vpath[p];
			auto &localGraph = _m_vGraph[p];
			_pathIndex.push_back(_vitualPathIndex.front());
			_path.push_back(bex::DPoint(_startPnt.first + 0.5, _startPnt.second + 0.5));
			cmpWayPnt.insert(_mainMap.pnt2IndexInBaseGrid(_path.front()));
			bool nei_bool = true;
			for (size_t i = 1; i < _vitualPathIndex.size(); i++)
			{
				auto  &ind = this->_ob_tgraph2map[_vlocal2T[p][_vitualPathIndex[i]]];
				if (_ob_tGrid[ind] == bex::vertType::WayVert && _vNoPathInd[p].count(_vitualPathIndex[i]) == 0)
				{
					if (nei_bool)
					{
						//						_pathIndex.push_back(_vitualPathIndex[i]);
						_path.push_back(localGraph[_vitualPathIndex[i]].pnt);
						//_mainMap.
						cmpWayPnt.insert(_mainMap.pnt2IndexInBaseGrid(localGraph[_vitualPathIndex[i]].pnt));
					}
					else
					{
						//astar
						auto lastInd = _ob_tgraph2map[_vlocal2T[p][_pathIndex.back()]];
						aplan.init(lastInd, ind);
						aplan.plan();
						//for (auto &it :aplan.m_path)
						//{
						//	_path.push_back(it);
						//}
						for (size_t n = 1; n < aplan.m_path.size(); n++)
						{
							_path.push_back(aplan.m_path[n]);
							cmpWayPnt.insert(_mainMap.pnt2IndexInBaseGrid(aplan.m_path[n]));
						}
						//_path.emplace(aplan.m_path.begin(), aplan.m_path.end());
					}
					_pathIndex.push_back(_vitualPathIndex[i]);
					nei_bool = true;
				}
				else
				{
					nei_bool = false;
				}
				cout << "size = " << cmpWayPnt.size() << endl;
				//		if (cmpWayPnt.size() == wayPntCount)
				//			break;
			}
			cout << "stop" << endl;
		}
		cout << "generate real path" << endl;
	}

	bool MultiAuctionSTC::exchange2TreeSgs()
	{
		
		this->_vTreeSgs.clear();
		this->_vTreeSgs.resize(this->_robNum);

		_vLeafSet.clear();
		_vLeafSet.resize(_robNum);
		_vLeafSTCSet.clear();
		_vLeafSTCSet.resize(_robNum);
		_vNoLeafSTCSet.clear();
		_vNoLeafSTCSet.resize(_robNum);
		_vNoPathInd.clear();
		_vNoPathInd.resize(_robNum);

		for (size_t p = 0; p< _robNum; p++)
		{
			auto &robEdges = this->_vRobEdgePtr->at(p);
			updateMap(p);
			for (size_t i = 0; i < robEdges.size(); i++)
			{
				auto &sInd = robEdges[i].first;
				auto &tInd = robEdges[i].second;
				auto &gridInd = _ob_graphVd2GridInd[sInd].first;
				if (sInd == 216)
					cout << "wtf" << endl;
				if (_mainMap.vitualVertSet.count(gridInd) == 1)
				{
					if (_vRobMapPtr->at(p)[sInd] == 1)
					{
						_vLeafSTCSet[p].insert(sInd);
						continue;
					}
					_vNoLeafSTCSet[p].insert(sInd);
				}
				auto sg = bex::DSegment(_ob_sGraph[sInd].pnt, _ob_sGraph[tInd].pnt);
				_vTreeSgs[p].push_back(sg);
				//if(robEdges)
			}
		}
		return false;
		return false;
	}

	bool MultiAuctionSTC::updateMap(size_t const & robID)
	{
		auto &robLoserEdgeSet = _vRobEdgePtr->at(robID);
		auto &robMap = _vRobMapPtr->at(robID);
		robMap.clear();
		for (size_t i = 0; i < robLoserEdgeSet.size(); i++)
		{
			if (robMap.count(robLoserEdgeSet[i].first) == 1)
			{
				robMap[robLoserEdgeSet[i].first]++;
			}
			else
			{
				robMap[robLoserEdgeSet[i].first] = 1;
			}

			if (robMap.count(robLoserEdgeSet[i].second) == 1)
			{
				robMap[robLoserEdgeSet[i].second]++;
			}
			else
			{
				robMap[robLoserEdgeSet[i].second] = 1;
			}
		}
		return false;
	}


	bool MultiAuctionSTC::treeIntersection(bex::DSegment const sg, size_t const robID) const
	{
		auto &vTreeSg = this->_vTreeSgs[robID];
		for (auto &it : vTreeSg)
		{
			if (bg::intersects(it, sg))
			{
				return true;
			}
		}
		return false;
	}

	size_t MultiAuctionSTC::getDir(bex::VertexDescriptor const & lcen_index, bex::VertexDescriptor const & ln_index, size_t const & robID)
	{
		auto &local2T = _vlocal2T[robID];
		bex::VertexDescriptor cen_index = local2T[lcen_index];
		bex::VertexDescriptor n_index = local2T[ln_index];

		GridIndex cenGridInd = _ob_tgraph2map[cen_index];
		GridIndex nGridInd = _ob_tgraph2map[n_index];
		// x equal 
		if (cenGridInd.first == nGridInd.first)
		{
			if (cenGridInd.second == (nGridInd.second - 1)) {
				return DirType::bottom;
			}
			if (cenGridInd.second == (nGridInd.second + 1)) {
				return DirType::top;
			}
		}
		if (cenGridInd.second == nGridInd.second)
		{
			if (cenGridInd.first == (nGridInd.first - 1)) {
				return DirType::left;
			}
			if (cenGridInd.first == (nGridInd.first + 1)) {
				return DirType::right;
			}
		}
		return DirType::center;
		//return size_t();
	}

	bool MultiAuctionSTC::isAdjacent(bex::VertexDescriptor const & vd1, bex::VertexDescriptor const & vd2)
	{
		auto &graph = _ob_sGraph;
		auto neighborIter = boost::adjacent_vertices(vd1, graph);
		std::vector<size_t> vVd;
		for (auto ni = neighborIter.first; ni != neighborIter.second; ++ni)
			vVd.push_back(*ni);
		auto iter = std::find(vVd.begin(), vVd.end(), vd2);
		if (iter == vVd.end())
			return false;
		return true;
	}

	bool MultiAuctionSTC::calAucVertID(size_t const & aucNeer, size_t & aucVertID)
	{

		double UnCoverMinFit = 99999;
		double coverMinFit = 999999;
		double unCoverMaxPri = -1;
		int UnCoverResVd = -1;
		int coverResVd = -1;
		bool allCovered = true;
		auto &graph = _ob_sGraph;
		auto &robSet = (*_vRobSetPtr)[aucNeer];
		auto &robNeiSet = (*_vRobNeiPtr)[aucNeer];
		//auto &robNghSet = (*_vRobNghSetPtr)[robID];

		if (robNeiSet.empty())
		{
			(*_vRobSleepPtr)[aucNeer] = true;
			return false;
		}
		vector<tuple<size_t,size_t,size_t>> robOpNeiSet;
		vector<size_t> robOpCandi;
		for (auto it = robNeiSet.begin(); it != robNeiSet.end(); it++)
		{
			bool inOtherSet = false;
			if (_GridMap[*it] != -1)
			{
				inOtherSet = true;
				auto &opRobID = _GridMap[*it];
				robOpCandi.push_back(_vRobSetPtr->at(opRobID).size());
				robOpNeiSet.push_back(tuple<size_t, size_t, size_t>(opRobID,*it, _vRobSetPtr->at(opRobID).size()));
			}
			if (!inOtherSet) {
				double fit = this->calUnopPriority(aucNeer, *it);
				// need optimal
				//double dis = bg::distance(graph[it->first].pnt, graph[it->second].pnt);
				if (fit > unCoverMaxPri)
				{
					unCoverMaxPri = fit;
					UnCoverResVd = *it;
					allCovered = false;
				}
			}
		}

		if (UnCoverResVd != -1) { aucVertID = UnCoverResVd; return allCovered; }


		//end the rule 1
		auto cmpOp = [](tuple<size_t, size_t, size_t> ind0, tuple<size_t, size_t, size_t> ind1)
		{
			if (get<2>(ind0) < get<2>(ind1))
			{
				return true;
			}
			return false;
		};

		std::sort(robOpNeiSet.begin(), robOpNeiSet.end(), cmpOp);
		//for (size_t i = 0; i < robOpNeiSet.size(); i++)
		//{
		//	cout<<i << "	robOpNeiSet[i].first = " << robOpNeiSet[i].first << "	robOpNeiSet[i].second =  " << robOpNeiSet[i].second << endl;
		//}
		auto maxCandi = get<2>(robOpNeiSet.back());
		if (robSet.size() >= (maxCandi - 1))
		{
			(*_vRobSleepPtr)[aucNeer] = true;
			return allCovered;
		}
		else
		{
			(*_vRobSleepPtr)[aucNeer] = false;
		}
		auto size = robOpNeiSet.size();
		for (size_t i = 0; i < robOpNeiSet.size(); i++)
		{
			if (calOpPriority(aucNeer, get<0>(robOpNeiSet[size - 1 - i]), get<1>(robOpNeiSet[size - 1 - i])) != std::numeric_limits<double>::max())
			{
				aucVertID = get<1>(robOpNeiSet[size - 1 - i]);
				return allCovered;
			}
		}
		//_vRobSleepPtr->at(robID) = true;
		return allCovered;
	}

	size_t MultiAuctionSTC::maxBiddingRob(size_t const & aucVertID)
	{
		size_t maxBidding = -1;
		vector<double> vBidding;
		for (size_t i = 0; i < _robNum; i++)
		{
			//in the neighbor vertex set 
			if (_vRobNeiPtr->at(i).count(aucVertID) == 1)
			{
				double bidding = 1 / double(_vRobSetPtr->at(i).size());
				vBidding.push_back(bidding);
			}
			else
			{
				// in the occupy neighbor vertex set 
				if (_vRobSetPtr->at(i).count(aucVertID) == 1)
				{
					double bidding = 1 / double(_vRobSetPtr->at(i).size());
					vBidding.push_back(bidding);
				}
				// nothing
				else
				{
					//bidding = 0;
					vBidding.push_back(0);
				}
			}
		}
		size_t robWinner = (std::max_element(vBidding.begin(), vBidding.end())-vBidding.begin());
		return robWinner;
	}

	double MultiAuctionSTC::calBidding(size_t const & bidding)
	{
		return 0.0;
	}


	double MultiAuctionSTC::calUnopPriority(size_t const & robID, bex::VertexDescriptor const & vd)
	{
		double fitNess = 0;
		auto &graph = _ob_sGraph;
		for (size_t i = 0; i < _robNum; i++)
		{
			auto &robSet = (*_vRobSetPtr)[i];
			if (i == robID) continue;
			for (auto it = robSet.begin(); it != robSet.end(); it++)
			{
				double dis = bg::distance(graph[*it].pnt, graph[vd].pnt);
				fitNess += dis;
			}
		}
		return fitNess;
	}

	double MultiAuctionSTC::calOpPriority(size_t const &aucNeer, size_t const & OpRobId, bex::VertexDescriptor const &vd)
	{
		auto &robNeiSet = _vRobSetPtr->at(OpRobId);
		vector<bex::VertexDescriptor> v_vd;
		
		for (auto it = robNeiSet.begin(); it != robNeiSet.end(); it++)
		{
			if (vd == *it) continue;
			v_vd.push_back(*it);
		}
		if (_mainMap.allConnectedBase(v_vd))
		{
			return 0;
		}
		return std::numeric_limits<double>::max();
	}

	size_t MultiAuctionSTC::leafCost(bex::VertexDescriptor const & tvd)
	{
		size_t cost;
		switch (_ob_sGraph[tvd].Type)
		{
		case bex::vertType::SingleOb:
		{
			cost += 6;
			break;
		}
		case bex::vertType::DoubleSameOb:
		{
			cost += 2;
			break;
		}
		case bex::vertType::DoubleDiffOb:
		{
			cost += 2;
			break;
		}
		case bex::vertType::WayVert:
		{
			cost += 4;
			break;
		}
		default:
			break;
		}
		return cost;
	}

	size_t MultiAuctionSTC::estAddCost(size_t const & robID, bex::VertexDescriptor const & svd, bex::VertexDescriptor const & tvd)
	{
		//for
		double cost = 0;
		if (_ob_sGraph[svd].Type == bex::vertType::DoubleSameOb || _ob_sGraph[svd].Type == bex::vertType::DoubleDiffOb)
		{
			cost = 2;
		}
		switch (_ob_sGraph[tvd].Type)
		{
		case bex::vertType::SingleOb:
		{
			cost += 6;
			break;
		}
		case bex::vertType::DoubleSameOb:
		{
			cost += 2;
			break;
		}
		case bex::vertType::DoubleDiffOb:
		{
			cost += 2;
			break;
		}
		case bex::vertType::WayVert:
		{
			cost += 4;
			break;
		}
		default:
			break;
		}
		return cost;
	}


	//double MultiAuctionSTC::estAddCost(size_t const & robID, bex::VertexDescriptor const & vd)
	//{
	//	
	//	return 0.0;
	//}

	bool MultiAuctionSTC::updateNeiGraph(size_t const & robID)
	{
		auto &robNghSet = _vRobNeiPtr->at(robID);
		auto &robSet = _vRobSetPtr->at(robID);
		robNghSet.clear();
		for (auto it = robSet.begin(); it != robSet.end(); it++)
		{
			auto &cenInd = *it;
			auto neighborsIter = bt::adjacent_vertices(cenInd, _ob_sGraph);
			for (auto ni = neighborsIter.first; ni != neighborsIter.second; ++ni)
			{
				if (robSet.count(*ni) == 0)
				{
					robNghSet.insert(*ni);
				}
			}
		}
		return false;
	}

	bool MultiAuctionSTC::updateNeiGraph(size_t const & succBidID, bex::VertexDescriptor const & vd)
	{
		auto &robSet = _vRobSetPtr->at(succBidID);
		auto &robNghSet = _vRobNeiPtr->at(succBidID);
		robSet.insert(vd);
		robNghSet.clear();
		for (auto it = robSet.begin(); it != robSet.end(); it++)
		{
			auto &cenInd = *it;
			auto neighborsIter = bt::adjacent_vertices(cenInd, _ob_sGraph);
			for (auto ni = neighborsIter.first; ni != neighborsIter.second; ++ni)
			{
				if (robSet.count(*ni) == 0)
				{
					robNghSet.insert(*ni);
				}
			}
		}
		return false;
	}
	bool MultiAuctionSTC::updateNeiGraphWithErase(size_t const &loseID, bex::VertexDescriptor const & vd)
	{
		auto &robSet = _vRobSetPtr->at(loseID);
		auto &robNghSet = _vRobNeiPtr->at(loseID);
		robSet.erase(vd);
		robNghSet.clear();
		for (auto it = robSet.begin(); it != robSet.end(); it++)
		{
			auto &cenInd = *it;
			auto neighborsIter = bt::adjacent_vertices(cenInd, _ob_sGraph);
			for (auto ni = neighborsIter.first; ni != neighborsIter.second; ++ni)
			{
				if (robSet.count(*ni) == 0)
				{
					robNghSet.insert(*ni);
				}
			}
		}
		return false;
	}
	bool MultiAuctionSTC::calMakeSpan()
	{
		vector<size_t> vCompleteTime;
		for (auto &it : _vpath)
			vCompleteTime.push_back(it.size());
		size_t makeSpan = *std::max_element(vCompleteTime.begin(), vCompleteTime.end());
		writeDebug(c_deg, "makeSpan", makeSpan);
		return false;
		//return false;
	}
}