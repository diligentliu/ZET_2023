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
	double speed1;
	double speed2;
	double compose;

	explicit Flow(int id = -1, int bandwidth = 0, int startTime = 0, int sendTime = 0);
	bool isNull() const;
	void setBeginTime(int bt);
	void setEndTime(int bt);
	bool operator<(const Flow &other) const;
	bool operator>(const Flow &other) const;
	bool operator==(const Flow &other) const;
};

Flow::Flow(int id, int bandwidth, int startTime, int sendTime) {
	this->id = id;
	this->portId = -1;
	this->bandwidth = bandwidth;
	this->startTime = startTime;
	this->sendTime = sendTime;
	this->beginTime = 0;
	this->endTime = INT_MAX;
	this->speed1 = double (bandwidth) / double (sendTime);
	this->speed2 = double (sendTime) / double (bandwidth);
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

class CompareAsCompose {
public:
	bool operator() (Flow &flow1, Flow &flow2) {
		return flow1.compose > flow2.compose;
	}
};

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
		int mid = (left + right) / 2;
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
			 const double &a, const double &b, const double &c, const double &d) {
	unsigned portNum = ports.size();
	int resultPos = 0;
	sort(ports.begin(), ports.end(), less<>());
	int maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
	int time = 0;
	int maxTime = 0;
	Flow temp;
	Flow flow, flowAtPort, flowAtDispatch;
	priority_queue<Flow, vector<Flow>, greater<>> min_heap;
	priority_queue<Flow, vector<Flow>, CompareAsCompose> dispatch;
	while (!flows.empty() || !dispatch.empty()) {
		flow = (!flows.empty() ? flows.front() : temp);
		flow.compose = a * double (flow.sendTime) + b * double (flow.bandwidth) + c * flow.speed1 + d * flow.speed2;
		flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
		while (!flowAtPort.isNull() && flowAtPort.endTime == time) {
			for (auto &port : ports) {
				if (port.id == flowAtPort.portId) {
					port.modifyRemain(-flowAtPort.bandwidth);
					min_heap.pop();
					sort(ports.begin(), ports.end(), less<>());
					maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
					flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
					break;
				}
			}
		}
		while (!flow.isNull() && flow.startTime <= time) {
			dispatch.push(flow);
			flows.pop_front();
			flow = (!flows.empty() ? flows.front() : temp);
			flow.compose = a * double(flow.sendTime) + b * double(flow.bandwidth) + c * flow.speed1 + d * flow.speed2;
		}
		while (!dispatch.empty()) {
			flowAtDispatch = dispatch.top();
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
				resultPos++;
				maxTime = max(maxTime, flowAtDispatch.endTime);
				min_heap.push(flowAtDispatch);
				ports[i].modifyRemain(flowAtDispatch.bandwidth);
				sort(ports.begin(), ports.end(), less<>());
				maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
				dispatch.pop();
			} else {
				break;
			}
		}
		++time;
	}
	return maxTime;
}

void write_file(const char *outFilePath, vector<vector<int>> results, const unsigned long &num) {
	FILE *fpWrite = fopen(outFilePath, "w");
	for (int i = 0; i < num; ++i) {
		fprintf(fpWrite, "%d,%d,%d\n", results[i][0], results[i][1], results[i][2]);
	}
	fclose(fpWrite);
}

int main() {
	int dirNum = 0;
	auto lambda = [](Flow &first, Flow &second) {
		return first.startTime < second.startTime;
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
		int flowsNum = flows.size();

		double a, b, c, d;
		// vector<vector<vector<int>>> results(6, vector<vector<int>>(flowsNum, vector<int>(3)));
		// vector<int> rets(6, INT_MAX);
		// // (a, b) = (-6.4, 0) --> 30.11
		// a = -6.4, b = .0, c = 1.0, d = 0.0;
		// rets[0] = transfer(flows, ports, results[0], a, b, c, d);
		//
		// a = -6.2;
		// rets[1] = transfer(flows, ports, results[1], a, b, c, d);
		//
		// a = -7.7;
		// rets[2] = transfer(flows, ports, results[2], a, b, c, d);
		//
		// a = -9.9, b = 0.1, c = 0.0, d = 1.0;
		// rets[3] = transfer(flows, ports, results[3], a, b, c, d);
		//
		// a = -9.8;
		// rets[4] = transfer(flows, ports, results[4], a, b, c, d);
		//
		// a = -9.5;
		// rets[5] = transfer(flows, ports, results[5], a, b, c, d);
		//
		// for (auto &r : rets) {
		// 	cout << r << ",";
		// }
		// cout << endl;
		//
		// auto it = min_element(rets.begin(), rets.end());
		// write_file(resultsFilePath.c_str(), results[it - rets.begin()], flowsNum);
		//
		int ret = INT_MAX;
		vector<vector<int>> temp(flowsNum, vector<int>(3));
		vector<vector<int>> results;
		vector<double> a1_value = {-6.4, -6.2, -7.7, -6.5, -6.1, -5.9, -4.9, -4.1};
		b = .0, c = 1.0, d = 0.0;
		for (auto &value : a1_value) {
			a = value;
			int r = transfer(flows, ports, temp, a, b, c, d);
			if (r < ret) {
				ret = r;
				results = temp;
			}
		}
		b = 0.1, c = 0.0, d = 1.0;
		vector<double> a2_value = {-9.9, -9.8, -9.5, -9.3, -9.2, -9.1, -9.0, -8.8, -8.7, -8.3};
		for (auto &value : a2_value) {
			a = value;
			int r = transfer(flows, ports, temp, a, b, c, d);
			if (r < ret) {
				ret = r;
				results = temp;
			}
		}
		write_file(resultsFilePath.c_str(), results, flowsNum);
		dirNum++;
	}
}
