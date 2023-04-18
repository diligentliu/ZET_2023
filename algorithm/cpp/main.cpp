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
#include <random>
#include <chrono>

using namespace std;

string dataPath = "../data";
string flowFile = "flow.txt";
string portFile = "port.txt";
string resultFile = "result.txt";

class Flow {
public:
	int id;
	int bandwidth;
	int startTime;
	int beginTime;
	int endTime;
	int sendTime;
	double speed;

	Flow(int id = -1, int bandwidth = 0, int startTime = 0, int sendTime = 0);
	bool isNull() const;
	void setBeginTime(int );
	void setEndTime(int beginTime);
	bool operator<(const Flow &other) const;
	bool operator>(const Flow &other) const;
	bool operator==(const Flow &other) const;
	static bool compareAsSSB(Flow &first, Flow &second);
	static bool compareAsSpeed(Flow &first, Flow &second);
	friend ostream &operator<<(ostream &out, Flow &flow);
};

Flow::Flow(int id, int bandwidth, int startTime, int sendTime) {
	this->id = id;
	this->bandwidth = bandwidth;
	this->startTime = startTime;
	this->sendTime = sendTime;
	this->beginTime = 0;
	this->endTime = INT_MAX;
	if (sendTime != 0) {
		this->speed = (double) (bandwidth) / (double) (sendTime);
	} else {
		this->speed = 0.0;
	}
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

bool Flow::compareAsSSB(Flow &first, Flow &second) {
	if (first.startTime != second.startTime) {
		return first.startTime < second.startTime;
	} else if (first.bandwidth != second.bandwidth) {
		return first.bandwidth < second.bandwidth;
	} else {
		return first.sendTime < second.sendTime;
	}
}

bool Flow::compareAsSpeed(Flow &first, Flow &second) {
	if (first.startTime != second.startTime) {
		return first.startTime < second.startTime;
	} else {
		return first.speed > second.speed;
	}
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
	static bool compareAsBandwidth(Port &first, Port &second);
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

bool Port::compareAsBandwidth(Port &first, Port &second) {
	return first.bandwidth < second.bandwidth;
}

ostream &operator<<(ostream &out, Port &port) {
	out << port.id << "," << port.bandwidth;
	return out;
}

int loadFlow(const char *filePath, list<Flow> &flows) {
	int total = 0;
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
		total += bandwidth;
		flows.emplace_back(id, bandwidth, startTime, sendTime);
	}
	return total;
}

int loadPort(const char *filePath, vector<Port> &posts) {
	int total = 0;
	FILE *fpRead = fopen(filePath, "r");
	if (fpRead == nullptr) {
		return -1;
	}
	int id;
	int bandwidth;
	// 忽略第一行
	fscanf(fpRead, "%*[^\n]%*c");
	while (fscanf(fpRead, "%d,%d\n", &id, &bandwidth) != EOF) {
		total += bandwidth;
		posts.emplace_back(id, bandwidth);
	}
	return total;
}

void getPortWeight(vector<int> &portsWeight, vector<Port> &ports, int &postsTotalBandwidth, int &flowsTotalBandwidth) {
	int portNum = ports.size();
	portsWeight.resize(portNum);
	for (int i = 0; i < portNum; ++i) {
		portsWeight[i] = flowsTotalBandwidth * (double) ports[i].bandwidth / (double) postsTotalBandwidth;
	}
}

void getQueue(list<Flow> *queues, vector<int> &queueBandwidth, vector<int> &portsWeight, int &queueNum,
              list<Flow> &flows, vector<Port>& ports) {
	auto lambda = [&](Flow first, Flow second) {
		if (first.startTime != second.startTime) {
			return first.startTime < second.startTime;
		} else if (first.bandwidth != second.bandwidth) {
			return first.bandwidth < second.bandwidth;
		} else {
			return first.sendTime < second.sendTime;
		}
	};
	int queuePos = queueNum - 1;
	Flow flow;
	for (int i = 0; i < queueNum - 1; ++i) {
		while (queueBandwidth[i] < portsWeight[i]) {
			flow = flows.front();
			if (flow.bandwidth <= ports[i].bandwidth) {
				queues[i].emplace_back(flow.id, flow.bandwidth, flow.startTime, flow.sendTime);
				queueBandwidth[i] += flow.bandwidth;
				flows.pop_front();
			} else {
				flows.push_back(flow);
				flows.pop_front();
			}
		}
		queues[i].sort(lambda);
	}
	while (!flows.empty()) {
		flow = flows.front();
		queues[queuePos].emplace_back(flow.id, flow.bandwidth, flow.startTime, flow.sendTime);
		queueBandwidth[queuePos] += flow.bandwidth;
		flows.pop_front();
	}
	queues[queuePos].sort(lambda);
}

void write_file(const char *outFilePath, int result[][3] , int num) {
	FILE *fpWrite = fopen(outFilePath, "w");
	for (int i = 0; i <= num; ++i) {
		fprintf(fpWrite, "%d,%d,%d\n", result[i][0], result[i][1], result[i][2]);
	}
	fclose(fpWrite);
}

void transfer(list<Flow> queue, Port &port, int results[][3], int& num) {
	int time = 0;
	int portId = port.id;
	Flow temp;
	Flow flowAtPort = temp, flowAtQueue = temp;
	priority_queue<Flow, vector<Flow>, greater<Flow>> min_heap;
	while (!queue.empty()) {
		if (!queue.empty()) {
			flowAtQueue = queue.front();
		}
		if (!min_heap.empty()) {
			flowAtPort = min_heap.top();
		}
		if (!flowAtPort.isNull()) {
			int portPopTime = flowAtPort.endTime;
			int queuePopTime = flowAtQueue.startTime;
			if (queuePopTime < portPopTime && port.remainBandwidth >= flowAtQueue.bandwidth) {
				time = max(time, queuePopTime);
				flowAtQueue.setBeginTime(time);
				flowAtQueue.setEndTime(time);
				// cout << flowAtQueue.id << "," << portId << "," << flowAtQueue.beginTime << endl;
				// write_file(outFilePath, flowAtQueue.id, portId, flowAtQueue.beginTime);
				// results.emplace_back(flowAtQueue.id, portId, flowAtQueue.beginTime);
				results[num][0] = flowAtQueue.id;
				results[num][1] = portId;
				results[num][2] = flowAtQueue.beginTime;
				num++;
				min_heap.push(flowAtQueue);
				port.modifyRemain(flowAtQueue.bandwidth);
				queue.pop_front();
			} else {
				time = portPopTime;
				port.modifyRemain(-flowAtPort.bandwidth);
				min_heap.pop();
				if (queuePopTime == portPopTime && port.remainBandwidth >= flowAtQueue.bandwidth) {
					flowAtQueue.setBeginTime(time);
					flowAtQueue.setEndTime(time);
					// cout << flowAtQueue.id << "," << portId << "," << flowAtQueue.beginTime << endl;
					// write_file(outFilePath, flowAtQueue.id, portId, flowAtQueue.beginTime);
					// results.emplace_back(flowAtQueue.id, portId, flowAtQueue.beginTime);
					results[num][0] = flowAtQueue.id;
					results[num][1] = portId;
					results[num][2] = flowAtQueue.beginTime;
					num++;
					min_heap.push(flowAtQueue);
					port.modifyRemain(flowAtQueue.bandwidth);
					queue.pop_front();
				}
			}
		} else {
			time = max(time, flowAtQueue.startTime);
			flowAtQueue.setBeginTime(time);
			flowAtQueue.setEndTime(time);
			// cout << flowAtQueue.id << "," << portId << "," << flowAtQueue.beginTime << endl;
			// write_file(outFilePath, flowAtQueue.id, portId, flowAtQueue.beginTime);
			// results.emplace_back(flowAtQueue.id, portId, flowAtQueue.beginTime);
			results[num][0] = flowAtQueue.id;
			results[num][1] = portId;
			results[num][2] = flowAtQueue.beginTime;
			num++;
			min_heap.push(flowAtQueue);
			port.modifyRemain(flowAtQueue.bandwidth);
			queue.pop_front();
		}
		flowAtPort = temp;
		flowAtQueue = temp;
	}
}

int main() {
	int dirNum = 0;
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
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
		int flowsTotalBandwidth = loadFlow(flowsFile.c_str(), flows);
		int postsTotalBandwidth = loadPort(portsFile.c_str(), ports);
		int flowsNum = flows.size();
		int (*results)[3] = new int[flowsNum][3];
		sort(ports.begin(), ports.end(), Port::compareAsBandwidth);
		vector<Flow> vectorFlows(flows.begin(), flows.end());
		shuffle(vectorFlows.begin(), vectorFlows.end(), default_random_engine(seed));
		flows = list<Flow>(vectorFlows.begin(), vectorFlows.end());
		int queueNum = ports.size();
		list<Flow> queues[queueNum];
		vector<int> queueBandwidth(queueNum);
		vector<int> portsWeight(queueNum);
		getPortWeight(portsWeight, ports, postsTotalBandwidth, flowsTotalBandwidth);
		getQueue(queues, queueBandwidth, portsWeight, queueNum, flows, ports);
		// for (Flow flow : queues[0]) {
		// 	cout << flow << endl;
		// }
		int num = 0;
		for (int i = 0; i < queueNum; ++i) {
			// cout << queues[i].size() << endl;
			transfer(queues[i], ports[i], results, num);
		}
		write_file(resultsFile.c_str(), results, num - 1);
		dirNum++;
		delete [] results;
	}
}
