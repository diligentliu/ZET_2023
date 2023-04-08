#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
using namespace std;

class Flow {
private:
	int id;
	int bandwidth;
	int startTime;
	int sendTime;
public:
	Flow(int id = 0, int bandwidth = 0, int startTime = 0, int sendTime = 0);
	int getBandwidth() const;
	int getStartTime() const;
	int getSendTime() const;
	static bool compareAsStart(Flow first, Flow second);
	static bool compareAsSend(Flow first, Flow second);
	static bool compareAsBandwidth(Flow first, Flow second);
	static bool compareAsCompose(Flow first, Flow second);
	friend ostream& operator << (ostream& out, Flow& flow);
};

Flow::Flow(int id, int bandwidth, int startTime, int sendTime) {
	this->id = id;
	this->bandwidth = bandwidth;
	this->startTime = startTime;
	this->sendTime = sendTime;
}

int Flow::getBandwidth() const {
	return bandwidth;
}

int Flow::getStartTime() const {
	return startTime;
}

int Flow::getSendTime() const {
	return sendTime;
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
	} else {
		return first.bandwidth < second.bandwidth;
	}
}

ostream& operator << (ostream& out, Flow& flow) {
	out << flow.id << "," << flow.bandwidth << "," << flow.startTime << "," << flow.sendTime;
	return out;
}

class Port {
private:
	int id;
	int bandwidth;
	int remainBandwidth;
public:
	Port(int id = 0, int bandwidth = 0);
	bool modifyRemain(int bw);
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

class Result {
private:
	int flowId;
	int portId;
	int startSendTime;
public:
	Result(int flowId, int portId, int startSendTime);
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

string dataPath = "../data";
string flowFile = "flow.txt";
string portFile = "port.txt";

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

void loadFlow(const string& filePath, vector<Flow>& flows) {
	fstream f(filePath.c_str());
	if (!f.is_open()) {
		return;
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
		flows.emplace_back(tempInt[0], tempInt[1], tempInt[2], tempInt[3]);
        tempString.clear();
	}
	f.close();
}

void loadPort(const string& filePath, vector<Port>& posts) {
	fstream f(filePath.c_str());
	if (!f.is_open()) {
		return;
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
		posts.emplace_back(tempInt[0], tempInt[1]);
        tempString.clear();
	}
	f.close();
}
int main() {
	int dirNum = 0;
	string flowsFile = dataPath + "/" + to_string(dirNum) + "/" + flowFile;
	string portsFile = dataPath + "/" + to_string(dirNum) + "/" + portFile;
	vector<Flow> flows;
	vector<Port> ports;
	vector<Result> results;
	loadFlow(flowsFile, flows);
	loadPort(portsFile, ports);
	sort(flows.begin(), flows.end(), Flow::compareAsCompose);
	sort(ports.begin(), ports.end(), Port::compareAsBandwidth);
	for (auto flow : flows) {
		cout << flow << endl;
	}
	for (auto port : ports) {
		cout << port << endl;
	}
	return 0;
}
