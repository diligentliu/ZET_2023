#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <queue>
#include <stdio.h>

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
	bool isNull();
	void setBeginTime(int beginTime);
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
	if (sendTime != 0) {
		this->speed = (double) (bandwidth) / (double) (sendTime);
	} else {
		this->speed = 0.0;
	}
}

bool Flow::isNull() {
	return id == -1;
}

void Flow::setBeginTime(int beginTime) {
	this->beginTime = beginTime;
}

void Flow::setEndTime(int beginTime) {
	this->endTime = beginTime + sendTime;
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

class FlowComparer {
public:
	bool operator()(Flow &first, Flow &second) {
		return !Flow::compareAsSSB(first, second);
	}
};

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

class PortComparer {
public:
	bool operator()(Port &first, Port &second) {
		return !Port::compareAsBandwidth(first, second);
	}
};

void split(std::string str, std::vector<std::string> &result) {
	char delimiter = ',';
	size_t pos = str.find(delimiter);
	str += delimiter;
	while (pos != std::string::npos) {
		result.push_back(str.substr(0, pos));
		str = str.substr(pos + 1);
		pos = str.find(delimiter);
	}
}

int loadFlow(const char *filePath, priority_queue<Flow, deque<Flow>, FlowComparer> &flows) {
	int total = 0;
	FILE *fpRead = fopen(filePath, "r");
	if (fpRead == NULL) {
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
		flows.emplace(id, bandwidth, startTime, sendTime);
	}
	return total;
}

int loadFlow(const char *filePath, vector<Flow> &flows) {
	int total = 0;
	FILE *fpRead = fopen(filePath, "r");
	if (fpRead == NULL) {
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
	if (fpRead == NULL) {
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

// 静态选择队列，先这样搞吧，之后再搞动态的
void getQueue(queue<Flow> *queues, vector<int> &queueBandwidth, vector<int> portsWeight, vector<Port> &ports,
              vector<Flow> &flows) {
	int queueNum = ports.size();
	int time = 0;
	int flowPos = 0;
	int flowsNum = flows.size();
	int queuePos = 0;
	while (flowPos < flowsNum) {
		Flow flow = flows[flowPos];
		if (flow.startTime <= time && queueBandwidth[queuePos] < portsWeight[queuePos]) {
			queues[queuePos].push(flow);
			queueBandwidth[queuePos] += flow.bandwidth;
			flowPos++;
			queuePos = (queuePos + 1) % queueNum;
		} else if (flow.startTime == time && queueBandwidth[queuePos] >= portsWeight[queuePos]) {
			queuePos = queuePos + 1;
			if (queuePos == queueNum) {
				queuePos--;
				queues[queuePos].push(flow);
				queueBandwidth[queuePos] += flow.bandwidth;
				flowPos++;
			}
		} else if (flow.startTime > time) {
			time++;
		}
	}
}

void getQueue(queue<Flow> *queues, vector<int> &queueBandwidth, vector<int> &portsWeight, vector<Port> &ports,
              priority_queue<Flow, deque<Flow>, FlowComparer> &flows) {
	int queueNum = ports.size();
	int time = 0;
	int queuePos = 0;
	while (!flows.empty()) {
		Flow flow = flows.top();
		if (flow.startTime == time && queueBandwidth[queuePos] < portsWeight[queuePos]) {
			queues[queuePos].push(flow);
			queueBandwidth[queuePos] += flow.bandwidth;
			queuePos = (queuePos + 1) % queueNum;
			flows.pop();
		} else if (flow.startTime == time && queueBandwidth[queuePos] >= portsWeight[queuePos]) {
			queuePos = queuePos + 1;
			if (queuePos == queueNum) {
				queuePos--;
				queues[queuePos].push(flow);
				queueBandwidth[queuePos] += flow.bandwidth;
				flows.pop();
			}
		} else if (flow.startTime > time) {
			time++;
		}
	}
}

void write_file(string outFilePath, int flowPort, int portId, int beginTime) {
	ofstream out(outFilePath, ios::app);
	out << flowPort << "," << portId << "," << beginTime << endl;
	out.close();
}

void write_file(const char *outFilePath, int result[][3] , int num) {
	FILE *fpWrite = fopen(outFilePath, "w");
	for (int i = 0; i <= num; ++i) {
		fprintf(fpWrite, "%d,%d,%d\n", result[i][0], result[i][1], result[i][2]);
	}
	fclose(fpWrite);
}

/*
void transfer(queue<Flow> queue, Port &port, vector<Result> &results) {
	int time = 0;
	int portId = port.id;
	priority_queue<Flow, vector<Flow>, greater<Flow>> min_heap;
	while (!queue.empty() || !min_heap.empty()) {
		while (true) {
			if (min_heap.empty()) {
				break;
			}
			Flow flowAtPort = min_heap.top();
			if (flowAtPort.endTime != time) {
				break;
			}
			// write_file(outFilePath, flowAtPort.id, portId, flowAtPort.beginTime);
			// cout << flowAtPort.id << "," << portId << "," << flowAtPort.beginTime << endl;
			results.emplace_back(flowAtPort.id, portId, flowAtPort.beginTime);
			port.modifyRemain(-flowAtPort.bandwidth);
			min_heap.pop();
		}
		while (true) {
			if (queue.empty()) {
				break;
			}
			Flow flowAtQueue = queue.front();
			if (flowAtQueue.startTime > time || port.remainBandwidth < flowAtQueue.bandwidth) {
				break;
			}
			flowAtQueue.setBeginTime(time);
			flowAtQueue.setEndTime(time);
			min_heap.push(flowAtQueue);
			port.modifyRemain(flowAtQueue.bandwidth);
			queue.pop();
		}
		time++;
	}
}
*/

void transfer(queue<Flow> queue, Port &port, int results[][3], int& num) {
	int time = 0;
	int portId = port.id;
	Flow temp;
	Flow &flowAtPort = temp, flowAtQueue = temp;
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
				queue.pop();
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
					queue.pop();
				}
			}
			// if (queuePopTime >= portPopTime ||
			//     (queuePopTime < portPopTime && port.remainBandwidth < flowAtQueue.bandwidth)) {
			// 	time = portPopTime;
			// 	port.modifyRemain(-flowAtPort.bandwidth);
			// 	min_heap.pop();
			// 	if (queuePopTime == portPopTime && port.remainBandwidth >= flowAtQueue.bandwidth) {
			// 		flowAtQueue.setBeginTime(time);
			// 		flowAtQueue.setEndTime(time);
			// 		// cout << flowAtQueue.id << "," << portId << "," << flowAtQueue.beginTime << endl;
			// 		write_file(outFilePath, flowAtQueue.id, portId, flowAtQueue.beginTime);
			// 		min_heap.push(flowAtQueue);
			// 		port.modifyRemain(flowAtQueue.bandwidth);
			// 		queue.pop();
			// 	}
			// } else {
			// 	time = max(time, queuePopTime);
			// 	flowAtQueue.setBeginTime(time);
			// 	flowAtQueue.setEndTime(time);
			// 	// cout << flowAtQueue.id << "," << portId << "," << flowAtQueue.beginTime << endl;
			// 	write_file(outFilePath, flowAtQueue.id, portId, flowAtQueue.beginTime);
			// 	min_heap.push(flowAtQueue);
			// 	port.modifyRemain(flowAtQueue.bandwidth);
			// 	queue.pop();
			// }
		} else {
			time = flowAtQueue.startTime;
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
			queue.pop();
		}
		flowAtPort = temp;
		flowAtQueue = temp;
	}
}

int main() {
	int dirNum = 1;
	while (true) {
		string flowsFile = dataPath + "/" + to_string(dirNum) + "/" + flowFile;
		string portsFile = dataPath + "/" + to_string(dirNum) + "/" + portFile;
		string resultsFile = dataPath + "/" + to_string(dirNum) + "/" + resultFile;
		priority_queue<Flow, deque<Flow>, FlowComparer> flows;
		vector<Port> ports;
		FILE *f = fopen(portsFile.c_str(), "r");
		if (f == NULL) {
			return 0;
		}
		fclose(f);
		int flowsTotalBandwidth = loadFlow(flowsFile.c_str(), flows);
		int postsTotalBandwidth = loadPort(portsFile.c_str(), ports);
		int flowsNum = flows.size();
		int (*results)[3] = new int[flowsNum][3];
		// while (!flows.empty()) {
		// 	Flow flow = flows.top();
		// 	cout << flow << endl;
		// 	flows.pop();
		// }
		sort(ports.begin(), ports.end(), Port::compareAsBandwidth);
		int queueNum = ports.size();
		queue<Flow> queues[queueNum];
		vector<int> queueBandwidth(queueNum);
		vector<int> portsWeight(queueNum);
		getPortWeight(portsWeight, ports, postsTotalBandwidth, flowsTotalBandwidth);
		getQueue(queues, queueBandwidth, portsWeight, ports, flows);
		// while (!queues[0].empty()) {
		// 	Flow flow = queues[0].front();
		// 	cout << flow << endl;
		// 	queues[0].pop();
		// }
		// for (auto &queue : queues) {
		// 	cout << queue.size() << endl;
		// }
		int num = 0;
		for (int i = 0; i < queueNum; ++i) {
			// cout << queues[i].size() << endl;
			transfer(queues[i], ports[i], results, num);
		}
		// for (Result result : results) {
		// 	write_file(resultsFile, result.flowId, result.portId, result.startSendTime);
		// }
		write_file(resultsFile.c_str(), results, num - 1);
		// while (!queues[0].empty()) {
		// 	Flow flow = queues[0].front();
		// 	cout << flow << endl;
		// 	queues[0].pop();
		// }
		dirNum++;
		delete [] results;
	}
}
