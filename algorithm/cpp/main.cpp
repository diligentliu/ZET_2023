#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <sstream>
#include <fstream>
#include <string>
#include <queue>
#include <climits>
#include <cstdio>

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
	this->speed = (double) (sendTime) / (double) (bandwidth);
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
	bool operator<(const Port &other) const ;
	bool operator==(const Port &other) const ;
	bool operator>(const Port &other) const ;
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
	sort(ports.begin(), ports.end(), less<Port>());
	unsigned portNum = ports.size();
	int num = 0;
	int time = 0;
	Flow temp;
	Flow flowAtQueue, flowAtPort;
	priority_queue<Flow, vector<Flow>, greater<Flow>> min_heap;
	while (!flows.empty()) {
		flowAtQueue = flows.front();
		if (!min_heap.empty()) {
			flowAtPort = min_heap.top();
		}
		if (!flowAtPort.isNull()) {
			int portPopTime = flowAtPort.endTime;
			int queuePopTime = flowAtQueue.startTime;
			if (queuePopTime <= portPopTime) {
				int i = 0;
				for (; i < portNum; ++i) {
					if (ports[i].remainBandwidth >= flowAtQueue.bandwidth) {
						break;
					}
				}
				if (i != portNum) {
					time = max(time, queuePopTime);
					flowAtQueue.setBeginTime(time);
					flowAtQueue.setEndTime(time);
					flowAtQueue.portId = ports[i].id;
					fprintf(fpWrite, "%d,%d,%d\n", flowAtQueue.id, ports[i].id, flowAtQueue.beginTime);
					num++;
					min_heap.push(flowAtQueue);
					ports[i].modifyRemain(flowAtQueue.bandwidth);
					sort(ports.begin(), ports.end(), less<Port>());
					flows.pop_front();
				} else {
					time = portPopTime;
					int j = 0;
					for (; j < portNum; ++j) {
						if (ports[j].id == flowAtPort.portId) {
							break;
						}
					}
					ports[j].modifyRemain(-flowAtPort.bandwidth);
					sort(ports.begin(), ports.end(), less<Port>());
					min_heap.pop();
				}
			} else {
				time = portPopTime;
				int j = 0;
				for (; j < portNum; ++j) {
					if (ports[j].id == flowAtPort.portId) {
						break;
					}
				}
				ports[j].modifyRemain(-flowAtPort.bandwidth);
				sort(ports.begin(), ports.end(), less<Port>());
				min_heap.pop();
			}
		} else {
			time = max(time, flowAtQueue.startTime);
			flowAtQueue.setBeginTime(time);
			flowAtQueue.setEndTime(time);
			int i = 0;
			for (; i < portNum; ++i) {
				if (ports[i].remainBandwidth >= flowAtQueue.bandwidth) {
					break;
				}
			}
			flowAtQueue.portId = ports[i].id;
			fprintf(fpWrite, "%d,%d,%d\n", flowAtQueue.id, ports[i].id, flowAtQueue.beginTime);
			num++;
			min_heap.push(flowAtQueue);
			ports[i].modifyRemain(flowAtQueue.bandwidth);
			sort(ports.begin(), ports.end(), less<Port>());
			flows.pop_front();
		}
		flowAtPort = temp;
	}
	fclose(fpWrite);
}

int main() {
	int dirNum = 0;
	auto lambda = [&](Flow first, Flow second) {
		if (first.startTime != second.startTime) {
			return first.startTime < second.startTime;
		} else {
			return first.sendTime < second.sendTime;
		}
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
