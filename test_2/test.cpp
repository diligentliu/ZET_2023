#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <queue>
#include <climits>

using namespace std;

string dataPath = "../dataAnalysis";
string flowFile = "flow.txt";
string portFile = "port.txt";
string resultFile = "result.txt";

class Flow {
public:
	int id;
	int portId;
	int bandwidth;
	int startTime;
	int beginTime;
	int endTime;
	int sendTime;
	double speed;
	double compose;
	double score;

	explicit Flow(int id = -1, int bandwidth = 0, int sendTime = 0, int startTime = 0);
	bool isNull() const;
	void setBeginTime(int bt);
	void setEndTime(int bt);
	bool operator<(const Flow &other) const;
	bool operator>(const Flow &other) const;
	bool operator==(const Flow &other) const;
};

Flow::Flow(int id, int bandwidth, int sendTime, int startTime) {
	this->id = id;
	this->portId = -1;
	this->bandwidth = bandwidth;
	this->startTime = startTime;
	this->sendTime = sendTime;
	this->beginTime = 0;
	this->endTime = INT_MAX;
	this->speed = (double) bandwidth / (double) sendTime;
}

bool Flow::isNull() const {
	return id == -1;
}

void Flow::setBeginTime(int bt) {
	this->beginTime = bt;
}

void Flow::setEndTime(int bt) {
	this->endTime = bt + sendTime;
}

bool Flow::operator<(const Flow &other) const {
	return this->endTime < other.endTime;
}

bool Flow::operator>(const Flow &other) const {
	return this->endTime > other.endTime;
}

bool Flow::operator==(const Flow &other) const {
	return this->endTime == other.endTime;
}

class Port {
public:
	int id;
	int bandwidth;
	int remainBandwidth;

	Port(int id, int bandwidth);
	bool modifyRemain(int bw);
	bool operator<(const Port &other) const;
	bool operator==(const Port &other) const;
	bool operator>(const Port &other) const;
};

Port::Port(int id, int bandwidth) {
	this->id = id;
	this->bandwidth = bandwidth;
	this->remainBandwidth = bandwidth;
}

bool Port::modifyRemain(int bw) {
	if (bw > remainBandwidth) {
		return false;
	}
	remainBandwidth -= bw;
	return true;
}

bool Port::operator<(const Port &other) const {
	return this->remainBandwidth < other.remainBandwidth;
}

bool Port::operator>(const Port &other) const {
	return this->remainBandwidth > other.remainBandwidth;
}

bool Port::operator==(const Port &other) const {
	return this->remainBandwidth == other.remainBandwidth;
}

// 返回流的最大发送时间
int loadFlow(const char *filePath, list<Flow> &flows) {
	int maxSendTime = 0;
	FILE *fpRead = fopen(filePath, "r");
	if (fpRead == nullptr) {
		return -1;
	}
	int id;
	int bandwidth;
	int startTime;
	int sendTime;
	// 忽略第一行
	fscanf(fpRead, "%*[^\n]%*c");
	while (fscanf(fpRead, "%d,%d,%d,%d\n", &id, &bandwidth, &startTime, &sendTime) != EOF) {
		flows.emplace_back(id, bandwidth, sendTime, startTime);
		maxSendTime = max(maxSendTime, sendTime);
	}
	return maxSendTime;
}

void loadPort(const char *filePath, vector<Port> &posts) {
	FILE *fpRead = fopen(filePath, "r");
	if (fpRead == nullptr) {
		return;
	}
	int id;
	int bandwidth;
	// 忽略第一行
	fscanf(fpRead, "%*[^\n]%*c");
	while (fscanf(fpRead, "%d,%d\n", &id, &bandwidth) != EOF) {
		posts.emplace_back(id, bandwidth);
	}
}

// 二分查找 posts 中第一个大于 bandwidth 的索引
int binary_search(vector<Port> &posts, int bandwidth) {
	auto n = posts.size();
	int left = 0, right = n - 1;
	int res = -1;
	while (left <= right) {
		int mid = (right + left) / 2;
		if (posts[mid].remainBandwidth >= bandwidth) {
			res = mid;
			right = mid - 1;
		} else {
			left = mid + 1;
		}
	}
	return res;
}

int transfer(list<Flow> flows, vector<Port> ports, vector<vector<int>> &results,
             const double &a, const double &b, const double &c) {
	// FILE *fpWrite = fopen(resultsFile.c_str(), "w");
	unsigned portNum = ports.size();
	vector<int> portBandwidths(portNum);
	for (int i = 0; i < portNum; ++i) {
		portBandwidths[i] = ports[i].bandwidth;
	}
	sort(ports.begin(), ports.end(), less<>());
	int maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
	int time = 0;
	// 抛弃流罚时
	int over = 0;
	int resultPos = 0;
	Flow temp;
	Flow flow, flowAtPort, flowAtDispatch;
	priority_queue<Flow, vector<Flow>, greater<>> min_heap;
	list<Flow> dispatch;
	vector<list<Flow>> portQueues(portNum);
	unsigned maxDispatchFlow = 20 * portNum;
	while (!flows.empty() || !dispatch.empty() || !min_heap.empty()) {
		flow = (!flows.empty() ? flows.front() : temp);
		flow.compose = (double) flow.sendTime + a * (double) flow.bandwidth + b * flow.speed;
		flow.score = double(flow.sendTime) + c * double(flow.bandwidth);
		flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
		while (!flowAtPort.isNull() && flowAtPort.endTime == time) {
			for (int i = 0; i < portNum; ++i) {
				if (ports[i].id == flowAtPort.portId) {
					ports[i].modifyRemain(-flowAtPort.bandwidth);
					min_heap.pop();
					while (!portQueues[ports[i].id].empty() &&
					       portQueues[ports[i].id].front().bandwidth <= ports[i].remainBandwidth) {
						Flow flowAtPortQueue = portQueues[ports[i].id].front();
						flowAtPortQueue.setBeginTime(time);
						flowAtPortQueue.setEndTime(time);
						min_heap.push(flowAtPortQueue);
						ports[i].modifyRemain(flowAtPortQueue.bandwidth);
						portQueues[ports[i].id].pop_front();
					}
					sort(ports.begin(), ports.end(), less<>());
					maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
					flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
					break;
				}
			}
		}
		while (!flow.isNull() && flow.startTime == time) {
			// 流内的数据不能直接发送到端口，只能通过排队区和缓存区发送到端口
			// (2.3, 7.9) + (0.8, 0.0) --> 50.52
			dispatch.insert(
					std::lower_bound(dispatch.begin(), dispatch.end(), flow, [](const Flow &f1, const Flow &f2) {
						return f1.compose < f2.compose;
					}), flow);
			flowAtDispatch = dispatch.front();
			if (dispatch.size() > maxDispatchFlow) {
				// 缓存区已满, 想要把流放入端口排队区, 取流数量最小的排队区
				// 优化思路: 如果排队区已满则抛弃 sendTime 最小的, 如果未满, 将带宽最小的放入排队区
				// 优化后 50.35 --> 50.35(a = 0.1) 50.47(a = 0.8)
				int portPos = 0;
				for (int j = 0; j < portNum; ++j) {
					if (portBandwidths[j] >= flowAtDispatch.bandwidth) {
						if (portBandwidths[portPos] < flowAtDispatch.bandwidth) {
							portPos = j;
						} else {
							portPos = (portQueues[portPos].size() > portQueues[j].size() ? j : portPos);
						}
					}
				}
				if (portQueues[portPos].size() != 30) {
					flowAtDispatch.portId = portPos;
					portQueues[portPos].push_back(flowAtDispatch);
					// fprintf(fpWrite, "%d,%d,%d\n", flowAtDispatch.id, portPos, time);
					// cout << flowAtDispatch.id << "," << portPos << "," << time << endl;
					results[resultPos][0] = flowAtDispatch.id;
					results[resultPos][1] = flowAtDispatch.portId;
					results[resultPos][2] = time;
					results[resultPos][3] = flowAtDispatch.bandwidth;
					results[resultPos][4] = flowAtDispatch.sendTime;
					++resultPos;
					dispatch.pop_front();
				} else {
					auto f = min_element(dispatch.begin(), dispatch.end(), [](Flow &flow1, Flow &flow2) {
						return flow1.score < flow2.score;
					});
					portPos = 0;
					for (int j = 0; j < portNum; ++j) {
						if (portBandwidths[j] >= f->bandwidth) {
							if (portBandwidths[portPos] < f->bandwidth) {
								portPos = j;
							} else {
								portPos = (portQueues[portPos].size() > portQueues[j].size() ? j : portPos);
							}
						}
					}
					f->portId = portPos;
					if (portQueues[portPos].size() != 30) {
						portQueues[portPos].push_back(*f);
					} else {
						over += (2 * f->sendTime);
					}
					// fprintf(fpWrite, "%d,%d,%d\n", f->id, portPos, time);
					// cout << f->id << "," << portPos << "," << f->sendTime << endl;
					results[resultPos][0] = f->id;
					results[resultPos][1] = portPos;
					results[resultPos][2] = time;
					results[resultPos][3] = f->bandwidth;
					results[resultPos][4] = f->sendTime;
					++resultPos;
					dispatch.erase(f++);
				}
			}
			flows.pop_front();
			flow = (!flows.empty() ? flows.front() : temp);
			flow.compose = (double) flow.sendTime + a * (double) flow.bandwidth + b * flow.speed;
		}
		while (!dispatch.empty()) {
			flowAtDispatch = dispatch.front();
			if (flowAtDispatch.bandwidth <= maxRemainBandwidth) {
				int i = binary_search(ports, flowAtDispatch.bandwidth);
				flowAtDispatch.setBeginTime(time);
				flowAtDispatch.setEndTime(time);
				flowAtDispatch.portId = ports[i].id;
				// fprintf(fpWrite, "%d,%d,%d\n", flowAtDispatch.id, flowAtDispatch.portId, flowAtDispatch.beginTime);
				// cout << flowAtDispatch.id << "," << flowAtDispatch.portId << "," << flowAtDispatch.beginTime << endl;
				results[resultPos][0] = flowAtDispatch.id;
				results[resultPos][1] = flowAtDispatch.portId;
				results[resultPos][2] = flowAtDispatch.beginTime;
				results[resultPos][3] = flowAtDispatch.bandwidth;
				results[resultPos][4] = flowAtDispatch.sendTime;
				++resultPos;
				min_heap.push(flowAtDispatch);
				ports[i].modifyRemain(flowAtDispatch.bandwidth);
				sort(ports.begin(), ports.end(), less<>());
				maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
				dispatch.pop_front();
			} else {
				break;
			}
		}
		++time;
	}
	// fclose(fpWrite);
	return time + over;
}

void write_file(const char *outFilePath, vector<vector<int>> &results, const unsigned long &num) {
	FILE *fpWrite = fopen(outFilePath, "w");
	for (int i = 0; i < num; ++i) {
		fprintf(fpWrite, "%d,%d,%d\n", results[i][0], results[i][1], results[i][2]);
	}
	fclose(fpWrite);
}

pair<int, int> estimate(vector<Port> ports, vector<vector<int>> results) {
	stable_sort(results.begin(), results.end(), [](const vector<int> &a, const vector<int> &b) {
		return a[2] < b[2];
	});
	auto portNum = ports.size();
	priority_queue<Flow, vector<Flow>, greater<>> min_heap;
	vector<list<Flow>> portQueues(portNum);
	int maxTime = 0;
	int overTime = 0;
	int time;
	int drop = 0;
	for (auto &result: results) {
		time = result[2];
		while (!min_heap.empty() && min_heap.top().endTime == time) {
			Flow flowAtPort = min_heap.top();
			ports[flowAtPort.portId].modifyRemain(-flowAtPort.bandwidth);
			min_heap.pop();
			while (!portQueues[flowAtPort.portId].empty() &&
			       portQueues[flowAtPort.portId].front().bandwidth <= ports[flowAtPort.portId].remainBandwidth) {
				Flow flowAtPortQueue = portQueues[flowAtPort.portId].front();
				flowAtPortQueue.setBeginTime(time);
				flowAtPortQueue.setEndTime(time);
				maxTime = max(maxTime, flowAtPortQueue.endTime);
				min_heap.push(flowAtPortQueue);
				ports[flowAtPort.portId].modifyRemain(flowAtPortQueue.bandwidth);
				portQueues[flowAtPort.portId].pop_front();
			}
		}
		Flow flow(result[0], result[3], result[4]);
		flow.portId = result[1];
		if (result[3] <= ports[result[1]].remainBandwidth) {
			flow.setBeginTime(result[2]);
			flow.setEndTime(result[2]);
			maxTime = max(maxTime, flow.endTime);
			ports[result[1]].modifyRemain(result[3]);
			min_heap.push(flow);
		} else if (portQueues[result[1]].size() < 30) {
			portQueues[result[1]].push_back(flow);
		} else {
			overTime += result[4];
			++drop;
		}
	}
	return {overTime * 2 + maxTime, drop};
}

int main(int argc, char const *argv[]) {
	int dirNum = 0;
	auto lambda = [](Flow &first, Flow &second) {
		if (first.startTime != second.startTime) {
			return first.startTime < second.startTime;
		} else if (first.bandwidth != second.bandwidth) {
			return first.bandwidth < second.bandwidth;
		} else {
			return first.sendTime < second.sendTime;
		}
	};
	// double a = stod(argv[1]);
	// double b = stod(argv[2]);
	// double c = stod(argv[3]);
	double a, b, c;
	double pa, pb, pc;
	while (true) {
		string flowsFilePath;
		flowsFilePath.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(flowFile);
		string portsFilePath;
		portsFilePath.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(portFile);
		string resultsFilePath;
		resultsFilePath.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(resultFile);
		list<Flow> flows;
		vector<Port> ports;
		FILE *f = fopen(portsFilePath.c_str(), "r");
		if (f == nullptr) {
			return 0;
		}
		fclose(f);
		loadFlow(flowsFilePath.c_str(), flows);
		loadPort(portsFilePath.c_str(), ports);

		flows.sort(lambda);
		FILE *fpWrite = fopen(resultsFilePath.c_str(), "w");
		auto flowsNum = flows.size();
		// 优化思路跑两次，每次用不同的权重，取最好的那一次，(2.3, -7.9) + (0.8, 0.0) --> 50.52
		vector<vector<int>> temp(flowsNum, vector<int>(5));
		vector<vector<int>> res;
		int ret = INT_MAX;
		for (int i = 23; i <= 23; ++i) {
			a = double (i) / 10;
			for (int j = -79; j <= -79; ++j) {
				b = double (j) / 10;
				for (int k = -100; k < 100; ++k) {
					c = double (k) / 10;
					int tempRes = transfer(flows, ports, temp, a, b, c);
					cout << a << "," << b << "," << c << "," << tempRes << endl;
					fprintf(fpWrite, "%lf,%lf,%lf,%d\n", a, b, c, tempRes);
					if (tempRes < ret) {
						ret = tempRes;
						pa = a;
						pb = b;
						pc = c;
						res = temp;
					}
				}
			}
		}
		cout << pa << "," << pb << "," << pc << "," << ret << endl;
		fclose(fpWrite);
		// transfer(flows, ports, temp, a, b);
		write_file(resultsFilePath.c_str(), temp, flowsNum);
		dirNum++;
	}
}
