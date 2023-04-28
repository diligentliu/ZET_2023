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

	Flow(int id = -1, int bandwidth = 0, int startTime = 0, int sendTime = 0);
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
	this->compose = (double) bandwidth / (double) sendTime;
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

void transfer(list<Flow> &flows, vector<Port> &ports, const string &resultsFile) {
	FILE *fpWrite = fopen(resultsFile.c_str(), "w");
	unsigned portNum = ports.size();
	vector<int> portBandwidths(portNum);
	for (int i = 0; i < portNum; ++i) {
		portBandwidths[i] = ports[i].bandwidth;
	}
	sort(ports.begin(), ports.end(), less<Port>());
	int time = 0;
	Flow temp;
	Flow flow, flowAtPort, flowAtDispatch;
	priority_queue<Flow, vector<Flow>, greater<Flow>> min_heap;
	list<Flow> dispatch;
	vector<list<Flow>> portQueues(portNum);
	int maxDispatchFlow = 20 * portNum;
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
					sort(ports.begin(), ports.end(), less<Port>());
					flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
					break;
				}
			}
		}
		while (!flowAtDispatch.isNull()) {
			int i = 0;
			for (; i < portNum; ++i) {
				if (ports[i].remainBandwidth >= flowAtDispatch.bandwidth) {
					flowAtDispatch.setBeginTime(time);
					flowAtDispatch.setEndTime(time);
					flowAtDispatch.portId = ports[i].id;
					fprintf(fpWrite, "%d,%d,%d\n", flowAtDispatch.id, flowAtDispatch.portId, flowAtDispatch.beginTime);
					// cout << flowAtDispatch.id << "," << flowAtDispatch.portId << "," << flowAtDispatch.beginTime << endl;
					min_heap.push(flowAtDispatch);
					ports[i].modifyRemain(flowAtDispatch.bandwidth);
					sort(ports.begin(), ports.end(), less<Port>());
					dispatch.pop_front();
					flowAtDispatch = (!dispatch.empty() ? dispatch.front() : temp);
					break;
				}
			}
			if (i == portNum) {
				break;
			}
		}
		while (!flow.isNull() && flow.startTime == time) {
			int i = 0;
			for (; i < portNum; ++i) {
				if (ports[i].remainBandwidth >= flow.bandwidth) {
					flow.setBeginTime(time);
					flow.setEndTime(time);
					flow.portId = ports[i].id;
					fprintf(fpWrite, "%d,%d,%d\n", flow.id, flow.portId, flow.beginTime);
					// cout << flow.id << "," << flow.portId << "," << flow.beginTime << endl;
					min_heap.push(flow);
					ports[i].modifyRemain(flow.bandwidth);
					sort(ports.begin(), ports.end(), less<Port>());
					flows.pop_front();
					break;
				}
			}
			if (i == portNum) {
				dispatch.push_back(flow);
				dispatch.sort([](Flow &flow1, Flow &flow2) {
					return flow1.bandwidth < flow2.bandwidth;
				});
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
				flows.pop_front();
			}
			flowAtDispatch = dispatch.front();
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
		// return first.compose < second.compose;
	};
	while (true) {
		string flowsFile;
		flowsFile.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(flowFile);
		string portsFile;
		portsFile.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(portFile);
		string resultsFile;
		resultsFile.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(resultFile);
		list<Flow> flows;
		vector<Port> ports;
		FILE *f = fopen(portsFile.c_str(), "r");
		if (f == nullptr) {
			return 0;
		}
		fclose(f);
		loadFlow(flowsFile.c_str(), flows);
		loadPort(portsFile.c_str(), ports);

		flows.sort(lambda);

		transfer(flows, ports, resultsFile);
		dirNum++;
	}
}
