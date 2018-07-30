#include "MultiAuctionSTC.h"

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
				vRow.push_back(_ob_sgraph2map[it].first);
				vCol.push_back(_ob_sgraph2map[it].second);
			}
			writeDebug(c_deg, str_row, vRow);
			writeDebug(c_deg, str_col, vCol);
		}

		for (size_t i = 0; i < _robNum; i++)
		{
			string str_row = "neiRow";
			string str_col = "neiCol";
			str_row += std::to_string(i);
			str_col += std::to_string(i);
			vector<size_t> vRow, vCol;
			for (auto &it : (_vRobNeiPtr->at(i)))
			{				
				vRow.push_back(_ob_sgraph2map[it].first);
				vCol.push_back(_ob_sgraph2map[it].second);
			}
			writeDebug(c_deg, str_row, vRow);
			writeDebug(c_deg, str_col, vCol);
		}
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
		writeDebug(c_deg, "sPntx", sPntx);
		writeDebug(c_deg, "sPnty", sPnty);
		writeDebug(c_deg, "tPntx", tPntx);
		writeDebug(c_deg, "tPnty", tPnty);
	}


	void pl::MultiAuctionSTC::auction()
	{
		_vRobSetPtr = make_shared<vector<set<size_t>>>(_robNum);
		_vRobGridPtr = make_shared<vector<vector<GridIndex>>>(_robNum);
		_vRobNeiPtr = make_shared<vector<set<bex::VertexDescriptor>>>(_robNum);
		_vRobSleepPtr = make_shared<vector<bool>>(_robNum, false);

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
			size_t aucNeer = dis(eng);
			bex::VertexDescriptor aucVertID;

			if (circleTime == 253)
				c_deg << "";
			bool allOccupied = calAucVertID(aucNeer, aucVertID);
			c_deg << "circleTime = " << circleTime << endl;
			c_deg << "aucNeer = " << aucNeer << endl;
			c_deg << "aucVertID = " << aucVertID << endl;
			c_deg << "aucVertGridInd.first =  " << this->_ob_sgraph2map[aucVertID].first <<
				"aucVertGrid.second  = " << this->_ob_sgraph2map[aucVertID].second << endl;
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
			if ( ++circleTime >= 3/*maxcircleTime*/)
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
}