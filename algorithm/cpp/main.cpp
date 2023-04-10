#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <queue>
using namespace std;

string dataPath = "../data";
string flowFile = "flow.txt";
string portFile = "port.txt";
string resultFile = "result.txt";

class Flow {
private:
	int id;
	int bandwidth;
	int startTime;
	int beginTime;
	int endTime;
	int sendTime;
	double compose;
	double speed;
public:
	Flow(int id = 0, int bandwidth = 0, int startTime = 0, int sendTime = 0);
	int getId() const;
	int getBandwidth() const;
	int getStartTime() const;
	int getBeginTime() const;
	int getSendTime() const;
	int getEndTime() const;
	void setBeginTime(int beginTime);
	void setEndTime(int beginTime);
	bool operator < (const Flow& other) const;
	bool operator > (const Flow& other) const;
	bool operator == (const Flow& other) const;
	static bool compareAsStart(Flow first, Flow second);
	static bool compareAsSend(Flow first, Flow second);
	static bool compareAsBandwidth(Flow first, Flow second);
	static bool compareAsCompose(Flow first, Flow second);
	static bool compareAsSpeed(Flow first, Flow second);
	friend ostream& operator << (ostream& out, Flow& flow);
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
	this->compose = 0.0;
}

int Flow::getId() const {
	return id;
}

int Flow::getBandwidth() const {
	return bandwidth;
}

int Flow::getStartTime() const {
	return startTime;
}

int Flow::getBeginTime() const {
	return beginTime;
}

int Flow::getSendTime() const {
	return sendTime;
}

int Flow::getEndTime() const {
	return endTime;
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

bool Flow::compareAsStart(Flow first, Flow second) {
	return first.startTime < second.startTime;
}

bool Flow::compareAsSend(Flow first, Flow second) {
	return first.sendTime < second.sendTime;
}

bool Flow::compareAsBandwidth(Flow first, Flow second) {
	return first.bandwidth < second.bandwidth;
}

bool Flow::compareAsCompose(Flow first, Flow second) {
	if (first.startTime != second.startTime) {
		return first.startTime < second.startTime;
	} else if (first.sendTime != second.sendTime) {
		return first.sendTime < second.sendTime;
	} else {
		return first.bandwidth < second.bandwidth;
	}
}

bool Flow::compareAsSpeed(Flow first, Flow second) {
	if (first.startTime != second.startTime) {
		return first.startTime < second.startTime;
	} else {
		return first.speed > second.speed;
	}
}

ostream& operator << (ostream& out, Flow& flow) {
	out << flow.id << "," << flow.bandwidth << "," << flow.startTime << "," << flow.sendTime << "," << flow.speed;
	return out;
}

class FlowComparer {
public:
	bool operator()(Flow& first, Flow& second) {
		return !Flow::compareAsCompose(first, second);
	}
};

class Port {
private:
	int id;
	int bandwidth;
	int remainBandwidth;
public:
	Port(int id, int bandwidth);
	bool modifyRemain(int bw);
	int getId() const;
	int getBandwidth() const;
	int getRemain() const;
	static bool compareAsBandwidth(Port first, Port second);
	friend ostream& operator << (ostream& out, Port& port);
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

int Port::getId() const {
	return id;
}

int Port::getBandwidth() const {
	return bandwidth;
}

int Port::getRemain() const {
	return remainBandwidth;
}

bool Port::compareAsBandwidth(Port first, Port second) {
	return first.bandwidth < second.bandwidth;
}

ostream& operator << (ostream& out, Port& port) {
	out << port.id << "," << port.bandwidth;
	return out;
}

class PortComparer {
public:
	bool operator()(Port& first, Port& second) {
		return !Port::compareAsBandwidth(first, second);
	}
};

class Result {
private:
	int flowId;
	int portId;
	int startSendTime;
public:
	Result(int flowId = 0, int portId = 0, int startSendTime = 0);
	friend ostream &operator << (ostream& out, Result& result);
};

Result::Result(int flowId, int portId, int startSendTime) {
	this->flowId = flowId;
	this->portId = portId;
	this->startSendTime = startSendTime;
}

ostream & operator << (ostream& out, Result& result) {
	out << result.flowId << "," << result.portId << "," << result.startSendTime;
	return out;
}

void split(std::string str, std::vector<std::string>& result) {
	char delimiter = ',';
	size_t pos = str.find(delimiter);
	str += delimiter;
	while (pos != std::string::npos) {
		result.push_back(str.substr(0, pos));
		str = str.substr(pos + 1);
		pos = str.find(delimiter);
	}
}

int loadFlow(const string& filePath, priority_queue<Flow, vector<Flow>, FlowComparer>& flows) {
	int total = 0;
	fstream f(filePath.c_str());
	if (!f.is_open()) {
		return -1;
	}
	string line;
	vector<string> tempString;
	vector<int> tempInt(4);
	stringstream stream;
	// 忽略第一行
	getline(f, line);
	while (getline(f, line)) {
		split(line, tempString);
		for (int i = 0; i < 4; ++i) {
			stream << tempString[i];
			stream >> tempInt[i];
            stream.clear();
		}
		total += tempInt[1];
		flows.emplace(tempInt[0], tempInt[1], tempInt[2], tempInt[3]);
        tempString.clear();
	}
	f.close();
	return total;
}

int loadPort(const string& filePath, priority_queue<Port, vector<Port>, PortComparer>& posts) {
	int total = 0;
	fstream f(filePath.c_str());
	if (!f.is_open()) {
		return -1;
	}
	string line;
	vector<string> tempString;
	vector<int> tempInt(2);
	stringstream stream;
	// 忽略第一行
	getline(f, line);
	while (getline(f, line)) {
		split(line, tempString);
		for (int i = 0; i < 2; ++i) {
			stream << tempString[i];
			stream >> tempInt[i];
            stream.clear();
		}
		total += tempInt[1];
		posts.emplace(tempInt[0], tempInt[1]);
        tempString.clear();
	}
	f.close();
	return total;
}

int loadFlow(const string& filePath, vector<Flow>& flows) {
	int total = 0;
	fstream f(filePath.c_str());
	if (!f.is_open()) {
		return -1;
	}
	string line;
	vector<string> tempString;
	vector<int> tempInt(4);
	stringstream stream;
	// 忽略第一行
	getline(f, line);
	while (getline(f, line)) {
		split(line, tempString);
		for (int i = 0; i < 4; ++i) {
			stream << tempString[i];
			stream >> tempInt[i];
            stream.clear();
		}
		total += tempInt[1];
		flows.emplace_back(tempInt[0], tempInt[1], tempInt[2], tempInt[3]);
        tempString.clear();
	}
	f.close();
	return total;
}

int loadPort(const string& filePath, vector<Port>& posts) {
	int total = 0;
	fstream f(filePath.c_str());
	if (!f.is_open()) {
		return -1;
	}
	string line;
	vector<string> tempString;
	vector<int> tempInt(2);
	stringstream stream;
	// 忽略第一行
	getline(f, line);
	while (getline(f, line)) {
		split(line, tempString);
		for (int i = 0; i < 2; ++i) {
			stream << tempString[i];
			stream >> tempInt[i];
            stream.clear();
		}
		total += tempInt[1];
		posts.emplace_back(tempInt[0], tempInt[1]);
        tempString.clear();
	}
	f.close();
	return total;
}

void getPortWeight(vector<int>& portsWeight, vector<Port>& ports, int& postsTotalBandwidth, int& flowsTotalBandwidth) {
	int portNum = ports.size();
	portsWeight.resize(portNum);
	for (int i = 0; i < portNum; ++i) {
		portsWeight[i] = flowsTotalBandwidth * (double)ports[i].getBandwidth() / (double)postsTotalBandwidth;
	}
}

// 静态选择队列，先这样搞吧，之后再搞动态的
void getQueue(queue<Flow>* queues, vector<int>& queueBandwidth, vector<int> portsWeight, vector<Port>& ports, vector<Flow>& flows) {
	int queueNum = ports.size();
	int time = 0;
	int flowPos = 0;
	int flowsNum = flows.size();
	int queuePos = 0;
	while (flowPos < flowsNum) {
		Flow flow = flows[flowPos];
		if (flow.getStartTime() == time && queueBandwidth[queuePos] < portsWeight[queuePos]) {
			queues[queuePos].push(flow);
			queueBandwidth[queuePos] += flow.getBandwidth();
			flowPos++;
			queuePos = (queuePos + 1) % queueNum;
		} else if (flow.getStartTime() == time && queueBandwidth[queuePos] >= portsWeight[queuePos]) {
			queuePos = queuePos + 1;
			if (queuePos == queueNum) {
				queuePos--;
				queues[queuePos].push(flow);
				queueBandwidth[queuePos] += flow.getBandwidth();
				flowPos++;
			}
		} else if (flow.getStartTime() > time) {
			time = flow.getStartTime();
		}
	}
}


void write_file(string outFilePath, int flowPort, int portId, int beginTime) {
	ofstream out(outFilePath, ios::app);
	out << flowPort << "," << portId << "," << beginTime << endl;
	out.close();
}

void transfer(queue<Flow>& queue, Port& port, string& outFilePath) {
	int time = 0;
	int portId = port.getId();
	priority_queue<Flow, vector<Flow>, greater<Flow>> min_heap;
	while (!queue.empty() || !min_heap.empty()) {
		while (true) {
			if (min_heap.empty()) {
				break;
			}
			Flow flowAtPort = min_heap.top();
			if (flowAtPort.getEndTime() != time) {
				break;
			}
			write_file(outFilePath, flowAtPort.getId(), portId, flowAtPort.getBeginTime());
			// cout << flowAtPort.getId() << "," << portId << "," << flowAtPort.getBeginTime() << endl;
			port.modifyRemain(- flowAtPort.getBandwidth());
			min_heap.pop();
		}
		while (true) {
			if (queue.empty()) {
				break;
			}
			Flow flowAtQueue = queue.front();
			if (flowAtQueue.getStartTime() > time || port.getRemain() < flowAtQueue.getBandwidth()) {
				break;
			}
			flowAtQueue.setBeginTime(time);
			flowAtQueue.setEndTime(time);
			min_heap.push(flowAtQueue);
			port.modifyRemain(flowAtQueue.getBandwidth());
			queue.pop();
		}
		time++;
	}
}

int main() {
	int dirNum = 0;
	while (true) {
		string flowsFile = dataPath + "/" + to_string(dirNum) + "/" + flowFile;
		string portsFile = dataPath + "/" + to_string(dirNum) + "/" + portFile;
		string resultsFile = dataPath + "/" + to_string(dirNum) + "/" + resultFile;
		vector<Flow> flows;
		vector<Port> ports;
		fstream f(portsFile.c_str());
		if (!f.is_open()) {
			return 0;
		}
		f.close();
		int flowsTotalBandwidth = loadFlow(flowsFile, flows);
		int postsTotalBandwidth = loadPort(portsFile, ports);
		sort(flows.begin(), flows.end(), Flow::compareAsCompose);
		sort(ports.begin(), ports.end(), Port::compareAsBandwidth);
		int queueNum = ports.size();
		queue<Flow> queues[queueNum];
		vector<int> queueBandwidth(queueNum);
		vector<int> portsWeight(queueNum);
		getPortWeight(portsWeight, ports, postsTotalBandwidth, flowsTotalBandwidth);
		getQueue(queues, queueBandwidth, portsWeight, ports, flows);

		for (int i = 0; i < queueNum; ++i) {
			// cout << queues[i].size() << endl;
			transfer(queues[i], ports[i], resultsFile);
		}

		// while (!queues[0].empty()) {
		// 	Flow flow = queues[0].front();
		// 	cout << flow << endl;
		// 	queues[0].pop();
		// }
		dirNum++;
	}
}
