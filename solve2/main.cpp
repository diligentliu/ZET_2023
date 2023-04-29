#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <queue>
#include <climits>

using namespace std;

string dataPath = "../data";
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

	explicit Flow(int id = -1, int bandwidth = 0, int startTime = 0, int sendTime = 0);
	bool isNull() const;
	void setBeginTime(int bt);
	void setEndTime(int bt);
	bool operator<(const Flow &other) const;
	bool operator>(const Flow &other) const;
	bool operator==(const Flow &other) const;
	friend ostream &operator<<(ostream &out, Flow &flow);
};

Flow::Flow(int id, int bandwidth, int startTime, int sendTime) {
	this->id = id;
	this->portId = -1;
	this->bandwidth = bandwidth;
	this->startTime = startTime;
	this->sendTime = sendTime;
	this->beginTime = 0;
	this->endTime = INT_MAX;
	this->speed = (double) bandwidth / (double) sendTime;
	// 1.24 和 1.3 ==> 50.35
	// 1.24 ~ 1.83
	this->compose = (double) sendTime + 1.3 * (double) bandwidth;
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

ostream &operator<<(ostream &out, Flow &flow) {
	out << flow.id << "," << flow.bandwidth << "," << flow.startTime << "," << flow.sendTime << "," << flow.speed;
	return out;
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
	friend ostream &operator<<(ostream &out, Port &port);
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

ostream &operator<<(ostream &out, Port &port) {
	out << port.id << "," << port.bandwidth;
	return out;
}

void loadFlow(const char *filePath, list<Flow> &flows) {
	FILE *fpRead = fopen(filePath, "r");
	if (fpRead == nullptr) {
		return;
	}
	int id;
	int bandwidth;
	int startTime;
	int sendTime;
	// 忽略第一行
	fscanf(fpRead, "%*[^\n]%*c");
	while (fscanf(fpRead, "%d,%d,%d,%d\n", &id, &bandwidth, &startTime, &sendTime) != EOF) {
		flows.emplace_back(id, bandwidth, startTime, sendTime);
	}
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
	int n = posts.size();
	int left = 0, right = n - 1;
	int res = -1;
	while (left <= right) {
		int mid = left + (right - left) / 2;
		if (posts[mid].remainBandwidth >= bandwidth) {
			res = mid;
			right = mid - 1;
		} else {
			left = mid + 1;
		}
	}
	return res;
}

void transfer(list<Flow> &flows, vector<Port> &ports, const string &resultsFile) {
	FILE *fpWrite = fopen(resultsFile.c_str(), "w");
	unsigned portNum = ports.size();
	vector<int> portBandwidths(portNum);
	for (int i = 0; i < portNum; ++i) {
		portBandwidths[i] = ports[i].bandwidth;
	}
	sort(ports.begin(), ports.end(), less<>());
	int maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
	int time = 0;
	Flow temp;
	Flow flow, flowAtPort, flowAtDispatch;
	priority_queue<Flow, vector<Flow>, greater<>> min_heap;
	list<Flow> dispatch;
	vector<list<Flow>> portQueues(portNum);
	unsigned maxDispatchFlow = 20 * portNum;
	while (!flows.empty() || !dispatch.empty()) {
		flow = (!flows.empty() ? flows.front() : temp);
		flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
		flowAtDispatch = (!dispatch.empty() ? dispatch.front() : temp);
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
		while (!flowAtDispatch.isNull()) {
			if (flowAtDispatch.bandwidth <= maxRemainBandwidth) {
				int i = binary_search(ports, flowAtDispatch.bandwidth);
				flowAtDispatch.setBeginTime(time);
				flowAtDispatch.setEndTime(time);
				flowAtDispatch.portId = ports[i].id;
				fprintf(fpWrite, "%d,%d,%d\n", flowAtDispatch.id, flowAtDispatch.portId, flowAtDispatch.beginTime);
				// cout << flowAtDispatch.id << "," << flowAtDispatch.portId << "," << flowAtDispatch.beginTime << endl;
				min_heap.push(flowAtDispatch);
				ports[i].modifyRemain(flowAtDispatch.bandwidth);
				sort(ports.begin(), ports.end(), less<>());
				maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
				dispatch.pop_front();
				flowAtDispatch = (!dispatch.empty() ? dispatch.front() : temp);
			} else {
				break;
			}
		}
		while (!flow.isNull() && flow.startTime == time) {
			if (flow.bandwidth <= maxRemainBandwidth) {
				int i = binary_search(ports, flow.bandwidth);
				flow.setBeginTime(time);
				flow.setEndTime(time);
				flow.portId = ports[i].id;
				fprintf(fpWrite, "%d,%d,%d\n", flow.id, flow.portId, flow.beginTime);
				// cout << flow.id << "," << flow.portId << "," << flow.beginTime << endl;
				min_heap.push(flow);
				ports[i].modifyRemain(flow.bandwidth);
				sort(ports.begin(), ports.end(), less<>());
				maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
				flows.pop_front();
			} else {
				dispatch.insert(std::lower_bound(dispatch.begin(), dispatch.end(), flow, [](const Flow& f1, const Flow& f2) {
				    return f1.compose < f2.compose;
				}), flow);
				if (dispatch.size() > maxDispatchFlow) {
					// 缓存区已满, 想要把流放入端口排队区, 取流数量最小的排队区
					auto f = min_element(dispatch.begin(), dispatch.end(), [](Flow &flow1, Flow &flow2) {
						return flow1.sendTime < flow2.sendTime;
					});
					int portPos = 0;
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
					}
					fprintf(fpWrite, "%d,%d,%d\n", f->id, portPos, time);
					// cout << f->id << "," << portPos << "," << time << endl;
					dispatch.erase(f++);
				}
				flowAtDispatch = (!dispatch.empty() ? dispatch.front() : temp);
				flows.pop_front();
			}
			flow = (!flows.empty() ? flows.front() : temp);
		}
		++time;
	}
	fclose(fpWrite);
}

int main() {
	int dirNum = 0;
	auto lambda = [&](Flow first, Flow second) {
		if (first.startTime != second.startTime) {
			return first.startTime < second.startTime;
		} else if (first.bandwidth != second.bandwidth) {
			return first.bandwidth < second.bandwidth;
		} else {
			return first.sendTime < second.sendTime;
		}
		return first.compose < second.compose;
	};
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

		transfer(flows, ports, resultsFilePath);
		dirNum++;
	}
}
